// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * LPE <offset> implementation
 */
/*
 * Authors:
 *   Maximilian Albert
 *   Jabiertxo Arraiza
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 * Copyright (C) Maximilian Albert 2008 <maximilian.albert@gmail.com>
 * Copyright (C) Jabierto Arraiza 2015 <jabier.arraiza@marker.es>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "lpe-offset.h"

#include <2geom/path-intersection.h>
#include <2geom/piecewise.h>

#include "inkscape.h"
#include "style.h"

#include "display/curve.h"
#include "helper/geom-pathstroke.h"
#include "helper/geom.h"
#include "live_effects/parameter/enum.h"
#include "object/sp-shape.h"
#include "path/path-boolop.h"
#include "svg/svg.h"
#include "ui/knot/knot-holder.h"
#include "ui/knot/knot-holder-entity.h"
#include "util/units.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

namespace OfS {
    class KnotHolderEntityOffsetPoint : public LPEKnotHolderEntity {
    public:
        KnotHolderEntityOffsetPoint(LPEOffset * effect) : LPEKnotHolderEntity(effect) {}
        ~KnotHolderEntityOffsetPoint() override
        {
            LPEOffset *lpe = dynamic_cast<LPEOffset *>(_effect);
            if (lpe) {
                lpe->_knot_entity = nullptr;
            }
        }
        void knot_set(Geom::Point const &p, Geom::Point const &origin, guint state) override;
        void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override;
        Geom::Point knot_get() const override;
    private:
    };
} // OfS


static const Util::EnumData<unsigned> JoinTypeData[] = {
    // clang-format off
    {JOIN_BEVEL,       N_("Beveled"),    "bevel"},
    {JOIN_ROUND,       N_("Rounded"),    "round"},
    {JOIN_MITER,       N_("Miter"),      "miter"},
    {JOIN_MITER_CLIP,  N_("Miter Clip"), "miter-clip"},
    {JOIN_EXTRAPOLATE, N_("Extrapolated arc"), "extrp_arc"},
    {JOIN_EXTRAPOLATE1, N_("Extrapolated arc Alt1"), "extrp_arc1"},
    {JOIN_EXTRAPOLATE2, N_("Extrapolated arc Alt2"), "extrp_arc2"},
    {JOIN_EXTRAPOLATE3, N_("Extrapolated arc Alt3"), "extrp_arc3"},
    // clang-format on
};

static const Util::EnumDataConverter<unsigned> JoinTypeConverter(JoinTypeData, sizeof(JoinTypeData)/sizeof(*JoinTypeData));


LPEOffset::LPEOffset(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    unit(_("Unit"), _("Unit of measurement"), "unit", &wr, this, "mm"),
    offset(_("Offset:"), _("Offset"), "offset", &wr, this, 0.0),
    linejoin_type(_("Join:"), _("Determines the shape of the path's corners"),  "linejoin_type", JoinTypeConverter, &wr, this, JOIN_MITER),
    miter_limit(_("Miter limit:"), _("Maximum length of the miter join (in units of stroke width)"), "miter_limit", &wr, this, 4.0),
    attempt_force_join(_("Force miter"), _("Overrides the miter limit and forces a join."), "attempt_force_join", &wr, this, false),
    update_on_knot_move(_("Live update"), _("Update while moving handle"), "update_on_knot_move", &wr, this, true)
{
    show_orig_path = true;
    registerParameter(&linejoin_type);
    registerParameter(&unit);
    registerParameter(&offset);
    registerParameter(&miter_limit);
    registerParameter(&attempt_force_join);
    registerParameter(&update_on_knot_move);
    offset.param_set_increments(0.1, 0.1);
    offset.param_set_digits(6);
    offset_pt = Geom::Point(Geom::infinity(), Geom::infinity());
    _knot_entity = nullptr;
    _provides_knotholder_entities = true;
    apply_to_clippath_and_mask = true;
    prev_unit = unit.get_abbreviation();
    liveknot = false;
    fillrule = fill_nonZero;
}

LPEOffset::~LPEOffset()
{
    modified_connection.disconnect();
};

void
LPEOffset::modified(SPObject *obj, guint flags)
{
    if (flags & SP_OBJECT_STYLE_MODIFIED_FLAG) {
        // Get the used fillrule
        SPCSSAttr *css;
        const gchar *val;
        css = sp_repr_css_attr (sp_lpe_item->getRepr() , "style");
        val = sp_repr_css_property (css, "fill-rule", nullptr);
        FillRuleFlatten fillrule_chan = fill_nonZero;
        if (val && strcmp (val, "evenodd") == 0)
        {
            fillrule_chan = fill_oddEven;
        }
        if (fillrule != fillrule_chan) {
            sp_lpe_item_update_patheffect (sp_lpe_item, true, true);
        }
    }
}

static void
sp_flatten(Geom::PathVector &pathvector, FillRuleFlatten fillkind)
{
    Path *orig = new Path;
    orig->LoadPathVector(pathvector);
    Shape *theShape = new Shape;
    Shape *theRes = new Shape;
    orig->ConvertWithBackData (1.0);
    orig->Fill (theShape, 0);
    theRes->ConvertToShape (theShape, FillRule(fillkind));
    Path *originaux[1];
    originaux[0] = orig;
    Path *res = new Path;
    theRes->ConvertToForme (res, 1, originaux, true);

    delete theShape;
    delete theRes;
    char *res_d = res->svg_dump_path ();
    delete res;
    delete orig;
    pathvector  = sp_svg_read_pathv(res_d);
}

Geom::Point get_nearest_point(Geom::PathVector pathv, Geom::Point point)
{
    Geom::Point res = Geom::Point(Geom::infinity(), Geom::infinity());
    boost::optional< Geom::PathVectorTime > pathvectortime = pathv.nearestTime(point);
    if (pathvectortime) {
        Geom::PathTime pathtime = pathvectortime->asPathTime();
        res = pathv[(*pathvectortime).path_index].pointAt(pathtime.curve_index + pathtime.t);
    }
    return res;
}

/* double get_separation(Geom::PathVector pathv, Geom::Point point, Geom::Point point_b)
{
    Geom::Point res = Geom::Point(Geom::infinity(), Geom::infinity());
    boost::optional<Geom::PathVectorTime> pathvectortime = pathv.nearestTime(point);
    boost::optional<Geom::PathVectorTime> pathvectortime_b = pathv.nearestTime(point_b);
    if (pathvectortime && pathvectortime_b) {
        Geom::PathTime pathtime = pathvectortime->asPathTime();
        Geom::PathTime pathtime_b = pathvectortime_b->asPathTime();
        if ((*pathvectortime).path_index == (*pathvectortime_b).path_index) {
            return std::abs((pathtime.curve_index + pathtime.t) - (pathtime_b.curve_index + pathtime_b.t));
        }
    }
    return -1;
} */

void LPEOffset::transform_multiply(Geom::Affine const &postmul, bool /*set*/)
{
    refresh_widgets = true;
    if (!postmul.isTranslation()) {
        Geom::Affine current_affine = sp_item_transform_repr(sp_lpe_item);
        offset.param_transform_multiply(postmul * current_affine.inverse(), true);
    }
    offset_pt *= postmul;
}

Geom::Point LPEOffset::get_default_point(Geom::PathVector pathv)
{
    Geom::Point origin = Geom::Point(Geom::infinity(), Geom::infinity());
    Geom::OptRect bbox = pathv.boundsFast();
    if (bbox) {
        origin = Geom::Point((*bbox).midpoint()[Geom::X], (*bbox).top());
        origin = get_nearest_point(pathv, origin);
    }
    return origin;
}

double
LPEOffset::sp_get_offset(Geom::Point origin)
{
    double ret_offset = 0;
    int winding_value = mix_pathv_all.winding(origin);
    bool inset = false;
    if (winding_value % 2 != 0) {
        inset = true;
    }
    ret_offset = Geom::distance(origin, get_nearest_point(mix_pathv_all, origin));
    if (inset) {
        ret_offset *= -1;
    }
    return Inkscape::Util::Quantity::convert(ret_offset, "px", unit.get_abbreviation()) * this->scale;
}

void
LPEOffset::addCanvasIndicators(SPLPEItem const *lpeitem, std::vector<Geom::PathVector> &hp_vec)
{
    hp_vec.push_back(helper_path);
}

void
LPEOffset::doBeforeEffect (SPLPEItem const* lpeitem)
{
    SPObject *obj = dynamic_cast<SPObject *>(sp_lpe_item);
    if (is_load && obj) {
        modified_connection = obj->connectModified(sigc::mem_fun(*this, &LPEOffset::modified));
    }
    original_bbox(lpeitem);
    SPGroup *group = dynamic_cast<SPGroup *>(sp_lpe_item);
    if (group) {
        mix_pathv_all.clear();
    }
    this->scale = lpeitem->i2doc_affine().descrim();
    if (!is_load && prev_unit != unit.get_abbreviation()) {
        offset.param_set_value(Inkscape::Util::Quantity::convert(offset, prev_unit, unit.get_abbreviation()));
    }
    prev_unit = unit.get_abbreviation();
}

int offset_winding(Geom::PathVector pathvector, Geom::Path path)
{
    int wind = 0;
    Geom::Point p = path.initialPoint();
    for (auto i:pathvector) {
        if (i == path)  continue;
        if (!i.boundsFast().contains(p)) continue;
        wind += i.winding(p);
    }
    return wind;
}

void LPEOffset::doAfterEffect(SPLPEItem const * /*lpeitem*/, SPCurve *curve)
{
    if (offset_pt == Geom::Point(Geom::infinity(), Geom::infinity())) {
        if (_knot_entity) {
            _knot_entity->knot_get();
        }
    }
    if (is_load) {
        offset_pt = Geom::Point(Geom::infinity(), Geom::infinity());
    }
    if (_knot_entity && sp_lpe_item && !liveknot) {
        Geom::PathVector out;
        // we don do this on groups, editing is joining ito so no need to update knot
        SPShape *shape = dynamic_cast<SPShape *>(sp_lpe_item);
        if (shape) {
            out = SP_SHAPE(sp_lpe_item)->curve()->get_pathvector();
            offset_pt = get_nearest_point(out, offset_pt);
            _knot_entity->knot_get();
        }
    }
    is_load = false;
}

// Taked from Knot LPE duple code
static Geom::Path::size_type size_nondegenerate(Geom::Path const &path)
{
    Geom::Path::size_type retval = path.size_default();
    const Geom::Curve &closingline = path.back_closed();
    // the closing line segment is always of type
    // Geom::LineSegment.
    if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
        // closingline.isDegenerate() did not work, because it only checks for
        // *exact* zero length, which goes wrong for relative coordinates and
        // rounding errors...
        // the closing line segment has zero-length. So stop before that one!
        retval = path.size_open();
    }
    return retval;
}

/* gint get_nearest_corner(Geom::OptRect bbox, Geom::Point point)
{
    if (bbox) {
        double distance_a = Geom::distance(point, (*bbox).corner(0));
        double distance_b = Geom::distance(point, (*bbox).corner(1));
        double distance_c = Geom::distance(point, (*bbox).corner(2));
        double distance_d = Geom::distance(point, (*bbox).corner(3));
        std::vector<double> distances{distance_a, distance_b, distance_c, distance_d};
        std::vector<double>::iterator mindistance = std::min_element(distances.begin(), distances.end());
        return std::distance(distances.begin(), mindistance);
    }
    return -1;
} */

// This way not work with good selfintersections on consecutive curves
// and when there is nodes nearest to points
// I try diferent methods to cleanup without luky
// if anyone is interested there is a way to explore
// if in original path the section into the 2 nearest point dont have
// self intersections we can supose this is a intersection to remove
// it works very well but in good selfintersections work only in one offset direction
/* bool consecutiveCurves(Geom::Path pathin, Geom::Point p) {
    Geom::Coord mindist = std::numeric_limits<Geom::Coord>::max();
    size_t pos;
    for (size_t i = 0; i < pathin.size_default(); ++i) {
        Geom::Curve const &c = pathin.at(i);
        double dist = Geom::distance(p, c.boundsFast());
        if (dist >= mindist) {
            continue;
        }
        Geom::Coord t = c.nearestTime(p);
        Geom::Coord d = Geom::distance(c.pointAt(t), p);
        if (d < mindist) {
            pos = i;
            mindist = d;
        }
    }
    size_t pos_b;
    mindist = std::numeric_limits<Geom::Coord>::max();
    for (size_t i = 0; i < pathin.size_default(); ++i) {
        Geom::Curve const &c = pathin.at(i);
        double dist = Geom::distance(p, c.boundsFast());
        if (dist >= mindist || pos == i) {
            continue;
        }
        Geom::Coord t = c.nearestTime(p);
        Geom::Coord d = Geom::distance(c.pointAt(t), p);
        if (d < mindist) {
            pos_b = i;
            mindist = d;
        }
    }
    if (Geom::are_near(Geom::distance(pos,pos_b), 1)) {
        return true;
    }
    return false;
} */

/* Geom::Path removeIntersects(Geom::Path pathin, Geom::Path pathorig, size_t skipcross)
{
    Geom::Path out;
    Geom::Crossings crossings = Geom::self_crossings(pathin);
    size_t counter = 0;
    for (auto cross : crossings) {
        if (!Geom::are_near(cross.ta, cross.tb, 0.01)) {
            size_t sizepath = size_nondegenerate(pathin);
            double ta = cross.ta > cross.tb ? cross.tb : cross.ta;
            double tb = cross.ta > cross.tb ? cross.ta : cross.tb;
            if (!pathin.closed()) {
                counter++;
                if (skipcross >= counter) {
                    continue;
                }
                bool removeint = consecutiveCurves(pathorig, pathin.pointAt(ta));
                Geom::Path p0 = pathin;
                if (removeint) {
                    Geom::Path p1 = pathin.portion(ta, tb);
                    if ((*p1.boundsFast()).maxExtent() > 0.01) {
                        p0 = pathin.portion(0, ta);
                        if (!Geom::are_near(tb, sizepath, 0.01)) {
                            Geom::Path p2 = pathin.portion(tb, sizepath);
                            p0.setFinal(p2.initialPoint());
                            p0.append(p2);
                        }
                    } else {
                        skipcross++;
                    }
                } else {
                    skipcross++;
                }
                out = removeIntersects(p0, pathorig, skipcross);
                return out;
            }
        }
    }
    return pathin;
} */
// TODO: find a way to not remove wanted self intersections
// previouly are some failed attemps

Geom::Path removeIntersects(Geom::Path pathin)
{
    Geom::Path out;
    Geom::Crossings crossings = Geom::self_crossings(pathin);
    for (auto cross : crossings) {
        if (!Geom::are_near(cross.ta, cross.tb, 0.01)) {
            size_t sizepath = size_nondegenerate(pathin);
            double ta = cross.ta > cross.tb ? cross.tb : cross.ta;
            double tb = cross.ta > cross.tb ? cross.ta : cross.tb;
            if (!pathin.closed()) {
                Geom::Path p0 = pathin;
                Geom::Path p1 = pathin.portion(ta, tb);
                p0 = pathin.portion(0, ta);
                if (!Geom::are_near(tb, sizepath, 0.01)) {
                    Geom::Path p2 = pathin.portion(tb, sizepath);
                    p0.setFinal(p2.initialPoint());
                    p0.append(p2);
                }
                out = removeIntersects(p0);
                return out;
            }
        }
    }
    return pathin;
}

Geom::PathVector 
LPEOffset::doEffect_path(Geom::PathVector const & path_in)
{
    Geom::PathVector ret_closed;
    Geom::PathVector ret_open;
    SPItem * item = SP_ITEM(current_shape);
    SPDocument *document = getSPDoc();
    if (!item || !document) {
        return path_in;
    }
    // Get the used fillrule
    SPCSSAttr *css;
    const gchar *val;
    css = sp_repr_css_attr (item->getRepr() , "style");
    val = sp_repr_css_property (css, "fill-rule", nullptr);

    fillrule = fill_nonZero;
    if (val && strcmp (val, "evenodd") == 0)
    {
        fillrule = fill_oddEven;
    }

    double tolerance = -1;
    if (liveknot) {
        tolerance = 3;
    }
    // Get the doc units offset
    double to_offset = Inkscape::Util::Quantity::convert(offset, unit.get_abbreviation(), "px") / this->scale;
    Geom::PathVector open_pathv;
    Geom::PathVector closed_pathv;
    Geom::PathVector mix_pathv;
    Geom::PathVector mix_pathv_workon;
    Geom::PathVector orig_pathv = pathv_to_linear_and_cubic_beziers(path_in);
    helper_path = orig_pathv;
    // Store separated open/closed paths
    Geom::PathVector splitter;
    for (auto &i : orig_pathv) {
        if (!Geom::path_direction(i)) {
            i = i.reversed();
        }
        if (i.closed()) {
            closed_pathv.push_back(i);
        } else {
            open_pathv.push_back(i);
        }
    }
    sp_flatten(closed_pathv, fillrule);

    // we flatten using original fill rule
    mix_pathv = open_pathv;
    for (auto path : closed_pathv) {
        mix_pathv.push_back(path);
    }
    SPGroup *group = dynamic_cast<SPGroup *>(sp_lpe_item);
    // Calculate the original pathvector used outside this function
    // to calculate the offset
    if (group) {
        mix_pathv_all.insert(mix_pathv_all.begin(), mix_pathv.begin(), mix_pathv.end());
    } else {
        mix_pathv_all = mix_pathv;
    }
    if (to_offset < 0) {
        Geom::OptRect bbox = mix_pathv.boundsFast();
        if (bbox) {
            (*bbox).expandBy(to_offset / 2.0);
            if ((*bbox).hasZeroArea()) {
                Geom::PathVector empty;
                return empty;
            }
        }
    }
    for (auto pathin : closed_pathv) {
        Geom::OptRect pbbox = pathin.boundsFast();
        // if (pbbox && (*pbbox).minExtent() > to_offset) {
        mix_pathv_workon.push_back(pathin);
        //}
    }
    mix_pathv_workon.insert(mix_pathv_workon.begin(), open_pathv.begin(), open_pathv.end());

    if (offset == 0.0) {
        if (is_load && offset_pt == Geom::Point(Geom::infinity(), Geom::infinity())) {
            offset_pt = get_default_point(path_in);
            if (_knot_entity) {
                _knot_entity->knot_get();
            }
        }
        return path_in;
    }
    Geom::OptRect bbox = closed_pathv.boundsFast();
    double bboxsize = 0;
    if (bbox) {
        bboxsize = (*bbox).maxExtent();
    }
    LineJoinType join = static_cast<LineJoinType>(linejoin_type.get_value());
    Geom::PathVector ret_closed_tmp;
    if (to_offset > 0) {
        for (auto &i : mix_pathv_workon) {
            Geom::Path tmp = half_outline(
                i, to_offset, (attempt_force_join ? std::numeric_limits<double>::max() : miter_limit), join, tolerance);
            if (tmp.closed()) {
                Geom::OptRect pbbox = tmp.boundsFast();
                if (pbbox && (*pbbox).minExtent() > to_offset) {
                    ret_closed_tmp.push_back(tmp);
                }
            } else {
                Geom::Path tmp_b = half_outline(i.reversed(), to_offset,
                                                (attempt_force_join ? std::numeric_limits<double>::max() : miter_limit),
                                                join, tolerance);
                Geom::PathVector switch_pv_a(tmp);
                Geom::PathVector switch_pv_b(tmp_b);
                double distance_b = Geom::distance(offset_pt, get_nearest_point(switch_pv_a, offset_pt));
                double distance_a = Geom::distance(offset_pt, get_nearest_point(switch_pv_b, offset_pt));
                if (distance_b < distance_a) {
                    ret_open.push_back(removeIntersects(tmp));
                } else {
                    ret_open.push_back(removeIntersects(tmp_b));
                }
            }
        }
        sp_flatten(ret_closed_tmp, fill_nonZero);
        for (auto path : ret_closed_tmp) {
            ret_closed.push_back(path);
        }
    } else if (to_offset < 0) {
        for (auto &i : mix_pathv_workon) {
            Geom::Path tmp =
                half_outline(i.reversed(), std::abs(to_offset),
                             (attempt_force_join ? std::numeric_limits<double>::max() : miter_limit), join, tolerance);
            if (i.closed()) {
                Geom::PathVector out(tmp);
                for (auto path : out) {
                    ret_closed.push_back(path);
                }
            } else {
                Geom::Path tmp_b = half_outline(i, std::abs(to_offset),
                                                (attempt_force_join ? std::numeric_limits<double>::max() : miter_limit),
                                                join, tolerance);
                Geom::PathVector switch_pv_a(tmp);
                Geom::PathVector switch_pv_b(tmp_b);
                double distance_b = Geom::distance(offset_pt, get_nearest_point(switch_pv_a, offset_pt));
                double distance_a = Geom::distance(offset_pt, get_nearest_point(switch_pv_b, offset_pt));
                if (distance_b < distance_a) {
                    ret_open.push_back(removeIntersects(tmp));
                } else {
                    ret_open.push_back(removeIntersects(tmp_b));
                }
            }
        }
        if (!ret_closed.empty()) {
            Geom::PathVector outline;
            for (const auto &i : mix_pathv_workon) {
                if (i.closed()) {
                    Geom::PathVector tmp = Inkscape::outline(i, std::abs(to_offset * 2), 4.0, join,
                                                             static_cast<LineCapType>(BUTT_FLAT), tolerance);
                    outline.insert(outline.begin(), tmp.begin(), tmp.end());
                }
            }
            sp_flatten(outline, fill_nonZero);
            ret_closed = sp_pathvector_boolop(outline, ret_closed, bool_op_diff, fill_nonZero, fill_nonZero);
        }
    }
    ret_closed.insert(ret_closed.begin(), ret_open.begin(), ret_open.end());
    return ret_closed;
}

void LPEOffset::addKnotHolderEntities(KnotHolder *knotholder, SPItem *item)
{
    _knot_entity = new OfS::KnotHolderEntityOffsetPoint(this);
    _knot_entity->create(nullptr, item, knotholder, Inkscape::CANVAS_ITEM_CTRL_TYPE_LPE,
                         "LPEOffset", _("Offset point"));
    _knot_entity->knot->setMode(Inkscape::CANVAS_ITEM_CTRL_MODE_COLOR);
    _knot_entity->knot->setFill(0x0088FFFF, 0x4BA1C7FF, 0xCF1410FF, 0x0088FFFF);
    _knot_entity->knot->setStroke(0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF);
    offset_pt = Geom::Point(Geom::infinity(), Geom::infinity());
    knotholder->add(_knot_entity);
}

namespace OfS {
void KnotHolderEntityOffsetPoint::knot_set(Geom::Point const &p, Geom::Point const& /*origin*/, guint state)
{
    using namespace Geom;
    LPEOffset* lpe = dynamic_cast<LPEOffset *>(_effect);
    lpe->refresh_widgets = true;
    Geom::Point s = snap_knot_position(p, state);
    double offset = lpe->sp_get_offset(s);
    lpe->offset_pt = s;
    if (lpe->update_on_knot_move) {
        lpe->liveknot = true;
        lpe->offset.param_set_value(offset);
        sp_lpe_item_update_patheffect (SP_LPE_ITEM(item), false, false);
    } else {
        lpe->liveknot = false;
    }
}

void KnotHolderEntityOffsetPoint::knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state)
{
    LPEOffset *lpe = dynamic_cast<LPEOffset *>(_effect);
    lpe->refresh_widgets = true;
    lpe->liveknot = false;
    using namespace Geom;
    Geom::Point s = lpe->offset_pt;
    double offset = lpe->sp_get_offset(s);
    lpe->offset.param_set_value(offset);
    sp_lpe_item_update_patheffect(SP_LPE_ITEM(item), false, false);
}

Geom::Point KnotHolderEntityOffsetPoint::knot_get() const
{
    LPEOffset *lpe = dynamic_cast<LPEOffset *>(_effect);
    if (!lpe) {
        return Geom::Point();
    }
    if (!lpe->update_on_knot_move) {
        return lpe->offset_pt;
    }
    Geom::Point nearest = lpe->offset_pt;
    if (nearest == Geom::Point(Geom::infinity(), Geom::infinity())) {
        Geom::PathVector out;
        SPGroup *group = dynamic_cast<SPGroup *>(item);
        SPShape *shape = dynamic_cast<SPShape *>(item);
        if (group) {
            std::vector<SPItem *> item_list = sp_item_group_item_list(group);
            for (auto child : item_list) {
                SPShape *subchild = dynamic_cast<SPShape *>(child);
                if (subchild) {
                    Geom::PathVector tmp = subchild->curve()->get_pathvector();
                    out.insert(out.begin(), tmp.begin(), tmp.end());
                    sp_flatten(out, fill_oddEven);
                }
            }
        } else if (shape) {
            SPCurve const *c = shape->curve();
            if (c) {
                out = c->get_pathvector();
            }
        }
        if (!out.empty()) {
            nearest = lpe->get_default_point(out);
        }
    }
    lpe->offset_pt = nearest;
    return lpe->offset_pt;
}

} // namespace OfS
} //namespace LivePathEffect
} /* namespace Inkscape */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

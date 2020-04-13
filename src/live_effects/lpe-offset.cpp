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

#include "live_effects/parameter/enum.h"
#include "live_effects/lpe-offset.h"
#include "display/curve.h"
#include "inkscape.h"
#include "helper/geom.h"
#include "helper/geom-pathstroke.h"
#include <2geom/sbasis-to-bezier.h>
#include <2geom/piecewise.h>
#include <2geom/path-intersection.h>
#include <2geom/intersection-graph.h>
#include <2geom/elliptical-arc.h>
#include <2geom/angle.h>
#include <2geom/curve.h>
#include "object/sp-shape.h"
#include "path/path-boolop.h"
#include "knot-holder-entity.h"
#include "knotholder.h"
#include "util/units.h"
#include "knot.h"
#include <algorithm>
// this is only to flatten nonzero fillrule
#include "livarot/Path.h"
#include "livarot/Shape.h"

#include "svg/svg.h"

#include <2geom/elliptical-arc.h>
// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

namespace OfS {
    class KnotHolderEntityOffsetPoint : public LPEKnotHolderEntity {
    public:
        KnotHolderEntityOffsetPoint(LPEOffset * effect) : LPEKnotHolderEntity(effect) {}
        void knot_set(Geom::Point const &p, Geom::Point const &origin, guint state) override;
        Geom::Point knot_get() const override;
    private:
    };
} // OfS


static const Util::EnumData<unsigned> JoinTypeData[] = {
    {JOIN_BEVEL,       N_("Beveled"),    "bevel"},
    {JOIN_ROUND,       N_("Rounded"),    "round"},
    {JOIN_MITER,       N_("Miter"),      "miter"},
    {JOIN_MITER_CLIP,  N_("Miter Clip"), "miter-clip"},
    {JOIN_EXTRAPOLATE, N_("Extrapolated arc"), "extrp_arc"},
    {JOIN_EXTRAPOLATE1, N_("Extrapolated arc Alt1"), "extrp_arc1"},
    {JOIN_EXTRAPOLATE2, N_("Extrapolated arc Alt2"), "extrp_arc2"},
    {JOIN_EXTRAPOLATE3, N_("Extrapolated arc Alt3"), "extrp_arc3"},
};

static const Util::EnumDataConverter<unsigned> JoinTypeConverter(JoinTypeData, sizeof(JoinTypeData)/sizeof(*JoinTypeData));


LPEOffset::LPEOffset(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    unit(_("Unit"), _("Unit of measurement"), "unit", &wr, this, "mm"),
    offset(_("Offset:"), _("Offset"), "offset", &wr, this, 0.0),
    linejoin_type(_("Join:"), _("Determines the shape of the path's corners"),  "linejoin_type", JoinTypeConverter, &wr, this, JOIN_MITER),
    miter_limit(_("Miter limit:"), _("Maximum length of the miter join (in units of stroke width)"), "miter_limit", &wr, this, 4.0),
    attempt_force_join(_("Force miter"), _("Overrides the miter limit and forces a join."), "attempt_force_join", &wr, this, true),
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
    offset.param_set_digits(4);
    offset_pt = Geom::Point(Geom::infinity(), Geom::infinity());
    _knot_entity = nullptr;
    _provides_knotholder_entities = true;
    apply_to_clippath_and_mask = true;
    prev_unit = unit.get_abbreviation();
}

LPEOffset::~LPEOffset()
= default;

typedef FillRule FillRuleFlatten;

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

Geom::Point 
LPEOffset::get_nearest_point(Geom::PathVector pathv, Geom::Point point)  const
{
    Geom::Point res = Geom::Point(Geom::infinity(), Geom::infinity());
    boost::optional< Geom::PathVectorTime > pathvectortime = pathv.nearestTime(point);
    if (pathvectortime) {
        Geom::PathTime pathtime = pathvectortime->asPathTime();
        res = pathv[(*pathvectortime).path_index].pointAt(pathtime.curve_index + pathtime.t);
    }
    return res;
}

void LPEOffset::transform_multiply(Geom::Affine const &postmul, bool /*set*/)
{
    if (sp_lpe_item && sp_lpe_item->pathEffectsEnabled() && sp_lpe_item->optimizeTransforms()) {
        offset.param_transform_multiply(postmul, true);
        offset_pt = Geom::Point(Geom::infinity(), Geom::infinity());
    }
}

Geom::Point 
LPEOffset::get_default_point(Geom::PathVector pathv)  const
{
    Geom::Point origin = Geom::Point(Geom::infinity(), Geom::infinity());
    Geom::OptRect bbox = pathv.boundsFast();
    if (bbox) {
        if (SP_IS_GROUP(sp_lpe_item)) {
            origin = Geom::Point(boundingbox_X.min(),boundingbox_Y.min());
        } else {
            origin = Geom::Point((*bbox).midpoint()[Geom::X],(*bbox).top());
            origin = get_nearest_point(pathv, origin);
        }
        
    }
    return origin;
}
double 
sp_get_distance_point(Geom::PathVector pathv, Geom::Point origin) {
    boost::optional< Geom::PathVectorTime > pathvectortime = pathv.nearestTime(origin);
    Geom::Point nearest = origin;
    if (pathvectortime) {
        Geom::PathTime pathtime = pathvectortime->asPathTime();
        nearest = pathv[(*pathvectortime).path_index].pointAt(pathtime.curve_index + pathtime.t);
        // we are not on simplet case on a parallel offset
        if (Geom::are_near(pathtime.t, 0) || Geom::are_near(pathtime.t, 1)) {
            bool start = Geom::are_near(nearest, pathv[(*pathvectortime).path_index].initialPoint());
            bool end = Geom::are_near(nearest, pathv[(*pathvectortime).path_index].finalPoint());
            bool closed = Geom::are_near(pathv[(*pathvectortime).path_index].initialPoint(),
                                         pathv[(*pathvectortime).path_index].finalPoint());
            double dist_corner = 0;
            Geom::Point pa = pathv[(*pathvectortime).path_index].pointAt(pathtime.curve_index + 1);
            Geom::Point pb = pathv[(*pathvectortime).path_index].pointAt(pathtime.curve_index);
            if (closed && start) {
                pa = pathv[(*pathvectortime).path_index].pointAt(pathtime.curve_index);
                pb = pathv[(*pathvectortime).path_index].pointAt(pathtime.curve_index + 1);
            }
            if (!closed && (start || end)) {
                if (start) {
                    pa = pathv[(*pathvectortime).path_index].pointAt(pathtime.curve_index);
                    pb = pathv[(*pathvectortime).path_index].pointAt(pathtime.curve_index + 1);
                }
            }
            Geom::Line line;
            line.setPoints(pa, pb);
            line.setAngle(line.angle() + Geom::rad_from_deg(90));
            Geom::Point nearestline = line.pointAt(line.nearestTime(origin));
            boost::optional<Geom::PathVectorTime> pathvectortime_line = pathv.nearestTime(nearestline);
            if (pathvectortime_line) {
                Geom::PathTime pathtime_line = pathvectortime_line->asPathTime();
                nearest = pathv[(*pathvectortime_line).path_index].pointAt(pathtime_line.curve_index + pathtime_line.t);
                dist_corner = Geom::distance(nearestline, nearest);
            }
            if (closed || !(start || end)) {
                if (closed && start) {
                    size_t sizepath = pathv[(*pathvectortime).path_index].size();
                    pa = pathv[(*pathvectortime).path_index].pointAt(sizepath - 1);
                    pb = pathv[(*pathvectortime).path_index].pointAt(sizepath - 2);
                } else {
                    pa = pathv[(*pathvectortime).path_index].pointAt(pathtime.curve_index + 1);
                    pb = pathv[(*pathvectortime).path_index].pointAt(pathtime.curve_index + 2);
                }
                line;
                line.setPoints(pa, pb);
                line.setAngle(line.angle() + Geom::rad_from_deg(90));
                Geom::Point nearestline = line.pointAt(line.nearestTime(origin));
                boost::optional<Geom::PathVectorTime> pathvectortime_line = pathv.nearestTime(nearestline);
                if (pathvectortime_line) {
                    Geom::PathTime pathtime_line = pathvectortime_line->asPathTime();
                    nearest =
                        pathv[(*pathvectortime_line).path_index].pointAt(pathtime_line.curve_index + pathtime_line.t);
                    return std::max(dist_corner, Geom::distance(nearestline, nearest));
                }
            }
        }
    }
    return Geom::distance(origin, nearest);
}

double
LPEOffset::sp_get_offset(Geom::Point origin)
{
    SPGroup * group = dynamic_cast<SPGroup *>(sp_lpe_item);
    double ret_offset = 0;
    if (group) {
        Geom::Point initial = get_default_point(filled_rule_pathv);
        ret_offset = Geom::distance(origin, initial);
        if (origin[Geom::Y] < initial[Geom::Y]) {
            ret_offset *= -1;
        }
        return Inkscape::Util::Quantity::convert(ret_offset, "px", unit.get_abbreviation()) * this->scale;
    }
    int winding_value = filled_rule_pathv.winding(origin); 
    bool inset = false;
    if (winding_value % 2 != 0) {
        inset = true;
    }
    
    ret_offset = sp_get_distance_point(filled_rule_pathv, origin);
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
    original_bbox(lpeitem);
    SPDocument *document = getSPDoc();
    if (!document) {
        return;
    }
    if (prev_unit != unit.get_abbreviation()) {
        offset.param_set_value(Inkscape::Util::Quantity::convert(offset, prev_unit, unit.get_abbreviation()));
    }
    prev_unit = unit.get_abbreviation();
    SPGroup const *group = dynamic_cast<SPGroup const *>(lpeitem);
    this->scale = lpeitem->i2doc_affine().descrim();
    if (group) {
        helper_path.clear();
        Geom::Point origin = Geom::Point(boundingbox_X.min(), boundingbox_Y.min());
        Geom::Point endpont = Geom::Point(boundingbox_X.min(), boundingbox_Y.min());
        endpont[Geom::Y] = endpont[Geom::Y] + Inkscape::Util::Quantity::convert(offset, unit.get_abbreviation(), "px")/ this->scale;
        Geom::Path hp(origin);
        hp.appendNew<Geom::LineSegment>(endpont);
        helper_path.push_back(hp);
    }
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

/* Geom::Path 
sp_get_outer(Geom::PathVector pathv) {
    sp_flatten(pathv, fill_nonZero);
    Geom::Path out_bounds;
    Geom::Path out_size;
    Geom::OptRect bounds;
    double size = 0;
    bool get_bounds = false;
    for (auto path_child:pathv) {
        Geom::OptRect path_bounds = path_child.boundsFast();
        if (path_bounds) {
            double path_size = (*path_bounds).width() + (*path_bounds).height();
            if (path_bounds.contains(bounds)) {
                bounds = path_bounds;
                out_bounds = path_child;
                get_bounds = true;
            }
            if (size < path_size) {
                size = path_size;
                out_size = path_child;
            }
        }
    }
    pathv.clear();
    if (get_bounds) {
        return out_bounds;
    }
    return out_size;
}

Geom::PathVector 
sp_get_inner(Geom::PathVector pathv, Geom::Path outer) {
    Geom::PathVector out;
    for (auto path_child:pathv) {
        if (path_child != outer) {
            out.push_back(path_child);
        }
    }
    return out;
} */

Geom::PathVector 
LPEOffset::doEffect_path(Geom::PathVector const & path_in)
{
    SPItem * item = SP_ITEM(current_shape);
    if (!item) {
        return path_in;
    }
    SPCSSAttr *css;
    const gchar *val;
    css = sp_repr_css_attr (item->getRepr() , "style");
    val = sp_repr_css_property (css, "fill-rule", nullptr);
    FillRuleFlatten fillrule = fill_nonZero;
    if (val && strcmp (val, "evenodd") == 0)
    {
        fillrule = fill_oddEven;
    }
    Geom::PathVector original_pathv = pathv_to_linear_and_cubic_beziers(path_in);
    filled_rule_pathv = original_pathv;
    sp_flatten(filled_rule_pathv, fillrule);
    if (offset == 0.0) {
        return path_in;
    }
    Geom::PathVector ret;
    Geom::PathVector open_ret;
    Geom::PathVector ret_outline;
    for (Geom::PathVector::const_iterator path_it = filled_rule_pathv.begin(); path_it != filled_rule_pathv.end(); ++path_it) {
        Geom::Path original = (*path_it);
        if (original.empty()) {
            continue;
        }
        int wdg = offset_winding(filled_rule_pathv, original);
        bool path_inside = wdg % 2 != 0;
        double gap_size = -0.01;
        bool closed = original.closed();
        double to_offset =
            Inkscape::Util::Quantity::convert(std::abs(offset), unit.get_abbreviation(), "px") / this->scale;
        if (to_offset <= 0.01) {
            return path_in;
        }
        Geom::OptRect original_bounds = original.boundsFast();
        double original_height = 0;
        double original_width = 0;
        if (original_bounds) {
            original_height = (*original_bounds).height();
            original_width = (*original_bounds).width();
        }
        if (path_inside && (offset * 2 > (original_height + original_width) / 2.0)) {
            continue;
        }
        Geom::Path with_dir = half_outline(original, 
                                to_offset,
                                (attempt_force_join ? std::numeric_limits<double>::max() : miter_limit),
                                static_cast<LineJoinType>(linejoin_type.get_value()),
                                static_cast<LineCapType>(BUTT_FLAT));
        Geom::Path against_dir = half_outline(original.reversed(), 
                                to_offset,
                                (attempt_force_join ? std::numeric_limits<double>::max() : miter_limit),
                                static_cast<LineJoinType>(linejoin_type.get_value()),
                                static_cast<LineCapType>(BUTT_FLAT));
        Geom::Path with_dir_gap =
            half_outline(original, std::abs(to_offset + gap_size),
                         (attempt_force_join ? std::numeric_limits<double>::max() : miter_limit),
                         static_cast<LineJoinType>(linejoin_type.get_value()), static_cast<LineCapType>(BUTT_FLAT));
        Geom::Path against_dir_gap =
            half_outline(original.reversed(), std::abs(to_offset + gap_size),
                         (attempt_force_join ? std::numeric_limits<double>::max() : miter_limit),
                         static_cast<LineJoinType>(linejoin_type.get_value()), static_cast<LineCapType>(BUTT_FLAT));
        bool reversed = false;
        Geom::OptRect against_dir_bounds = against_dir.boundsFast();
        Geom::OptRect with_dir_bounds = with_dir.boundsFast();
        double with_dir_height = 0;
        double against_dir_height = 0;
        double with_dir_width = 0;
        double against_dir_width = 0;
        if (with_dir_bounds) {
            with_dir_height = (*with_dir_bounds).height();
            with_dir_width = (*with_dir_bounds).width();
        }
        if (against_dir_bounds) {
            against_dir_height = (*against_dir_bounds).height();
            against_dir_width = (*against_dir_bounds).width();
        }
        reversed = against_dir_bounds.contains(with_dir_bounds) == false;
        // We can have a strange result for the bounding box container
        // Gives a wrong result, in theory, it happens sometimes on expand offset
        if (offset > 0 &&
            ((original_width  < against_dir_width &&
              original_width  < with_dir_width) ||
             (original_height < against_dir_height &&
              original_height < with_dir_height)))

        {
            Geom::Path with_dir_size = half_outline(original, 
                                    2,
                                    (attempt_force_join ? std::numeric_limits<double>::max() : miter_limit),
                                    static_cast<LineJoinType>(linejoin_type.get_value()),
                                    static_cast<LineCapType>(BUTT_FLAT));
            Geom::Path against_dir_size = half_outline(original.reversed(), 
                                    2,
                                    (attempt_force_join ? std::numeric_limits<double>::max() : miter_limit),
                                    static_cast<LineJoinType>(linejoin_type.get_value()),
                                    static_cast<LineCapType>(BUTT_FLAT));
        
            Geom::OptRect against_dir_size_bounds = against_dir_size.boundsFast();
            Geom::OptRect with_dir_size_bounds = with_dir_size.boundsFast();
            reversed = against_dir_size_bounds.contains(with_dir_size_bounds) == false;
        }
        Geom::PathVector tmp;
        Geom::PathVector outline;
        Geom::Path big;
        Geom::Path gap;
        Geom::Path small;
        if (offset < 0) {
            outline.push_back(with_dir);
            outline.push_back(against_dir);
            sp_flatten(outline, fill_nonZero);
        }
        if (reversed || !closed) {
            big = with_dir;
            gap   = with_dir_gap;
            small = against_dir;
        } else {
            big  = against_dir;
            gap = against_dir_gap;
            small = with_dir;
        }
        //big = sp_get_outer(big);
        //gap = sp_get_outer(gap);

        if (!closed) {
            tmp.push_back(small);
            double smalldist = sp_get_distance_point(tmp, offset_pt);
            tmp.clear();
            tmp.push_back(big);
            double bigdist = sp_get_distance_point(tmp, offset_pt);
            tmp.clear();
            if (bigdist > smalldist) {
                open_ret.push_back(small);
            } else {
                open_ret.push_back(big);
            }
            continue;
        }
        bool fix_reverse = (original_width + original_height) / 2.0  > to_offset * 2;
        if (offset < 0) {
            tmp.push_back(gap);
        } else {
            if (path_inside) {
                if (fix_reverse) {
                    tmp.push_back(small);
                }
            } else {
                tmp.push_back(big);
            } 
        }
        ret.insert(ret.end(), tmp.begin(), tmp.end());
        if (offset < 0) {
            ret_outline.insert(ret_outline.end(), outline.begin(), outline.end());
        }
    }

    if (offset < 0) {
        sp_flatten(ret_outline, fill_nonZero);
        if (!ret_outline.empty() && !ret.empty()) {
            ret = sp_pathvector_boolop(ret_outline, ret, bool_op_diff, fill_nonZero, fill_oddEven);
        }
    }

    sp_flatten(ret, fill_nonZero);
    ret.insert(ret.end(), open_ret.begin(), open_ret.end());
    
    if (offset_pt == Geom::Point(Geom::infinity(), Geom::infinity())) {
        offset_pt = get_default_point(ret);
    }
    return ret;
}

void LPEOffset::addKnotHolderEntities(KnotHolder *knotholder, SPItem *item)
{
    _knot_entity = new OfS::KnotHolderEntityOffsetPoint(this);
    _knot_entity->create(nullptr, item, knotholder, Inkscape::CTRL_TYPE_LPE, _("Offset point"), SP_KNOT_SHAPE_CIRCLE);
    knotholder->add(_knot_entity);
}

namespace OfS {
void KnotHolderEntityOffsetPoint::knot_set(Geom::Point const &p, Geom::Point const& /*origin*/, guint state)
{
    using namespace Geom;
    SPGroup * group = dynamic_cast<SPGroup *>(item);
    LPEOffset* lpe = dynamic_cast<LPEOffset *>(_effect);
    Geom::Point s = snap_knot_position(p, state);
    if (group) {
        s[Geom::X] = lpe->boundingbox_X.min();
    }
    double offset = lpe->sp_get_offset(s);
    lpe->offset_pt = s;
    lpe->offset.param_set_value(offset);
    if (lpe->update_on_knot_move) {
        sp_lpe_item_update_patheffect (SP_LPE_ITEM(item), false, false);
    }
}

Geom::Point KnotHolderEntityOffsetPoint::knot_get() const
{
    SPGroup * group = dynamic_cast<SPGroup *>(item);
    LPEOffset * lpe = dynamic_cast<LPEOffset *> (_effect);
    if (!lpe->update_on_knot_move) {
        return lpe->offset_pt;
    }
    Geom::Point nearest = lpe->offset_pt;
    Geom::PathVector out = SP_SHAPE(item)->getCurve(true)->get_pathvector();
    if (lpe->offset_pt == Geom::Point(Geom::infinity(), Geom::infinity())) {
        if (group) {
            nearest = Geom::Point(lpe->boundingbox_X.min(), lpe->boundingbox_Y.min());
        } else {
            nearest = lpe->get_default_point(out);
        }
    }
    boost::optional<Geom::PathVectorTime> pathvectortime = out.nearestTime(nearest);
    if (pathvectortime) {
        Geom::PathTime pathtime = pathvectortime->asPathTime();
        nearest = out[(*pathvectortime).path_index].pointAt(pathtime.curve_index + pathtime.t);
    }
    return nearest;
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

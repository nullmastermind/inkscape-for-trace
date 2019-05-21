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
        void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override;
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
    offset(_("Offset:"), _("Offset)"), "offset", &wr, this, 0.0),
    linejoin_type(_("Join:"), _("Determines the shape of the path's corners"),  "linejoin_type", JoinTypeConverter, &wr, this, JOIN_ROUND),
    miter_limit(_("Miter limit:"), _("Maximum length of the miter join (in units of stroke width)"), "miter_limit", &wr, this, 4.0),
    attempt_force_join(_("Force miter"), _("Overrides the miter limit and forces a join."), "attempt_force_join", &wr, this, true),
    update_on_knot_move(_("Update on knot move"), _("Update on knot move"), "update_on_knot_move", &wr, this, true)
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
    evenodd = true;
    _knot_entity = nullptr;
    _provides_knotholder_entities = true;
    apply_to_clippath_and_mask = true;
}

LPEOffset::~LPEOffset()
= default;

static void
sp_flatten(Geom::PathVector &pathvector, bool evenodd)
{
    Path *orig = new Path;
    orig->LoadPathVector(pathvector);
    Shape *theShape = new Shape;
    Shape *theRes = new Shape;
    orig->ConvertWithBackData (1.0);
    orig->Fill (theShape, 0);
    if (evenodd) {
        theRes->ConvertToShape (theShape, fill_oddEven);
    } else {
        theRes->ConvertToShape (theShape, fill_nonZero);
    }
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

Geom::Point 
LPEOffset::get_default_point(Geom::PathVector pathv)  const
{
    if (SP_IS_GROUP(sp_lpe_item)) {
        return Geom::Point(Geom::infinity(), Geom::infinity());
    }
    Geom::Point origin = Geom::Point(Geom::infinity(), Geom::infinity());
    Geom::OptRect bbox = pathv.boundsFast();
    if (bbox) {
        origin = Geom::Point((*bbox).midpoint()[Geom::X],(*bbox).top());
        origin = get_nearest_point(pathv, origin);
    }
    return origin;
}

double
LPEOffset::sp_get_offset(Geom::Point origin)
{
    int winding_value = filled_rule_pathv.winding(origin); 
    bool inset = false;
    if (winding_value % 2 != 0) {
        inset = true;
    }
    double ret_offset = 0;
    boost::optional< Geom::PathVectorTime > pathvectortime = filled_rule_pathv.nearestTime(origin);
    Geom::Point nearest = origin;
    double distance = 0;
    if (pathvectortime) {
        Geom::PathTime pathtime = pathvectortime->asPathTime();
        nearest = filled_rule_pathv[(*pathvectortime).path_index].pointAt(pathtime.curve_index + pathtime.t);
    }
    ret_offset = Geom::distance(origin, nearest);
    if (inset) {
        ret_offset *= -1;
    }
    return Inkscape::Util::Quantity::convert(ret_offset, display_unit.c_str(), unit.get_abbreviation());
}

void
LPEOffset::doBeforeEffect (SPLPEItem const* lpeitem)
{
    original_bbox(lpeitem);
    SPDocument * document = SP_ACTIVE_DOCUMENT;
    if (!document) {
        return;
    }
    display_unit = document->getDisplayUnit()->abbr.c_str();
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

Geom::Path 
sp_get_outer(Geom::Path path) {
    Geom::PathVector pathv;
    pathv.push_back(path);
    sp_flatten(pathv, true);
    Geom::Path out = pathv[0];
    Geom::OptRect bounds = out.boundsFast();
    for (auto path_child:pathv) {
        Geom::OptRect path_bounds = path_child.boundsFast();
        if (path_bounds.contains(bounds)) {
            bounds = path_bounds;
            out = path_child;
        }
    }
    pathv.clear();
    return out;
}

Geom::PathVector 
LPEOffset::doEffect_path(Geom::PathVector const & path_in)
{
    Geom::PathVector original_pathv = pathv_to_linear_and_cubic_beziers(path_in);
    filled_rule_pathv = original_pathv;
    sp_flatten(filled_rule_pathv, evenodd);
    if (offset == 0.0) {
        return path_in;
    }
    SPItem * item = SP_ITEM(current_shape);
    if (!item) {
        return path_in;
    }
    SPCSSAttr *css;
    const gchar *val;
    css = sp_repr_css_attr (item->getRepr() , "style");
    val = sp_repr_css_property (css, "fill-rule", nullptr);
    bool upd_fill_rule = false;
    if (val && strcmp (val, "nonzero") == 0)
    {
        if (evenodd == true) {
            upd_fill_rule = true;
        }
        evenodd = false;
    }
    else if (val && strcmp (val, "evenodd") == 0)
    {
        if (evenodd == false) {
            upd_fill_rule = true;
        }
        evenodd = true;
    }
    Geom::PathVector work;
    Geom::PathVector ret;
    Geom::PathVector ret_outline;
    Geom::PathIntersectionGraph *pig;
    for (Geom::PathVector::const_iterator path_it = original_pathv.begin(); path_it != original_pathv.end(); ++path_it) {
        Geom::Path original = (*path_it);
        if (original.empty()) {
            continue;
        }
        bool added = false;
        for (auto cross:Geom::self_crossings(original)) {
            if (!Geom::are_near(cross.ta, cross.tb)) {
                Geom::PathVector tmp;
                tmp.push_back(original);
                sp_flatten(tmp, false);
                work.insert(work.begin(), tmp.begin(), tmp.end());
                added = true;
                break;
            }
        } 
        if (!added) {
            work.push_back(original);
        } 
    }
    for (Geom::PathVector::const_iterator path_it = work.begin(); path_it != work.end(); ++path_it) {
        Geom::Path original = (*path_it);
        if (original.empty()) {
            continue;
        }
        int wdg = offset_winding(work, original);
        bool path_inside = wdg % 2 != 0;
        double gap_size = -0.5;
        bool closed = original.closed();
        double to_offset = Inkscape::Util::Quantity::convert(std::abs(offset), unit.get_abbreviation(), display_unit.c_str());
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
        Geom::Path with_dir_gap = half_outline(original, 
                                to_offset + gap_size,
                                (attempt_force_join ? std::numeric_limits<double>::max() : miter_limit),
                                static_cast<LineJoinType>(linejoin_type.get_value()),
                                static_cast<LineCapType>(BUTT_FLAT));
        Geom::Path against_dir_gap = half_outline(original.reversed(), 
                                to_offset + gap_size,
                                (attempt_force_join ? std::numeric_limits<double>::max() : miter_limit),
                                static_cast<LineJoinType>(linejoin_type.get_value()),
                                static_cast<LineCapType>(BUTT_FLAT));
    
        Geom::PathVector tmp;
        Geom::PathVector outline;
        Geom::OptRect original_bounds = original.boundsFast();
        Geom::Path big;
        Geom::Path gap;
        Geom::Path small;
        Geom::OptRect against_dir_bounds = against_dir.boundsFast();
        Geom::OptRect against_dir_bounds_gap = against_dir_gap.boundsFast();
        Geom::OptRect with_dir_bounds = with_dir.boundsFast();
        Geom::OptRect with_dir_bounds_gap = with_dir_gap.boundsFast();
        if (against_dir_bounds.contains(with_dir_bounds)) {
            big  = against_dir;
            gap = against_dir_gap;
            small = with_dir;
        } else {
            big = with_dir;
            gap   = with_dir_gap;
            small = against_dir;
        }
        sp_get_outer(big);
        sp_get_outer(gap);
        outline.push_back(with_dir);
        outline.push_back(against_dir);
        tmp.clear();
        if (!closed) {
            if (offset < 0) {
                ret.push_back(small);
                return ret;
            } else {
                ret.push_back(big);
                return ret;
            }
        }
        bool fix_reverse = ((*original_bounds).width() + (*original_bounds).height()) / 2.0  > to_offset * 2;
        if (offset < 0) {
            if (fix_reverse) {
                tmp.push_back(gap);
            }
        } else {
            if (path_inside) {
                if (fix_reverse) {
                    tmp.push_back(small);
                }
            } else {
                tmp.push_back(big);
            } 
        }
        if (path_inside) {
            outline.clear();
            outline.push_back(big);
        } else {
            sp_flatten(outline, false);
        }
        ret.insert(ret.end(), tmp.begin(), tmp.end());
        ret_outline.insert(ret_outline.end(), outline.begin(), outline.end());
    }
    sp_flatten(ret_outline, false);
    if (offset < 0) {
        pig = new Geom::PathIntersectionGraph(ret, ret_outline);
        if (pig && !ret_outline.empty() && !ret.empty()) {
            ret = pig->getAminusB();
        }
    }
    sp_flatten(ret, false);
    if (offset_pt == Geom::Point(Geom::infinity(), Geom::infinity())) {
        offset_pt = get_default_point(ret);
    }
    return ret;
}

void LPEOffset::addKnotHolderEntities(KnotHolder *knotholder, SPItem *item)
{
    _knot_entity = new OfS::KnotHolderEntityOffsetPoint(this);
    _knot_entity->create(nullptr, item, knotholder, Inkscape::CTRL_TYPE_UNKNOWN, _("Offset point"), SP_KNOT_SHAPE_CIRCLE);
    knotholder->add(_knot_entity);
}


namespace OfS {
void KnotHolderEntityOffsetPoint::knot_ungrabbed(Geom::Point const &p, Geom::Point const& /*origin*/, guint state)
{
    LPEOffset* lpe = dynamic_cast<LPEOffset *>(_effect);
    lpe->upd_params = true;
}

void KnotHolderEntityOffsetPoint::knot_set(Geom::Point const &p, Geom::Point const& /*origin*/, guint state)
{
    using namespace Geom;
    if (SP_IS_GROUP(item)) {
        return;
    }
    LPEOffset* lpe = dynamic_cast<LPEOffset *>(_effect);
    Geom::Point s = snap_knot_position(p, state);
    double offset = lpe->sp_get_offset(s);
    lpe->offset_pt = s;
    lpe->offset.param_set_value(offset);
    
    if (lpe->update_on_knot_move) {
        sp_lpe_item_update_patheffect (SP_LPE_ITEM(item), false, false);
    }
}

Geom::Point KnotHolderEntityOffsetPoint::knot_get() const
{
    if (SP_IS_GROUP(item)) {
        return Geom::Point(Geom::infinity(), Geom::infinity());
    }
    LPEOffset const * lpe = dynamic_cast<LPEOffset const*> (_effect);
    Geom::Point nearest = lpe->offset_pt;
    if (lpe->offset_pt == Geom::Point(Geom::infinity(), Geom::infinity())) {
        Geom::PathVector out = SP_SHAPE(item)->getCurve()->get_pathvector();
        nearest = lpe->get_default_point(out);
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

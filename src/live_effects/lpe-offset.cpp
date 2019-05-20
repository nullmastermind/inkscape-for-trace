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
    offset(_("Offset:"), _("Offset)"), "offset", &wr, this, 0.0),
    linejoin_type(_("Join:"), _("Determines the shape of the path's corners"),  "linejoin_type", JoinTypeConverter, &wr, this, JOIN_ROUND),
    miter_limit(_("Miter limit:"), _("Maximum length of the miter join (in units of stroke width)"), "miter_limit", &wr, this, 4.0),
    attempt_force_join(_("Force miter"), _("Overrides the miter limit and forces a join."), "attempt_force_join", &wr, this, true),
    update_on_knot_move(_("Update on knot move"), _("Update on knot move"), "update_on_knot_move", &wr, this, true)
{
    show_orig_path = true;
    registerParameter(&linejoin_type);
    registerParameter(&offset);
    registerParameter(&miter_limit);
    registerParameter(&attempt_force_join);
    registerParameter(&update_on_knot_move);
    offset.param_set_increments(0.1, 0.1);
    offset.param_set_digits(4);
    offset_pt = Geom::Point(Geom::infinity(), Geom::infinity());
    origin = Geom::Point();
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

double
LPEOffset::sp_get_offset(Geom::Point &origin)
{
    int winding_value = filled_rule_pathv.winding(origin); 
    bool inset = false;
    if (winding_value % 2 != 0) {
        inset = true;
    }
    double offset = 0;
    boost::optional< Geom::PathVectorTime > pathvectortime = filled_rule_pathv.nearestTime(origin);
    Geom::Point nearest = origin;
    double distance = 0;
    if (pathvectortime) {
        Geom::PathTime pathtime = pathvectortime->asPathTime();
        nearest = filled_rule_pathv[(*pathvectortime).path_index].pointAt(pathtime.curve_index + pathtime.t);
    }
    offset = Geom::distance(origin, nearest);
    if (inset) {
        offset *= -1;
    }
    return offset;
}

void
LPEOffset::doOnApply(SPLPEItem const* lpeitem)
{
    if (!SP_IS_SHAPE(lpeitem)) {
        g_warning("LPE measure line can only be applied to shapes (not groups).");
        SPLPEItem * item = const_cast<SPLPEItem*>(lpeitem);
        item->removeCurrentPathEffect(false);
        return;
    }
}

void
LPEOffset::doBeforeEffect (SPLPEItem const* lpeitem)
{
    if (!hp.empty()) {
        hp.clear();
    }
    original_bbox(lpeitem);
    origin = Geom::Point(boundingbox_X.middle(), boundingbox_Y.min());
    SPItem * item = SP_ITEM(sp_lpe_item);
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
    original_pathv = pathv_to_linear_and_cubic_beziers(pathvector_before_effect);
    filled_rule_pathv = original_pathv;
    sp_flatten(filled_rule_pathv, evenodd);
    origin = offset_pt;
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

Geom::PathVector 
LPEOffset::doEffect_path(Geom::PathVector const & path_in)
{
    if (offset == 0.0) {
        offset_pt = original_pathv[0].initialPoint();
        return original_pathv;
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
            if (cross.ta != cross.tb) {
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
        double gap = path_inside ? 0.05 :-0.05;
        Geom::PathVector outline_gap = Inkscape::outline(original, std::abs(offset) * 2 + gap , 
                               (attempt_force_join ? std::numeric_limits<double>::max() : miter_limit),
                                static_cast<LineJoinType>(linejoin_type.get_value()),
                                static_cast<LineCapType>(BUTT_FLAT));
        Geom::PathVector outline = Inkscape::outline(original, std::abs(offset) * 2 , 
                               (attempt_force_join ? std::numeric_limits<double>::max() : miter_limit),
                                static_cast<LineJoinType>(linejoin_type.get_value()),
                                static_cast<LineCapType>(BUTT_FLAT));
        if (outline.size() < 2) {
            continue;
        }
        Geom::PathVector big_gap_ret;
        Geom::PathVector big;
        big.push_back(outline[0]);
        Geom::PathVector big_gap;
        big_gap.push_back(outline_gap[0]);
        Geom::PathVector small;
        small.insert(small.end(), outline.begin() + 1, outline.end());
        Geom::PathVector small_gap;
        small_gap.insert(small_gap.end(), outline_gap.begin() + 1, outline_gap.end());
        Geom::OptRect big_bounds = big.boundsFast();
        Geom::OptRect small_bounds = small.boundsFast();
        bool reversed = small_bounds.contains(big_bounds);
        if (reversed) {
            big.clear();
            big.insert(big.end(), outline.begin() + 1, outline.end());
            big_gap.clear();
            big_gap.insert(big_gap.end(), outline_gap.begin() + 1, outline_gap.end());
            small.clear();
            small.push_back(outline[0]);
            small_gap.clear();
            small_gap.push_back(outline[0]);
        }
        big_bounds = big.boundsFast();
        small_bounds = small.boundsFast();
        Geom::OptRect big_gap_bounds = big.boundsFast();
        Geom::OptRect small_gap_bounds = small.boundsFast();
        Geom::OptRect original_bounds = original.boundsFast();
        if (offset < 0) {
            if (((*original_bounds).width() + (*original_bounds).height()) / 2.0 > std::abs(offset) * 2) {
                big_gap_ret.insert(big_gap_ret.end(), big_gap.begin(), big_gap.end());
            }
        } else {
            if (path_inside) {
                if (((*original_bounds).width() + (*original_bounds).height()) / 2.0 > offset * 2) {
                    big_gap_ret.insert(big_gap_ret.end(),small_gap.begin(), small_gap.end());
                }
            } else {
                big_gap_ret.insert(big_gap_ret.end(), big_gap.begin(), big_gap.end());
            } 
        }

        if (path_inside) {
            Geom::OptRect bounds;
            Geom::Path tmp = outline[0];
            for (auto path:outline) {
                Geom::OptRect path_bounds = path.boundsFast();
                if (path_bounds.contains(bounds)) {
                    tmp = path;
                }
                bounds = path_bounds;
            }
            outline.clear();
            outline.push_back(tmp);
        } else {
            sp_flatten(outline, false);
        }
        ret.insert(ret.end(), big_gap_ret.begin(),big_gap_ret.end());
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
    return ret;
}

void
LPEOffset::addCanvasIndicators(SPLPEItem const */*lpeitem*/, std::vector<Geom::PathVector> &hp_vec)
{
    hp_vec.push_back(hp);
}

void
LPEOffset::drawHandle(Geom::Point p)
{
    double r = 3.0;
    char const * svgd;
    svgd = "M 0.7,0.35 A 0.35,0.35 0 0 1 0.35,0.7 0.35,0.35 0 0 1 0,0.35 0.35,0.35 0 0 1 0.35,0 0.35,0.35 0 0 1 0.7,0.35 Z";
    Geom::PathVector pathv = sp_svg_read_pathv(svgd);
    pathv *= Geom::Scale(r) * Geom::Translate(p - Geom::Point(0.35*r,0.35*r));
    hp.push_back(pathv[0]);
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
    LPEOffset const * lpe = dynamic_cast<LPEOffset const*> (_effect);
    if (lpe->offset_pt == Geom::Point(Geom::infinity(), Geom::infinity())) {
        if(boost::optional< Geom::Point > offset_point = SP_SHAPE(item)->getCurve()->first_point()) {
            return *offset_point;
        }
    }
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

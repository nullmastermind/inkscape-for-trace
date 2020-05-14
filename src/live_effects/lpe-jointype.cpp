// SPDX-License-Identifier: GPL-2.0-or-later
/* Authors:
 *
 *	 Liam P White
 *
 * Copyright (C) 2014 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/parameter/enum.h"
#include "live_effects/fill-conversion.h"
#include "helper/geom-pathstroke.h"

#include "desktop-style.h"

#include "display/curve.h"

#include "object/sp-item-group.h"
#include "object/sp-shape.h"
#include "style.h"

#include "svg/css-ostringstream.h"
#include "svg/svg-color.h"

#include <2geom/elliptical-arc.h>

#include "lpe-jointype.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

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

static const Util::EnumData<unsigned> CapTypeData[] = {
    {BUTT_FLAT, N_("Butt"), "butt"},
    {BUTT_ROUND, N_("Rounded"), "round"},
    {BUTT_SQUARE, N_("Square"), "square"},
    {BUTT_PEAK, N_("Peak"), "peak"},
    //{BUTT_LEANED, N_("Leaned"), "leaned"}
};

static const Util::EnumDataConverter<unsigned> CapTypeConverter(CapTypeData, sizeof(CapTypeData)/sizeof(*CapTypeData));
static const Util::EnumDataConverter<unsigned> JoinTypeConverter(JoinTypeData, sizeof(JoinTypeData)/sizeof(*JoinTypeData));

LPEJoinType::LPEJoinType(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    line_width(_("Line width"), _("Thickness of the stroke"), "line_width", &wr, this, 1.),
    linecap_type(_("Line cap"), _("The end shape of the stroke"), "linecap_type", CapTypeConverter, &wr, this, BUTT_FLAT),
    linejoin_type(_("Join:"), _("Determines the shape of the path's corners"),  "linejoin_type", JoinTypeConverter, &wr, this, JOIN_EXTRAPOLATE),
    //start_lean(_("Start path lean"), _("Start path lean"), "start_lean", &wr, this, 0.),
    //end_lean(_("End path lean"), _("End path lean"), "end_lean", &wr, this, 0.),
    miter_limit(_("Miter limit:"), _("Maximum length of the miter join (in units of stroke width)"), "miter_limit", &wr, this, 100.),
    attempt_force_join(_("Force miter"), _("Overrides the miter limit and forces a join."), "attempt_force_join", &wr, this, true)
{
    show_orig_path = true;
    registerParameter(&linecap_type);
    registerParameter(&line_width);
    registerParameter(&linejoin_type);
    //registerParameter(&start_lean);
    //registerParameter(&end_lean);
    registerParameter(&miter_limit);
    registerParameter(&attempt_force_join);
    //start_lean.param_set_range(-1,1);
    //start_lean.param_set_increments(0.1, 0.1);
    //start_lean.param_set_digits(4);
    //end_lean.param_set_range(-1,1);
    //end_lean.param_set_increments(0.1, 0.1);
    //end_lean.param_set_digits(4);
}

LPEJoinType::~LPEJoinType()
= default;

void LPEJoinType::doOnApply(SPLPEItem const* lpeitem)
{
    if (!SP_IS_SHAPE(lpeitem)) {
        return;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    SPShape* item = SP_SHAPE(lpeitem);
    double width = (lpeitem && lpeitem->style) ? lpeitem->style->stroke_width.computed : 1.;

    lpe_shape_convert_stroke_and_fill(item);

    Glib::ustring pref_path = (Glib::ustring)"/live_effects/" +
                                    (Glib::ustring)LPETypeConverter.get_key(effectType()).c_str() +
                                    (Glib::ustring)"/" + 
                                    (Glib::ustring)"line_width";

    bool valid = prefs->getEntry(pref_path).isValid();

    if (!valid) {
        line_width.param_set_value(width);
    }

    line_width.write_to_SVG();
}

void LPEJoinType::transform_multiply(Geom::Affine const &postmul, bool /*set*/)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool transform_stroke = prefs ? prefs->getBool("/options/transform/stroke", true) : true;
    if (transform_stroke) {
        line_width.param_transform_multiply(postmul, false);
    }
}

//from LPEPowerStroke -- sets stroke color from existing fill color

void LPEJoinType::doOnRemove(SPLPEItem const* lpeitem)
{
    if (!SP_IS_SHAPE(lpeitem)) {
        return;
    }

    lpe_shape_revert_stroke_and_fill(SP_SHAPE(lpeitem), line_width);
}

Geom::PathVector LPEJoinType::doEffect_path(Geom::PathVector const & path_in)
{
    Geom::PathVector ret;
    for (const auto & i : path_in) {
        Geom::PathVector tmp = Inkscape::outline(i, line_width, 
                                                 (attempt_force_join ? std::numeric_limits<double>::max() : miter_limit),
                                                 static_cast<LineJoinType>(linejoin_type.get_value()),
                                                 static_cast<LineCapType>(linecap_type.get_value()));
        ret.insert(ret.begin(), tmp.begin(), tmp.end());
    }

    return ret;
}

} // namespace LivePathEffect
} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :

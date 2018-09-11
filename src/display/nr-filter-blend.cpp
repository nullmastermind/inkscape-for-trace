// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * SVG feBlend renderer
 *//*
 * "This filter composites two objects together using commonly used
 * imaging software blending modes. It performs a pixel-wise combination
 * of two input images."
 * http://www.w3.org/TR/SVG11/filters.html#feBlend
 *
 * Authors:
 *   Niko Kiirala <niko@kiirala.com>
 *   Jasper van de Gronde <th.v.d.gronde@hccnet.nl>
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2007-2008 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm.h>
#include "display/cairo-templates.h"
#include "display/cairo-utils.h"
#include "display/nr-filter-blend.h"
#include "display/nr-filter-primitive.h"
#include "display/nr-filter-slot.h"
#include "display/nr-filter-types.h"
#include "preferences.h"

namespace Inkscape {
namespace Filters {

const std::set<FilterBlendMode> FilterBlend::_valid_modes {
        BLEND_NORMAL,      BLEND_MULTIPLY,
        BLEND_SCREEN,      BLEND_DARKEN,
        BLEND_LIGHTEN,     BLEND_OVERLAY,
        BLEND_COLORDODGE,  BLEND_COLORBURN,
        BLEND_HARDLIGHT,   BLEND_SOFTLIGHT,
        BLEND_DIFFERENCE,  BLEND_EXCLUSION,
        BLEND_HUE,         BLEND_SATURATION,
        BLEND_COLOR,       BLEND_LUMINOSITY
        };

FilterBlend::FilterBlend()
    : _blend_mode(BLEND_NORMAL),
      _input2(NR_FILTER_SLOT_NOT_SET)
{}

FilterPrimitive * FilterBlend::create() {
    return new FilterBlend();
}

FilterBlend::~FilterBlend()
= default;

static inline cairo_operator_t get_cairo_op(FilterBlendMode _blend_mode)
{
    switch (_blend_mode) {
    case BLEND_MULTIPLY:
        return CAIRO_OPERATOR_MULTIPLY;
    case BLEND_SCREEN:
        return CAIRO_OPERATOR_SCREEN;
    case BLEND_DARKEN:
        return CAIRO_OPERATOR_DARKEN;
    case BLEND_LIGHTEN:
        return CAIRO_OPERATOR_LIGHTEN;
    // New in CSS Compositing and Blending Level 1
    case BLEND_OVERLAY:
        return CAIRO_OPERATOR_OVERLAY;
    case BLEND_COLORDODGE:
        return CAIRO_OPERATOR_COLOR_DODGE;
    case BLEND_COLORBURN:
        return CAIRO_OPERATOR_COLOR_BURN;
    case BLEND_HARDLIGHT:
        return CAIRO_OPERATOR_HARD_LIGHT;
    case BLEND_SOFTLIGHT:
        return CAIRO_OPERATOR_SOFT_LIGHT;
    case BLEND_DIFFERENCE:
        return CAIRO_OPERATOR_DIFFERENCE;
    case BLEND_EXCLUSION:
        return CAIRO_OPERATOR_EXCLUSION;
    case BLEND_HUE:
        return CAIRO_OPERATOR_HSL_HUE;
    case BLEND_SATURATION:
        return CAIRO_OPERATOR_HSL_SATURATION;
    case BLEND_COLOR:
        return CAIRO_OPERATOR_HSL_COLOR;
    case BLEND_LUMINOSITY:
        return CAIRO_OPERATOR_HSL_LUMINOSITY;

    case BLEND_NORMAL:
    default:
        return CAIRO_OPERATOR_OVER;
    }
}

void FilterBlend::render_cairo(FilterSlot &slot)
{
    cairo_surface_t *input1 = slot.getcairo(_input);
    cairo_surface_t *input2 = slot.getcairo(_input2);

    // We may need to transform input surface to correct color interpolation space. The input surface
    // might be used as input to another primitive but it is likely that all the primitives in a given
    // filter use the same color interpolation space so we don't copy the input before converting.
    SPColorInterpolation ci_fp  = SP_CSS_COLOR_INTERPOLATION_AUTO;
    if( _style ) {
        ci_fp = (SPColorInterpolation)_style->color_interpolation_filters.computed;
    }
    set_cairo_surface_ci( input1, ci_fp );
    set_cairo_surface_ci( input2, ci_fp );

    // input2 is the "background" image
    // out should be ARGB32 if any of the inputs is ARGB32
    cairo_surface_t *out = ink_cairo_surface_create_output(input1, input2);
    set_cairo_surface_ci( out, ci_fp );

    ink_cairo_surface_blit(input2, out);
    cairo_t *out_ct = cairo_create(out);
    cairo_set_source_surface(out_ct, input1, 0, 0);

    // All of the blend modes are implemented in Cairo as of 1.10.
    // For a detailed description, see:
    // http://cairographics.org/operators/
    cairo_set_operator(out_ct, get_cairo_op(_blend_mode));

    cairo_paint(out_ct);
    cairo_destroy(out_ct);

    slot.set(_output, out);
    cairo_surface_destroy(out);
}

bool FilterBlend::can_handle_affine(Geom::Affine const &)
{
    // blend is a per-pixel primitive and is immutable under transformations
    return true;
}

double FilterBlend::complexity(Geom::Affine const &)
{
    return 1.1;
}

bool FilterBlend::uses_background()
{
    return (_input == NR_FILTER_BACKGROUNDIMAGE || _input == NR_FILTER_BACKGROUNDALPHA ||
            _input2 == NR_FILTER_BACKGROUNDIMAGE || _input2 == NR_FILTER_BACKGROUNDALPHA);
}

void FilterBlend::set_input(int slot) {
    _input = slot;
}

void FilterBlend::set_input(int input, int slot) {
    if (input == 0) _input = slot;
    if (input == 1) _input2 = slot;
}

void FilterBlend::set_mode(FilterBlendMode mode) {
    if (_valid_modes.count(mode))
    {
        _blend_mode = mode;
    }
}

} /* namespace Filters */
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

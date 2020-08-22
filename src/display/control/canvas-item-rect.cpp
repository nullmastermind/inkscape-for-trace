// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * A class to represent a control rectangle. Used for rubberband selector, page outline, etc.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of CtrlRect
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cairo/cairo.h>

#include "canvas-item-rect.h"

#include "color.h"    // SP_RGBA_x_F
#include "inkscape.h" //
#include "ui/widget/canvas.h"

namespace Inkscape {

/**
 * Create an null control rect.
 */
CanvasItemRect::CanvasItemRect(CanvasItemGroup *group)
    : CanvasItem(group)
{
    _name = "CanvasItemRect:Null";
    _pickable = false; // For now, nobody gets events from this class!
}

/**
 * Create a control rect. Point are in document coordinates.
 */
CanvasItemRect::CanvasItemRect(CanvasItemGroup *group, Geom::Rect const &rect)
    : CanvasItem(group)
    , _rect(rect)
{
    _name = "CanvasItemRect";
    _pickable = false; // For now, nobody gets events from this class!
    request_update();
}

/**
 * Set a control rect. Points are in document coordinates.
 */
void CanvasItemRect::set_rect(Geom::Rect const &rect)
{
    _rect = rect;
    request_update();
}

/**
 * Returns distance between point in canvas units and nearest point in rect (zero if inside rect).
 * Only valid if canvas is not rotated. (A rotated Geom::Rect yields a new axis-aligned Geom::Rect
 * that contains the original rectangle; not a rotated rectangle.)
 */
double CanvasItemRect::closest_distance_to(Geom::Point const &p)
{
    if (_affine.isNonzeroRotation()) {
        std::cerr << "CanvasItemRect::closest_distance_to: Affine includes rotation!" << std::endl;
    }

    Geom::Rect rect = _rect;
    rect *= _affine; // Convert from document to canvas coordinates. (TODO Cache this.)
    return Geom::distance(p, rect);
}

/**
 * Returns true if point p (in canvas units) is within tolerance (canvas units) distance of rect.
 * Non-zero tolerance not implemented! Is valid for a rotated canvas.
 */
bool CanvasItemRect::contains(Geom::Point const &p, double tolerance)
{
    if (tolerance != 0) {
        std::cerr << "CanvasItemRect::contains: Non-zero tolerance not implemented!" << std::endl;
    }

    Geom::Point p0 = _rect.corner(0) * _affine;
    Geom::Point p1 = _rect.corner(1) * _affine;
    Geom::Point p2 = _rect.corner(2) * _affine;
    Geom::Point p3 = _rect.corner(3) * _affine;

    // From 2geom rotated-rect.cpp
    return
        Geom::cross(p1 - p0, p - p0) >= 0 &&
        Geom::cross(p2 - p1, p - p1) >= 0 &&
        Geom::cross(p3 - p2, p - p2) >= 0 &&
        Geom::cross(p0 - p3, p - p3) >= 0;
}

/**
 * Update and redraw control rect.
 */
void CanvasItemRect::update(Geom::Affine const &affine)
{
    if (_affine == affine && !_need_update) {
        // Nothing to do.
        return;
    }

    if (_rect.area() == 0) {
        return; // Nothing to show
    }

    // Queue redraw of old area (erase previous content).
    _canvas->redraw_area(_bounds);

    // Get new bounds
    _affine = affine;

    // Enlarge bbox by twice shadow size (to allow for shadow on any side with a 45deg rotation).
    _bounds = _rect;
    _bounds *= _affine;
    _bounds.expandBy(2*_shadow_width + 2); // Room for stroke.

    // Queue redraw of new area
    _canvas->redraw_area(_bounds);

    _need_update = false;
}

/**
 * Render rect to screen via Cairo.
 */
void CanvasItemRect::render(Inkscape::CanvasItemBuffer *buf)
{
    if (!buf) {
        std::cerr << "CanvasItemRect::Render: No buffer!" << std::endl;
         return;
    }

    if (!_bounds.intersects(buf->rect)) {
        return; // Rectangle not inside buffer rectangle.
    }

    if (!_visible) {
        // Hidden
        return;
    }

    // Geom::Rect is an axis-aligned rectangle. We need to rotate it if the canvas is rotated!

    // Get canvas rotation (scale is isotropic).
    double rotation = atan2(_affine[1], _affine[0]);

    // Are we axis aligned?
    double mod_rot = fmod(rotation * M_2_PI, 1);
    bool axis_aligned = Geom::are_near(mod_rot, 0) || Geom::are_near(mod_rot, 1.0);

    // Get the points we need transformed into window coordinates.
    Geom::Point rect_transformed[4];
    for (unsigned int i = 0; i < 4; ++i) {
        rect_transformed[i] = _rect.corner(i) * _affine;
    }

    buf->cr->save();
    buf->cr->translate(-buf->rect.left(), -buf->rect.top());

    if (_inverted) {
        // buf->cr->set_operator(Cairo::OPERATOR_XOR); // Blend mode operators do not have C++ bindings!
        cairo_set_operator(buf->cr->cobj(), CAIRO_OPERATOR_DIFFERENCE);
    }

    using Geom::X;
    using Geom::Y;

    // Draw shadow first. Shadow extends under rectangle to reduce aliasing effects.
    if (_shadow_width > 0 && !_dashed) {

        Geom::Point const * corners = rect_transformed;
        double shadowydir = _affine.det() > 0 ? -1 : 1;

        // Is the desktop y-axis downwards?
        if (SP_ACTIVE_DESKTOP && SP_ACTIVE_DESKTOP->is_yaxisdown()) {
            ++corners; // Need corners 1/2/3 instead of 0/1/2.
            shadowydir *= -1;
        }

        // Offset by half stroke width (_shadow_width is in window coordinates).
        // Need to handle change in handedness with flips.
        Geom::Point shadow( _shadow_width/2.0, shadowydir * _shadow_width/2.0 );
        shadow *= Geom::Rotate( rotation );

        if (axis_aligned) {
            // Snap to pixel grid (add 0.5 to center on pixel).
            buf->cr->move_to(floor(corners[0][X] + shadow[X]+0.5) + 0.5,
                             floor(corners[0][Y] + shadow[Y]+0.5) + 0.5 );
            buf->cr->line_to(floor(corners[1][X] + shadow[X]+0.5) + 0.5,
                             floor(corners[1][Y] + shadow[Y]+0.5) + 0.5 );
            buf->cr->line_to(floor(corners[2][X] + shadow[X]+0.5) + 0.5,
                             floor(corners[2][Y] + shadow[Y]+0.5) + 0.5 );
        } else {
            buf->cr->move_to(corners[0][X] + shadow[X], corners[0][Y] + shadow[Y] );
            buf->cr->line_to(corners[1][X] + shadow[X], corners[1][Y] + shadow[Y] );
            buf->cr->line_to(corners[2][X] + shadow[X], corners[2][Y] + shadow[Y] );
        }               

        buf->cr->set_source_rgba(SP_RGBA32_R_F(_shadow_color), SP_RGBA32_G_F(_shadow_color),
                                 SP_RGBA32_B_F(_shadow_color), SP_RGBA32_A_F(_shadow_color));
        buf->cr->set_line_width(_shadow_width + 1);
        buf->cr->stroke();
    }
    
    // Setup rectangle path
    if (axis_aligned) {

        // Snap to pixel grid
        Geom::Rect outline( _rect.min() * _affine, _rect.max() * _affine);
        buf->cr->rectangle(floor(outline.min()[X])+0.5,
                           floor(outline.min()[Y])+0.5,
                           floor(outline.max()[X]) - floor(outline.min()[X]),
                           floor(outline.max()[Y]) - floor(outline.min()[Y]));
    } else {

        // Rotated
        buf->cr->move_to(rect_transformed[0][X], rect_transformed[0][Y] );
        buf->cr->line_to(rect_transformed[1][X], rect_transformed[1][Y] );
        buf->cr->line_to(rect_transformed[2][X], rect_transformed[2][Y] );
        buf->cr->line_to(rect_transformed[3][X], rect_transformed[3][Y] );
        buf->cr->close_path();
    }

    // Draw border (stroke).
    buf->cr->set_source_rgba(SP_RGBA32_R_F(_stroke), SP_RGBA32_G_F(_stroke),
                             SP_RGBA32_B_F(_stroke), SP_RGBA32_A_F(_stroke));
    buf->cr->set_line_width(1);

    static std::valarray<double> dashes = {4.0, 4.0};
    if (_dashed) {
        buf->cr->set_dash(dashes, 0);
    }
    buf->cr->stroke_preserve();

    // Highlight the border by drawing it in _shadow_color.
    if (_shadow_width == 1 && _dashed) {
        buf->cr->set_source_rgba(SP_RGBA32_R_F(_shadow_color), SP_RGBA32_G_F(_shadow_color),
                                 SP_RGBA32_B_F(_shadow_color), SP_RGBA32_A_F(_shadow_color));
        buf->cr->set_dash(dashes, 4); // Dash offset by dash length.
        buf->cr->stroke_preserve();
    }

    buf->cr->begin_new_path(); // Clear path or get wierd artifacts.

    // Uncomment to show bounds
    // Geom::Rect bounds = _bounds;
    // bounds.expandBy(-1);
    // bounds -= buf->rect.min();
    // buf->cr->set_source_rgba(1.0, 0.0, 0.0, 1.0);
    // buf->cr->rectangle(bounds.min().x(), bounds.min().y(), bounds.width(), bounds.height());
    // buf->cr->stroke();

    buf->cr->restore();
}

void CanvasItemRect::set_dashed(bool dashed)
{
    if (_dashed != dashed) {
        _dashed = dashed;
        _canvas->redraw_area(_bounds);
    }
}

void CanvasItemRect::set_inverted(bool inverted)
{
    if (_inverted != inverted) {
        _inverted = inverted;
        _canvas->redraw_area(_bounds);
    }
}

void CanvasItemRect::set_shadow(guint32 color, int width)
{
    if (_shadow_color != color || _shadow_width != width) {
        _shadow_color = color;
        _shadow_width = width;
        _canvas->redraw_area(_bounds);
    }
}

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

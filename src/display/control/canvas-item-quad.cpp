// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * A class to represent a control quadrilateral. Used to hightlight selected text.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of SPCtrlQuadr
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "canvas-item-quad.h"

#include "color.h" // SP_RGBA_x_F

#include "ui/widget/canvas.h"

namespace Inkscape {

/**
 * Create an null control quad.
 */
CanvasItemQuad::CanvasItemQuad(CanvasItemGroup *group)
    : CanvasItem(group)
{
    _name = "CanvasItemQuad:Null";
    _pickable = false; // For now, nobody gets events from this class!
}

/**
 * Create a control quad. Point are in document coordinates.
 */
CanvasItemQuad::CanvasItemQuad(CanvasItemGroup *group,
                               Geom::Point const &p0, Geom::Point const &p1,
                               Geom::Point const &p2, Geom::Point const &p3)
    : CanvasItem(group)
    , _p0(p0)
    , _p1(p1)
    , _p2(p2)
    , _p3(p3)
{
    _name = "CanvasItemQuad";
    _pickable = false; // For now, nobody gets events from this class!

    request_update();
}

/**
 * Set a control quad. Points are in document coordinates.
 */
void CanvasItemQuad::set_coords(Geom::Point const &p0, Geom::Point const &p1, Geom::Point const &p2, Geom::Point const &p3)
{
    std::cout << "Canvas_ItemQuad::set_cords: " << p0 << ", " << p1 << ", " << p2 << ", " << p3 << std::endl;
    _p0 = p0;
    _p1 = p1;
    _p2 = p2;
    _p3 = p3;

    request_update();
}

/**
 * Returns distance between point in canvas units and nearest point on quad.
 */
double CanvasItemQuad::closest_distance_to(Geom::Point const &p)
{
    double d = Geom::infinity();
    std::cerr << "CanvasItemQuad::closest_distance_to: Not implemented!" << std::endl;
    return d;
}

/**
 * Returns true if point p (in canvas units) is within tolerance (canvas units) distance of quad.
 */
bool CanvasItemQuad::contains(Geom::Point const &p, double tolerance)
{
    if (tolerance != 0) {
        std::cerr << "CanvasItemQuad::contains: Non-zero tolerance not implemented!" << std::endl;
    }

    Geom::Point p0 = _p0 * _affine;
    Geom::Point p1 = _p1 * _affine;
    Geom::Point p2 = _p2 * _affine;
    Geom::Point p3 = _p3 * _affine;

    // From 2geom rotated-rect.cpp
    return
        Geom::cross(p1 - p0, p - p0) >= 0 &&
        Geom::cross(p2 - p1, p - p1) >= 0 &&
        Geom::cross(p3 - p2, p - p2) >= 0 &&
        Geom::cross(p0 - p3, p - p3) >= 0;
}

/**
 * Update and redraw control quad.
 */
void CanvasItemQuad::update(Geom::Affine const &affine)
{
    if (_affine == affine && !_need_update) {
        // Nothing to do.
        return;
    }

    if (_p0 == _p1 ||
        _p1 == _p2 ||
        _p2 == _p3 ||
        _p3 == _p0) {
        return; // Not quad or not initialized.
    }

    // Queue redraw of old area (erase previous content).
    _canvas->redraw_area(_bounds); // This is actually never useful as quads are always deleted
    // and recreated when a node is moved! But keep it in case we
    // change that. CHECK
    // Get new bounds
    _affine = affine;

    Geom::Rect bounds;
    bounds.expandTo(_p0);
    bounds.expandTo(_p1);
    bounds.expandTo(_p2);
    bounds.expandTo(_p3);
    bounds *= _affine;                       // Document to canvas.
    bounds.expandBy(2);                      // Room for anti-aliasing effects.
    _bounds = bounds;

    // Queue redraw of new area
    _canvas->redraw_area(_bounds);

    _need_update = false;
}

/**
 * Render quad to screen via Cairo.
 */
void CanvasItemQuad::render(Inkscape::CanvasItemBuffer *buf)
{
    if (!buf) {
        std::cerr << "CanvasItemQuad::Render: No buffer!" << std::endl;
         return;
    }

    if (_p0 == _p1 ||
        _p1 == _p2 ||
        _p2 == _p3 ||
        _p3 == _p0) {
        return; // Not quad or not initialized.
    }

    if (!_visible) {
        // Hidden
        return;
    }

    // Document to canvas
    Geom::Point p0 = _p0 * _affine;
    Geom::Point p1 = _p1 * _affine;
    Geom::Point p2 = _p2 * _affine;
    Geom::Point p3 = _p3 * _affine;

    // Canvas to screen
    p0 *= Geom::Translate(-buf->rect.min());
    p1 *= Geom::Translate(-buf->rect.min());
    p2 *= Geom::Translate(-buf->rect.min());
    p3 *= Geom::Translate(-buf->rect.min());

    buf->cr->save();

    buf->cr->begin_new_path();

    buf->cr->move_to(p0.x(), p0.y());
    buf->cr->line_to(p1.x(), p1.y());
    buf->cr->line_to(p2.x(), p2.y());
    buf->cr->line_to(p3.x(), p3.y());
    buf->cr->close_path();
    buf->cr->set_source_rgba(SP_RGBA32_R_F(_fill), SP_RGBA32_G_F(_fill),
                             SP_RGBA32_B_F(_fill), SP_RGBA32_A_F(_fill));
    buf->cr->fill();

    // Uncomment to show bounds
    // Geom::Rect bounds = _bounds;
    // bounds.expandBy(-1);
    // bounds -= buf->rect.min();
    // buf->cr->set_source_rgba(1.0, 0.0, 0.0, 1.0);
    // buf->cr->rectangle(bounds.min().x(), bounds.min().y(), bounds.width(), bounds.height());
    // buf->cr->stroke();

    buf->cr->restore();
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

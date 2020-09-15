// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * A class for previewing a canvas rotation.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of SPCanvasRotate.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cairomm/cairomm.h>
#include <gdkmm/event.h>

#include "canvas-item-rotate.h"

#include "color.h" // SP_RGBA_x_F
#include "desktop.h"
#include "inkscape.h"

#include "display/cairo-utils.h" // Copy Cairo surface.
#include "ui/widget/canvas.h"

namespace Inkscape {

/**
 * Create an null control rotate.
 */
CanvasItemRotate::CanvasItemRotate(CanvasItemGroup *group)
    : CanvasItem(group)
{
    _name = "CanvasItemRotate";
    _pickable = true; // We need the events!
    _bounds = Geom::Rect(-Geom::infinity(), -Geom::infinity(), Geom::infinity(), Geom::infinity());
}


/**
 * Returns distance between point in canvas units and nearest point on rotate.
 */
double CanvasItemRotate::closest_distance_to(Geom::Point const &p)
{
    double d = Geom::infinity();
    std::cerr << "CanvasItemRotate::closest_distance_to: Not implemented!" << std::endl;
    return d;
}

/**
 * Returns true if point p (in canvas units) is within tolerance (canvas units) distance of rotate.
 */
bool CanvasItemRotate::contains(Geom::Point const &p, double tolerance)
{
    return true; // We're always picked!
}

/**
 * Update and redraw control rotate.
 */
void CanvasItemRotate::update(Geom::Affine const &affine)
{
    _affine = affine;
    _canvas->redraw_area(_bounds);
}

/**
 * Render rotate to screen via Cairo.
 */
void CanvasItemRotate::render(Inkscape::CanvasItemBuffer *buf)
{
    return; // We do no proper rendering!
}

/**
 * Start
 */
void CanvasItemRotate::start(SPDesktop *desktop)
{
    _desktop = desktop;

    _current_angle = 0.0;

    _surface_copy = ink_cairo_surface_copy(_canvas->get_backing_store());
}

/**
 * Render to widget.
 */
void CanvasItemRotate::paint()
{
    auto background = _canvas->get_backing_store();

    if (!background) {
        std::cerr << "CanvasItemRotate::paint(): No background!" << std::endl;
        return;
    }

    double width = background->get_width();
    double height = background->get_height();
    
    // Draw rotated canvas.
    auto context = Cairo::Context::create(background);
    context->set_operator(Cairo::OPERATOR_SOURCE);
    context->translate(width/2.0, height/2.0);
    context->rotate(Geom::rad_from_deg(-_current_angle));
    context->translate(-width/2.0, -height/2.0);
    context->set_source(_surface_copy, 0, 0);
    context->paint();

    _canvas->queue_draw();
}

/**
 * Handle events.
 */
bool CanvasItemRotate::handle_event(GdkEvent *event)
{
    // Get geometry
    Geom::Rect viewbox = _canvas->get_area_world();
    _center = viewbox.midpoint();

    switch (event->type) {
        case GDK_MOTION_NOTIFY:
        {
            Geom::Point cursor( event->motion.x, event->motion.y );

            // Both cursor and center are in window coordinates
            Geom::Point rcursor( cursor - _center );
            double angle = Geom::deg_from_rad( Geom::atan2(rcursor) );


            // Set start angle
            if (_start_angle < -360) {
                _start_angle = angle;
            }

            const double rotation_snap = 15;

            double delta_angle = _start_angle - angle;

            if (event->motion.state & GDK_SHIFT_MASK &&
                event->motion.state & GDK_CONTROL_MASK) {
                delta_angle = 0;
            } else if (event->motion.state & GDK_SHIFT_MASK) {
                delta_angle = round(delta_angle/rotation_snap) * rotation_snap;
            } else if (event->motion.state & GDK_CONTROL_MASK) {
                // ?
            } else if (event->motion.state & GDK_MOD1_MASK) {
                // Decimal raw angle
            } else {
                delta_angle = floor(delta_angle);
            }

            _current_angle = delta_angle;

            // Correct line for snapping of angle
            double distance = rcursor.length();
            _cursor = Geom::Point::polar( Geom::rad_from_deg(angle), distance );

            // Update screen
            paint();
            break;
        }
        case GDK_BUTTON_RELEASE:
        {
            // Rotate the actual canvas
            SPDesktop *desktop = SP_ACTIVE_DESKTOP;  // FIXME..
            desktop->rotate_relative_center_point (desktop->w2d(_center),
                                                   (desktop->w2d().det() > 0 ? -1 : 1) *
                                                   Geom::rad_from_deg(_current_angle) );

            // We're done
            ungrab();
            hide();

            _start_angle = -1000;
            break;
        }
        case GDK_KEY_PRESS:
            // std::cout << "  Key press: " << std::endl;
            break;
        case GDK_KEY_RELEASE:
            // std::cout << "  Key release: " << std::endl;
            break;
        default:
            // ui_dump_event (event, "sp_canvas_rotate_event: unwanted event: ");
            break;
    }

    if (event->type == GDK_KEY_PRESS) return false; // Why?

    // Don't emit event signal!

    return true;
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

// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_CANVAS_ROTATE_H
#define SEEN_SP_CANVAS_ROTATE_H

/*
 * Temporary surface for previewing rotated canvas.
 *
 * Author:
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright (C) 2017 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-canvas-item.h"
#include "2geom/line.h"

class SPItem;

#define SP_TYPE_CANVAS_ROTATE (sp_canvas_rotate_get_type ())
#define SP_CANVAS_ROTATE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_CANVAS_ROTATE, SPCanvasRotate))
#define SP_IS_CANVAS_ROTATE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_CANVAS_ROTATE))

struct SPCanvasRotate : public SPCanvasItem {
    Geom::Point center; // Center of screen
    Geom::Point cursor; // Position of cursor relative to center (after angle snapping)
    double angle;       // Rotation angle in degrees
    double start_angle; // Starting angle of cursor
    cairo_surface_t *surface_copy; // Copy of original surface
    cairo_surface_t *surface_rotated; // Copy of original surface, rotated
};

void sp_canvas_rotate_start( SPCanvasRotate *canvas_rotate, cairo_surface_t *background );

GType sp_canvas_rotate_get_type ();

struct SPCanvasRotateClass : public SPCanvasItemClass{};

#endif // SEEN_SP_CANVAS_ROTATE_H

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

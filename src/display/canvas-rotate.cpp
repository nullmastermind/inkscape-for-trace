/*
 * Temporary surface for previewing rotated canvas.
 *
 * Author:
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright (C) 2017 Tavmjong Bah
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "inkscape.h"
#include "desktop.h"

#include "canvas-rotate.h"
#include "sp-canvas.h"
#include "cairo-utils.h"
#include "ui/event-debug.h"

#include "2geom/point.h"
#include "2geom/rect.h"

#include <gtk/gtk.h>

namespace {

static void   sp_canvas_rotate_destroy(SPCanvasItem *item);
static void   sp_canvas_rotate_update (SPCanvasItem *item, Geom::Affine const &affine, unsigned int flags);
static void   sp_canvas_rotate_render (SPCanvasItem *item, SPCanvasBuf *buf);
static int    sp_canvas_rotate_event  (SPCanvasItem *item, GdkEvent *event);

} // namespace

void sp_canvas_rotate_paint (SPCanvasRotate *canvas_rotate, cairo_surface_t *background);

G_DEFINE_TYPE(SPCanvasRotate, sp_canvas_rotate, SP_TYPE_CANVAS_ITEM);

static void sp_canvas_rotate_class_init (SPCanvasRotateClass *klass)
{
    klass->destroy = sp_canvas_rotate_destroy;
    //klass->update  = sp_canvas_rotate_update;
    //klass->render  = sp_canvas_rotate_render;
    klass->event   = sp_canvas_rotate_event;
}

static void sp_canvas_rotate_init (SPCanvasRotate *rotate)
{
    rotate->pickable = true; // So we can receive events.
    rotate->angle = 0.0;
    rotate->start_angle = -1000;
    rotate->surface_copy = NULL;
    rotate->surface_rotated = NULL;
}

namespace {
static void sp_canvas_rotate_destroy (SPCanvasItem *object)
{
    g_return_if_fail (object != NULL);
    g_return_if_fail (SP_IS_CANVAS_ROTATE (object));

    if (SP_CANVAS_ITEM_CLASS(sp_canvas_rotate_parent_class)->destroy) {
        SP_CANVAS_ITEM_CLASS(sp_canvas_rotate_parent_class)->destroy(object);
    }
}

// NOT USED... TOO SLOW
static void sp_canvas_rotate_update( SPCanvasItem *item, Geom::Affine const &/*affine*/, unsigned int /*flags*/ )
{
    SPCanvasRotate *cr = SP_CANVAS_ROTATE(item);

    if (cr->surface_copy == NULL) {
        // std::cout << "sp_canvas_rotate_update: surface_copy is NULL" << std::endl;
        return;
    }
    
    // Destroy surface_rotated if it already exists.
    if (cr->surface_rotated != NULL) {
        cairo_surface_destroy (cr->surface_rotated);
        cr->surface_rotated = NULL;
    }
    
    // Create rotated surface
    cr->surface_rotated = ink_cairo_surface_create_identical(cr->surface_copy);
    double width  = cairo_image_surface_get_width  (cr->surface_rotated);
    double height = cairo_image_surface_get_height (cr->surface_rotated);
    cairo_t *context = cairo_create( cr->surface_rotated );
    cairo_set_operator( context, CAIRO_OPERATOR_SOURCE );
    cairo_translate(    context, width/2.0, height/2.0 );
    cairo_rotate(       context, Geom::rad_from_deg(-cr->angle) );
    cairo_translate(    context, -width/2.0, -height/2.0 );
    cairo_set_source_surface( context, cr->surface_copy, 0, 0 );
    cairo_paint(        context );
    cairo_destroy(      context);
    
    // We cover the entire canvas
    item->x1 = -G_MAXINT;
    item->y1 = -G_MAXINT;
    item->x2 = G_MAXINT;
    item->y2 = G_MAXINT;

    item->canvas->requestRedraw(item->x1, item->y1, item->x2, item->y2);
}

// NOT USED... TOO SLOW
static void sp_canvas_rotate_render( SPCanvasItem *item, SPCanvasBuf *buf)
{
    // std::cout << "sp_canvas_rotate_render:" << std::endl;
    // std::cout << "  buf->rect:         " << buf->rect << std::endl;
    // std::cout << "  buf->canvas_rect: " << buf->canvas_rect << std::endl;
    SPCanvasRotate *cr = SP_CANVAS_ROTATE(item);

    if (!buf->ct) {
        return;
    }

    if (cr->surface_rotated == NULL ) {
        // std::cout << "  surface_rotated is NULL" << std::endl;
        return;
    }

    // Draw rotated canvas
    cairo_save (buf->ct);
    cairo_translate (buf->ct,
                     buf->canvas_rect.left() - buf->rect.left(),
                     buf->canvas_rect.top()  - buf->rect.top() );
    cairo_set_operator (buf->ct, CAIRO_OPERATOR_SOURCE );
    cairo_set_source_surface (buf->ct, cr->surface_rotated, 0, 0 );
    cairo_paint   (buf->ct);
    cairo_restore (buf->ct);


    // Draw line from center to cursor
    cairo_save (buf->ct);
    cairo_translate   (buf->ct, -buf->rect.left(), -buf->rect.top());
    cairo_new_path    (buf->ct);
    cairo_move_to     (buf->ct, cr->center[Geom::X], cr->center[Geom::Y]);
    cairo_rel_line_to (buf->ct, cr->cursor[Geom::X], cr->cursor[Geom::Y]);
    cairo_set_line_width (buf->ct, 2);
    ink_cairo_set_source_rgba32 (buf->ct, 0xff00007f);
    cairo_stroke (buf->ct);
    cairo_restore (buf->ct);

}

static int sp_canvas_rotate_event  (SPCanvasItem *item, GdkEvent *event)
{
    SPCanvasRotate *cr = SP_CANVAS_ROTATE(item);

//    ui_dump_event (event, Glib::ustring("sp_canvas_rotate_event"));

    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    Geom::Rect viewbox = desktop->canvas->getViewbox(); // Not SVG viewbox!
    cr->center = viewbox.midpoint();

    switch (event->type) {
        case GDK_MOTION_NOTIFY:
        {
            Geom::Point cursor( event->motion.x, event->motion.y );

            // Both cursor and center are in window coordinates
            Geom::Point rcursor( cursor - cr->center );
            double angle = Geom::deg_from_rad( Geom::atan2(rcursor) );

            // Set start angle
            if (cr->start_angle < -360) {
                cr->start_angle = angle;
            }

            double rotation_snap = 15;

            double delta_angle = cr->start_angle - angle;

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

            cr->angle = delta_angle;

            // Correct line for snapping of angle
            double distance = rcursor.length();
            cr->cursor = Geom::Point::polar( Geom::rad_from_deg(angle), distance );        

            // Update screen
            // sp_canvas_item_request_update( item );
            sp_canvas_rotate_paint (cr, cr->canvas->_backing_store);
            break;
        }
        case GDK_BUTTON_RELEASE:

            // Rotate the actual canvas
            desktop->rotate_relative_center_point (desktop->w2d(cr->center),
                                                   Geom::rad_from_deg(cr->angle) );

            // We're done
            sp_canvas_item_ungrab (item, event->button.time);
            sp_canvas_item_hide (item);

            cr->start_angle = -1000;
            if (cr->surface_copy != NULL) {
                cairo_surface_destroy( cr->surface_copy );
                cr->surface_copy = NULL;
            }
            if (cr->surface_rotated != NULL) {
                cairo_surface_destroy( cr->surface_rotated );
                cr->surface_rotated = NULL;
            }
            // sp_canvas_item_show (desktop->drawing);
            
            break;
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

    if (event->type == GDK_KEY_PRESS) return false;

    return true;
}

} // namespace

void sp_canvas_rotate_start (SPCanvasRotate *canvas_rotate, cairo_surface_t *background)
{
    if (background == NULL) {
        std::cerr << "sp_canvas_rotate_start: background is NULL!" << std::endl;
        return;
    }

    canvas_rotate->angle = 0.0;

    // Copy current background
    canvas_rotate->surface_copy = ink_cairo_surface_copy( background );

    // Paint canvas with background... since we are hiding drawing.
    sp_canvas_item_request_update( canvas_rotate );
}

// Paint the canvas ourselves for speed....
void sp_canvas_rotate_paint (SPCanvasRotate *canvas_rotate, cairo_surface_t *background)
{
    if (background == NULL) {
        std::cerr << "sp_canvas_rotate_paint: background is NULL!" << std::endl;
        return;
    }

    double width  = cairo_image_surface_get_width  (background);
    double height = cairo_image_surface_get_height (background);
    
    // Draw rotated canvas
    cairo_t *context = cairo_create( background );
    
    cairo_save (context);
    cairo_set_operator( context, CAIRO_OPERATOR_SOURCE );
    cairo_translate(    context, width/2.0, height/2.0 );
    cairo_rotate(       context, Geom::rad_from_deg(-canvas_rotate->angle) );
    cairo_translate(    context, -width/2.0, -height/2.0 );
    cairo_set_source_surface( context, canvas_rotate->surface_copy, 0, 0 );
    cairo_paint(        context );
    cairo_restore(      context );
    cairo_destroy(      context );

    gtk_widget_queue_draw (GTK_WIDGET (canvas_rotate->canvas));
}
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

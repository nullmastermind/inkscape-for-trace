// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A simple surface for debugging the canvas. Shows how tiles are drawn.
 *
 * Author:
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright (C) 2017 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "canvas-debug.h"
#include "sp-canvas.h"
#include "cairo-utils.h"
#include "ui/event-debug.h"

namespace {

static void   sp_canvas_debug_destroy(SPCanvasItem *item);
static void   sp_canvas_debug_update (SPCanvasItem *item, Geom::Affine const &affine, unsigned int flags);
static void   sp_canvas_debug_render (SPCanvasItem *item, SPCanvasBuf *buf);
static int    sp_canvas_debug_event  (SPCanvasItem *item, GdkEvent *event);

} // namespace

G_DEFINE_TYPE(SPCanvasDebug, sp_canvas_debug, SP_TYPE_CANVAS_ITEM);

static void sp_canvas_debug_class_init (SPCanvasDebugClass *klass)
{
    klass->destroy = sp_canvas_debug_destroy;
    klass->update  = sp_canvas_debug_update;
    klass->render  = sp_canvas_debug_render;
    klass->event   = sp_canvas_debug_event;
}

static void sp_canvas_debug_init (SPCanvasDebug *debug)
{
    debug->pickable = true; // So we can receive events.
}

namespace {
static void sp_canvas_debug_destroy (SPCanvasItem *object)
{
    g_return_if_fail (object != nullptr);
    g_return_if_fail (SP_IS_CANVAS_DEBUG (object));

    if (SP_CANVAS_ITEM_CLASS(sp_canvas_debug_parent_class)->destroy) {
        SP_CANVAS_ITEM_CLASS(sp_canvas_debug_parent_class)->destroy(object);
    }
}

static void sp_canvas_debug_update( SPCanvasItem *item, Geom::Affine const &/*affine*/, unsigned int /*flags*/ )
{
    // We cover the entire canvas
    item->x1 = -G_MAXINT;
    item->y1 = -G_MAXINT;
    item->x2 = G_MAXINT;
    item->y2 = G_MAXINT;
}

static void sp_canvas_debug_render( SPCanvasItem *item, SPCanvasBuf *buf)
{
    if (!buf->ct) {
        return;
    }

    cairo_set_line_width (buf->ct, 2);

    // Draw box around buffer (for debugging)
    cairo_new_path (buf->ct);
    cairo_move_to (buf->ct, 0, 0);
    cairo_line_to (buf->ct, buf->rect.width(), 0);
    cairo_line_to (buf->ct, buf->rect.width(), buf->rect.height());
    cairo_line_to (buf->ct,                 0, buf->rect.height());
    cairo_close_path (buf->ct);
    ink_cairo_set_source_rgba32 (buf->ct, 0xff7f7f7f);
    cairo_stroke (buf->ct);
}

static int sp_canvas_debug_event  (SPCanvasItem *item, GdkEvent *event)
{
    ui_dump_event (event, Glib::ustring("sp_canvas_debug_event"));
    return false; // We don't use any events...
}

} // namespace

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

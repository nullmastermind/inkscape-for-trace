// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Port of GnomeCanvas for Inkscape needs
 *
 * Authors:
 *   Federico Mena <federico@nuclecu.unam.mx>
 *   Raph Levien <raph@gimp.org>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   fred
 *   bbyak
 *   Jon A. Cruz <jon@joncruz.org>
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 1998 The Free Software Foundation
 * Copyright (C) 2002-2006 authors
 * Copyright (C) 2016 Google Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#include <gdkmm/devicemanager.h>
#include <gdkmm/display.h>
#include <gdkmm/rectangle.h>
#include <gdkmm/seat.h>

#include <cairomm/region.h>

#include <2geom/affine.h>
#include <2geom/rect.h>

#include "sp-canvas.h"
#include "sp-canvas-group.h"
#include "sp-canvas-item.h"

#include "cairo-utils.h"
#include "canvas-arena.h"
#include "rendermode.h"
#include "sodipodi-ctrlrect.h"

#include "cms-system.h"
#include "color.h"
#include "desktop.h"
#include "inkscape-window.h"
#include "inkscape.h"
#include "preferences.h"

#include "debug/gdk-event-latency-tracker.h"
#include "helper/sp-marshal.h"
#include "ui/tools/node-tool.h"
#include "ui/tools/tool-base.h"
#include "widgets/desktop-widget.h"

using Inkscape::Debug::GdkEventLatencyTracker;


// Disabled by Mentalguy, many years ago in commit 427a81 
static bool const HAS_BROKEN_MOTION_HINTS = true;

// Define this to visualize the regions to be redrawn
//#define DEBUG_REDRAW 1;

// Define this to output the time spent in a full idle loop and the number of "tiles" painted
//#define DEBUG_PERFORMANCE 1;

void ungrab_default_client_pointer()
{
    auto const display = Gdk::Display::get_default();
    auto const seat    = display->get_default_seat();
    seat->ungrab();
}

GdkWindow *getWindow(SPCanvas *canvas)
{
    return gtk_widget_get_window(reinterpret_cast<GtkWidget *>(canvas));
}


/**
 * The SPCanvas vtable.
 */
struct SPCanvasClass {
    GtkWidgetClass parent_class;
};

G_DEFINE_TYPE(SPCanvas, sp_canvas, GTK_TYPE_WIDGET);

static void sp_canvas_finalize(GObject *object)
{
    SPCanvas *canvas = SP_CANVAS(object);

#if defined(HAVE_LIBLCMS2)
    using S = decltype(canvas->_cms_key);
    canvas->_cms_key.~S();
#endif

    G_OBJECT_CLASS(sp_canvas_parent_class)->finalize(object);
}

void sp_canvas_class_init(SPCanvasClass *klass)
{
    GObjectClass   *object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    object_class->dispose = SPCanvas::dispose;
    object_class->finalize = sp_canvas_finalize;

    widget_class->realize              = SPCanvas::handle_realize;
    widget_class->unrealize            = SPCanvas::handle_unrealize;
    widget_class->get_preferred_width  = SPCanvas::handle_get_preferred_width;
    widget_class->get_preferred_height = SPCanvas::handle_get_preferred_height;
    widget_class->draw                 = SPCanvas::handle_draw;
    widget_class->size_allocate        = SPCanvas::handle_size_allocate;
    widget_class->button_press_event   = SPCanvas::handle_button;
    widget_class->button_release_event = SPCanvas::handle_button;
    widget_class->motion_notify_event  = SPCanvas::handle_motion;
    widget_class->scroll_event         = SPCanvas::handle_scroll;
    widget_class->key_press_event      = SPCanvas::handle_key_event;
    widget_class->key_release_event    = SPCanvas::handle_key_event;
    widget_class->enter_notify_event   = SPCanvas::handle_crossing;
    widget_class->leave_notify_event   = SPCanvas::handle_crossing;
    widget_class->focus_in_event       = SPCanvas::handle_focus_in;
    widget_class->focus_out_event      = SPCanvas::handle_focus_out;
}

static void sp_canvas_init(SPCanvas *canvas)
{
    gtk_widget_set_has_window (GTK_WIDGET (canvas), TRUE);
    gtk_widget_set_can_focus (GTK_WIDGET (canvas), TRUE);

    canvas->_pick_event.type = GDK_LEAVE_NOTIFY;
    canvas->_pick_event.crossing.x = 0;
    canvas->_pick_event.crossing.y = 0;

    // Create the root item as a special case
    canvas->_root = SP_CANVAS_ITEM(g_object_new(SP_TYPE_CANVAS_GROUP, nullptr));
    canvas->_root->canvas = canvas;

    g_object_ref (canvas->_root);
    g_object_ref_sink (canvas->_root);

    canvas->_need_repick = TRUE;

    // See comment at in sp-canvas.h.
    canvas->_gen_all_enter_events = false;

    canvas->_drawing_disabled = false;

    canvas->_backing_store = nullptr;
    canvas->_surface_for_similar = nullptr;
    canvas->_clean_region = cairo_region_create();
    canvas->_background = cairo_pattern_create_rgb(1, 1, 1);
    canvas->_background_is_checkerboard = false;

    canvas->_forced_redraw_count = 0;
    canvas->_forced_redraw_limit = -1;

    // Split view controls
    canvas->_spliter = Geom::OptIntRect();
    canvas->_spliter_area = Geom::OptIntRect();
    canvas->_spliter_control = Geom::OptIntRect();
    canvas->_spliter_top = Geom::OptIntRect();
    canvas->_spliter_bottom = Geom::OptIntRect();
    canvas->_spliter_left = Geom::OptIntRect();
    canvas->_spliter_right = Geom::OptIntRect();
    canvas->_spliter_control_pos = Geom::Point();
    canvas->_spliter_in_control_pos = Geom::Point();
    canvas->_xray_rect = Geom::OptIntRect();
    canvas->_split_value = 0.5;
    canvas->_split_vertical = true;
    canvas->_split_inverse = false;
    canvas->_split_hover_vertical = false;
    canvas->_split_hover_horizontal = false;
    canvas->_split_hover = false;
    canvas->_split_pressed = false;
    canvas->_split_control_pressed = false;
    canvas->_split_dragging = false;
    canvas->_xray_radius = 100;
    canvas->_xray = false;
    canvas->_xray_orig = Geom::Point();
    canvas->_changecursor = 0;
    canvas->_splits = 0;
    canvas->_totalelapsed = 0;
    canvas->_idle_time = g_get_monotonic_time();
    canvas->_is_dragging = false;

#if defined(HAVE_LIBLCMS2)
    canvas->_enable_cms_display_adj = false;
    new (&canvas->_cms_key) decltype(canvas->_cms_key)();
#endif // defined(HAVE_LIBLCMS2)
}

void SPCanvas::shutdownTransients()
{
    // Reset the clean region
    dirtyAll();

    if (_grabbed_item) {
        _grabbed_item = nullptr;
        ungrab_default_client_pointer();
    }
    removeIdle();
}

void SPCanvas::dispose(GObject *object)
{
    SPCanvas *canvas = SP_CANVAS(object);

    if (canvas->_root) {
        g_object_unref (canvas->_root);
        canvas->_root = nullptr;
    }
    if (canvas->_backing_store) {
        cairo_surface_destroy(canvas->_backing_store);
        canvas->_backing_store = nullptr;
    }
    if (canvas->_surface_for_similar) {
        cairo_surface_destroy(canvas->_surface_for_similar);
        canvas->_surface_for_similar = nullptr;
    }
    if (canvas->_clean_region) {
        cairo_region_destroy(canvas->_clean_region);
        canvas->_clean_region = nullptr;
    }
    if (canvas->_background) {
        cairo_pattern_destroy(canvas->_background);
        canvas->_background = nullptr;
    }

    canvas->shutdownTransients();
    if (G_OBJECT_CLASS(sp_canvas_parent_class)->dispose) {
        (* G_OBJECT_CLASS(sp_canvas_parent_class)->dispose)(object);
    }
}

namespace {

void trackLatency(GdkEvent const *event)
{
    GdkEventLatencyTracker &tracker = GdkEventLatencyTracker::default_tracker();
    boost::optional<double> latency = tracker.process(event);
    if (latency && *latency > 2.0) {
        //g_warning("Event latency reached %f sec (%1.4f)", *latency, tracker.getSkew());
    }
}

} // namespace

GtkWidget *SPCanvas::createAA()
{
    SPCanvas *canvas = SP_CANVAS(g_object_new(SP_TYPE_CANVAS, nullptr));
    return GTK_WIDGET(canvas);
}

void SPCanvas::handle_realize(GtkWidget *widget)
{
    GdkWindowAttr attributes;
    GtkAllocation allocation;
    attributes.window_type = GDK_WINDOW_CHILD;
    gtk_widget_get_allocation (widget, &allocation);
    attributes.x = allocation.x;
    attributes.y = allocation.y;
    attributes.width = allocation.width;
    attributes.height = allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gdk_screen_get_system_visual(gdk_screen_get_default());

    attributes.event_mask = (gtk_widget_get_events (widget) |
                             GDK_EXPOSURE_MASK |
                             GDK_BUTTON_PRESS_MASK |
                             GDK_BUTTON_RELEASE_MASK |
                             GDK_POINTER_MOTION_MASK |
                             ( HAS_BROKEN_MOTION_HINTS ?
                               0 : GDK_POINTER_MOTION_HINT_MASK ) |
                             GDK_PROXIMITY_IN_MASK |
                             GDK_PROXIMITY_OUT_MASK |
                             GDK_KEY_PRESS_MASK |
                             GDK_KEY_RELEASE_MASK |
                             GDK_ENTER_NOTIFY_MASK |
                             GDK_LEAVE_NOTIFY_MASK |
                             GDK_SCROLL_MASK |
                             GDK_SMOOTH_SCROLL_MASK |
                             GDK_FOCUS_CHANGE_MASK);

    gint attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

    GdkWindow *window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
    gtk_widget_set_window (widget, window);
    gdk_window_set_user_data (window, widget);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/options/useextinput/value", true)) {
        gtk_widget_set_events(widget, attributes.event_mask);
    }

    gtk_widget_set_realized (widget, TRUE);
}

void SPCanvas::handle_unrealize(GtkWidget *widget)
{
    SPCanvas *canvas = SP_CANVAS (widget);

    canvas->_current_item = nullptr;
    canvas->_grabbed_item = nullptr;
    canvas->_focused_item = nullptr;

    canvas->shutdownTransients();

    if (GTK_WIDGET_CLASS(sp_canvas_parent_class)->unrealize)
        (* GTK_WIDGET_CLASS(sp_canvas_parent_class)->unrealize)(widget);
}

void SPCanvas::handle_get_preferred_width(GtkWidget *widget, gint *minimum_width, gint *natural_width)
{
    static_cast<void>(SP_CANVAS (widget));
    *minimum_width = 256;
    *natural_width = 256;
}

void SPCanvas::handle_get_preferred_height(GtkWidget *widget, gint *minimum_height, gint *natural_height)
{
    static_cast<void>(SP_CANVAS (widget));
    *minimum_height = 256;
    *natural_height = 256;
}

void SPCanvas::handle_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
    SPCanvas *canvas = SP_CANVAS (widget);
    // Allocation does not depend on device scale.
    GtkAllocation old_allocation;
    gtk_widget_get_allocation(widget, &old_allocation);

    // For HiDPI monitors.
    canvas->_device_scale = gtk_widget_get_scale_factor( widget );

    Geom::IntRect new_area = Geom::IntRect::from_xywh(canvas->_x0, canvas->_y0,
        allocation->width, allocation->height);

    // Resize backing store.
    cairo_surface_t *new_backing_store = nullptr;
    if (canvas->_surface_for_similar != nullptr) {

        // Size in device pixels. Does not set device scale.
        new_backing_store =
            cairo_surface_create_similar_image(canvas->_surface_for_similar,
                                               CAIRO_FORMAT_ARGB32,
                                               allocation->width  * canvas->_device_scale,
                                               allocation->height * canvas->_device_scale);
    }
    if (new_backing_store == nullptr) {

        // Size in device pixels. Does not set device scale.
        new_backing_store =
            cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                       allocation->width  * canvas->_device_scale,
                                       allocation->height * canvas->_device_scale);
    }

    // Set device scale
    cairo_surface_set_device_scale(new_backing_store, canvas->_device_scale, canvas->_device_scale);

    if (canvas->_backing_store) {
        cairo_t *cr = cairo_create(new_backing_store);
        cairo_translate(cr, -canvas->_x0, -canvas->_y0);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_source(cr, canvas->_background);
        cairo_paint(cr);
        cairo_set_source_surface(cr, canvas->_backing_store, canvas->_x0, canvas->_y0);
        cairo_paint(cr);
        cairo_destroy(cr);
        cairo_surface_destroy(canvas->_backing_store);
    }
    canvas->_backing_store = new_backing_store;

    // Clip the clean region to the new allocation
    cairo_rectangle_int_t crect = { canvas->_x0, canvas->_y0, allocation->width, allocation->height };
    cairo_region_intersect_rectangle(canvas->_clean_region, &crect);

    gtk_widget_set_allocation (widget, allocation);

    if (SP_CANVAS_ITEM_GET_CLASS (canvas->_root)->viewbox_changed)
        SP_CANVAS_ITEM_GET_CLASS (canvas->_root)->viewbox_changed (canvas->_root, new_area);

    if (gtk_widget_get_realized (widget)) {
        gdk_window_move_resize (gtk_widget_get_window (widget),
                                allocation->x, allocation->y,
                                allocation->width, allocation->height);
    }
    // Schedule redraw of any newly exposed regions
    canvas->_split_value = 0.5;
    canvas->_spliter_control_pos = Geom::Point();
    canvas->requestFullRedraw();
}

int SPCanvas::emitEvent(GdkEvent *event)
{
    guint mask;

    if (_grabbed_item) {
        switch (event->type) {
        case GDK_ENTER_NOTIFY:
            mask = GDK_ENTER_NOTIFY_MASK;
            break;
        case GDK_LEAVE_NOTIFY:
            mask = GDK_LEAVE_NOTIFY_MASK;
            break;
        case GDK_MOTION_NOTIFY:
            mask = GDK_POINTER_MOTION_MASK;
            break;
        case GDK_BUTTON_PRESS:
        case GDK_2BUTTON_PRESS:
        case GDK_3BUTTON_PRESS:
            mask = GDK_BUTTON_PRESS_MASK;
            break;
        case GDK_BUTTON_RELEASE:
            mask = GDK_BUTTON_RELEASE_MASK;
            break;
        case GDK_KEY_PRESS:
            mask = GDK_KEY_PRESS_MASK;
            break;
        case GDK_KEY_RELEASE:
            mask = GDK_KEY_RELEASE_MASK;
            break;
        case GDK_SCROLL:
            mask = GDK_SCROLL_MASK;
            mask |= GDK_SMOOTH_SCROLL_MASK;
            break;
        default:
            mask = 0;
            break;
        }

        if (!(mask & _grabbed_event_mask)) return FALSE;
    }

    // Convert to world coordinates -- we have two cases because of different
    // offsets of the fields in the event structures.

    GdkEvent *ev = gdk_event_copy(event);
    switch (ev->type) {
    case GDK_ENTER_NOTIFY:
    case GDK_LEAVE_NOTIFY:
        ev->crossing.x += _x0;
        ev->crossing.y += _y0;
        break;
    case GDK_MOTION_NOTIFY:
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
    case GDK_3BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
        ev->motion.x += _x0;
        ev->motion.y += _y0;
        break;
    default:
        break;
    }
    // Block Undo and Redo while we drag /anything/
    if(event->type == GDK_BUTTON_PRESS && event->button.button == 1)
        _is_dragging = true;
    else if(event->type == GDK_BUTTON_RELEASE)
        _is_dragging = false;

    // Choose where we send the event

    // canvas->current_item becomes NULL in some cases under Win32
    // (e.g. if the pointer leaves the window).  So this is a hack that
    // Lauris applied to SP to get around the problem.
    //
    SPCanvasItem* item = nullptr;
    if (_grabbed_item && !is_descendant(_current_item, _grabbed_item)) {
        item = _grabbed_item;
    } else {
        item = _current_item;
    }

    if (_focused_item &&
        ((event->type == GDK_KEY_PRESS) ||
         (event->type == GDK_KEY_RELEASE) ||
         (event->type == GDK_FOCUS_CHANGE))) {
        item = _focused_item;
    }

    // The event is propagated up the hierarchy (for if someone connected to
    // a group instead of a leaf event), and emission is stopped if a
    // handler returns TRUE, just like for GtkWidget events.

    gint finished = FALSE;

    while (item && !finished) {
        g_object_ref (item);
        g_signal_emit (G_OBJECT (item), item_signals[ITEM_EVENT], 0, ev, &finished);
        SPCanvasItem *parent = item->parent;
        g_object_unref (item);
        item = parent;
    }

    gdk_event_free(ev);

    return finished;
}

int SPCanvas::pickCurrentItem(GdkEvent *event)
{
    int button_down = 0;

    if (!_root) // canvas may have already be destroyed by closing desktop during interrupted display!
        return FALSE;

    int retval = FALSE;

    if (_gen_all_enter_events == false) {
        // If a button is down, we'll perform enter and leave events on the
        // current item, but not enter on any other item.  This is more or
        // less like X pointer grabbing for canvas items.
        //
        button_down = _state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK |
                GDK_BUTTON3_MASK | GDK_BUTTON4_MASK | GDK_BUTTON5_MASK);

        if (!button_down) _left_grabbed_item = FALSE;
    }

    // Save the event in the canvas.  This is used to synthesize enter and
    // leave events in case the current item changes.  It is also used to
    // re-pick the current item if the current one gets deleted.  Also,
    // synthesize an enter event.

    if (event != &_pick_event) {
        if ((event->type == GDK_MOTION_NOTIFY) || (event->type == GDK_BUTTON_RELEASE)) {
            // these fields have the same offsets in both types of events

            _pick_event.crossing.type       = GDK_ENTER_NOTIFY;
            _pick_event.crossing.window     = event->motion.window;
            _pick_event.crossing.send_event = event->motion.send_event;
            _pick_event.crossing.subwindow  = nullptr;
            _pick_event.crossing.x          = event->motion.x;
            _pick_event.crossing.y          = event->motion.y;
            _pick_event.crossing.mode       = GDK_CROSSING_NORMAL;
            _pick_event.crossing.detail     = GDK_NOTIFY_NONLINEAR;
            _pick_event.crossing.focus      = FALSE;
            _pick_event.crossing.state      = event->motion.state;

            // these fields don't have the same offsets in both types of events

            if (event->type == GDK_MOTION_NOTIFY) {
                _pick_event.crossing.x_root = event->motion.x_root;
                _pick_event.crossing.y_root = event->motion.y_root;
            } else {
                _pick_event.crossing.x_root = event->button.x_root;
                _pick_event.crossing.y_root = event->button.y_root;
            }
        } else {
            _pick_event = *event;
        }
    }

    // Don't do anything else if this is a recursive call
    if (_in_repick) {
        return retval;
    }

    // LeaveNotify means that there is no current item, so we don't look for one
    if (_pick_event.type != GDK_LEAVE_NOTIFY) {
        // these fields don't have the same offsets in both types of events
        double x, y;

        if (_pick_event.type == GDK_ENTER_NOTIFY) {
            x = _pick_event.crossing.x;
            y = _pick_event.crossing.y;
        } else {
            x = _pick_event.motion.x;
            y = _pick_event.motion.y;
        }

        // world coords
        x += _x0;
        y += _y0;

        // find the closest item
        if (_root->visible) {
            sp_canvas_item_invoke_point (_root, Geom::Point(x, y), &_new_current_item);
        } else {
            _new_current_item = nullptr;
        }
    } else {
        _new_current_item = nullptr;
    }

    if ((_new_current_item == _current_item) && !_left_grabbed_item) {
        return retval; // current item did not change
    }

    // Synthesize events for old and new current items

    if ((_new_current_item != _current_item) &&
        _current_item != nullptr && !_left_grabbed_item)
    {
        GdkEvent new_event;

        new_event = _pick_event;
        new_event.type = GDK_LEAVE_NOTIFY;

        new_event.crossing.detail = GDK_NOTIFY_ANCESTOR;
        new_event.crossing.subwindow = nullptr;
        _in_repick = TRUE;
        retval = emitEvent(&new_event);
        _in_repick = FALSE;
    }

    if (_gen_all_enter_events == false) {
        // new_current_item may have been set to NULL during the call to
        // emitEvent() above
        if ((_new_current_item != _current_item) && button_down) {
            _left_grabbed_item = TRUE;
            return retval;
        }
    }

    // Handle the rest of cases
    _left_grabbed_item = FALSE;
    _current_item = _new_current_item;

    if (_current_item != nullptr) {
        GdkEvent new_event;

        new_event = _pick_event;
        new_event.type = GDK_ENTER_NOTIFY;
        new_event.crossing.detail = GDK_NOTIFY_ANCESTOR;
        new_event.crossing.subwindow = nullptr;
        retval = emitEvent(&new_event);
    }

    return retval;
}

gint SPCanvas::handle_doubleclick(GtkWidget *widget, GdkEventButton *event)
{
    // Maybe we want to use double click on canvas so retain here
    return 0;
}

gint SPCanvas::handle_button(GtkWidget *widget, GdkEventButton *event)
{
    SPCanvas *canvas = SP_CANVAS (widget);

    int retval = FALSE;

    // dispatch normally regardless of the event's window if an item
    // has a pointer grab in effect
    if (!canvas->_grabbed_item &&
        event->window != getWindow(canvas))
        return retval;

    int mask;
    switch (event->button) {
    case 1:
        mask = GDK_BUTTON1_MASK;
        break;
    case 2:
        mask = GDK_BUTTON2_MASK;
        break;
    case 3:
        mask = GDK_BUTTON3_MASK;
        break;
    case 4:
        mask = GDK_BUTTON4_MASK;
        break;
    case 5:
        mask = GDK_BUTTON5_MASK;
        break;
    default:
        mask = 0;
    }
    static unsigned next_canvas_doubleclick = 0;

    switch (event->type) {
    case GDK_BUTTON_PRESS:
        // Pick the current item as if the button were not pressed, and
        // then process the event.
        next_canvas_doubleclick = 0;
        if (!canvas->_split_hover) {
            canvas->_state = event->state;
            canvas->pickCurrentItem(reinterpret_cast<GdkEvent *>(event));
            canvas->_state ^= mask;
            retval = canvas->emitEvent((GdkEvent *)event);
        } else {
            canvas->_split_pressed = true;
            Geom::IntPoint cursor_pos = Geom::IntPoint(event->x, event->y);
            canvas->_spliter_in_control_pos = cursor_pos - (*canvas->_spliter_control).midpoint();
            if (canvas->_spliter && canvas->_spliter_control.contains(cursor_pos) && !canvas->_is_dragging) {
                canvas->_split_control_pressed = true;
            }
            retval = TRUE;
        }
        break;
    case GDK_2BUTTON_PRESS:
        // Pick the current item as if the button were not pressed, and
        // then process the event.
        next_canvas_doubleclick = reinterpret_cast<GdkEvent *>(event)->button.button;

        if (!canvas->_split_hover) {
            canvas->_state = event->state;
            canvas->pickCurrentItem(reinterpret_cast<GdkEvent *>(event));
            canvas->_state ^= mask;
            retval = canvas->emitEvent((GdkEvent *)event);
        } else {
            canvas->_split_pressed = true;
            retval = TRUE;
        }
        break;
    case GDK_3BUTTON_PRESS:
        // Pick the current item as if the button were not pressed, and
        // then process the event.
        if (!canvas->_split_hover) {
            canvas->_state = event->state;
            canvas->pickCurrentItem(reinterpret_cast<GdkEvent *>(event));
            canvas->_state ^= mask;
            retval = canvas->emitEvent((GdkEvent *)event);
        } else {
            canvas->_split_pressed = true;
            retval = TRUE;
        }
        break;

    case GDK_BUTTON_RELEASE:
        // Process the event as if the button were pressed, then repick
        // after the button has been released
        canvas->_split_pressed = false;
        if (next_canvas_doubleclick) {
            GdkEventButton *event2 = reinterpret_cast<GdkEventButton *>(event);
            handle_doubleclick(GTK_WIDGET(canvas), event2);
        }
        if (canvas->_split_hover) {
            retval = TRUE;
            bool spliter_clicked = false;
            bool reset = false;
            if (!canvas->_split_dragging) {
                GtkAllocation allocation;
                gtk_widget_get_allocation(GTK_WIDGET(canvas), &allocation);
                Geom::Point pos = canvas->_spliter_control_pos;
                double value = canvas->_split_vertical ? 1 / (allocation.height / (double)pos[Geom::Y])
                                                       : 1 / (allocation.width  / (double)pos[Geom::X]);
                if (canvas->_split_hover_vertical) {
                    canvas->_split_inverse = !canvas->_split_inverse;
                    spliter_clicked = true;
                    reset = canvas->_split_vertical ? true : false;
                    if (reset) {
                        canvas->_split_value = value;
                    }
                    canvas->_split_vertical = false;
                } else if (canvas->_split_hover_horizontal) {
                    canvas->_split_inverse = !canvas->_split_inverse;
                    spliter_clicked = true;
                    reset = !canvas->_split_vertical ? true : false;
                    if (reset) {
                        canvas->_split_value = value;
                    }
                    canvas->_split_vertical = true;
                }
                if (spliter_clicked) {
                    canvas->requestFullRedraw();
                }
            }
            canvas->_split_control_pressed = false;
            canvas->_split_dragging = false;
        } else {
            canvas->_state = event->state;
            retval = canvas->emitEvent((GdkEvent *)event);
            event->state ^= mask;
            canvas->_state = event->state;
            canvas->pickCurrentItem(reinterpret_cast<GdkEvent *>(event));
            event->state ^= mask;
        }
        break;

    default:
        g_assert_not_reached ();
    }

    return retval;
}

gint SPCanvas::handle_scroll(GtkWidget *widget, GdkEventScroll *event)
{
    return SP_CANVAS(widget)->emitEvent(reinterpret_cast<GdkEvent *>(event));
}

static inline void request_motions(GdkWindow *w, GdkEventMotion *event) {
    gdk_window_get_device_position(w,
                                   gdk_event_get_device((GdkEvent *)(event)),
                                   nullptr, nullptr, nullptr);
    gdk_event_request_motions(event);
}

void SPCanvas::set_cursor(GtkWidget *widget)
{
    SPCanvas *canvas = SP_CANVAS(widget);
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    GdkDisplay *display = gdk_display_get_default();
    GdkCursor *cursor = nullptr;
    if (canvas->_split_hover_vertical) {
        if (canvas->_changecursor != 1) {
            cursor = gdk_cursor_new_from_name(display, "pointer");
            gdk_window_set_cursor(gtk_widget_get_window(widget), cursor);
            g_object_unref(cursor);
            canvas->paintSpliter();
            canvas->_changecursor = 1;
        }
    } else if (canvas->_split_hover_horizontal) {
        if (canvas->_changecursor != 2) {
            cursor = gdk_cursor_new_from_name(display, "pointer");
            gdk_window_set_cursor(gtk_widget_get_window(widget), cursor);
            g_object_unref(cursor);
            canvas->paintSpliter();
            canvas->_changecursor = 2;
        }  
    } else if (canvas->_split_hover) {
        if (canvas->_changecursor != 3) {
            if (_split_vertical) {
                cursor = gdk_cursor_new_from_name(display, "ew-resize");
            } else {
                cursor = gdk_cursor_new_from_name(display, "ns-resize");
            }
            gdk_window_set_cursor(gtk_widget_get_window(widget), cursor);
            g_object_unref(cursor);
            canvas->paintSpliter();
            canvas->_changecursor = 3;
        }
    } else {
        if (desktop && 
            desktop->event_context && 
            !canvas->_split_pressed &&
            (canvas->_changecursor != 0 && canvas->_changecursor != 4)) 
        {
            desktop->event_context->sp_event_context_update_cursor();
            canvas->paintSpliter();
            canvas->_changecursor = 4;
        }
    }
}


void sp_reset_spliter(SPCanvas *canvas)
{
    canvas->_spliter = Geom::OptIntRect();
    canvas->_spliter_area = Geom::OptIntRect();
    canvas->_spliter_control = Geom::OptIntRect();
    canvas->_spliter_top = Geom::OptIntRect();
    canvas->_spliter_bottom = Geom::OptIntRect();
    canvas->_spliter_left = Geom::OptIntRect();
    canvas->_spliter_right = Geom::OptIntRect();
    canvas->_spliter_control_pos = Geom::Point();
    canvas->_spliter_in_control_pos = Geom::Point();
    canvas->_split_value = 0.5;
    canvas->_split_vertical = true;
    canvas->_split_inverse = false;
    canvas->_split_hover_vertical = false;
    canvas->_split_hover_horizontal = false;
    canvas->_split_hover = false;
    canvas->_split_pressed = false;
    canvas->_split_control_pressed = false;
    canvas->_split_dragging = false;
}


int SPCanvas::handle_motion(GtkWidget *widget, GdkEventMotion *event)
{
    int status;
    SPCanvas *canvas = SP_CANVAS (widget);
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    trackLatency((GdkEvent *)event);

    if (event->window != getWindow(canvas)) {
        return FALSE;
    }

    if (canvas->_root == nullptr) // canvas being deleted
        return FALSE;
    
    Geom::IntPoint cursor_pos = Geom::IntPoint(event->x, event->y);
        
    if (desktop && desktop->splitMode()) {
        if (canvas->_spliter &&
            ((*canvas->_spliter).contains(cursor_pos) || canvas->_spliter_control.contains(cursor_pos)) &&
            !canvas->_is_dragging) {
            canvas->_split_hover = true;
        } else {
            canvas->_split_hover = false;
        }
        if (canvas->_spliter_left && canvas->_spliter_right &&
            ((*canvas->_spliter_left).contains(cursor_pos) || (*canvas->_spliter_right).contains(cursor_pos)) &&
            !canvas->_is_dragging) {
            canvas->_split_hover_horizontal = true;
        } else {
            canvas->_split_hover_horizontal = false;
        }
        if (!canvas->_split_hover_horizontal && canvas->_spliter_top && canvas->_spliter_bottom &&
            ((*canvas->_spliter_top).contains(cursor_pos) || (*canvas->_spliter_bottom).contains(cursor_pos)) &&
            !canvas->_is_dragging) {
            canvas->_split_hover_vertical = true;
        } else {
            canvas->_split_hover_vertical = false;
        }

        canvas->set_cursor(widget);
    }
    if (canvas->_split_pressed && desktop && desktop->event_context && desktop->splitMode()) {
        GtkAllocation allocation;
        canvas->_split_dragging = true;
        gtk_widget_get_allocation(GTK_WIDGET(canvas), &allocation);
        double hide_horiz = 1 / (allocation.width  / (double)cursor_pos[Geom::X]);
        double hide_vert  = 1 / (allocation.height / (double)cursor_pos[Geom::Y]);
        double value = canvas->_split_vertical ? hide_horiz : hide_vert;
        if (hide_horiz < 0.03 || hide_horiz > 0.97 || hide_vert < 0.03 || hide_vert > 0.97) {
            if (desktop && desktop->event_context) {
                desktop->event_context->sp_event_context_update_cursor();
                desktop->toggleSplitMode();
                sp_reset_spliter(canvas);
            }
        } else {
            canvas->_split_value = value;
            if (canvas->_split_control_pressed && !canvas->_is_dragging) {
                canvas->_spliter_control_pos = cursor_pos - canvas->_spliter_in_control_pos;
            }
        }
        canvas->requestFullRedraw();
        status = 1;
    } else {
        if (desktop && desktop->event_context && desktop->xrayMode()) {
            sp_reset_spliter(canvas);
            canvas->_xray_orig = desktop->point(true);
            canvas->_xray_orig *= desktop->current_zoom();
            if (!SP_ACTIVE_DOCUMENT->is_yaxisdown()) {
                canvas->_xray_orig[Geom::Y] *= -1.0;
            }
            canvas->_xray = true;
            if (canvas->_xray_orig[Geom::X] != Geom::infinity()) {
                if (canvas->_xray_rect) {
                    canvas->dirtyRect(*canvas->_xray_rect);
                    canvas->_xray_rect = Geom::OptIntRect();
                }
                canvas->addIdle();
            }
            status = 1;
        } else {
            if (canvas->_xray_rect) {
                canvas->dirtyRect(*canvas->_xray_rect);
                canvas->_xray_rect = Geom::OptIntRect();
            }
            canvas->_xray = false;
        }
        canvas->_state = event->state;
        canvas->pickCurrentItem(reinterpret_cast<GdkEvent *>(event));
        status = canvas->emitEvent(reinterpret_cast<GdkEvent *>(event));
        if (event->is_hint) {
            request_motions(gtk_widget_get_window(widget), event);
        }
    }

    if (desktop) {
        SPCanvasArena *arena = SP_CANVAS_ARENA(desktop->drawing);
        if (desktop->splitMode()) {
            bool contains = canvas->_spliter_area.contains(cursor_pos);
            bool setoutline = canvas->_split_inverse ? !contains : contains;
            arena->drawing.setOutlineSensitive(setoutline);
        } else if (canvas->_xray) {
            arena->drawing.setOutlineSensitive(true);
        } else {
            arena->drawing.setOutlineSensitive(false);
        }
    }
    return status;
}

void SPCanvas::paintSingleBuffer(Geom::IntRect const &paint_rect, Geom::IntRect const &canvas_rect, int /*sw*/)
{

    // Prevent crash if paintSingleBuffer is called before _backing_store is
    // initialized.
    if (_backing_store == nullptr)
        return;

    SPCanvasBuf buf;
    buf.buf = nullptr;
    buf.buf_rowstride = 0;
    buf.rect = paint_rect;
    buf.canvas_rect = canvas_rect;
    buf.device_scale = _device_scale;
    buf.is_empty = true;

    // Make sure the following code does not go outside of _backing_store's data
    // FIXME for device_scale.
    assert(cairo_image_surface_get_format(_backing_store) == CAIRO_FORMAT_ARGB32);
    assert(paint_rect.left() - _x0 >= 0);
    assert(paint_rect.top() - _y0 >= 0);
    assert(paint_rect.right() - _x0 <= cairo_image_surface_get_width(_backing_store));
    assert(paint_rect.bottom() - _y0 <= cairo_image_surface_get_height(_backing_store));

    // Create a temporary surface that draws directly to _backing_store
    cairo_surface_flush(_backing_store);
    // cairo_surface_write_to_png( _backing_store, "debug0.png" );
    unsigned char *data = cairo_image_surface_get_data(_backing_store);
    int stride = cairo_image_surface_get_stride(_backing_store);

    // Check we are using correct device scale
    double x_scale = 0;
    double y_scale = 0;
    cairo_surface_get_device_scale(_backing_store, &x_scale, &y_scale);
    assert (_device_scale == (int)x_scale);
    assert (_device_scale == (int)y_scale);

    // Move to the right row
    data += stride * (paint_rect.top() - _y0) * (int)y_scale;
    // Move to the right pixel inside of that row
    data += 4 * (paint_rect.left() - _x0) * (int)x_scale;
    cairo_surface_t *imgs =
        cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32,
                                            paint_rect.width()  * _device_scale,
                                            paint_rect.height() * _device_scale,
                                            stride);
    cairo_surface_set_device_scale(imgs, _device_scale, _device_scale);

    buf.ct = cairo_create(imgs);
    cairo_save(buf.ct);
    cairo_translate(buf.ct, -paint_rect.left(), -paint_rect.top());
    cairo_set_source(buf.ct, _background);
    cairo_set_operator(buf.ct, CAIRO_OPERATOR_SOURCE);
    cairo_paint(buf.ct);
    cairo_restore(buf.ct);
    // cairo_surface_write_to_png( imgs, "debug1.png" );

    if (_root->visible) {
        SP_CANVAS_ITEM_GET_CLASS(_root)->render(_root, &buf);
    }

    // cairo_surface_write_to_png( imgs, "debug2.png" );

    // output to X
    cairo_destroy(buf.ct);

#if defined(HAVE_LIBLCMS2)
    if (_enable_cms_display_adj) {
        cmsHTRANSFORM transf = nullptr;
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        bool fromDisplay = prefs->getBool( "/options/displayprofile/from_display");
        if ( fromDisplay ) {
            transf = Inkscape::CMSSystem::getDisplayPer(_cms_key);
        } else {
            transf = Inkscape::CMSSystem::getDisplayTransform();
        }

        if (transf) {
            cairo_surface_flush(imgs);
            unsigned char *px = cairo_image_surface_get_data(imgs);
            int stride = cairo_image_surface_get_stride(imgs);
            for (int i=0; i<paint_rect.height(); ++i) {
                unsigned char *row = px + i*stride;
                Inkscape::CMSSystem::doTransform(transf, row, row, paint_rect.width());
            }
            cairo_surface_mark_dirty(imgs);
        }
    }
#endif // defined(HAVE_LIBLCMS2)

    cairo_surface_mark_dirty(_backing_store);
    // cairo_surface_write_to_png( _backing_store, "debug3.png" );

    // Mark the painted rectangle clean
    markRect(paint_rect, 0);

    cairo_surface_destroy(imgs);

    gtk_widget_queue_draw_area(GTK_WIDGET(this), paint_rect.left() -_x0, paint_rect.top() - _y0,
        paint_rect.width(), paint_rect.height());
}

void SPCanvas::paintXRayBuffer(Geom::IntRect const &paint_rect, Geom::IntRect const &canvas_rect)
{

    // Prevent crash if paintSingleBuffer is called before _backing_store is
    // initialized.
    if (_backing_store == nullptr)
        return;
    if (!canvas_rect.contains(paint_rect) && !canvas_rect.intersects(paint_rect)) {
        return;
    }
    SPCanvasBuf buf;
    buf.buf = nullptr;
    buf.buf_rowstride = 0;
    buf.rect = paint_rect;
    buf.canvas_rect = canvas_rect;
    buf.device_scale = _device_scale;
    buf.is_empty = true;
    // Make sure the following code does not go outside of _backing_store's data
    // FIXME for device_scale.
    assert(cairo_image_surface_get_format(_backing_store) == CAIRO_FORMAT_ARGB32);
    cairo_surface_t *copy_backing = cairo_surface_create_similar_image(_backing_store, CAIRO_FORMAT_ARGB32,
                                                                       paint_rect.width(), paint_rect.height());
    buf.ct = cairo_create(copy_backing);
    cairo_t *result = cairo_create(_backing_store);
    cairo_translate(result, -_x0, -_y0);
    cairo_save(buf.ct);
    cairo_set_source_rgba(buf.ct, 1, 1, 1, 0);
    cairo_fill(buf.ct);
    cairo_arc(buf.ct, _xray_radius, _xray_radius, _xray_radius, 0, 2 * M_PI);
    cairo_clip(buf.ct);
    cairo_paint(buf.ct);
    cairo_translate(buf.ct, -paint_rect.left(), -paint_rect.top());
    cairo_set_source(buf.ct, _background);
    cairo_set_operator(buf.ct, CAIRO_OPERATOR_SOURCE);
    cairo_paint(buf.ct);
    cairo_translate(buf.ct, paint_rect.left(), paint_rect.top());
    // cairo_surface_write_to_png( copy_backing, "debug1.png" );



    if (_root->visible) {
        SP_CANVAS_ITEM_GET_CLASS(_root)->render(_root, &buf);
    }
    cairo_restore(buf.ct);
    cairo_arc(buf.ct, _xray_radius, _xray_radius, _xray_radius, 0, 2 * M_PI);
    cairo_clip(buf.ct);
    cairo_set_operator(buf.ct, CAIRO_OPERATOR_DEST_IN);
    cairo_paint(buf.ct);
    // cairo_surface_write_to_png( copy_backing, "debug2.png" );

    // output to X
    cairo_arc(buf.ct, _xray_radius, _xray_radius, _xray_radius, 0, 2 * M_PI);

    cairo_set_source_surface(result, copy_backing, paint_rect.left(), paint_rect.top());
    cairo_set_operator(buf.ct, CAIRO_OPERATOR_IN);
    cairo_paint(result);
    cairo_surface_destroy(copy_backing);
    cairo_destroy(buf.ct);
    cairo_destroy(result);
    
    // cairo_surface_write_to_png( _backing_store, "debug3.png" );
    cairo_surface_mark_dirty(_backing_store);
    // Mark the painted rectangle un-clean to remove old x-ray when mouse change position
    _xray_rect = paint_rect;
    gtk_widget_queue_draw_area(GTK_WIDGET(this), paint_rect.left() - _x0, paint_rect.top() - _y0, paint_rect.width(),
                               paint_rect.height());
}

void SPCanvas::paintSpliter()
{
    // Prevent crash if paintSingleBuffer is called before _backing_store is
    // initialized.
    if (_backing_store == nullptr)
        return;
    // Todo: scale for HiDPI screens
    SPCanvas *canvas = SP_CANVAS(this);
    double ds = canvas->_device_scale;
    Geom::IntRect linerect = (*canvas->_spliter);
    Geom::IntPoint c0 = Geom::IntPoint(linerect.corner(0));
    Geom::IntPoint c1 = Geom::IntPoint(linerect.corner(1));
    Geom::IntPoint c2 = Geom::IntPoint(linerect.corner(2));
    Geom::IntPoint c3 = Geom::IntPoint(linerect.corner(3));
    // We need to draw the line in middle of pixel
    // https://developer.gnome.org/gtkmm-tutorial/stable/sec-cairo-drawing-lines.html.en:17.2.3
    double gapx = _split_vertical ? 0.5 : 0;
    double gapy = _split_vertical ? 0 : 0.5;
    Geom::Point start = _split_vertical ? Geom::middle_point(c0, c1) : Geom::middle_point(c0, c3);
    Geom::Point end = _split_vertical ? Geom::middle_point(c2, c3) : Geom::middle_point(c1, c2);
    Geom::Point middle = Geom::middle_point(start, end);
    if (canvas->_spliter_control_pos != Geom::Point()) {
        middle[Geom::X] = _split_vertical ? middle[Geom::X] : canvas->_spliter_control_pos[Geom::X];
        middle[Geom::Y] = _split_vertical ? canvas->_spliter_control_pos[Geom::Y] : middle[Geom::Y];
    }
    canvas->_spliter_control_pos = middle;
    canvas->_spliter_control = Geom::OptIntRect(Geom::IntPoint(int(middle[0] - (25 * ds)), int(middle[1] - (25 * ds))),
                                                Geom::IntPoint(int(middle[0] + (25 * ds)), int(middle[1] + (25 * ds))));
    canvas->_spliter_top = Geom::OptIntRect(Geom::IntPoint(int(middle[0] - (25 * ds)), int(middle[1] - (25 * ds))),
                                            Geom::IntPoint(int(middle[0] + (25 * ds)), int(middle[1] - (10 * ds))));
    canvas->_spliter_bottom = Geom::OptIntRect(Geom::IntPoint(int(middle[0] - (25 * ds)), int(middle[1] + (25 * ds))),
                                               Geom::IntPoint(int(middle[0] + (25 * ds)), int(middle[1] + (10 * ds))));
    canvas->_spliter_left = Geom::OptIntRect(Geom::IntPoint(int(middle[0] - (25 * ds)), int(middle[1] - (10 * ds))),
                                             Geom::IntPoint(int(middle[0]), int(middle[1] + (10 * ds))));
    canvas->_spliter_right = Geom::OptIntRect(Geom::IntPoint(int(middle[0] + (25 * ds)), int(middle[1] + (10 * ds))),
                                              Geom::IntPoint(int(middle[0]), int(middle[1] - (10 * ds))));
    cairo_t *ct = cairo_create(_backing_store);
    cairo_set_antialias(ct, CAIRO_ANTIALIAS_BEST);
    cairo_set_line_width(ct, 1.0 * ds);
    cairo_line_to(ct, start[0] + gapx, start[1] + gapy);
    cairo_line_to(ct, end[0] + gapx, end[1] + gapy);
    cairo_stroke_preserve(ct);
    if (canvas->_split_hover || canvas->_split_pressed) {
        cairo_set_source_rgba(ct, 0.15, 0.15, 0.15, 1);
    } else {
        cairo_set_source_rgba(ct, 0.3, 0.3, 0.3, 1);
    }
    cairo_stroke(ct);
    /*
    Get by: https://gitlab.com/snippets/1777221
    M 40,19.999997 C 39.999998,8.9543032 31.045694,0 20,0 8.9543062,0 1.6568541e-6,8.9543032 0,19.999997
    0,31.045692 8.954305,39.999997 20,39.999997 31.045695,39.999997 40,31.045692 40,19.999997 Z M
    11.109859,15.230724 2.8492384,19.999997 11.109861,24.769269 Z M 29.249158,15.230724
    37.509779,19.999997 29.249158,24.769269 Z M 15.230728,29.03051 20,37.29113 24.769272,29.030509 Z
    M 15.230728,10.891209 20,2.630586 24.769272,10.891209 Z */
    double updwidth = _split_vertical ? 0 : linerect.width();
    double updheight = _split_vertical ? linerect.height() : 0;
    cairo_translate(ct, middle[0] - (20 * ds), middle[1] - (20 * ds));
    cairo_scale(ct, ds, ds);
    cairo_move_to(ct, 40, 19.999997);
    cairo_curve_to(ct, 39.999998, 8.9543032, 31.045694, 0, 20, 0);
    cairo_curve_to(ct, 8.9543062, 0, 0, 8.9543032, 0, 19.999997);
    cairo_curve_to(ct, 0, 31.045692, 8.954305, 39.999997, 20, 39.999997);
    cairo_curve_to(ct, 31.045695, 39.999997, 40, 31.045692, 40, 19.999997);
    cairo_close_path(ct);
    cairo_fill(ct);
    cairo_move_to(ct, 15.230728, 10.891209);
    cairo_line_to(ct, 20, 2.630586);
    cairo_line_to(ct, 24.769272, 10.891209);
    cairo_close_path(ct);
    if (canvas->_split_hover_vertical) {
        cairo_set_source_rgba(ct, 0.90, 0.90, 0.90, 1);
    } else {
        cairo_set_source_rgba(ct, 0.6, 0.6, 0.6, 1);
    }
    cairo_fill(ct);
    cairo_move_to(ct, 15.230728, 29.03051);
    cairo_line_to(ct, 20, 37.29113);
    cairo_line_to(ct, 24.769272, 29.030509);
    cairo_close_path(ct);
    cairo_fill(ct);
    cairo_move_to(ct, 11.109859, 15.230724);
    cairo_line_to(ct, 2.8492384, 19.999997);
    cairo_line_to(ct, 11.109861, 24.769269);
    cairo_close_path(ct);
    if (canvas->_split_hover_horizontal) {
        cairo_set_source_rgba(ct, 0.90, 0.90, 0.90, 1);
    } else {
        cairo_set_source_rgba(ct, 0.6, 0.6, 0.6, 1);
    }
    cairo_fill(ct);
    cairo_move_to(ct, 29.249158, 15.230724);
    cairo_line_to(ct, 37.509779, 19.999997);
    cairo_line_to(ct, 29.249158, 24.769269);
    cairo_close_path(ct);
    cairo_fill(ct);
    cairo_scale(ct, 1 / ds, 1 / ds);
    cairo_translate(ct, -middle[0] - (20 * ds), -middle[1] - (20 * ds));
    cairo_restore(ct);
    cairo_destroy(ct);
    gtk_widget_queue_draw_area(GTK_WIDGET(this), start[0] - (21 * ds), start[1] - (21 * ds), updwidth + (42 * ds),
                               updheight + (42 * ds));
}

struct PaintRectSetup {
    Geom::IntRect canvas_rect;
    gint64 start_time;
    int max_pixels;
    Geom::Point mouse_loc;
};

int SPCanvas::paintRectInternal(PaintRectSetup const *setup, Geom::IntRect const &this_rect)
{
    gint64 now = g_get_monotonic_time();
    gint64 elapsed = now - setup->start_time;

    // Allow only very fast buffers to be run together;
    // as soon as the total redraw time exceeds 1ms, cancel;
    // this returns control to the idle loop and allows Inkscape to process user input
    // (potentially interrupting the redraw); as soon as Inkscape has some more idle time,
    if (elapsed > 1000) {

        // Interrupting redraw isn't always good.
        // For example, when you drag one node of a big path, only the buffer containing
        // the mouse cursor will be redrawn again and again, and the rest of the path
        // will remain stale because Inkscape never has enough idle time to redraw all
        // of the screen. To work around this, such operations set a forced_redraw_limit > 0.
        // If this limit is set, and if we have aborted redraw more times than is allowed,
        // interrupting is blocked and we're forced to redraw full screen once
        // (after which we can again interrupt forced_redraw_limit times).
        if (_forced_redraw_limit < 0 ||
            _forced_redraw_count < _forced_redraw_limit) {

            if (_forced_redraw_limit != -1) {
                _forced_redraw_count++;
            }
            return false;
        }
        _forced_redraw_count = 0;
    }

    // Find the optimal buffer dimensions
    int bw = this_rect.width();
    int bh = this_rect.height();
    // we dont want to stop the idle process if the area is empty
    if ((bw < 1) || (bh < 1))
        return 1;

    if (bw * bh < setup->max_pixels) {
        // We are small enough
        /*
        GdkRectangle r;
        r.x = this_rect.x0 - setup->canvas->x0;
        r.y = this_rect.y0 - setup->canvas->y0;
        r.width = this_rect.x1 - this_rect.x0;
        r.height = this_rect.y1 - this_rect.y0;

        GdkWindow *window = gtk_widget_get_window(GTK_WIDGET(setup->canvas));
        gdk_window_begin_paint_rect(window, &r);
        */

        paintSingleBuffer(this_rect, setup->canvas_rect, bw);
        _splits++;
        //gdk_window_end_paint(window);
        return 1;
    }

    Geom::IntRect lo, hi;

    /*
      This test determines the redraw strategy:

      bw < bh (strips mode) splits across the smaller dimension of the rect and therefore (on
      horizontally-stretched windows) results in redrawing in horizontal strips (from cursor point, in
      both directions if the cursor is in the middle). This is traditional for Inkscape since old days,
      and seems to be faster for drawings with many smaller objects at zoom-out.

      bw > bh (chunks mode) splits across the larger dimension of the rect and therefore paints in
      almost-square chunks, again from the cursor point. It's sometimes faster for drawings with few slow
      (e.g. blurred) objects crossing the entire screen. It also appears to be somewhat psychologically
      faster.

      The default for now is the strips mode.
    */

    static int TILE_SIZE = 16;

    if (bw < bh || bh < 2 * TILE_SIZE) {
        int mid = this_rect[Geom::X].middle();

        lo = Geom::IntRect(this_rect.left(), this_rect.top(), mid, this_rect.bottom());
        hi = Geom::IntRect(mid, this_rect.top(), this_rect.right(), this_rect.bottom());

        if (setup->mouse_loc[Geom::X] < mid) {
            // Always paint towards the mouse first
            return paintRectInternal(setup, lo)
                && paintRectInternal(setup, hi);
        } else {
            return paintRectInternal(setup, hi)
                && paintRectInternal(setup, lo);
        }
    } else {
        int mid = this_rect[Geom::Y].middle();

        lo = Geom::IntRect(this_rect.left(), this_rect.top(), this_rect.right(), mid);
        hi = Geom::IntRect(this_rect.left(), mid, this_rect.right(), this_rect.bottom());

        if (setup->mouse_loc[Geom::Y] < mid) {
            // Always paint towards the mouse first
            return paintRectInternal(setup, lo)
                && paintRectInternal(setup, hi);
        } else {
            return paintRectInternal(setup, hi)
                && paintRectInternal(setup, lo);
        }
    }
}


bool SPCanvas::paintRect(int xx0, int yy0, int xx1, int yy1)
{
    GtkAllocation allocation;
    g_return_val_if_fail (!_need_update, false);

    gtk_widget_get_allocation(GTK_WIDGET(this), &allocation);

    // Find window rectangle in 'world coordinates'.
    Geom::IntRect canvas_rect = Geom::IntRect::from_xywh(_x0, _y0,
        allocation.width, allocation.height);
    Geom::IntRect paint_rect(xx0, yy0, xx1, yy1);

    Geom::OptIntRect area = paint_rect & canvas_rect;
    // we dont want to stop the idle process if the area is empty
    if (!area || area->hasZeroArea()) {
        return true;
    }
    paint_rect = *area;

    PaintRectSetup setup;
    setup.canvas_rect = canvas_rect;

    // Save the mouse location
    gint x, y;

    auto const display = Gdk::Display::get_default();
    auto const seat    = display->get_default_seat();
    auto const device  = seat->get_pointer();

    gdk_window_get_device_position(gtk_widget_get_window(GTK_WIDGET(this)),
                                   device->gobj(),
                                   &x, &y, nullptr);

    setup.mouse_loc = sp_canvas_window_to_world(this, Geom::Point(x,y));

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    unsigned tile_multiplier = prefs->getIntLimited("/options/rendering/tile-multiplier", 16, 1, 512);

    if (_rendermode != Inkscape::RENDERMODE_OUTLINE) {
        // use 256K as a compromise to not slow down gradients
        // 256K is the cached buffer and we need 4 channels
        setup.max_pixels = 65536 * tile_multiplier; // 256K/4
    } else {
        // paths only, so 1M works faster
        // 1M is the cached buffer and we need 4 channels
        setup.max_pixels = 262144;
    }

    // Start the clock
    setup.start_time = g_get_monotonic_time();
    // Go
    return paintRectInternal(&setup, paint_rect);
}

void SPCanvas::forceFullRedrawAfterInterruptions(unsigned int count, bool reset)
{
    _forced_redraw_limit = count;
    if (reset) {
        _forced_redraw_count = 0;
    }
}

void SPCanvas::endForcedFullRedraws()
{
    _forced_redraw_limit = -1;
}

gboolean SPCanvas::handle_draw(GtkWidget *widget, cairo_t *cr) {

    SPCanvas *canvas = SP_CANVAS(widget);

    if (canvas->_surface_for_similar == nullptr && canvas->_backing_store != nullptr) {

        // Device scale is copied but since this is only created one time, we'll
        // need to check/set device scale anytime it is used in case window moved
        // to monitor with different scale.
        canvas->_surface_for_similar =
            cairo_surface_create_similar(cairo_get_target(cr), CAIRO_CONTENT_COLOR_ALPHA, 1, 1);

        // Check we are using correct device scale
        double x_scale = 0;
        double y_scale = 0;
        cairo_surface_get_device_scale(canvas->_backing_store, &x_scale, &y_scale);
        assert (canvas->_device_scale == (int)x_scale);
        assert (canvas->_device_scale == (int)y_scale);

        // Reallocate backing store so that cairo can use shared memory
        // Function does NOT copy device scale! Width and height are in device pixels.
        cairo_surface_t *new_backing_store = cairo_surface_create_similar_image(
                canvas->_surface_for_similar, CAIRO_FORMAT_ARGB32,
                cairo_image_surface_get_width(canvas->_backing_store),
                cairo_image_surface_get_height(canvas->_backing_store));

        cairo_surface_set_device_scale(new_backing_store, canvas->_device_scale, canvas->_device_scale);

        // Copy the old backing store contents
        cairo_t *cr = cairo_create(new_backing_store);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_surface(cr, canvas->_backing_store, 0, 0);
        cairo_paint(cr);
        cairo_destroy(cr);
        cairo_surface_destroy(canvas->_backing_store);
        canvas->_backing_store = new_backing_store;
    }

    // Blit from the backing store, without regard for the clean region.
    // This is necessary because GTK clears the widget for us, which causes
    // severe flicker while drawing if we don't blit the old contents.
    cairo_set_source_surface(cr, canvas->_backing_store, 0, 0);
    cairo_paint(cr);

    cairo_rectangle_list_t *rects = cairo_copy_clip_rectangle_list(cr);
    cairo_region_t *dirty_region = cairo_region_create();

    for (int i = 0; i < rects->num_rectangles; i++) {
        cairo_rectangle_t rectangle = rects->rectangles[i];
        Geom::Rect dr = Geom::Rect::from_xywh(rectangle.x + canvas->_x0, rectangle.y + canvas->_y0,
                                              rectangle.width, rectangle.height);

        Geom::IntRect ir = dr.roundOutwards();
        cairo_rectangle_int_t irect = { ir.left(), ir.top(), ir.width(), ir.height() };
        cairo_region_union_rectangle(dirty_region, &irect);
    }
    cairo_rectangle_list_destroy(rects);
    cairo_region_subtract(dirty_region, canvas->_clean_region);

    // Render the dirty portion in the background
    if (!cairo_region_is_empty(dirty_region)) {
        canvas->addIdle();
    }
    cairo_region_destroy(dirty_region);

    return TRUE;
}

gint SPCanvas::handle_key_event(GtkWidget *widget, GdkEventKey *event)
{
    return SP_CANVAS(widget)->emitEvent(reinterpret_cast<GdkEvent *>(event));
}

gint SPCanvas::handle_crossing(GtkWidget *widget, GdkEventCrossing *event)
{
    SPCanvas *canvas = SP_CANVAS (widget);

    if (event->window != getWindow(canvas)) {
        return FALSE;
    }
    canvas->_state = event->state;
    return canvas->pickCurrentItem(reinterpret_cast<GdkEvent *>(event));
}

gint SPCanvas::handle_focus_in(GtkWidget *widget, GdkEventFocus *event)
{
    gtk_widget_grab_focus (widget);

    SPCanvas *canvas = SP_CANVAS (widget);

    if (canvas->_focused_item) {
        return canvas->emitEvent(reinterpret_cast<GdkEvent *>(event));
    } else {
        return FALSE;
    }
}

gint SPCanvas::handle_focus_out(GtkWidget *widget, GdkEventFocus *event)
{
    SPCanvas *canvas = SP_CANVAS(widget);

    if (canvas->_focused_item) {
        return canvas->emitEvent(reinterpret_cast<GdkEvent *>(event));
    } else {
        return FALSE;
    }
}

int SPCanvas::paint()
{
    if (_need_update) {
        sp_canvas_item_invoke_update(_root, Geom::identity(), 0);
        _need_update = FALSE;
    }
    SPCanvas *canvas = SP_CANVAS(this);
    GtkAllocation allocation;
    gtk_widget_get_allocation(GTK_WIDGET(canvas), &allocation);
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    SPCanvasArena *arena = nullptr;
    bool split = false;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _xray_radius = prefs->getIntLimited("/options/rendering/xray-radius", 100, 1, 1500);

    double split_x = 1;
    double split_y = 1;
    Inkscape::RenderMode rm = Inkscape::RENDERMODE_NORMAL;
    if (desktop) {
        split = desktop->splitMode();
        arena = SP_CANVAS_ARENA(desktop->drawing);
        rm = arena->drawing.renderMode();
        if (split) {

            // Start: Line
            split_x = !_split_vertical ? 0 : _split_value;
            split_y =  _split_vertical ? 0 : _split_value;
            Geom::IntCoord coord1x = (int((allocation.width) * split_x))  - (3 * canvas->_device_scale);
            Geom::IntCoord coord1y = (int((allocation.height) * split_y)) - (3 * canvas->_device_scale);

            // End: Line
            split_x = !_split_vertical ? 1 : _split_value;
            split_y =  _split_vertical ? 1 : _split_value;
            Geom::IntCoord coord2x = (int((allocation.width)  * split_x)) + (3 * canvas->_device_scale);
            Geom::IntCoord coord2y = (int((allocation.height) * split_y)) + (3 * canvas->_device_scale);

            _spliter = Geom::OptIntRect(coord1x, coord1y, coord2x, coord2y);

            // Start: Area
            split_x = !_split_vertical ? 0 : _split_value;
            split_y =  _split_vertical ? 0 : _split_value;
            coord1x = (int((allocation.width ) * split_x));
            coord1y = (int((allocation.height) * split_y));

            // End: area
            split_x = !_split_vertical ? 1 : _split_value;
            split_y =  _split_vertical ? 1 : _split_value;
            coord2x = int(allocation.width);
            coord2y = int(allocation.height);

            _spliter_area = Geom::OptIntRect(coord1x, coord1y, coord2x, coord2y);

        } else {
            sp_reset_spliter(canvas);
        }
    } else {
        sp_reset_spliter(canvas);
    }
    cairo_rectangle_int_t crect = { _x0, _y0, int(allocation.width * split_x), int(allocation.height * split_y) };
    split_x = !_split_vertical ? 0 : _split_value;
    split_y = _split_vertical ? 0 : _split_value;
    cairo_rectangle_int_t crect_outline = { _x0 + int(allocation.width * split_x),
                                            _y0 + int(allocation.height * split_y),
                                            int(allocation.width * (1 - split_x)),
                                            int(allocation.height * (1 - split_y)) };
    cairo_region_t *draw = nullptr;
    cairo_region_t *to_draw = nullptr;
    cairo_region_t *to_draw_outline = nullptr;
    if (_split_inverse && split) {
        to_draw = cairo_region_create_rectangle(&crect_outline);
        to_draw_outline = cairo_region_create_rectangle(&crect);
    } else {
        to_draw = cairo_region_create_rectangle(&crect);
        to_draw_outline = cairo_region_create_rectangle(&crect_outline);
    }

    cairo_region_get_extents(_clean_region, &crect);
    draw = cairo_region_create_rectangle(&crect);
    cairo_region_subtract(draw, _clean_region);
    cairo_region_get_extents(draw, &crect);
    cairo_region_subtract_rectangle(_clean_region, &crect);
    cairo_region_subtract(to_draw, _clean_region);
    cairo_region_subtract(to_draw_outline, _clean_region);
    cairo_region_destroy(draw);
    int n_rects = cairo_region_num_rectangles(to_draw);
    for (int i = 0; i < n_rects; ++i) {
        cairo_rectangle_int_t crect;
        cairo_region_get_rectangle(to_draw, i, &crect);
        if (!paintRect(crect.x, crect.y, crect.x + crect.width, crect.y + crect.height)) {
            // Aborted
            cairo_region_destroy(to_draw);
            cairo_region_destroy(to_draw_outline);
            return FALSE;
        };
    }

    if (split) {
        arena->drawing.setRenderMode(Inkscape::RENDERMODE_OUTLINE);
        bool exact = arena->drawing.getExact();
        arena->drawing.setExact(false);
        int n_rects = cairo_region_num_rectangles(to_draw_outline);
        for (int i = 0; i < n_rects; ++i) {
            cairo_rectangle_int_t crect;
            cairo_region_get_rectangle(to_draw_outline, i, &crect);
            if (!paintRect(crect.x, crect.y, crect.x + crect.width, crect.y + crect.height)) {
                // Aborted
                arena->drawing.setExact(exact);
                arena->drawing.setRenderMode(rm);
                cairo_region_destroy(to_draw);
                cairo_region_destroy(to_draw_outline);
                return FALSE;
            };
        }
        arena->drawing.setExact(exact);
        arena->drawing.setRenderMode(rm);
        canvas->paintSpliter();
    } else if (desktop && _xray) {
        if (rm != Inkscape::RENDERMODE_OUTLINE) {
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            prefs->setBool("/desktop/xrayactive", true);
            arena->drawing.setRenderMode(Inkscape::RENDERMODE_OUTLINE);
            bool exact = arena->drawing.getExact();
            arena->drawing.setExact(false);
            Geom::IntRect canvas_rect = Geom::IntRect::from_xywh(_x0, _y0, allocation.width, allocation.height);
            Geom::IntRect _xray_rect = Geom::IntRect::from_xywh(
                _xray_orig[0] - _xray_radius, _xray_orig[1] - _xray_radius, (_xray_radius * 2), (_xray_radius * 2));
            paintXRayBuffer(_xray_rect, canvas_rect);
            arena->drawing.setExact(exact);
            arena->drawing.setRenderMode(rm);
            prefs->setBool("/desktop/xrayactive", false);
        }
    }

    // we've had a full unaborted redraw, reset the full redraw counter
    if (_forced_redraw_limit != -1) {
        _forced_redraw_count = 0;
    }

    cairo_region_destroy(to_draw);
    cairo_region_destroy(to_draw_outline);

    return TRUE;
}

int SPCanvas::doUpdate()
{
    if (!_root) { // canvas may have already be destroyed by closing desktop during interrupted display!
        return TRUE;
    }
    if (_drawing_disabled) {
        return TRUE;
    }

    // Cause the update if necessary
    if (_need_update) {
        sp_canvas_item_invoke_update(_root, Geom::identity(), 0);
        _need_update = FALSE;
    }

    // Paint if able to
    if (gtk_widget_is_drawable(GTK_WIDGET(this))) {
        return paint();
    }

    // Pick new current item
    while (_need_repick) {
        _need_repick = FALSE;
        pickCurrentItem(&_pick_event);
    }

    return TRUE;
}

gint SPCanvas::idle_handler(gpointer data)
{
    SPCanvas *canvas = SP_CANVAS (data);
#ifdef DEBUG_PERFORMANCE
    static int totaloops = 1;
    gint64 now = 0;
    gint64 elapsed = 0;
    now = g_get_monotonic_time();
    elapsed = now - canvas->_idle_time;
    g_message("[%i] start loop %i in split %i at %f", canvas->_idle_id, totaloops, canvas->_splits,
                canvas->_totalelapsed / (double)1000000 + elapsed / (double)1000000);
#endif
    int ret = canvas->doUpdate();
    int n_rects = cairo_region_num_rectangles(canvas->_clean_region);
    if (n_rects > 1) { // not fully painted, maybe clean region is updated in middle of idle, reload again
        ret = 0;
    }

#ifdef DEBUG_PERFORMANCE
    if (ret == 0) {
        now = g_get_monotonic_time();
        elapsed = now - canvas->_idle_time;
        g_message("[%i] loop ended unclean at %f", canvas->_idle_id,
                    canvas->_totalelapsed / (double)1000000 + elapsed / (double)1000000);
        totaloops += 1;
    }
    if (ret) {
        // Reset idle id
        now = g_get_monotonic_time();
        elapsed = now - canvas->_idle_time;
        canvas->_totalelapsed += elapsed;
        SPDesktop *desktop = SP_ACTIVE_DESKTOP;
        if (desktop) {
            SPCanvasArena *arena = SP_CANVAS_ARENA(desktop->drawing);
            Inkscape::RenderMode rm = arena->drawing.renderMode();
            if (rm == Inkscape::RENDERMODE_OUTLINE) {
                canvas->_totalelapsed = 0;
                g_message("Outline mode, we reset and stop total counter");
            }
            g_message("[%i] finished", canvas->_idle_id);
            g_message("[%i] loops %i", canvas->_idle_id, totaloops);
            g_message("[%i] splits %i", canvas->_idle_id, canvas->_splits);
            g_message("[%i] duration %f", canvas->_idle_id, elapsed / (double)1000000);
            g_message("[%i] total %f (toggle outline mode to reset)", canvas->_idle_id,
                      canvas->_totalelapsed / (double)1000000);
            g_message("[%i] :::::::::::::::::::::::::::::::::::::::", canvas->_idle_id);
        }
        canvas->_idle_id = 0;
        totaloops = 1;
        canvas->_splits = 0;
    }
#else
    if (ret) {
        // Reset idle id
        canvas->_idle_id = 0;
    }
#endif
    return !ret;
}

void SPCanvas::addIdle()
{
    if (_idle_id == 0) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        guint redrawPriority = prefs->getIntLimited("/options/redrawpriority/value", G_PRIORITY_HIGH_IDLE, G_PRIORITY_HIGH_IDLE, G_PRIORITY_DEFAULT_IDLE);

#ifdef DEBUG_PERFORMANCE
        _idle_time = g_get_monotonic_time();
#endif
        _idle_id = gdk_threads_add_idle_full(redrawPriority, idle_handler, this, nullptr);
#ifdef DEBUG_PERFORMANCE
        g_message("[%i] launched %f", _idle_id, _totalelapsed / (double)1000000);
#endif
    }
}
void SPCanvas::removeIdle()
{
    if (_idle_id) {
        g_source_remove(_idle_id);
#ifdef DEBUG_PERFORMANCE
        g_message("[%i] aborted in split %i", _idle_id, _splits);
        _splits = 0;
#endif
        _idle_id = 0;
    }
}

SPCanvasGroup *SPCanvas::getRoot()
{
    return SP_CANVAS_GROUP(_root);
}

/**
 * Scroll screen to point 'c'. 'c' is measured in screen pixels.
 */
void SPCanvas::scrollTo( Geom::Point const &c, unsigned int clear, bool /*is_scrolling*/)
{
    // To do: extract out common code with SPCanvas::handle_size_allocate()

    // For HiDPI monitors
    int device_scale = gtk_widget_get_scale_factor(GTK_WIDGET(this));
    assert( device_scale == _device_scale);

    double cx = c[Geom::X];
    double cy = c[Geom::Y];

    int ix = (int) round(cx); // ix and iy are the new canvas coordinates (integer screen pixels)
    int iy = (int) round(cy); // cx might be negative, so (int)(cx + 0.5) will not do!
    int dx = ix - _x0; // dx and dy specify the displacement (scroll) of the
    int dy = iy - _y0; // canvas w.r.t its previous position

    Geom::IntRect old_area = getViewboxIntegers();
    Geom::IntRect new_area = old_area + Geom::IntPoint(dx, dy);
    bool outsidescrool = false;
    if (!new_area.intersects(old_area)) {
        outsidescrool = true;
    }
    GtkAllocation allocation;
    gtk_widget_get_allocation(&_widget, &allocation);

    // cairo_surface_write_to_png( _backing_store, "scroll1.png" );
    bool split = false;
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop && desktop->splitMode()) {
        split = true;
    }
    if (clear || split || _xray || outsidescrool) {
        _dx0 = cx; // here the 'd' stands for double, not delta!
        _dy0 = cy;
        _x0 = ix;
        _y0 = iy;
        requestFullRedraw();
    } else {
        // Adjust backing store contents
        assert(_backing_store);
        // this cairo operation is slow, improvements welcome
        cairo_surface_t *new_backing_store = nullptr;
        if (_surface_for_similar != nullptr)

            // Size in device pixels. Does not set device scale.
            new_backing_store =
                cairo_surface_create_similar_image(_surface_for_similar,
                                                CAIRO_FORMAT_ARGB32,
                                                allocation.width  * _device_scale,
                                                allocation.height * _device_scale);
        if (new_backing_store == nullptr)
            // Size in device pixels. Does not set device scale.
            new_backing_store =
                cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                        allocation.width  * _device_scale,
                                        allocation.height * _device_scale);

        // Set device scale
        cairo_surface_set_device_scale(new_backing_store, _device_scale, _device_scale);

        cairo_t *cr = cairo_create(new_backing_store);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        // Paint the background
        cairo_translate(cr, -ix, -iy);
        cairo_set_source_surface(cr, _backing_store, ix, iy);
        cairo_paint(cr);

        // cairo_surface_write_to_png( _backing_store, "scroll0.png" );

        // Copy the old backing store contents
        cairo_set_source_surface(cr, _backing_store, _x0, _y0);
        cairo_rectangle(cr, _x0, _y0, allocation.width, allocation.height);
        cairo_clip(cr);
        cairo_paint(cr);
        cairo_destroy(cr);
        cairo_surface_destroy(_backing_store);
        _backing_store = new_backing_store;
        _dx0 = cx; // here the 'd' stands for double, not delta!
        _dy0 = cy;
        _x0 = ix;
        _y0 = iy;
        cairo_rectangle_int_t crect = { _x0, _y0, allocation.width, allocation.height };
        cairo_region_intersect_rectangle(_clean_region, &crect);
    }

    SPCanvasArena *arena = SP_CANVAS_ARENA(desktop->drawing);
    if (arena) {
        Geom::IntRect expanded = new_area;
        Geom::IntPoint expansion(new_area.width()/2, new_area.height()/2);
        expanded.expandBy(expansion);
        arena->drawing.setCacheLimit(expanded, false);
    }

    if (!clear) {
        // scrolling without zoom; redraw only the newly exposed areas
        if ((dx != 0) || (dy != 0)) {
            if (gtk_widget_get_realized(GTK_WIDGET(this))) {
                SPCanvas *canvas = SP_CANVAS(this);
                if (split) {
                    double scroll_horiz = 1 / (allocation.width  / (double)-dx);
                    double scroll_vert  = 1 / (allocation.height / (double)-dy);
                    double gap = canvas->_split_vertical ? scroll_horiz : scroll_vert;
                    canvas->_split_value = canvas->_split_value + gap;
                    if (scroll_horiz < 0.03 || scroll_horiz > 0.97 || scroll_vert < 0.03 || scroll_vert > 0.97) {
                        if (canvas->_split_value > 0.97) {
                            canvas->_split_value = 0.97;
                        } else if (canvas->_split_value < 0.03) {
                            canvas->_split_value = 0.03;
                        }
                    }
                }
                gdk_window_scroll(getWindow(this), -dx, -dy);
            }
        }
    }
}

void SPCanvas::updateNow()
{
    if (_need_update) {
#ifdef DEBUG_PERFORMANCE
        guint64 now = g_get_monotonic_time();
        gint64 elapsed = now - _idle_time;
        g_message("[%i] start updateNow(): %f at %f", _idle_id, elapsed / (double)1000000,
                  _totalelapsed / (double)1000000 + elapsed / (double)1000000);
#endif
        doUpdate();
#ifdef DEBUG_PERFORMANCE
        now = g_get_monotonic_time();
        elapsed = now - _idle_time;
        g_message("[%i] end updateNow(): %f at %f", _idle_id, elapsed / (double)1000000,
                  _totalelapsed / (double)1000000 + elapsed / (double)1000000);
#endif
    }
}

void SPCanvas::requestUpdate()
{
    _need_update = TRUE;
    addIdle();
}

void SPCanvas::requestRedraw(int x0, int y0, int x1, int y1)
{
    if (!gtk_widget_is_drawable( GTK_WIDGET(this) )) {
        return;
    }
    if (x0 >= x1 || y0 >= y1) {
        return;
    }

    Geom::IntRect bbox(x0, y0, x1, y1);
    dirtyRect(bbox);
    addIdle();
}
void SPCanvas::requestFullRedraw()
{
    dirtyAll();
    addIdle();
}

void SPCanvas::setBackgroundColor(guint32 rgba) {
    double new_r = SP_RGBA32_R_F(rgba);
    double new_g = SP_RGBA32_G_F(rgba);
    double new_b = SP_RGBA32_B_F(rgba);
    if (!_background_is_checkerboard) {
        double old_r, old_g, old_b;
        cairo_pattern_get_rgba(_background, &old_r, &old_g, &old_b, nullptr);
        if (new_r == old_r && new_g == old_g && new_b == old_b) return;
    }
    if (_background) {
        cairo_pattern_destroy(_background);
    }
    _background = cairo_pattern_create_rgb(new_r, new_g, new_b);
    _background_is_checkerboard = false;
    requestFullRedraw();
}

void SPCanvas::setBackgroundCheckerboard(guint32 rgba)
{
    if (_background_is_checkerboard) return;
    if (_background) {
        cairo_pattern_destroy(_background);
    }
    _background = ink_cairo_pattern_create_checkerboard(rgba);
    _background_is_checkerboard = true;
    requestFullRedraw();
}

/**
 * Sets world coordinates from win and canvas.
 */
void sp_canvas_window_to_world(SPCanvas const *canvas, double winx, double winy, double *worldx, double *worldy)
{
    g_return_if_fail (canvas != nullptr);
    g_return_if_fail (SP_IS_CANVAS (canvas));

    if (worldx) *worldx = canvas->_x0 + winx;
    if (worldy) *worldy = canvas->_y0 + winy;
}

/**
 * Sets win coordinates from world and canvas.
 */
void sp_canvas_world_to_window(SPCanvas const *canvas, double worldx, double worldy, double *winx, double *winy)
{
    g_return_if_fail (canvas != nullptr);
    g_return_if_fail (SP_IS_CANVAS (canvas));

    if (winx) *winx = worldx - canvas->_x0;
    if (winy) *winy = worldy - canvas->_y0;
}

/**
 * Converts point from win to world coordinates.
 */
Geom::Point sp_canvas_window_to_world(SPCanvas const *canvas, Geom::Point const win)
{
    g_assert (canvas != nullptr);
    g_assert (SP_IS_CANVAS (canvas));

    return Geom::Point(canvas->_x0 + win[0], canvas->_y0 + win[1]);
}

/**
 * Converts point from world to win coordinates.
 */
Geom::Point sp_canvas_world_to_window(SPCanvas const *canvas, Geom::Point const world)
{
    g_assert (canvas != nullptr);
    g_assert (SP_IS_CANVAS (canvas));

    return Geom::Point(world[0] - canvas->_x0, world[1] - canvas->_y0);
}

/**
 * Returns true if point given in world coordinates is inside window.
 */
bool sp_canvas_world_pt_inside_window(SPCanvas const *canvas, Geom::Point const &world)
{
    GtkAllocation allocation;

    g_assert( canvas != nullptr );
    g_assert(SP_IS_CANVAS(canvas));

    GtkWidget *w = GTK_WIDGET(canvas);
    gtk_widget_get_allocation (w, &allocation);

    return ( ( canvas->_x0 <= world[Geom::X] )  &&
             ( canvas->_y0 <= world[Geom::Y] )  &&
             ( world[Geom::X] < canvas->_x0 + allocation.width )  &&
             ( world[Geom::Y] < canvas->_y0 + allocation.height ) );
}

/**
 * Return canvas window coordinates as Geom::Rect.
 */
Geom::Rect SPCanvas::getViewbox() const
{
    GtkAllocation allocation;

    gtk_widget_get_allocation (GTK_WIDGET (this), &allocation);
    return Geom::Rect::from_xywh(_dx0, _dy0, allocation.width, allocation.height);
}

/**
 * Return canvas window coordinates as integer rectangle.
 */
Geom::IntRect SPCanvas::getViewboxIntegers() const
{
    GtkAllocation allocation;

    gtk_widget_get_allocation (GTK_WIDGET(this), &allocation);
    return Geom::IntRect::from_xywh(_x0, _y0, allocation.width, allocation.height);
}

void SPCanvas::dirtyRect(Geom::IntRect const &area) {
    markRect(area, 1);
}

void SPCanvas::dirtyAll() {
    if (_clean_region && !cairo_region_is_empty(_clean_region)) {
        cairo_region_destroy(_clean_region);
        _clean_region = cairo_region_create();
    }
}

void SPCanvas::markRect(Geom::IntRect const &area, uint8_t val)
{
    cairo_rectangle_int_t crect = { area.left(), area.top(), area.width(), area.height() };
    if (val) {
        cairo_region_subtract_rectangle(_clean_region, &crect);
    } else {
        cairo_region_union_rectangle(_clean_region, &crect);
    }
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

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

#include "cms-system.h"
#include "color.h"
#include "debug/gdk-event-latency-tracker.h"
#include "desktop.h"
#include "display/cairo-utils.h"
#include "display/canvas-arena.h"
#include "display/rendermode.h"
#include "display/sp-canvas-group.h"
#include "display/sp-canvas.h"
#include "helper/sp-marshal.h"
#include "inkscape-window.h"
#include "inkscape.h"
#include "preferences.h"
#include "sodipodi-ctrlrect.h"
#include "ui/tools/node-tool.h"
#include "ui/tools/tool-base.h"
#include "widgets/desktop-widget.h"
#include <2geom/affine.h>
#include <2geom/rect.h>

using Inkscape::Debug::GdkEventLatencyTracker;


// Disabled by Mentalguy, many years ago in commit 427a81 
static bool const HAS_BROKEN_MOTION_HINTS = true;

// Define this to visualize the regions to be redrawn
//#define DEBUG_REDRAW 1;

// Define this to output the time spent in a full idle loop and the number of "tiles" painted
//#define DEBUG_PERFORMANCE 1;

// Tiles are a way to minimize the number of redraws, eliminating too small redraws.
// The canvas stores a 2D array of ints, each representing a TILE_SIZExTILE_SIZE pixels tile.
// If any part of it is dirtied, the entire tile is dirtied (its int is nonzero) and repainted.
#define TILE_SIZE 16

/**
 * The SPCanvasGroup vtable.
 */
struct SPCanvasGroupClass {
    SPCanvasItemClass parent_class;
};

static void ungrab_default_client_pointer()
{
    auto const display = Gdk::Display::get_default();
    auto const seat    = display->get_default_seat();
    seat->ungrab();
}

/**
 * A group of items.
 */
struct SPCanvasGroup {
    /**
     * Adds an item to a canvas group.
     */
    void add(SPCanvasItem *item);

    /**
     * Removes an item from a canvas group.
     */
    void remove(SPCanvasItem *item);

    /**
     * Class initialization function for SPCanvasGroupClass.
     */
    static void classInit(SPCanvasGroupClass *klass);

    /**
     * Callback. Empty.
     */
    static void init(SPCanvasGroup *group);

    /**
     * Callback that destroys all items in group and calls group's virtual
     * destroy() function.
     */
    static void destroy(SPCanvasItem *object);

    /**
     * Update handler for canvas groups.
     */
    static void update(SPCanvasItem *item, Geom::Affine const &affine, unsigned int flags);

    /**
     * Point handler for canvas groups.
     */
    static double point(SPCanvasItem *item, Geom::Point p, SPCanvasItem **actual_item);

    /**
     * Renders all visible canvas group items in buf rectangle.
     */
    static void render(SPCanvasItem *item, SPCanvasBuf *buf);

    static void viewboxChanged(SPCanvasItem *item, Geom::IntRect const &new_area);


    // Data members: ----------------------------------------------------------

    SPCanvasItem item;

    SPCanvasItemList items;
};

/**
 * The SPCanvas vtable.
 */
struct SPCanvasClass {
    GtkWidgetClass parent_class;
};

namespace {

gint const UPDATE_PRIORITY = G_PRIORITY_DEFAULT_IDLE;

GdkWindow *getWindow(SPCanvas *canvas)
{
    return gtk_widget_get_window(reinterpret_cast<GtkWidget *>(canvas));
}


// SPCanvasItem

enum {
    ITEM_EVENT,
    ITEM_LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_VISIBLE
};


void trackLatency(GdkEvent const *event);

enum {
  DESTROY,
  LAST_SIGNAL
};

/**
 * Callback that removes item from all referrers and destroys it.
 */
void sp_canvas_item_dispose(GObject           *object);
void sp_canvas_item_finalize(GObject          *object);
void sp_canvas_item_real_destroy(SPCanvasItem *object);

static guint       object_signals[LAST_SIGNAL] = { 0 };

/**
 * Sets up the newly created SPCanvasItem.
 *
 * We make it static for encapsulation reasons since it was nowhere used.
 */
void sp_canvas_item_construct(SPCanvasItem *item, SPCanvasGroup *parent, gchar const *first_arg_name, va_list args);

/**
 * Helper that returns true iff item is descendant of parent.
 */
bool is_descendant(SPCanvasItem const *item, SPCanvasItem const *parent);

guint item_signals[ITEM_LAST_SIGNAL] = { 0 };

} // namespace

G_DEFINE_TYPE(SPCanvasItem, sp_canvas_item, G_TYPE_INITIALLY_UNOWNED);

static void
sp_canvas_item_class_init(SPCanvasItemClass *klass)
{
    GObjectClass *gobject_class = (GObjectClass *) klass;

    item_signals[ITEM_EVENT] = g_signal_new ("event",
                                             G_TYPE_FROM_CLASS (klass),
                                             G_SIGNAL_RUN_LAST,
                                             ((glong)((guint8*)&(klass->event) - (guint8*)klass)),
                                             nullptr, nullptr,
                                             sp_marshal_BOOLEAN__POINTER,
                                             G_TYPE_BOOLEAN, 1,
                                             GDK_TYPE_EVENT);

    gobject_class->dispose  = sp_canvas_item_dispose;
    gobject_class->finalize = sp_canvas_item_finalize;
    klass->destroy          = sp_canvas_item_real_destroy;

    object_signals[DESTROY] =
      g_signal_new ("destroy",
                    G_TYPE_FROM_CLASS (gobject_class),
                    (GSignalFlags)(G_SIGNAL_RUN_CLEANUP | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS),
		    G_STRUCT_OFFSET (SPCanvasItemClass, destroy),
		    nullptr, nullptr,
		    g_cclosure_marshal_VOID__VOID,
		    G_TYPE_NONE, 0);
}

static void
sp_canvas_item_init(SPCanvasItem *item)
{
    item->xform = Geom::Affine(Geom::identity());
    item->ctrlResize = 0;
    item->ctrlType = Inkscape::CTRL_TYPE_UNKNOWN;
    item->ctrlFlags = Inkscape::CTRL_FLAG_NORMAL;

    // TODO items should not be visible on creation - this causes kludges with items
    // that should be initially invisible; examples of such items: node handles, the CtrlRect
    // used for rubberbanding, path outline, etc.
    item->visible = TRUE;
    item->in_destruction = false;
    item->pickable = true;
}

SPCanvasItem *sp_canvas_item_new(SPCanvasGroup *parent, GType type, gchar const *first_arg_name, ...)
{
    va_list args;

    g_return_val_if_fail(parent != nullptr, NULL);
    g_return_val_if_fail(SP_IS_CANVAS_GROUP(parent), NULL);
    g_return_val_if_fail(g_type_is_a(type, SP_TYPE_CANVAS_ITEM), NULL);

    SPCanvasItem *item = SP_CANVAS_ITEM(g_object_new(type, nullptr));

    va_start(args, first_arg_name);
    sp_canvas_item_construct(item, parent, first_arg_name, args);
    va_end(args);

    return item;
}

namespace {

void sp_canvas_item_construct(SPCanvasItem *item, SPCanvasGroup *parent, gchar const *first_arg_name, va_list args)
{
    g_return_if_fail(SP_IS_CANVAS_GROUP(parent));
    g_return_if_fail(SP_IS_CANVAS_ITEM(item));

    // manually invoke list_member_hook constructor
    new (&(item->member_hook_)) boost::intrusive::list_member_hook<>();

    item->parent = SP_CANVAS_ITEM(parent);
    item->canvas = item->parent->canvas;

    g_object_set_valist(G_OBJECT(item), first_arg_name, args);

    SP_CANVAS_GROUP(item->parent)->add(item);

    sp_canvas_item_request_update(item);
}

} // namespace

/**
 * Helper function that requests redraw only if item's visible flag is set.
 */
static void redraw_if_visible(SPCanvasItem *item)
{
    if (item->visible) {
        int x0 = (int)(item->x1);
        int x1 = (int)(item->x2);
        int y0 = (int)(item->y1);
        int y1 = (int)(item->y2);

        if (x0 !=0 || x1 !=0 || y0 !=0 || y1 !=0) {
            item->canvas->requestRedraw((int)(item->x1 - 1), (int)(item->y1 -1), (int)(item->x2 + 1), (int)(item->y2 + 1));
        }
    }
}

void sp_canvas_item_destroy(SPCanvasItem *item)
{
  g_return_if_fail(SP_IS_CANVAS_ITEM(item));

  if (!item->in_destruction)
    g_object_run_dispose(G_OBJECT(item));
}

namespace {
void sp_canvas_item_dispose(GObject *object)
{
    SPCanvasItem *item = SP_CANVAS_ITEM (object);

    /* guard against reinvocations during
     * destruction with the in_destruction flag.
     */
    if (!item->in_destruction)
    {
      item->in_destruction=true;

      // Hack: if this is a ctrlrect, move it to 0,0;
      // this redraws only the stroke of the rect to be deleted,
      // avoiding redraw of the entire area
      if (SP_IS_CTRLRECT(item)) {
          SP_CTRLRECT(object)->setRectangle(Geom::Rect(Geom::Point(0,0),Geom::Point(0,0)));
          SP_CTRLRECT(object)->update(item->xform, 0);
      } else {
          redraw_if_visible (item);
      }
      item->visible = FALSE;

      if (item == item->canvas->_current_item) {
          item->canvas->_current_item = nullptr;
          item->canvas->_need_repick = TRUE;
      }

      if (item == item->canvas->_new_current_item) {
          item->canvas->_new_current_item = nullptr;
          item->canvas->_need_repick = TRUE;
      }

      if (item == item->canvas->_grabbed_item) {
          item->canvas->_grabbed_item = nullptr;
          ungrab_default_client_pointer();
      }

      if (item == item->canvas->_focused_item) {
          item->canvas->_focused_item = nullptr;
      }

      if (item->parent) {
          SP_CANVAS_GROUP(item->parent)->remove(item);
      }

      g_signal_emit (object, object_signals[DESTROY], 0);
      item->in_destruction = false;
    }

    G_OBJECT_CLASS(sp_canvas_item_parent_class)->dispose(object);
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

void sp_canvas_item_real_destroy(SPCanvasItem *object)
{
  g_signal_handlers_destroy(object);
}

void sp_canvas_item_finalize(GObject *gobject)
{
  SPCanvasItem *object = SP_CANVAS_ITEM(gobject);

  if (g_object_is_floating (object))
    {
      g_warning ("A floating object was finalized. This means that someone\n"
		 "called g_object_unref() on an object that had only a floating\n"
		 "reference; the initial floating reference is not owned by anyone\n"
		 "and must be removed with g_object_ref_sink().");
    }

  G_OBJECT_CLASS (sp_canvas_item_parent_class)->finalize (gobject);
}
} // namespace

/**
 * Helper function to update item and its children.
 *
 * NB! affine is parent2canvas.
 */
static void sp_canvas_item_invoke_update(SPCanvasItem *item, Geom::Affine const &affine, unsigned int flags)
{
    // Apply the child item's transform
    Geom::Affine child_affine = item->xform * affine;

    // apply object flags to child flags
    int child_flags = flags & ~SP_CANVAS_UPDATE_REQUESTED;

    if (item->need_update) {
        child_flags |= SP_CANVAS_UPDATE_REQUESTED;
    }

    if (item->need_affine) {
        child_flags |= SP_CANVAS_UPDATE_AFFINE;
    }

    if (child_flags & (SP_CANVAS_UPDATE_REQUESTED | SP_CANVAS_UPDATE_AFFINE)) {
        if (SP_CANVAS_ITEM_GET_CLASS (item)->update) {
            SP_CANVAS_ITEM_GET_CLASS (item)->update(item, child_affine, child_flags);
        }
    }

    item->need_update = FALSE;
    item->need_affine = FALSE;
}

/**
 * Helper function to invoke the point method of the item.
 *
 * The argument x, y should be in the parent's item-relative coordinate
 * system.  This routine applies the _split_inverse of the item's transform,
 * maintaining the affine invariant.
 */
static double sp_canvas_item_invoke_point(SPCanvasItem *item, Geom::Point p, SPCanvasItem **actual_item)
{
    if (SP_CANVAS_ITEM_GET_CLASS(item)->point) {
        return SP_CANVAS_ITEM_GET_CLASS (item)->point (item, p, actual_item);
    }

    return Geom::infinity();
}

/**
 * Makes the item's affine transformation matrix be equal to the specified
 * matrix.
 *
 * @item: A canvas item.
 * @affine: An affine transformation matrix.
 */
void sp_canvas_item_affine_absolute(SPCanvasItem *item, Geom::Affine const &affine)
{
    item->xform = affine;

    if (!item->need_affine) {
        item->need_affine = TRUE;
        if (item->parent != nullptr) {
            sp_canvas_item_request_update (item->parent);
        } else {
            item->canvas->requestUpdate();
        }
    }

    item->canvas->_need_repick = TRUE;
}

/**
 * Raises the item in its parent's stack by the specified number of positions.
 *
 * @param item A canvas item.
 * @param positions Number of steps to raise the item.
 *
 * If the number of positions is greater than the distance to the top of the
 * stack, then the item is put at the top.
 */
void sp_canvas_item_raise(SPCanvasItem *item, int positions)
{
    g_return_if_fail (item != nullptr);
    g_return_if_fail (SP_IS_CANVAS_ITEM (item));
    g_return_if_fail (positions >= 0);

    if (!item->parent || positions == 0) {
        return;
    }

    SPCanvasGroup *parent = SP_CANVAS_GROUP (item->parent);
    auto from = parent->items.iterator_to(*item);
    auto to = from;

    for (int i = 0; i <= positions && to != parent->items.end(); ++i) {
        ++to;
    }

    parent->items.erase(from);
    parent->items.insert(to, *item);

    redraw_if_visible (item);
    item->canvas->_need_repick = TRUE;
}

void sp_canvas_item_raise_to_top(SPCanvasItem *item)
{
    g_return_if_fail (item != nullptr);
    g_return_if_fail (SP_IS_CANVAS_ITEM (item));
    if (!item->parent)
        return;
    SPCanvasGroup *parent = SP_CANVAS_GROUP (item->parent);
    parent->items.erase(parent->items.iterator_to(*item));
    parent->items.push_back(*item);
    redraw_if_visible (item);
    item->canvas->_need_repick = TRUE;
}



/**
 * Lowers the item in its parent's stack by the specified number of positions.
 *
 * @param item A canvas item.
 * @param positions Number of steps to lower the item.
 *
 * If the number of positions is greater than the distance to the bottom of the
 * stack, then the item is put at the bottom.
 */
void sp_canvas_item_lower(SPCanvasItem *item, int positions)
{
    g_return_if_fail (item != nullptr);
    g_return_if_fail (SP_IS_CANVAS_ITEM (item));
    g_return_if_fail (positions >= 1);

    SPCanvasGroup *parent = SP_CANVAS_GROUP(item->parent);

    if (!parent || positions == 0 || item == &parent->items.front()) {
        return;
    }

    auto from = parent->items.iterator_to(*item);
    auto to = from;
    g_assert(from != parent->items.end());

    for (int i = 0; i < positions && to != parent->items.begin(); ++i) {
        --to;
    }

    parent->items.erase(from);
    parent->items.insert(to, *item);

    redraw_if_visible (item);
    item->canvas->_need_repick = TRUE;
}

void sp_canvas_item_lower_to_bottom(SPCanvasItem *item)
{
    g_return_if_fail (item != nullptr);
    g_return_if_fail (SP_IS_CANVAS_ITEM (item));
    if (!item->parent)
        return;
    SPCanvasGroup *parent = SP_CANVAS_GROUP (item->parent);
    parent->items.erase(parent->items.iterator_to(*item));
    parent->items.push_front(*item);
    redraw_if_visible (item);
    item->canvas->_need_repick = TRUE;
}

bool sp_canvas_item_is_visible(SPCanvasItem *item)
{
    return item->visible;
}

/**
 * Sets visible flag on item and requests a redraw.
 */
void sp_canvas_item_show(SPCanvasItem *item)
{
    g_return_if_fail (item != nullptr);
    g_return_if_fail (SP_IS_CANVAS_ITEM (item));

    if (item->visible) {
        return;
    }

    item->visible = TRUE;

    int x0 = (int)(item->x1);
    int x1 = (int)(item->x2);
    int y0 = (int)(item->y1);
    int y1 = (int)(item->y2);

    if (x0 !=0 || x1 !=0 || y0 !=0 || y1 !=0) {
        item->canvas->requestRedraw((int)(item->x1), (int)(item->y1), (int)(item->x2 + 1), (int)(item->y2 + 1));
        item->canvas->_need_repick = TRUE;
    }
}

/**
 * Clears visible flag on item and requests a redraw.
 */
void sp_canvas_item_hide(SPCanvasItem *item)
{
    g_return_if_fail (item != nullptr);
    g_return_if_fail (SP_IS_CANVAS_ITEM (item));

    if (!item->visible) {
        return;
    }

    item->visible = FALSE;

    int x0 = (int)(item->x1);
    int x1 = (int)(item->x2);
    int y0 = (int)(item->y1);
    int y1 = (int)(item->y2);

    if (x0 !=0 || x1 !=0 || y0 !=0 || y1 !=0) {
        item->canvas->requestRedraw((int)item->x1, (int)item->y1, (int)(item->x2 + 1), (int)(item->y2 + 1));
        item->canvas->_need_repick = TRUE;
    }
}

/**
 * Grab item under cursor.
 *
 * \pre !canvas->grabbed_item && item->flags & SP_CANVAS_ITEM_VISIBLE
 */
int sp_canvas_item_grab(SPCanvasItem *item, guint event_mask, GdkCursor *cursor, guint32 etime)
{
    g_return_val_if_fail (item != nullptr, -1);
    g_return_val_if_fail (SP_IS_CANVAS_ITEM (item), -1);
    g_return_val_if_fail (gtk_widget_get_mapped (GTK_WIDGET (item->canvas)), -1);

    if (item->canvas->_grabbed_item) {
        return -1;
    }

    // This test disallows grabbing events by an invisible item, which may be useful
    // sometimes. An example is the hidden control point used for the selector component,
    // where it is used for object selection and rubberbanding. There seems to be nothing
    // preventing this except this test, so I removed it.
    // -- Krzysztof Kosiński, 2009.08.12
    //if (!(item->flags & SP_CANVAS_ITEM_VISIBLE))
    //    return -1;

    if (HAS_BROKEN_MOTION_HINTS) {
        event_mask &= ~GDK_POINTER_MOTION_HINT_MASK;
    }

    // fixme: Top hack (Lauris)
    // fixme: If we add key masks to event mask, Gdk will abort (Lauris)
    // fixme: But Canvas actually does get key events, so all we need is routing these here
    auto display = gdk_display_get_default();
    auto seat    = gdk_display_get_default_seat(display);
    gdk_seat_grab(seat,
                  getWindow(item->canvas),
                  GDK_SEAT_CAPABILITY_ALL_POINTING,
                  FALSE,
                  cursor,
                  nullptr,
                  nullptr,
                  nullptr);

    item->canvas->_grabbed_item = item;
    item->canvas->_grabbed_event_mask = event_mask;
    item->canvas->_current_item = item; // So that events go to the grabbed item

    return 0;
}

/**
 * Ungrabs the item, which must have been grabbed in the canvas, and ungrabs the
 * mouse.
 *
 * @param item A canvas item that holds a grab.
 */
void sp_canvas_item_ungrab(SPCanvasItem *item)
{
    g_return_if_fail (item != nullptr);
    g_return_if_fail (SP_IS_CANVAS_ITEM (item));

    if (item->canvas->_grabbed_item != item) {
        return;
    }

    item->canvas->_grabbed_item = nullptr;
    ungrab_default_client_pointer();
}

/**
 * Returns the product of all transformation matrices from the root item down
 * to the item.
 */
Geom::Affine sp_canvas_item_i2w_affine(SPCanvasItem const *item)
{
    g_assert (SP_IS_CANVAS_ITEM (item)); // should we get this?

    Geom::Affine affine = Geom::identity();

    while (item) {
        affine *= item->xform;
        item = item->parent;
    }
    return affine;
}

namespace {

bool is_descendant(SPCanvasItem const *item, SPCanvasItem const *parent)
{
    while (item) {
        if (item == parent) {
            return true;
        }
        item = item->parent;
    }

    return false;
}

} // namespace

/**
 * Requests that the canvas queue an update for the specified item.
 *
 * To be used only by item implementations.
 */
void sp_canvas_item_request_update(SPCanvasItem *item)
{
    if (item->need_update) {
        return;
    }

    item->need_update = TRUE;

    if (item->parent != nullptr) {
        // Recurse up the tree
        sp_canvas_item_request_update (item->parent);
    } else {
        // Have reached the top of the tree, make sure the update call gets scheduled.
        item->canvas->requestUpdate();
    }
}

/**
 * Returns position of item in group.
 */
gint sp_canvas_item_order (SPCanvasItem * item)
{
    SPCanvasGroup * p = SP_CANVAS_GROUP(item->parent);
    size_t index = 0;
    for (auto it = p->items.begin(); it != p->items.end(); ++it, ++index) {
        if (item == &(*it)) {
            return index;
        }
    }

    return -1;
}

// SPCanvasGroup
G_DEFINE_TYPE(SPCanvasGroup, sp_canvas_group, SP_TYPE_CANVAS_ITEM);

static void sp_canvas_group_class_init(SPCanvasGroupClass *klass)
{
    SPCanvasItemClass *item_class = reinterpret_cast<SPCanvasItemClass *>(klass);

    item_class->destroy = SPCanvasGroup::destroy;
    item_class->update = SPCanvasGroup::update;
    item_class->render = SPCanvasGroup::render;
    item_class->point = SPCanvasGroup::point;
    item_class->viewbox_changed = SPCanvasGroup::viewboxChanged;
}

static void sp_canvas_group_init(SPCanvasGroup * group)
{
    new (&group->items) SPCanvasItemList;
}

void SPCanvasGroup::destroy(SPCanvasItem *object)
{
    g_return_if_fail(object != nullptr);
    g_return_if_fail(SP_IS_CANVAS_GROUP(object));

    SPCanvasGroup *group = SP_CANVAS_GROUP(object);

    for (auto it = group->items.begin(); it != group->items.end();) {
        SPCanvasItem *item = &(*it);
        it++;
        sp_canvas_item_destroy(item);
    }

    group->items.clear();
    group->items.~SPCanvasItemList(); // invoke manually

    if (SP_CANVAS_ITEM_CLASS(sp_canvas_group_parent_class)->destroy) {
        (* SP_CANVAS_ITEM_CLASS(sp_canvas_group_parent_class)->destroy)(object);
    }
}

void SPCanvasGroup::update(SPCanvasItem *item, Geom::Affine const &affine, unsigned int flags)
{
    SPCanvasGroup *group = SP_CANVAS_GROUP(item);
    Geom::OptRect bounds;

    for (auto & item : group->items) {
        SPCanvasItem *i = &item;

        sp_canvas_item_invoke_update (i, affine, flags);

        if ( (i->x2 > i->x1) && (i->y2 > i->y1) ) {
            bounds.expandTo(Geom::Point(i->x1, i->y1));
            bounds.expandTo(Geom::Point(i->x2, i->y2));
        }
    }

    if (bounds) {
        item->x1 = bounds->min()[Geom::X];
        item->y1 = bounds->min()[Geom::Y];
        item->x2 = bounds->max()[Geom::X];
        item->y2 = bounds->max()[Geom::Y];
    } else {
        // FIXME ?
        item->x1 = item->x2 = item->y1 = item->y2 = 0;
    }
}

double SPCanvasGroup::point(SPCanvasItem *item, Geom::Point p, SPCanvasItem **actual_item)
{
    SPCanvasGroup *group = SP_CANVAS_GROUP(item);
    double const x = p[Geom::X];
    double const y = p[Geom::Y];
    int x1 = (int)(x - item->canvas->_close_enough);
    int y1 = (int)(y - item->canvas->_close_enough);
    int x2 = (int)(x + item->canvas->_close_enough);
    int y2 = (int)(y + item->canvas->_close_enough);

    double best = 0.0;
    *actual_item = nullptr;

    double dist = 0.0;
    for (auto & it : group->items) {
        SPCanvasItem *child = &it;

        if ((child->x1 <= x2) && (child->y1 <= y2) && (child->x2 >= x1) && (child->y2 >= y1)) {
            SPCanvasItem *point_item = nullptr; // cater for incomplete item implementations

            int pickable;
            if (child->visible && child->pickable && SP_CANVAS_ITEM_GET_CLASS(child)->point) {
                dist = sp_canvas_item_invoke_point(child, p, &point_item);
                pickable = TRUE;
            } else {
                pickable = FALSE;
            }

            // TODO: This metric should be improved, because in case of (partly) overlapping items we will now
            // always select the last one that has been added to the group. We could instead select the one
            // of which the center is the closest, for example. One can then move to the center
            // of the item to be focused, and have that one selected. Of course this will only work if the
            // centers are not coincident, but at least it's better than what we have now.
            // See the extensive comment in Inkscape::SelTrans::_updateHandles()
            if (pickable && point_item && ((int) (dist + 0.5) <= item->canvas->_close_enough)) {
                best = dist;
                *actual_item = point_item;
            }
        }
    }

    return best;
}

void SPCanvasGroup::render(SPCanvasItem *item, SPCanvasBuf *buf)
{
    SPCanvasGroup *group = SP_CANVAS_GROUP(item);

    for (auto & item : group->items) {
        SPCanvasItem *child = &item;
        if (child->visible) {
            if ((child->x1 < buf->rect.right()) &&
                (child->y1 < buf->rect.bottom()) &&
                (child->x2 > buf->rect.left()) &&
                (child->y2 > buf->rect.top())) {
                if (SP_CANVAS_ITEM_GET_CLASS(child)->render) {
                    SP_CANVAS_ITEM_GET_CLASS(child)->render(child, buf);
                }
            }
        }
    }
}

void SPCanvasGroup::viewboxChanged(SPCanvasItem *item, Geom::IntRect const &new_area)
{
    SPCanvasGroup *group = SP_CANVAS_GROUP(item);
    for (auto & item : group->items) {
        SPCanvasItem *child = &item;
        if (child->visible) {
            if (SP_CANVAS_ITEM_GET_CLASS(child)->viewbox_changed) {
                SP_CANVAS_ITEM_GET_CLASS(child)->viewbox_changed(child, new_area);
            }
        }
    }
}

void SPCanvasGroup::add(SPCanvasItem *item)
{
    g_object_ref(item);
    g_object_ref_sink(item);

    items.push_back(*item);

    sp_canvas_item_request_update(item);
}

void SPCanvasGroup::remove(SPCanvasItem *item)
{
    g_return_if_fail(item != nullptr);

    auto position = items.iterator_to(*item);
    if (position != items.end()) {
        items.erase(position);
    }

    // Unparent the child
    item->parent = nullptr;
    g_object_unref(item);

}

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
            Geom::Point prev_orig = canvas->_xray_orig;
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
    if (bw < bh || bh < 2 * TILE_SIZE) {
        int mid = this_rect[Geom::X].middle();
        // Make sure that mid lies on a tile boundary
        mid = (mid / TILE_SIZE) * TILE_SIZE;

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
        // Make sure that mid lies on a tile boundary
        mid = (mid / TILE_SIZE) * TILE_SIZE;

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
            auto window = desktop->getInkscapeWindow();
            auto dtw = window->get_desktop_widget();
            bool hasrullers = prefs->getBool(desktop->is_fullscreen() ? "/fullscreen/rulers/state" : "/window/rulers/state");
            int hruler_gap = hasrullers ? dtw->get_hruler_thickness() : 1;
            int vruler_gap = hasrullers ? dtw->get_vruler_thickness() : 1;
            
            split_x = !_split_vertical ? 0 : _split_value;
            split_y = _split_vertical ? 0 : _split_value;

            Geom::IntCoord coord1x =
                allocation.x + (int((allocation.width) * split_x))  - (3 * canvas->_device_scale) - vruler_gap;
            Geom::IntCoord coord1y =
                allocation.y + (int((allocation.height) * split_y)) - (3 * canvas->_device_scale) - hruler_gap;
            split_x = !_split_vertical ? 1 : _split_value;
            split_y = _split_vertical ? 1 : _split_value;
            Geom::IntCoord coord2x =
                allocation.x + (int((allocation.width)  * split_x)) + (3 * canvas->_device_scale) - vruler_gap;
            Geom::IntCoord coord2y =
                allocation.y + (int((allocation.height) * split_y)) + (3 * canvas->_device_scale) - hruler_gap;
            _spliter = Geom::OptIntRect(coord1x, coord1y, coord2x, coord2y);
            split_x = !_split_vertical ? 0 : _split_value;
            split_y = _split_vertical ? 0 : _split_value;
            coord1x = allocation.x + (int((allocation.width ) * split_x)) - vruler_gap;
            coord1y = allocation.y + (int((allocation.height) * split_y)) - hruler_gap;
            split_x = !_split_vertical ? 1 : _split_value;
            split_y = _split_vertical ? 1 : _split_value;
            coord2x = allocation.x + allocation.width;
            coord2y = allocation.y + allocation.height;        
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
#ifdef DEBUG_PERFORMANCE
        _idle_time = g_get_monotonic_time();
#endif
        _idle_id = gdk_threads_add_idle_full(UPDATE_PRIORITY, idle_handler, this, nullptr);
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
void SPCanvas::scrollTo( Geom::Point const &c, unsigned int clear, bool is_scrolling)
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

inline int sp_canvas_tile_floor(int x)
{
    return (x & (~(TILE_SIZE - 1))) / TILE_SIZE;
}

inline int sp_canvas_tile_ceil(int x)
{
    return ((x + (TILE_SIZE - 1)) & (~(TILE_SIZE - 1))) / TILE_SIZE;
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

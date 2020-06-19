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

#include <gdkmm.h>

#include "sp-canvas-item.h"
#include "sp-canvas.h"
#include "sp-canvas-group.h"
#include "sodipodi-ctrlrect.h"

#include "helper/sp-marshal.h"


void trackLatency(GdkEvent const *event);

enum {
  DESTROY,
  LAST_SIGNAL
};

static guint object_signals[LAST_SIGNAL] = { 0 };

guint item_signals[] = {0};

/**
 * Callback that removes item from all referrers and destroys it.
 */
void sp_canvas_item_dispose(GObject           *object);
void sp_canvas_item_finalize(GObject          *object);
void sp_canvas_item_real_destroy(SPCanvasItem *object);


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

/**
 * Helper function to update item and its children.
 *
 * NB! affine is parent2canvas.
 */
void sp_canvas_item_invoke_update(SPCanvasItem *item, Geom::Affine const &affine, unsigned int flags)
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
double sp_canvas_item_invoke_point(SPCanvasItem *item, Geom::Point p, SPCanvasItem **actual_item)
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

    // fixme: Top hack (Lauris)
    // fixme: If we add key masks to event mask, Gdk will abort (Lauris)
    // fixme: But Canvas actually does get key events, so all we need is routing these here
    auto display = gdk_display_get_default();
    auto seat    = gdk_display_get_default_seat(display);
    GdkWindow *window = gtk_widget_get_window(reinterpret_cast<GtkWidget *>(item->canvas));
    gdk_seat_grab(seat,
                  window,
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

// For debugging: Print canvas item tree.
void
sp_canvas_item_recursive_print_tree (unsigned level, SPCanvasItem* item)
{
    if (level == 0) {
        std::cout << "Canvas Item Tree" << std::endl;
    }
    std::cout << "CI: ";
    for (unsigned i = 0; i < level; ++i) {
        std::cout << "  ";
    }

    char const *name = item->name;
    if (!name) {
        name = G_OBJECT_TYPE_NAME(item);
    }

    std::cout << name << std::endl;

    if (SP_IS_CANVAS_GROUP(item)) {
        SPCanvasGroup *group = SP_CANVAS_GROUP(item);
        for (auto & item : group->items) {
            sp_canvas_item_recursive_print_tree(level+1, &item);
        }
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

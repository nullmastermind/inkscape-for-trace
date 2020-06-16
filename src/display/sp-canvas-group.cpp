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

#include "sp-canvas-group.h"
#include "sp-canvas.h"

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
    SP_CANVAS_ITEM(group)->name = "CanvasGroup";
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

    double best = 0.0;
    *actual_item = nullptr;

    double dist = 0.0;
    for (auto & it : group->items) {
        SPCanvasItem *child = &it;

        if ((child->x1 <= x) && (x <= child->x2) && (child->y1 <= y) && (y <= child->y2)) {
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
            if (pickable && point_item && ((int) (dist + 0.5) <= 0)) { // THIS IS NEVER TRUE!
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

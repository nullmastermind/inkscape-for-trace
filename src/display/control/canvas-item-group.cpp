// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * A CanvasItem that contains other CanvasItem's.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of SPCanvasGroup
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "canvas-item-group.h"
#include "canvas-item-ctrl.h"  // Update sizes

namespace Inkscape {

CanvasItemGroup::CanvasItemGroup(CanvasItemGroup *group)
    : CanvasItem(group)
{
    _name = "CanvasItemGroup";
    _pickable = true; // For now all groups are pickable... look into turning this off for some groups (e.g. temp).
}

CanvasItemGroup::~CanvasItemGroup()
{
    while (!items.empty()) {
        CanvasItem & item = items.front();
        remove(&item);
    }

    if (_parent) {
        _parent->remove(this, false); // remove() should not delete this or we'll double delete!
    }
}

void CanvasItemGroup::add(CanvasItem *item)
{
#ifdef CANVAS_ITEM_DEBUG
    std::cout << "CanvasItemGroup::add: " << item->get_name() << " to " << _name << " " << items.size() << std::endl;
#endif
    items.push_back(*item);
    // canvas request update
}

void CanvasItemGroup::remove(CanvasItem *item, bool Delete)
{
#ifdef CANVAS_ITEM_DEBUG
    std::cout << "CanvasItemGroup::remove: " << item->get_name() << " from " << _name << " " << items.size() << std::endl;
#endif
    auto position = items.iterator_to(*item);
    if (position != items.end()) {
        position->set_parent(nullptr);
        items.erase(position);
        if (Delete) {
            delete (&*position);  // An item directly deleted should not be deleted here.
        }
    }
}

void CanvasItemGroup::update(Geom::Affine const &affine)
{
    if (_affine == affine && !_need_update) {
        // Nothing to do.
        return;
    }

    _affine = affine;
    _need_update = false;

    _bounds = Geom::Rect();  // Zero

    // Update all children and calculate new bounds.
    for (auto & item : items) {
        // We don't need to update what is not visible
        if (!item.is_visible()) continue;
        item.update(_affine);
        _bounds.unionWith(item.get_bounds());
    }
}

void CanvasItemGroup::render(Inkscape::CanvasItemBuffer *buf)
{
    if (_visible) {
        if (_bounds.interiorIntersects(buf->rect)) {
            for (auto & item : items) {
                item.render(buf);
            }
         }
    }
}

// Return last visible and pickable item that contains point.
// SPCanvasGroup returned distance but it was not used.
CanvasItem* CanvasItemGroup::pick_item(Geom::Point& p)
{
#ifdef CANVAS_ITEM_DEBUG
    std::cout << "CanvasItemGroup::pick_item:" << std::endl;
    std::cout << "  PICKING: In group: " << _name << "  bounds: " << _bounds << std::endl;
#endif
    for (auto item = items.rbegin(); item != items.rend(); ++item) { // C++20 will allow us to loop in reverse.
#ifdef CANVAS_ITEM_DEBUG
        std::cout << "    PICKING: Checking: " << item->get_name() << "  bounds: " << item->get_bounds() << std::endl;
#endif
        CanvasItem* picked_item = nullptr;
        if (item->is_visible()   &&
            item->is_pickable()  &&
            item->contains(p)    ) {

            auto group = dynamic_cast<CanvasItemGroup *>(&*item);
            if (group) {
                picked_item = group->pick_item(p);
            } else {
                picked_item = &*item;
            }
        }

        if (picked_item != nullptr) {
#ifdef CANVAS_ITEM_DEBUG
            std::cout << "  PICKING: pick_item: " << picked_item->get_name() << std::endl;
#endif
            return picked_item;
        }
    }

    return nullptr;   
}

void CanvasItemGroup::update_canvas_item_ctrl_sizes(int size_index)
{
    for (auto & item : items) {
        auto ctrl = dynamic_cast<CanvasItemCtrl *>(&item);
        if (ctrl) {
            // We can't use set_size_default as the preference file is updated ->after<- the signal is emitted!
            ctrl->set_size_via_index(size_index);
        }
        auto group = dynamic_cast<CanvasItemGroup *>(&item);
        if (group) {
            group->update_canvas_item_ctrl_sizes(size_index);
        }
    }
}

} // Namespace Inkscape
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

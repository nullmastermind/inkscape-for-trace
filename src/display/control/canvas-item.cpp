// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Abstract base class for on-canvas control items.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of SPCanvasItem
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "canvas-item.h"

#include "canvas-item-group.h"

#include "ui/widget/canvas.h"

namespace Inkscape {

CanvasItem::CanvasItem(CanvasItemGroup *group)
    : _name("CanvasItem")
{
    if (group) {
        group->add(this);
        _parent = group;
        _canvas = group->get_canvas();
        _affine = group->get_affine();
    }
}

CanvasItem::~CanvasItem()
{
    if (_parent) {
        _parent->remove(this, false); // remove() should not delete this or we'll double delete!
    }

    // Clear canvas of item.
    _canvas->redraw_area(_bounds);

    // Clear any pointers to this object in canvas.
    _canvas->canvas_item_clear(this);
}

bool CanvasItem::is_descendant_of(CanvasItem *ancestor)
{
    auto item = this;
    while (item) {
        if (item == ancestor) {
            return true;
        }
        item = item->get_parent();
    }
    return false;
}

int CanvasItem::get_z_position()
{
    if (!_parent) {
        std::cerr << "CanvasItem::get_z_position: No parent!" << std::endl;
        return -1;
    }

    size_t position = 0;
    for (auto it = _parent->items.begin(); it != _parent->items.end(); ++it, ++position) {
        if (&*it == this) {
            return position;
        }
    }

    std::cerr << "CanvasItem::get_z_position: item not found!" << std::endl;
    return -1;
}

void CanvasItem::set_z_position(unsigned int n)
{
    if (!_parent) {
        std::cerr << "CanvasItem::set_z_position: No parent!" << std::endl;
    }

    if (n == 0) {
        this->lower_to_bottom(); // Low cost operation
        return;
    }

    if (n > _parent->items.size() - 2) {
        this->raise_to_top(); // Low cost operation
        return;
    }

    _parent->items.erase(_parent->items.iterator_to(*this));

    size_t position = 0;
    for (auto it = _parent->items.begin(); it != _parent->items.end(); ++it, ++position) {
        if (position == n) {
            _parent->items.insert(it, *this);
            break;
        }
    }
}

void CanvasItem::raise_to_top()
{
    if (!_parent) {
        std::cerr << "CanvasItem::raise_to_top: No parent!" << std::endl;
    }

    _parent->items.erase(_parent->items.iterator_to(*this));
    _parent->items.push_back(*this);
}

void CanvasItem::lower_to_bottom()
{
    if (!_parent) {
        std::cerr << "CanvasItem::lower_to_bottom: No parent!" << std::endl;
    }

    _parent->items.erase(_parent->items.iterator_to(*this));
    _parent->items.push_front(*this);
}

// Indicate geometry changed and bounds needs recalculating.
void CanvasItem::request_update()
{
    _need_update = true;
    if (_parent) {
        _parent->request_update();
    } else {
        _canvas->request_update();
    }
}

void CanvasItem::show()
{
    if (_visible) {
        return; // Already visible.
    }

    _visible = true;
    _canvas->redraw_area(_bounds);
    _canvas->set_need_repick();
}

// Grab all events! TODO: Return boolean
int CanvasItem::grab(Gdk::EventMask event_mask, GdkCursor *cursor)
{
#ifdef CANVAS_ITEM_DEBUG
    std::cout << "CanvasItem::grab: " << _name << std::endl;
#endif
    // Don't grab if we already have a grabbed item!
    if (_canvas->get_grabbed_canvas_item()) {
        return -1;
    }

    auto const display = Gdk::Display::get_default();
    auto const seat    = display->get_default_seat();
    auto const window  = _canvas->get_window();
    auto cursor2 = Glib::wrap(cursor);
    seat->grab(window, Gdk::SEAT_CAPABILITY_ALL_POINTING, false, cursor2, nullptr);

    _canvas->set_grabbed_canvas_item(this, event_mask);
    _canvas->set_current_canvas_item(this); // So that all events go to grabbed item.
    return 0;
}

void CanvasItem::ungrab()
{
#ifdef CANVAS_ITEM_DEBUG
    std::cout << "CanvasItem::ungrab: " << _name << std::endl;
#endif
    if (_canvas->get_grabbed_canvas_item() != this) {
        return; // Sanity check
    }

    _canvas->set_grabbed_canvas_item(nullptr, (Gdk::EventMask)0); // Zero mask

    auto const display = Gdk::Display::get_default();
    auto const seat    = display->get_default_seat();
    seat->ungrab();
}

void CanvasItem::hide()
{
    if (!_visible) {
        return; // Already hidden
    }

    _visible = false;
    _canvas->redraw_area(_bounds);
    _canvas->set_need_repick();
}

void CanvasItem::set_fill(guint32 rgba)
{
    if (_fill != rgba) {
        _fill = rgba;
        _canvas->redraw_area(_bounds);
    }
}

void CanvasItem::set_stroke(guint32 rgba)
{
    if (_stroke != rgba) {
        _stroke = rgba;
        _canvas->redraw_area(_bounds);
    }
}

}  // Namespace Inkscape

void canvas_item_print_tree(Inkscape::CanvasItem *item)
{
    static int level = 0;
    if (level == 0) {
        std::cout << "Canvas Item Tree" << std::endl;
    }

    std::cout << "CC: ";
    for (unsigned i = 0; i < level; ++i) {
        std::cout << "  ";
    }

    std::cout << item->get_z_position() << ": " << item->get_name() << std::endl;

    auto group = dynamic_cast<Inkscape::CanvasItemGroup *>(item);
    if (group) {
        ++level;
        for (auto & item : group->items) {
            canvas_item_print_tree(&item);
        }
        --level;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

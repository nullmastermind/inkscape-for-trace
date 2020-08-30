// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_ITEM_H
#define SEEN_CANVAS_ITEM_H

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
 *
 * A note about coordinates:
 *
 *   1. Canvas items are constructed using document (SVG) coordinates.
 *   2. Calculations are made in canvas units, which is equivalent of SVG units multiplied by zoom factor.
 *      This is true for bounds and closest distance calculations.
 *   3  Drawing is done in screen units which is the same as canvas units but translated.
 *   The document and canvas origins overlap.
 *   The affine contains only scaling and rotating components.
 */

//#define CANVAS_ITEM_DEBUG

#include <gdk/gdk.h>  // GdkEvent
#include <gdkmm/device.h> // Gdk::EventMask
#include <glib.h>     // guint32
#include <sigc++/sigc++.h>

#include <2geom/rect.h>

#include <boost/intrusive/list.hpp>

#include "canvas-item-buffer.h"
#include "canvas-item-enums.h"

class SPItem;

namespace Inkscape {

static guint32 CANVAS_ITEM_COLORS[] = { 0x0000ff7f, 0xff00007f, 0xffff007f };

namespace UI::Widget {
class Canvas;
}

class CanvasItemGroup; // A canvas control that contains other canvas controls.

class CanvasItem {

public:
    CanvasItem(CanvasItemGroup* group);
    virtual ~CanvasItem();

    // Structure
    void set_canvas(UI::Widget::Canvas *canvas) { _canvas = canvas; }
    UI::Widget::Canvas* get_canvas() { return _canvas; }

    void set_parent(CanvasItemGroup *parent) { _parent = parent; }
    CanvasItemGroup* get_parent() { return _parent; }

    void set_item(SPItem *item) { _item = item; }
    SPItem* get_item() { return _item; }

    // Z Position
    bool is_descendant_of(CanvasItem *ancestor);
    void set_z_position(unsigned int n);
    int  get_z_position();    // z position in group.
    // void raise_by(unsigned int n);
    void raise_to_top();    // Move to top of group (last entry).
    // void lower_by(unsigned int n);
    void lower_to_bottom(); // Move to bottom of group (first entry).

    // Geometry
    void request_update();
    virtual void update(Geom::Affine const &affine) = 0;
    Geom::Affine get_affine() { return _affine; }
    Geom::Rect get_bounds() { return _bounds; }

    // Selection
    virtual bool contains(Geom::Point const &p, double tolerance = 0) { return _bounds.interiorContains(p); }
    int grab(Gdk::EventMask event_mask, GdkCursor *cursor = nullptr);
    void ungrab();

    // Display
    virtual void render(Inkscape::CanvasItemBuffer *buf) = 0;
    bool is_visible() { return _visible; }
    virtual void hide();
    virtual void show();

    // Properties
    virtual void set_fill(guint32 rgba);
    void set_fill(CanvasItemColor color) { set_fill(CANVAS_ITEM_COLORS[color]); }
    virtual void set_stroke(guint32 rgba);
    void set_stroke(CanvasItemColor color) { set_stroke(CANVAS_ITEM_COLORS[color]); }
    void set_name(std::string const &name) { _name = name; }
    std::string get_name() { return _name; }

    // Events
    void set_pickable(bool pickable) { _pickable = pickable; }
    bool is_pickable() { return _pickable; }
    sigc::connection connect_event(sigc::slot<bool, GdkEvent*> slot) {
        return _event_signal.connect(slot);
    }
    virtual bool handle_event(GdkEvent *event) {
        return _event_signal.emit(event); // Default just emit event.
    }

    // Boost linked list member hook, speeds deletion.
    boost::intrusive::list_member_hook<> member_hook;

protected:

    // Structure
    CanvasItemGroup *_parent = nullptr;
    Inkscape::UI::Widget::Canvas *_canvas = nullptr;
    SPItem *_item;  // The object this canvas item is linked to in some sense. Can be nullptr.

    // Geometry
    Geom::Rect _bounds;
    Geom::Affine _affine;
    bool _need_update = true; // Need update after creation!

    // Display
    bool _visible = true;
    bool _align_to_drawing = false; // Rotate if drawing is rotated. TODO: Implement!

    // Selection
    bool _pickable = false; // Most items are just for display and are not pickable!

    // Properties
    guint32 _fill    = CANVAS_ITEM_COLORS[CANVAS_ITEM_SECONDARY];
    guint32 _stroke  = CANVAS_ITEM_COLORS[CANVAS_ITEM_PRIMARY];
    std::string _name; // For debugging

    // Events
    sigc::signal<bool, GdkEvent*> _event_signal;
};


} // namespace Inkscape

/** Type for linked list storing CanvasItem's.
 *
 * Used to speed deletion when a group containes a large number of item's (as in nodes for a
 * complex path).
 */
typedef boost::intrusive::list<
    Inkscape::CanvasItem,
    boost::intrusive::member_hook<Inkscape::CanvasItem, boost::intrusive::list_member_hook<>,
                                  &Inkscape::CanvasItem::member_hook> > CanvasItemList;

// Recursively print CanvasItem tree.
void canvas_item_print_tree(Inkscape::CanvasItem *item);

#endif // SEEN_CANVAS_ITEM_H

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

// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_ITEM_DRAWING_H
#define SEEN_CANVAS_ITEM_DRAWING_H

/**
 * A class to render the SVG drawing.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of _SPCanvasArena.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <sigc++/sigc++.h>


#include "canvas-item.h"
#include "display/drawing-item.h" // Only for ctx... which should be remove (it's the same as _affine!).

namespace Inkscape {

class CachePref2Observer;

class Drawing;
class DrawingItem;
class Updatecontext;

class CanvasItemGroup; // A canvas control that contains other canvas controls.

class CanvasItemDrawing : public CanvasItem {

public:
    CanvasItemDrawing(CanvasItemGroup *group);
    ~CanvasItemDrawing() override;


    // Geometry
    UpdateContext get_context() { return _ctx; } // TODO Remove this as ctx only contains affine.

    void update(Geom::Affine const &affine) override;
    double closest_distance_to(Geom::Point const &p);

    // Selection
    bool contains(Geom::Point const &p, double tolerance = 0) override;

    // Display
    void render(Inkscape::CanvasItemBuffer *buf) override;
    Inkscape::Drawing * get_drawing() { return _drawing; }

    // Drawing items
    void set_active(Inkscape::DrawingItem *active) { _active_item = active; }
    Inkscape::DrawingItem *get_active() { return _active_item; }

    // Events
    bool handle_event(GdkEvent* event) override;
    void set_sticky(bool sticky) { _sticky = sticky; }

    // Signals
    sigc::connection connect_drawing_event(sigc::slot<bool, GdkEvent*, Inkscape::DrawingItem *> slot) {
        return _drawing_event_signal.connect(slot);
    }

protected:

    // Geometry

    // Selection
    Geom::Point _c;
    double _delta = Geom::infinity();
    Inkscape::DrawingItem *_active_item = nullptr;
    Inkscape::DrawingItem *_picked_item = nullptr;

    // Display
    Inkscape::Drawing *_drawing;
    Inkscape::UpdateContext _ctx;  // TODO Remove this... it's the same as _affine!

    // Events
    bool _cursor = false;
    bool _sticky = false; // Pick anything, even if hidden.

    // Properties
    CachePref2Observer *_observer = nullptr;

    // Signals
    sigc::signal<bool, GdkEvent*, Inkscape::DrawingItem *> _drawing_event_signal;
};


} // namespace Inkscape

#endif // SEEN_CANVAS_ITEM_DRAWING_H

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

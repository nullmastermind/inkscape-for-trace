// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_ITEM_ROTATE_H
#define SEEN_CANVAS_ITEM_ROTATE_H

/**
 * A class for previewing a canvas rotation.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of RotateCanvasItem.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/point.h>
#include <2geom/transforms.h>

#include "canvas-item.h"

class SPDesktop;

namespace Inkscape {

class CanvasRotate;

namespace UI::Widget {
class Canvas;
}

class CanvasItemGroup; // A canvas control that contains other canvas controls.

class CanvasItemRotate : public CanvasItem {

public:
    CanvasItemRotate(CanvasItemGroup *group);

    // Geometry
    void update(Geom::Affine const &affine) override;
    double closest_distance_to(Geom::Point const &p); // Maybe not needed

    // Selection
    bool contains(Geom::Point const &p, double tolerance = 0) override;

    // Display
    void render(Inkscape::CanvasItemBuffer *buf) override;

    // Events
    bool handle_event(GdkEvent *event) override;
    void start(SPDesktop *desktop);
    void paint();
    

protected:
    SPDesktop *_desktop = nullptr;
    Geom::Point _center;         // Center of screen.
    Geom::Point _cursor;         // Position of cursor relative to center (after angle snapping).
    double _current_angle = 0.0; // Rotation in degress.
    double _start_angle = -1000; // Initial angle, determined by cursor position.
    Cairo::RefPtr<Cairo::ImageSurface> _surface_copy;    // Copy of original surface.
};


} // namespace Inkscape

#endif // SEEN_CANVAS_ITEM_ROTATE_H

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

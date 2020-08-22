// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_ITEM_CURVE_H
#define SEEN_CANVAS_ITEM_CURVE_H

/**
 * A class to represent a single Bezier control curve.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of SPCtrlLine and SPCtrlCurve
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <memory>
#include <2geom/path.h>

#include "canvas-item.h"

namespace Inkscape {

class CanvasItemGroup; // A canvas control that contains other canvas controls.

class CanvasItemCurve : public CanvasItem {

public:
    CanvasItemCurve(CanvasItemGroup *group);
    CanvasItemCurve(CanvasItemGroup *group, Geom::Point const &p0, Geom::Point const &p1);
    CanvasItemCurve(CanvasItemGroup *group, Geom::Point const &p0, Geom::Point const &p1,
                                            Geom::Point const &p2, Geom::Point const &p3);

    // Geometry
    void set_coords(Geom::Point const &p0, Geom::Point const &p1);
    void set_coords(Geom::Point const &p0, Geom::Point const &p1, Geom::Point const &p2, Geom::Point const &p3);
    void set(Geom::BezierCurve &curve);
    bool is_line() { return _curve->size() == 2; }

    void update(Geom::Affine const &affine) override;
    double closest_distance_to(Geom::Point const &p); // Maybe not needed

    // Selection
    bool contains(Geom::Point const &p, double tolerance = 0) override;

    // Display
    void render(Inkscape::CanvasItemBuffer *buf) override;

    // Properties
    void set_is_fill(bool is_fill) { _is_fill = is_fill; }
    bool get_is_fill() { return _is_fill; }
    void set_corner0(int corner0) { _corner0 = corner0; } // Used for meshes
    int  get_corner0() { return _corner0; }
    void set_corner1(int corner1) { _corner1 = corner1; }
    int  get_corner1() { return _corner1; }
 
protected:
    std::unique_ptr<Geom::BezierCurve> _curve;

    bool _is_fill = true; // Fill or stroke, used by meshes.

    int _corner0 = -1; // For meshes
    int _corner1 = -1; // For meshes
};


} // namespace Inkscape

#endif // SEEN_CANVAS_ITEM_CURVE_H

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

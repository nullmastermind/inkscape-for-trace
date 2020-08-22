// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_ITEM_QUAD_H
#define SEEN_CANVAS_ITEM_QUAD_H

/**
 * A class to represent a control quadrilateral. Used to highlight selected text.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of SPCtrlQuadr
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/point.h>
#include <2geom/transforms.h>

#include "canvas-item.h"

namespace Inkscape {

class CanvasItemGroup; // A canvas control that contains other canvas controls.

class CanvasItemQuad : public CanvasItem {

public:
    CanvasItemQuad(CanvasItemGroup *group);
    CanvasItemQuad(CanvasItemGroup *group, Geom::Point const &p0, Geom::Point const &p1,
                                           Geom::Point const &p2, Geom::Point const &p3);

    // Geometry
    void set_coords(Geom::Point const &p0, Geom::Point const &p1, Geom::Point const &p2, Geom::Point const &p3);

    void update(Geom::Affine const &affine) override;
    double closest_distance_to(Geom::Point const &p); // Maybe not needed

    // Selection
    bool contains(Geom::Point const &p, double tolerance = 0) override;

    // Display
    void render(Inkscape::CanvasItemBuffer *buf) override;
 
protected:
    Geom::Point _p0;
    Geom::Point _p1;
    Geom::Point _p2;
    Geom::Point _p3;
};


} // namespace Inkscape

#endif // SEEN_CANVAS_ITEM_QUAD_H

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

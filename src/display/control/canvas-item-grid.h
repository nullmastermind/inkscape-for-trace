// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_ITEM_GRID_H
#define SEEN_CANVAS_ITEM_GRID_H

/**
 * A class to represent a grid.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of GridCanvasItem.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/point.h>
#include <2geom/transforms.h>

#include "canvas-item.h"

namespace Inkscape {

class CanvasGrid;

class CanvasItemGroup; // A canvas control that contains other canvas controls.

class CanvasItemGrid : public CanvasItem {

public:
    CanvasItemGrid(CanvasItemGroup *group, CanvasGrid *grid);
    ~CanvasItemGrid() override;

    // Geometry
    void update(Geom::Affine const &affine) override;
    double closest_distance_to(Geom::Point const &p); // Maybe not needed

    // Selection
    bool contains(Geom::Point const &p, double tolerance = 0) override;

    // Display
    void render(Inkscape::CanvasItemBuffer *buf) override;
 
protected:
    CanvasGrid *_grid = nullptr;
};


} // namespace Inkscape

#endif // SEEN_CANVAS_ITEM_GRID_H

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

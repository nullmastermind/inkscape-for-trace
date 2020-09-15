// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * A class to represent a grid. All the magic happens elsewhere.
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

#include "canvas-item-grid.h"

#include "canvas-grid.h"

#include "color.h" // SP_RGBA_x_F

#include "ui/widget/canvas.h"

namespace Inkscape {

/**
 * Create an null control grid.
 */
CanvasItemGrid::CanvasItemGrid(CanvasItemGroup *group, CanvasGrid *grid)
    : CanvasItem(group)
    , _grid(grid)
{
    _name = "CanvasItemGrid:";
    _name += grid->getName(grid->getGridType());
    _pickable = false; // For now, nobody gets events from this class!
    _bounds = Geom::Rect(-Geom::infinity(), -Geom::infinity(), Geom::infinity(), Geom::infinity());

    request_update(); // Update affine
}

/**
 * Destructor: must remove ourself from the CanvasGrid's vector of CanvasItemGrids.
 */
CanvasItemGrid::~CanvasItemGrid()
{
    if (_grid) {
        _grid->removeCanvasItem(this);
    }
}

/**
 * Returns distance between point in canvas units and nearest point on grid.
 */
double CanvasItemGrid::closest_distance_to(Geom::Point const &p)
{
    double d = Geom::infinity();
    std::cerr << "CanvasItemGrid::closest_distance_to: Not implemented!" << std::endl;
    return d;
}

/**
 * Returns true if point p (in canvas units) is within tolerance (canvas units) distance of grid.
 */
bool CanvasItemGrid::contains(Geom::Point const &p, double tolerance)
{
    return false; // We're not pickable!
}

/**
 * Update and redraw control grid.
 */
void CanvasItemGrid::update(Geom::Affine const &affine)
{
    if (_affine == affine && !_need_update) {
        // Nothing to do.
        return;
    }

    _affine = affine;
    _grid->Update(affine, 0); // TODO: Remove flag (not used).
    _need_update = false;
}

/**
 * Render grid to screen via Cairo.
 */
void CanvasItemGrid::render(Inkscape::CanvasItemBuffer *buf)
{
    if (!buf) {
        std::cerr << "CanvasItemGrid::Render: No buffer!" << std::endl;
         return;
    }

    if (!_visible) {
        // Hidden
        return;
    }

    if (!_grid->isVisible()) {
        // Hidden: Grid code doesn't set CanvasItem::visible!
        return;
    }

    _grid->Render(buf);
}

} // namespace Inkscape

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

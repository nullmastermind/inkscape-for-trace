// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * A class to catch events after everyone else has had a go.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of SPCanvasAcetate.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "canvas-item-catchall.h"

#include "color.h" // SP_RGBA_x_F

#include "ui/widget/canvas.h"

namespace Inkscape {

/**
 * Create an null control catchall.
 */
CanvasItemCatchall::CanvasItemCatchall(CanvasItemGroup *group)
    : CanvasItem(group)
{
    _name = "CanvasItemCatchall";
    _pickable = true; // Duh! That's the purpose of this class!
    _bounds = Geom::Rect(-Geom::infinity(), -Geom::infinity(), Geom::infinity(), Geom::infinity());
}

/**
 * Returns distance between point in canvas units and nearest point on catchall.
 */
double CanvasItemCatchall::closest_distance_to(Geom::Point const &p)
{
    return 0.0; // We're everywhere!
}

/**
 * Returns true if point p (in canvas units) is within tolerance (canvas units) distance of catchall.
 */
bool CanvasItemCatchall::contains(Geom::Point const &p, double tolerance)
{
    return true; // We contain every place!
}

/**
 * Update and redraw control catchall.
 */
void CanvasItemCatchall::update(Geom::Affine const &affine)
{
    // No geometry to update.
    _affine = affine;
}

/**
 * Render catchall to screen via Cairo.
 */
void CanvasItemCatchall::render(Inkscape::CanvasItemBuffer *buf)
{
    // Do nothing! (Needed as CanvasItem is abstract.)
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

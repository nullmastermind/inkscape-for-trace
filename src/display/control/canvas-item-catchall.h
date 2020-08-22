// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_ITEM_CATCHALL_H
#define SEEN_CANVAS_ITEM_CATCHALL_H

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

#include <2geom/point.h>
#include <2geom/transforms.h>

#include "canvas-item.h"

namespace Inkscape {

class CanvasItemGroup; // A canvas control that contains other canvas controls.

class CanvasItemCatchall : public CanvasItem {

public:
    CanvasItemCatchall(CanvasItemGroup *group);

    // Geometry
    void update(Geom::Affine const &affine) override;
    double closest_distance_to(Geom::Point const &p);

    // Selection
    bool contains(Geom::Point const &p, double tolerance = 0) override;

    // Display
    void render(Inkscape::CanvasItemBuffer *buf) override;
 
};


} // namespace Inkscape

#endif // SEEN_CANVAS_ITEM_CATCHALL_H

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

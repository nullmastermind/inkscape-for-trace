// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_ITEM_BPATH_H
#define SEEN_CANVAS_ITEM_BPATH_H

/**
 * A class to represent a Bezier path.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of SPCanvasBPath
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/transforms.h>
#include <2geom/pathvector.h>

#include "canvas-item.h"

#include "style-enums.h" // Fill rule

class SPCurve;

namespace Inkscape {

class CanvasItemGroup; // A canvas control that contains other canvas controls.

class CanvasItemBpath : public CanvasItem {

public:
    CanvasItemBpath(CanvasItemGroup *group);
    CanvasItemBpath(CanvasItemGroup *group, SPCurve *curve, bool phantom_line = false);
    CanvasItemBpath(CanvasItemGroup *group, Geom::PathVector path, bool phantom_line = false);

    // Geometry
    void set_bpath (SPCurve *curve, bool phantom_line = false);
    void set_bpath (Geom::PathVector const &path, bool phantom_line = false);
    void set_affine_absolute(Geom::Affine const &affine);

    void update(Geom::Affine const &affine) override;
    double closest_distance_to(Geom::Point const &p); // Maybe not needed

    // Selection
    bool contains(Geom::Point const &p, double tolerance = 0) override;

    // Display
    void render(Inkscape::CanvasItemBuffer *buf) override;

    // Properties
    void set_fill (guint32 rgba, SPWindRule fill_rule);
    void set_dashes (std::vector<double> & dashes) { _dashes = dashes; }

protected:

    // Geometry
    Geom::PathVector _path;

    // Properties
    SPWindRule _fill_rule = SP_WIND_RULE_EVENODD;
    std::vector<double> _dashes;
    bool _phantom_line = false;
};


} // namespace Inkscape

#endif // SEEN_CANVAS_ITEM_BPATH_H

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

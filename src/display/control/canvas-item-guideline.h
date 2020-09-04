// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_ITEM_GUIDELINE_H
#define SEEN_CANVAS_ITEM_GUIDELINE_H

/**
 * A class to represent a control guide line.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of SPGuideLine
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/ustring.h>

#include <2geom/point.h>
#include <2geom/transforms.h>

#include "canvas-item.h"

namespace Inkscape {

class CanvasItemCtrl;
class CanvasItemGroup; // A canvas control that contains other canvas controls.

class CanvasItemGuideLine : public CanvasItem {

public:
    CanvasItemGuideLine(CanvasItemGroup *group, Glib::ustring label,
                        Geom::Point const &origin, Geom::Point const &normal);
    ~CanvasItemGuideLine() override;

    // Geometry
    void set_origin(Geom::Point const &origin);
    void set_normal(Geom::Point const &normal);
    bool is_horizontal() const { return (_normal.x() == 0.0); }
    bool is_vertical()   const { return (_normal.y() == 0.0); }
    void update(Geom::Affine const &affine) override;
    double closest_distance_to(Geom::Point const &p);

    // Selection
    bool contains(Geom::Point const &p, double tolerance = 0) override;

    // Display
    void render(Inkscape::CanvasItemBuffer *buf) override;

    // Properties
    void hide() override;
    void show() override;
    void set_label(Glib::ustring const & label);
    void set_locked(bool locked);
    void set_sensitive(bool sensitive) { _sensitive = sensitive; }
 
protected:
    Geom::Point _origin;
    Geom::Point _normal = Geom::Point(0,1);
    Glib::ustring _label;
    bool _locked = true; // Flipped in constructor to trigger init of _origin_ctrl.
    bool _sensitive = false;
    Inkscape::CanvasItemCtrl *_origin_ctrl = nullptr; // To move guide line.

};


} // namespace Inkscape

#endif // SEEN_CANVAS_ITEM_GUIDELINE_H

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

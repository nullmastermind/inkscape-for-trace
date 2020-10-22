// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_ITEM_CTRL_H
#define SEEN_CANVAS_ITEM_CTRL_H

/**
 * A class to represent a control node.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of SPCtrl
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <memory>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <2geom/point.h>

#include "canvas-item.h"
#include "canvas-item-enums.h"

#include "enums.h" // SP_ANCHOR_X

namespace Inkscape {

class CanvasItemGroup; // A canvas control that contains other canvas controls.

class CanvasItemCtrl : public CanvasItem {

public:
    ~CanvasItemCtrl() override;
    CanvasItemCtrl(CanvasItemGroup *group);
    CanvasItemCtrl(CanvasItemGroup *group, CanvasItemCtrlType type);
    CanvasItemCtrl(CanvasItemGroup *group, CanvasItemCtrlType type, Geom::Point const &p);
    CanvasItemCtrl(CanvasItemGroup *group, CanvasItemCtrlShape shape);
    CanvasItemCtrl(CanvasItemGroup *group, CanvasItemCtrlShape shape, Geom::Point const &p);

    // Geometry
    void set_position(Geom::Point const &position);

    void update(Geom::Affine const &affine) override;
    double closest_distance_to(Geom::Point const &p);

    // Selection
    bool contains(Geom::Point const &p, double tolerance = 0) override;

    // Display
    void render(Inkscape::CanvasItemBuffer *buf) override;

    // Properties
    void set_fill(guint32 rgba) override;
    void set_stroke(guint32 rgba) override;
    void set_shape(int shape);
    void set_shape_default(); // Use type to determine shape.
    void set_mode(int mode);
    void set_mode_default();
    void set_size(int size);
    void set_size_via_index(int size_index);
    void set_size_default(); // Use preference and type to set size.
    void set_size_extra(int extra); // Used to temporary increase size of ctrl.
    void set_anchor(SPAnchorType anchor);
    void set_angle(double angle);
    void set_type(CanvasItemCtrlType type);
    void set_pixbuf(GdkPixbuf *pixbuf);
 
protected:
    void build_cache(int device_scale);

    // Geometry
    Geom::Point _position;

    // Display
    guint32 *_cache = nullptr;
    bool _built = false;

    // Properties
    CanvasItemCtrlType _type   = CANVAS_ITEM_CTRL_TYPE_DEFAULT;
    CanvasItemCtrlShape _shape = CANVAS_ITEM_CTRL_SHAPE_SQUARE;
    CanvasItemCtrlMode _mode   = CANVAS_ITEM_CTRL_MODE_XOR;
    unsigned int _width  = 5;   // Nominally width == height == size... unless we use a pixmap.
    unsigned int _height = 5;
    unsigned int _extra  = 0;   // Used to temporarily increase size.
    double       _angle  = 0.0; // Used for triangles, could be used for arrows.
    SPAnchorType _anchor = SP_ANCHOR_CENTER;
    GdkPixbuf *_pixbuf = nullptr;
};


} // namespace Inkscape

#endif // SEEN_CANVAS_ITEM_CTRL_H

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

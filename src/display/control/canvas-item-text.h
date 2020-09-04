// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_ITEM_TEXT_H
#define SEEN_CANVAS_ITEM_TEXT_H

/**
 * A class to represent on-screen text.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of SPCanvasText.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/ustring.h>

#include <2geom/point.h>
#include <2geom/transforms.h>

#include "canvas-item.h"

namespace Inkscape {

namespace UI::Widget {
class Canvas;
}

class CanvasItemGroup; // A canvas control that contains other canvas controls.

class CanvasItemText : public CanvasItem {

public:
    CanvasItemText(CanvasItemGroup *group);
    CanvasItemText(CanvasItemGroup *group, Geom::Point const &p, Glib::ustring text);

    // Geometry
    void set_coord(Geom::Point const &p);

    void update(Geom::Affine const &affine) override;
    double closest_distance_to(Geom::Point const &p); // Maybe not needed

    // Selection
    bool contains(Geom::Point const &p, double tolerance = 0) override;

    // Display
    void render(Inkscape::CanvasItemBuffer *buf) override;

    // Properties
    void set_text(Glib::ustring const &text);
    void set_fontsize(double fontsize);
    void set_background(guint32 background);
    void set_anchor(CanvasItemTextAnchor anchor);
    void set_anchor(Geom::Point const &anchor_pt);

protected:
    Geom::Point _p;  // Position of text (not box around text).
    CanvasItemTextAnchor _anchor = CANVAS_ITEM_TEXT_ANCHOR_CENTER;
    Geom::Point _anchor_offset;
    Geom::Point _anchor_position_manual;
    Glib::ustring _text;
    double _fontsize = 10;
    guint32 _background = 0x0000007f;
    bool _use_background = false;
    const double _border = 3; // Must be a const to allow alignment with other text boxes.
};


} // namespace Inkscape

#endif // SEEN_CANVAS_ITEM_TEXT_H

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

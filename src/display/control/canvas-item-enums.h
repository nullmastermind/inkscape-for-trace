// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_ITEM_ENUMS_H
#define SEEN_CANVAS_ITEM_ENUMS_H

/**
 * Enums for CanvasControlItem's.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 */

namespace Inkscape {

enum CanvasItemColor {
    CANVAS_ITEM_PRIMARY,
    CANVAS_ITEM_SECONDARY,
    CANVAS_ITEM_TERTIARY
};

enum CanvasItemCtrlShape {
    CANVAS_ITEM_CTRL_SHAPE_SQUARE,
    CANVAS_ITEM_CTRL_SHAPE_DIAMOND,
    CANVAS_ITEM_CTRL_SHAPE_CIRCLE,
    CANVAS_ITEM_CTRL_SHAPE_TRIANGLE,
    CANVAS_ITEM_CTRL_SHAPE_CROSS,
    CANVAS_ITEM_CTRL_SHAPE_PLUS,
    CANVAS_ITEM_CTRL_SHAPE_PIVOT,  // Fancy "plus"
    CANVAS_ITEM_CTRL_SHAPE_DARROW, // Double headed arrow.
    CANVAS_ITEM_CTRL_SHAPE_SARROW, // Double headed arrow, rotated (skew).
    CANVAS_ITEM_CTRL_SHAPE_CARROW, // Double headed curved arrow.
    CANVAS_ITEM_CTRL_SHAPE_SALIGN, // Side alignment.
    CANVAS_ITEM_CTRL_SHAPE_CALIGN, // Corner alignment.
    CANVAS_ITEM_CTRL_SHAPE_MALIGN, // Center (middle) alignment.
    CANVAS_ITEM_CTRL_SHAPE_BITMAP,
    CANVAS_ITEM_CTRL_SHAPE_IMAGE
};

// Applies to control points.
enum CanvasItemCtrlType {
    CANVAS_ITEM_CTRL_TYPE_DEFAULT,
    CANVAS_ITEM_CTRL_TYPE_ADJ_HANDLE, // Stretch & Scale
    CANVAS_ITEM_CTRL_TYPE_ADJ_SKEW,
    CANVAS_ITEM_CTRL_TYPE_ADJ_ROTATE,
    CANVAS_ITEM_CTRL_TYPE_ADJ_CENTER,
    CANVAS_ITEM_CTRL_TYPE_ADJ_SALIGN,
    CANVAS_ITEM_CTRL_TYPE_ADJ_CALIGN,
    CANVAS_ITEM_CTRL_TYPE_ADJ_MALIGN,
    CANVAS_ITEM_CTRL_TYPE_ANCHOR,
    CANVAS_ITEM_CTRL_TYPE_POINT,
    CANVAS_ITEM_CTRL_TYPE_ROTATE,
    CANVAS_ITEM_CTRL_TYPE_CENTER,
    CANVAS_ITEM_CTRL_TYPE_SIZER,
    CANVAS_ITEM_CTRL_TYPE_SHAPER,
    CANVAS_ITEM_CTRL_TYPE_LPE,
    CANVAS_ITEM_CTRL_TYPE_NODE_AUTO,
    CANVAS_ITEM_CTRL_TYPE_NODE_CUSP,
    CANVAS_ITEM_CTRL_TYPE_NODE_SMOOTH,
    CANVAS_ITEM_CTRL_TYPE_NODE_SYMETRICAL,
    CANVAS_ITEM_CTRL_TYPE_INVISIPOINT
};

enum CanvasItemCtrlMode {
    CANVAS_ITEM_CTRL_MODE_COLOR,
    CANVAS_ITEM_CTRL_MODE_XOR
};

enum CanvasItemTextAnchor {
    CANVAS_ITEM_TEXT_ANCHOR_CENTER,
    CANVAS_ITEM_TEXT_ANCHOR_TOP,
    CANVAS_ITEM_TEXT_ANCHOR_BOTTOM,
    CANVAS_ITEM_TEXT_ANCHOR_LEFT,
    CANVAS_ITEM_TEXT_ANCHOR_RIGHT,
    CANVAS_ITEM_TEXT_ANCHOR_ZERO,
    CANVAS_ITEM_TEXT_ANCHOR_MANUAL
};

} // namespace Inkscape

#endif // SEEN_CANVAS_ITEM_ENUMS_H

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

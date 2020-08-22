// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_ITEM_BUFFER_H
#define SEEN_CANVAS_ITEM_BUFFER_H

/**
 * Buffer for rendering canvas items.
 */

/*
 * Author:
 *   See git history.
 *
 * Copyright (C) 2020 Authors
 *
 * Rewrite of SPCanvasBuf.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 */

#include <2geom/rect.h>
#include <cairomm/context.h>

namespace Inkscape {

/**
 * Class used when rendering canvas items.
 */
class CanvasItemBuffer {
public:
    CanvasItemBuffer(Geom::IntRect const &paint_rect, Geom::IntRect const &canvas_rect, int device_scale)
        : rect(paint_rect)
        , canvas_rect(canvas_rect)
        , device_scale(device_scale)
    {}
    ~CanvasItemBuffer() = default;

    Geom::IntRect rect;
    Geom::IntRect canvas_rect; // visible window in world coordinates (i.e. offset by _x0, _y0)
    int device_scale; // For high DPI monitors.

    Cairo::RefPtr<Cairo::Context> cr;
    unsigned char *buf = nullptr;
    int buf_rowstride  = 0;
    bool is_empty      = true;
};

} // Namespace Inkscape

#endif // SEEN_CANVAS_ITEM_BUFFER_H

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

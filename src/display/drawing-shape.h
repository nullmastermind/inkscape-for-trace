// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Group belonging to an SVG drawing element.
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_DISPLAY_DRAWING_SHAPE_H
#define SEEN_INKSCAPE_DISPLAY_DRAWING_SHAPE_H

#include "display/drawing-item.h"
#include "display/nr-style.h"

class SPStyle;
class SPCurve;

namespace Inkscape {

class DrawingShape
    : public DrawingItem
{
public:
    DrawingShape(Drawing &drawing);
    ~DrawingShape() override;

    void setPath(SPCurve *curve);
    void setStyle(SPStyle *style, SPStyle *context_style = nullptr) override;
    void setChildrenStyle(SPStyle *context_style) override;

protected:
    unsigned _updateItem(Geom::IntRect const &area, UpdateContext const &ctx,
                                 unsigned flags, unsigned reset) override;
    unsigned _renderItem(DrawingContext &dc, Geom::IntRect const &area, unsigned flags,
                                 DrawingItem *stop_at) override;
    void _clipItem(DrawingContext &dc, Geom::IntRect const &area) override;
    DrawingItem *_pickItem(Geom::Point const &p, double delta, unsigned flags) override;
    bool _canClip() override;

    void _renderFill(DrawingContext &dc);
    void _renderStroke(DrawingContext &dc);
    void _renderMarkers(DrawingContext &dc, Geom::IntRect const &area, unsigned flags,
                        DrawingItem *stop_at);

    SPCurve *_curve;
    NRStyle _nrstyle;

    DrawingItem *_last_pick;
    unsigned _repick_after;
};

} // end namespace Inkscape

#endif // !SEEN_INKSCAPE_DISPLAY_DRAWING_ITEM_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

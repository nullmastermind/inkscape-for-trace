// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * SVG drawing for display.
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_DISPLAY_DRAWING_H
#define SEEN_INKSCAPE_DISPLAY_DRAWING_H

#include <2geom/rect.h>
#include <boost/operators.hpp>
#include <boost/utility.hpp>
#include <set>
#include <sigc++/sigc++.h>

#include "display/drawing-item.h"
#include "display/rendermode.h"
#include "nr-filter-gaussian.h" // BLUR_QUALITY_BEST
#include "nr-filter-colormatrix.h"

typedef unsigned int guint32;

namespace Inkscape {

class DrawingItem;
class CanvasItemDrawing;

class Drawing
    : boost::noncopyable
{
public:
    struct OutlineColors {
        guint32 paths;
        guint32 clippaths;
        guint32 masks;
        guint32 images;
    };

    Drawing(Inkscape::CanvasItemDrawing *drawing = nullptr);
    ~Drawing();

    DrawingItem *root() { return _root; }
    Inkscape::CanvasItemDrawing *getCanvasItemDrawing() { return _canvas_item_drawing; }
    void setRoot(DrawingItem *item);

    RenderMode renderMode() const;
    ColorMode colorMode() const;
    bool outline() const;
    bool visibleHairlines() const;
    bool outlineOverlay() const;
    bool renderFilters() const;
    int blurQuality() const;
    int filterQuality() const;
    void setRenderMode(RenderMode mode);
    void setColorMode(ColorMode mode);
    void setBlurQuality(int q);
    void setFilterQuality(int q);
    void setExact(bool e);
    bool getExact() const { return _exact; };
    void setOutlineSensitive(bool e);
    bool getOutlineSensitive() const { return _outline_sensitive; };

    Geom::OptIntRect const &cacheLimit() const;
    void setCacheLimit(Geom::OptIntRect const &r, bool update_cache = true);
    void setCacheBudget(size_t bytes);

    OutlineColors const &colors() const { return _colors; }

    void setGrayscaleMatrix(double value_matrix[20]);

    void update(Geom::IntRect const &area = Geom::IntRect::infinite(), unsigned flags = DrawingItem::STATE_ALL,
                unsigned reset = 0);

    void render(DrawingContext &dc, Geom::IntRect const &area, unsigned flags = 0, int antialiasing = -1);
    DrawingItem *pick(Geom::Point const &p, double delta, unsigned flags);

    void average_color(Geom::IntRect const &area, double &R, double &G, double &B, double &A);

    sigc::signal<void, DrawingItem *> signal_request_update;
    sigc::signal<void, Geom::IntRect const &> signal_request_render;
    sigc::signal<void, DrawingItem *> signal_item_deleted;

private:
    void _pickItemsForCaching();

    typedef std::list<CacheRecord> CandidateList;
    bool _outline_sensitive = false;
    DrawingItem *_root = nullptr;
    std::set<DrawingItem *> _cached_items; // modified by DrawingItem::setCached()
    CandidateList _candidate_items;        // keep this list always sorted with std::greater

public:
    // TODO: remove these temporarily public members
    guint32 outlinecolor = 0x000000ff;
    double delta = 0;

private:
    bool _exact = false;  // if true then rendering must be exact
    RenderMode _rendermode = RenderMode::NORMAL;
    ColorMode _colormode = ColorMode::NORMAL;
    int _blur_quality = BLUR_QUALITY_BEST;
    int _filter_quality = Filters::FILTER_QUALITY_BEST;
    Geom::OptIntRect _cache_limit;

    double _cache_score_threshold = 50000.0; ///< do not consider objects for caching below this score
    size_t _cache_budget = 0;                ///< maximum allowed size of cache

    OutlineColors _colors;
    Filters::FilterColorMatrix::ColorMatrixMatrix _grayscale_colormatrix;
    Inkscape::CanvasItemDrawing *_canvas_item_drawing = nullptr;

    friend class DrawingItem;
};

} // end namespace Inkscape

#endif // !SEEN_INKSCAPE_DRAWING_H

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

// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * SVG drawing for display.
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *   Johan Engelen <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Copyright (C) 2011-2012 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "display/drawing.h"
#include "display/control/canvas-item-drawing.h"
#include "nr-filter-gaussian.h"
#include "nr-filter-types.h"

//grayscale colormode:
#include "cairo-templates.h"
#include "drawing-context.h"


namespace Inkscape {

// hardcoded grayscale color matrix values as default
static const gdouble grayscale_value_matrix[20] = {
    0.21, 0.72, 0.072, 0, 0,
    0.21, 0.72, 0.072, 0, 0,
    0.21, 0.72, 0.072, 0, 0,
    0   , 0   , 0    , 1, 0
};

Drawing::Drawing(Inkscape::CanvasItemDrawing *canvas_item_drawing)
    : _canvas_item_drawing(canvas_item_drawing)
    , _grayscale_colormatrix(std::vector<gdouble>(grayscale_value_matrix, grayscale_value_matrix + 20))
{
    // _canvas_item_drawing can be null. Used this way by Eraser tool.
}

Drawing::~Drawing()
{
    delete _root;
}

void
Drawing::setRoot(DrawingItem *item)
{
    delete _root;
    _root = item;
    if (item) {
        assert(item->_child_type == DrawingItem::CHILD_ORPHAN);
        item->_child_type = DrawingItem::CHILD_ROOT;
    }
}

RenderMode
Drawing::renderMode() const
{
    return _exact ? RenderMode::NORMAL : _rendermode;
}
ColorMode
Drawing::colorMode() const
{
    return (outline() || _exact) ? ColorMode::NORMAL : _colormode;
}
bool
Drawing::outline() const
{
    return renderMode() == RenderMode::OUTLINE;
}
bool
Drawing::visibleHairlines() const
{
    return renderMode() == RenderMode::VISIBLE_HAIRLINES;
}
bool
Drawing::outlineOverlay() const
{
    return renderMode() == RenderMode::OUTLINE_OVERLAY;
}
bool
Drawing::renderFilters() const
{
    return renderMode() == RenderMode::NORMAL || renderMode() == RenderMode::VISIBLE_HAIRLINES || renderMode() == RenderMode::OUTLINE_OVERLAY;
}
int
Drawing::blurQuality() const
{
    if (renderMode() == RenderMode::NORMAL) {
        return _exact ? BLUR_QUALITY_BEST : _blur_quality;
    } else {
        return BLUR_QUALITY_WORST;
    }
}
int
Drawing::filterQuality() const
{
    if (renderMode() == RenderMode::NORMAL) {
        return _exact ? Filters::FILTER_QUALITY_BEST : _filter_quality;
    } else {
        return Filters::FILTER_QUALITY_WORST;
    }
}

void
Drawing::setRenderMode(RenderMode mode)
{
    _rendermode = mode;
}
void
Drawing::setColorMode(ColorMode mode)
{
    _colormode = mode;
}
void
Drawing::setBlurQuality(int q)
{
    _blur_quality = q;
}
void
Drawing::setFilterQuality(int q)
{
    _filter_quality = q;
}
void
Drawing::setExact(bool e)
{
    _exact = e;
}

void Drawing::setOutlineSensitive(bool e) { _outline_sensitive = e; };

Geom::OptIntRect const &
Drawing::cacheLimit() const
{
    return _cache_limit;
}
void
Drawing::setCacheLimit(Geom::OptIntRect const &r, bool update_cache)
{
    _cache_limit = r;
    if (update_cache) {
        for (auto _cached_item : _cached_items)
        {
            _cached_item->_markForUpdate(DrawingItem::STATE_CACHE, false);
        }
    }
}
void
Drawing::setCacheBudget(size_t bytes)
{
    _cache_budget = bytes;
    _pickItemsForCaching();
}

void
Drawing::setGrayscaleMatrix(gdouble value_matrix[20]) {
    _grayscale_colormatrix = Filters::FilterColorMatrix::ColorMatrixMatrix( 
        std::vector<gdouble> (value_matrix, value_matrix + 20) );
}

void
Drawing::update(Geom::IntRect const &area, unsigned flags, unsigned reset)
{
    if (_root) {
        auto ctx = _canvas_item_drawing ? _canvas_item_drawing->get_context() : UpdateContext();
        _root->update(area, ctx, flags, reset);
    }
    if ((flags & DrawingItem::STATE_CACHE) || (flags & DrawingItem::STATE_ALL)) {
        // process the updated cache scores
        _pickItemsForCaching();
    }
}

void
Drawing::render(DrawingContext &dc, Geom::IntRect const &area, unsigned flags, int antialiasing)
{
    if (_root) {
        int prev_a = _root->_antialias;
        if(antialiasing >= 0)
            _root->setAntialiasing(antialiasing);
        _root->render(dc, area, flags);
        _root->setAntialiasing(prev_a);
    }

    if (colorMode() == ColorMode::GRAYSCALE) {
        // apply grayscale filter on top of everything
        cairo_surface_t *input = dc.rawTarget();
        cairo_surface_t *out = ink_cairo_surface_create_identical(input);
        ink_cairo_surface_filter(input, out, _grayscale_colormatrix);
        Geom::Point origin = dc.targetLogicalBounds().min();
        dc.setSource(out, origin[Geom::X], origin[Geom::Y]);
        dc.setOperator(CAIRO_OPERATOR_SOURCE);
        dc.paint();
        dc.setOperator(CAIRO_OPERATOR_OVER);
    
        cairo_surface_destroy(out);
    }
}

DrawingItem *
Drawing::pick(Geom::Point const &p, double delta, unsigned flags)
{
    if (_root) {
        return _root->pick(p, delta, flags);
    } else {
        std::cerr << "Drawing::pick: _root is null!" << std::endl;
    }
    return nullptr;
}

void
Drawing::_pickItemsForCaching()
{
    size_t used = 0;
    CandidateList::iterator i;
    for (i = _candidate_items.begin(); i != _candidate_items.end(); ++i) {
        if (used + i->cache_size > _cache_budget) break;
        used += i->cache_size;
    }

    std::set<DrawingItem*> to_cache;
    for (CandidateList::iterator j = _candidate_items.begin(); j != i; ++j) {
        j->item->setCached(true);
        to_cache.insert(j->item);
    }
    // Everything which is now in _cached_items but not in to_cache must be uncached
    // Note that calling setCached on an item modifies _cached_items
    // TODO: find a way to avoid the set copy
    std::set<DrawingItem*> to_uncache;
    std::set_difference(_cached_items.begin(), _cached_items.end(),
                        to_cache.begin(), to_cache.end(),
                        std::inserter(to_uncache, to_uncache.end()));
    for (auto j : to_uncache) {
        j->setCached(false);
    }
}

/*
 * Return average color over area. Used by Calligraphic, Dropper, and Spray tools.
 */
void
Drawing::average_color(Geom::IntRect const &area, double &R, double &G, double &B, double &A)
{
    auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, area.width(), area.height());
    Inkscape::DrawingContext dc(surface->cobj(), area.min());
    render(dc, area);

    ink_cairo_surface_average_color_premul(surface->cobj(), R, G, B, A);
}


} // end namespace Inkscape

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

// SPDX-License-Identifier: GPL-2.0-or-later
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

#include "canvas-item-bpath.h"

#include <memory>  // unique_ptr
#include <utility> // std::move

#include "color.h" // SP_RGBA_x_F

#include "display/curve.h"
#include "display/cairo-utils.h"

#include "helper/geom.h"  // bounds_exact_transformed()

#include "ui/widget/canvas.h"

namespace Inkscape {

/**
 * Create an null control bpath.
 */
CanvasItemBpath::CanvasItemBpath(CanvasItemGroup *group)
    : CanvasItem(group)
{
    _name = "CanvasItemBpath:Null";
    _pickable = true; // For now, nobody gets events from this class!
}

/**
 * Create a control bpath. Curve is in document coordinates.
 */
CanvasItemBpath::CanvasItemBpath(CanvasItemGroup *group, SPCurve *curve, bool phantom_line)
    : CanvasItem(group)
    , _phantom_line(phantom_line)
{
    _name = "CanvasItemBpath";
    _pickable = true; // For now, everyone gets events from this class!
    if (curve) {
        _path = curve->get_pathvector();
    }

    request_update(); // Render immediately or temporary bpaths won't show.
}

/**
 * Create a control bpath. Path is in document coordinates.
 */
CanvasItemBpath::CanvasItemBpath(CanvasItemGroup *group, Geom::PathVector path, bool phantom_line)
    : CanvasItem(group)
    , _path(std::move(path))
    , _phantom_line(phantom_line)
{
    _name = "CanvasItemBpath";
    _pickable = true; // For now, everyone gets events from this class!
    request_update(); // Render immediately or temporary bpaths won't show.
}

/**
 * Set a control bpath. Curve is in document coordinates.
 */
void CanvasItemBpath::set_bpath(SPCurve *curve, bool phantom_line)
{
    if (curve) {   // No test to see if *curve == *_curve so we always do the swap.
        _path = curve->get_pathvector();
    } else {
        _path.clear();
    }

    _phantom_line = phantom_line;

    request_update();
}

/**
 * Set a control bpath. Path is in document coordinates.
 */
void CanvasItemBpath::set_bpath(Geom::PathVector const &path, bool phantom_line)
{
    _path = path;
    _phantom_line = phantom_line;

    request_update();
}

/**
 * Set the fill color and fill rule.
 */
void CanvasItemBpath::set_fill(guint rgba, SPWindRule fill_rule)
{
    if (_fill != rgba || _fill_rule != fill_rule) {
        _fill = rgba;
        _fill_rule = fill_rule;
        _canvas->redraw_area(_bounds);
    }
}

/**
 * Returns distance between point in canvas units and nearest point on bpath.
 */
double CanvasItemBpath::closest_distance_to(Geom::Point const &p)
{
    double d = Geom::infinity();

    // Convert p to document coordinates (quicker than converting path to canvas units).
    Geom::Point p_doc = p * _affine.inverse();
    _path.nearestTime(p_doc, &d);
    d *= _affine.descrim(); // Uniform scaling and rotation only.

    return d;
}

/**
 * Returns true if point p (in canvas units) is within tolerance (canvas units) distance of bpath.
 */
bool CanvasItemBpath::contains(Geom::Point const &p, double tolerance)
{
    if (tolerance == 0) {
        tolerance = 1; // Need a minium tolerance value or always returns false.
    }

    return closest_distance_to(p) < tolerance;
}

/**
 * Update and redraw control bpath.
 */
void CanvasItemBpath::update(Geom::Affine const &affine)
{
    if (_affine == affine && !_need_update) {
        // Nothing to do.
        return;
    }

    // Queue redraw of old area (erase previous content).
    _canvas->redraw_area(_bounds);

    // Get new bounds
    _affine = affine;

    _bounds = Geom::Rect(); // Reset bounds

    if (_path.empty()) return; // No path, no chocolate!

    Geom::OptRect bbox = bounds_exact_transformed(_path, _affine);

    if (bbox) {
        _bounds = *bbox;
        _bounds.expandBy(2);
    } else {
        _bounds = Geom::Rect();
    }

    // Queue redraw of new area
    _canvas->redraw_area(_bounds);

    _need_update = false;
}

/**
 * Render bpath to screen via Cairo.
 */
void CanvasItemBpath::render(Inkscape::CanvasItemBuffer *buf)
{
    if (!buf) {
        std::cerr << "CanvasItemBpath::Render: No buffer!" << std::endl;
         return;
    }

    if (!_visible) {
        // Hidden
        return;
    }

    if (_path.empty()) {
        return;
    }

    bool do_fill   = (_fill   & 0xff) != 0; // Not invisible.
    bool do_stroke = (_stroke & 0xff) != 0; // Not invisible.

    if (!do_fill && !do_stroke) {
        // Both fill and stroke invisible.
        return;
    }

    buf->cr->save();

    // Setup path
    buf->cr->set_tolerance(0.5);
    buf->cr->begin_new_path();

    feed_pathvector_to_cairo (buf->cr->cobj(), _path, _affine, buf->rect,
                              /* optimized_stroke = */ !do_fill, 1);

    // Do fill
    if (do_fill) {
        buf->cr->set_source_rgba(SP_RGBA32_R_F(_fill), SP_RGBA32_G_F(_fill),
                                 SP_RGBA32_B_F(_fill), SP_RGBA32_A_F(_fill));
        buf->cr->set_fill_rule(_fill_rule == SP_WIND_RULE_EVENODD ?
                               Cairo::FILL_RULE_EVEN_ODD : Cairo::FILL_RULE_WINDING);
        buf->cr->fill_preserve();
    }

    // Do stroke
    if (do_stroke) {

        if (!_dashes.empty()) {
            buf->cr->set_dash(_dashes, 0.0); // 0.0 is offset
        }

        if (_phantom_line) {
            buf->cr->set_source_rgba(1.0, 1.0, 1.0, 0.25);
            buf->cr->set_line_width(2.0);
            buf->cr->stroke_preserve();
        }

        buf->cr->set_source_rgba(SP_RGBA32_R_F(_stroke), SP_RGBA32_G_F(_stroke),
                                 SP_RGBA32_B_F(_stroke), SP_RGBA32_A_F(_stroke));
        buf->cr->set_line_width(1.0);
        buf->cr->stroke();

    } else {
        buf->cr->begin_new_path(); // Clears path
    }

    // Uncomment to show bounds
    // Geom::Rect bounds = _bounds;
    // bounds.expandBy(-1);
    // bounds -= buf->rect.min();
    // buf->cr->set_source_rgba(1.0, 0.0, 0.0, 1.0);
    // buf->cr->rectangle(bounds.min().x(), bounds.min().y(), bounds.width(), bounds.height());
    // buf->cr->stroke();

    buf->cr->restore();
}

} // namespace Inkscape

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

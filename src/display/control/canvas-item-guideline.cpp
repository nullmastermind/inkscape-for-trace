// SPDX-License-Identifier: GPL-2.0-or-later
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

#include "canvas-item-guideline.h"

#include <utility> // std::move

#include <2geom/line.h>

#include "canvas-item-ctrl.h"

#include "color.h" // SP_RGBA_x_F
#include "inkscape.h" // SP_ACTIVE_DESKTOP  FIXME!
#include "desktop.h" // Canvas orientation so label is orientated correctly.

#include "ui/widget/canvas.h"

namespace Inkscape {

/**
 * Create a control guide line. Points are in document units.
 */
CanvasItemGuideLine::CanvasItemGuideLine(CanvasItemGroup *group, Glib::ustring label,
                                         Geom::Point const &origin, Geom::Point const &normal)
    : CanvasItem(group)
    , _label(std::move(label))
    , _origin(origin)
    , _normal(normal)
{
    _name = "CanvasItemGuideLine:" + _label;
    _pickable = true; // For now, everybody gets events from this class!
    _bounds = Geom::Rect(-Geom::infinity(), -Geom::infinity(), Geom::infinity(), Geom::infinity()); // Required when rotating canvas.

    // Control to move guide line.
    _origin_ctrl = new CanvasItemCtrl(group, CANVAS_ITEM_CTRL_SHAPE_CIRCLE, _origin);
    _origin_ctrl->set_name("CanvasItemGuideLine:Ctrl:" + _label);
    _origin_ctrl->set_pickable(false); // Line beneath is pickable. Handle is display only!
    set_locked(false); // Init _origin_ctrl shape and stroke.
}

/**
 * Destructor: needed to destroy origin ctrl point.
 */
CanvasItemGuideLine::~CanvasItemGuideLine()
{
    delete _origin_ctrl;
    // Base class destructor automatically called.
}

/**
 * Sets origin of guide line (place where handle is located).
 */
void CanvasItemGuideLine::set_origin(Geom::Point const &origin)
{
    if (_origin != origin) {
        _origin = origin;
        _origin_ctrl->set_position(_origin);
        request_update();
    }
}

/**
 * Sets orientation of guide line.
 */
void CanvasItemGuideLine::set_normal(Geom::Point const &normal)
{
    if (_normal != normal) {
        _normal = normal;
        request_update();
    }
}

/**
 * Returns distance between point in canvas units and nearest point on guideLine.
 */
double CanvasItemGuideLine::closest_distance_to(Geom::Point const &p)
{
    // Maybe store guide as a Geom::Line?
    Geom::Line guide =
        Geom::Line::from_origin_and_vector(_origin, Geom::rot90(_normal));
    guide *= _affine;
    return Geom::distance(p, guide);
}

/**
 * Returns true if point p (in canvas units) is within tolerance (canvas units) distance of guideLine (or 1 if tolerance is zero).
 */
bool CanvasItemGuideLine::contains(Geom::Point const &p, double tolerance)
{
    if (tolerance == 0) {
        tolerance = 1; // Can't pick of zero!
    }

    return closest_distance_to(p) < tolerance;
}

/**
 * Update and redraw control guideLine.
 */
void CanvasItemGuideLine::update(Geom::Affine const &affine)
{
    if (_affine == affine && !_need_update) {
        // Nothing to do.
        return;
    }

    _affine = affine;

    // Queue redraw of new area (and old too).
    _canvas->redraw_area(_bounds);

    _need_update = false;
}

/**
 * Render guideLine to screen via Cairo.
 */
void CanvasItemGuideLine::render(Inkscape::CanvasItemBuffer *buf)
{
    if (!buf) {
        std::cerr << "CanvasItemGuideLine::Render: No buffer!" << std::endl;
         return;
    }

    if (!_visible) {
        // Hidden
        return;
    }

    // Document to canvas
    Geom::Point normal = _normal * _affine.withoutTranslation(); // Direction only
    Geom::Point origin = _origin * _affine;

    buf->cr->save();
    // Canvas to screen
    buf->cr->translate( -buf->rect.left(), -buf->rect.top());
    buf->cr->set_source_rgba(SP_RGBA32_R_F(_stroke), SP_RGBA32_G_F(_stroke),
                             SP_RGBA32_B_F(_stroke), SP_RGBA32_A_F(_stroke));
    buf->cr->set_line_width(1);

    if (!_label.empty()) {
        int px = std::round(origin.x());
        int py = std::round(origin.y());
        buf->cr->save();
        buf->cr->translate(px, py);
        SPDesktop * desktop = SP_ACTIVE_DESKTOP; // TODO Fix me!
        buf->cr->rotate(atan2(normal.cw()) + M_PI * (desktop && desktop->is_yaxisdown() ? 1 : 0));
        buf->cr->translate(0, -5); // Offset
        buf->cr->move_to(0, 0);
        buf->cr->show_text(_label);
        buf->cr->restore();
    }

    // Draw guide.
    // Special case horizontal and vertical lines so they accurately align to the to pixels.
    
    // Need to use floor()+0.5 such that Cairo will draw us lines with a width of a single pixel, without any aliasing.
    // For this we need to position the lines at exactly half pixels, see https://www.cairographics.org/FAQ/#sharp_lines
    // Must be consistent with the pixel alignment of the grid lines, see CanvasXYGrid::Render(), and the drawing of the rulers
    
    // Don't use isHorizontal()/isVertical() as they test only exact matches.
    if (Geom::are_near(normal.y(), 0.0)) {
        // Vertical
        double position = floor(origin.x()) + 0.5;
        buf->cr->move_to(position, buf->rect.top()    + 0.5);
        buf->cr->line_to(position, buf->rect.bottom() - 0.5);
    } else if (Geom::are_near(normal.x(), 0.0)) {
        // Horizontal
        double position = floor(origin.y()) + 0.5;
        buf->cr->move_to(buf->rect.left()   + 0.5, position);
        buf->cr->line_to(buf->rect.right()  - 0.5, position);
    } else {
        // Angled
        Geom::Line guide =
            Geom::Line::from_origin_and_vector( origin, Geom::rot90(normal) );

        // Find intersections of guide with buf rectangle. There should be zero or two.
        std::vector<Geom::Point> intersections;
        for (unsigned i = 0; i < 4; ++i) {
            Geom::LineSegment side( buf->rect.corner(i), buf->rect.corner((i+1)%4) );
            try {
                Geom::OptCrossing oc = Geom::intersection(guide, side);
                if (oc) {
                    intersections.push_back( guide.pointAt((*oc).ta));
                }
            } catch (Geom::InfiniteSolutions) {
                // Shouldn't happen as we have already taken care of horizontal/vertical guides.
                std::cerr << "CanvasItemGuideLine::render: Error: Infinite intersections." << std::endl;
            }
        }

        if (intersections.size() == 2) {
            double x0 = intersections[0].x();
            double x1 = intersections[1].x();
            double y0 = intersections[0].y();
            double y1 = intersections[1].y();
            buf->cr->move_to(x0, y0);
            buf->cr->line_to(x1, y1);
        }
    }
    buf->cr->stroke();

    buf->cr->restore();
}

void CanvasItemGuideLine::hide()
{
    CanvasItem::hide();
    if (_origin_ctrl) {
        _origin_ctrl->hide();
    }
}

void CanvasItemGuideLine::show()
{
    CanvasItem::show();
    if (_origin_ctrl) {
        _origin_ctrl->show();
    }
}

void CanvasItemGuideLine::set_label(Glib::ustring const & label)
{
    if (_label != label) {
        _label = label;
        request_update();
    }
}

void CanvasItemGuideLine::set_locked(bool locked)
{
    if (_locked != locked) {
        _locked = locked;
        if (_locked) {
            _origin_ctrl->set_shape(CANVAS_ITEM_CTRL_SHAPE_CROSS);
            _origin_ctrl->set_stroke(0x0000ff80);
            _origin_ctrl->set_size(7);
        } else {
            _origin_ctrl->set_shape(CANVAS_ITEM_CTRL_SHAPE_CIRCLE);
            _origin_ctrl->set_stroke(0xff000080);
            _origin_ctrl->set_size(5);
        }
    }
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

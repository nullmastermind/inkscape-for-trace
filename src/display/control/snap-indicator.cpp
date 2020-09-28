// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Provides a class that shows a temporary indicator on the canvas of where the snap was, and what kind of snap
 *
 * Authors:
 *   Johan Engelen
 *   Diederik van Lierop
 *
 * Copyright (C) Johan Engelen 2009 <j.b.c.engelen@utwente.nl>
 * Copyright (C) Diederik van Lierop 2010 - 2012 <mail@diedenrezi.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>

#include "snap-indicator.h"

#include "desktop.h"
#include "enums.h"
#include "preferences.h"


#include "canvas-item-ctrl.h"
#include "canvas-item-rect.h"
#include "canvas-item-text.h"

#include "ui/tools/measure-tool.h"

namespace Inkscape {
namespace Display {

SnapIndicator::SnapIndicator(SPDesktop * desktop)
    :   _snaptarget(nullptr),
        _snaptarget_tooltip(nullptr),
        _snaptarget_bbox(nullptr),
        _snapsource(nullptr),
        _snaptarget_is_presnap(false),
        _desktop(desktop)
{
}

SnapIndicator::~SnapIndicator()
{
    // remove item that might be present
    remove_snaptarget();
    remove_snapsource();
}

void
SnapIndicator::set_new_snaptarget(Inkscape::SnappedPoint const &p, bool pre_snap)
{
    remove_snaptarget(); //only display one snaptarget at a time

    g_assert(_desktop != nullptr);

    if (!p.getSnapped()) {
        return; // If we haven't snapped, then it is of no use to draw a snapindicator
    }

    if (p.getTarget() == SNAPTARGET_CONSTRAINT) {
        // This is not a real snap, although moving along the constraint did affect the mouse pointer's position.
        // Maybe we should only show a snap indicator when the user explicitly asked for a constraint by pressing ctrl?
        // We should not show a snap indicator when stretching a selection box, which is also constrained. That would be
        // too much information.
        return;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool value = prefs->getBool("/options/snapindicator/value", true);

    if (value) {
        // TRANSLATORS: undefined target for snapping
        Glib::ustring target_name = _("UNDEFINED");
        switch (p.getTarget()) {
            case SNAPTARGET_UNDEFINED:
                target_name = _("UNDEFINED");
                g_warning("Snap target has not been specified");
                break;
            case SNAPTARGET_GRID:
                target_name = _("grid line");
                break;
            case SNAPTARGET_GRID_INTERSECTION:
                target_name = _("grid intersection");
                break;
            case SNAPTARGET_GRID_PERPENDICULAR:
                target_name = _("grid line (perpendicular)");
                break;
            case SNAPTARGET_GUIDE:
                target_name = _("guide");
                break;
            case SNAPTARGET_GUIDE_INTERSECTION:
                target_name = _("guide intersection");
                break;
            case SNAPTARGET_GUIDE_ORIGIN:
                target_name = _("guide origin");
                break;
            case SNAPTARGET_GUIDE_PERPENDICULAR:
                target_name = _("guide (perpendicular)");
                break;
            case SNAPTARGET_GRID_GUIDE_INTERSECTION:
                target_name = _("grid-guide intersection");
                break;
            case SNAPTARGET_NODE_CUSP:
                target_name = _("cusp node");
                break;
            case SNAPTARGET_NODE_SMOOTH:
                target_name = _("smooth node");
                break;
            case SNAPTARGET_PATH:
                target_name = _("path");
                break;
            case SNAPTARGET_PATH_PERPENDICULAR:
                target_name = _("path (perpendicular)");
                break;
            case SNAPTARGET_PATH_TANGENTIAL:
                target_name = _("path (tangential)");
                break;
            case SNAPTARGET_PATH_INTERSECTION:
                target_name = _("path intersection");
                break;
            case SNAPTARGET_PATH_GUIDE_INTERSECTION:
                target_name = _("guide-path intersection");
                break;
            case SNAPTARGET_PATH_CLIP:
                target_name = _("clip-path");
                break;
            case SNAPTARGET_PATH_MASK:
                target_name = _("mask-path");
                break;
            case SNAPTARGET_BBOX_CORNER:
                target_name = _("bounding box corner");
                break;
            case SNAPTARGET_BBOX_EDGE:
                target_name = _("bounding box side");
                break;
            case SNAPTARGET_PAGE_BORDER:
                target_name = _("page border");
                break;
            case SNAPTARGET_LINE_MIDPOINT:
                target_name = _("line midpoint");
                break;
            case SNAPTARGET_OBJECT_MIDPOINT:
                target_name = _("object midpoint");
                break;
            case SNAPTARGET_ROTATION_CENTER:
                target_name = _("object rotation center");
                break;
            case SNAPTARGET_BBOX_EDGE_MIDPOINT:
                target_name = _("bounding box side midpoint");
                break;
            case SNAPTARGET_BBOX_MIDPOINT:
                target_name = _("bounding box midpoint");
                break;
            case SNAPTARGET_PAGE_CORNER:
                target_name = _("page corner");
                break;
            case SNAPTARGET_ELLIPSE_QUADRANT_POINT:
                target_name = _("quadrant point");
                break;
            case SNAPTARGET_RECT_CORNER:
            case SNAPTARGET_IMG_CORNER:
                target_name = _("corner");
                break;
            case SNAPTARGET_TEXT_ANCHOR:
                target_name = _("text anchor");
                break;
            case SNAPTARGET_TEXT_BASELINE:
                target_name = _("text baseline");
                break;
            case SNAPTARGET_CONSTRAINED_ANGLE:
                target_name = _("constrained angle");
                break;
            case SNAPTARGET_CONSTRAINT:
                target_name = _("constraint");
                break;
            default:
                g_warning("Snap target not in SnapTargetType enum");
                break;
        }

        Glib::ustring source_name = _("UNDEFINED");
        switch (p.getSource()) {
            case SNAPSOURCE_UNDEFINED:
                source_name = _("UNDEFINED");
                g_warning("Snap source has not been specified");
                break;
            case SNAPSOURCE_BBOX_CORNER:
                source_name = _("Bounding box corner");
                break;
            case SNAPSOURCE_BBOX_MIDPOINT:
                source_name = _("Bounding box midpoint");
                break;
            case SNAPSOURCE_BBOX_EDGE_MIDPOINT:
                source_name = _("Bounding box side midpoint");
                break;
            case SNAPSOURCE_NODE_SMOOTH:
                source_name = _("Smooth node");
                break;
            case SNAPSOURCE_NODE_CUSP:
                source_name = _("Cusp node");
                break;
            case SNAPSOURCE_LINE_MIDPOINT:
                source_name = _("Line midpoint");
                break;
            case SNAPSOURCE_OBJECT_MIDPOINT:
                source_name = _("Object midpoint");
                break;
            case SNAPSOURCE_ROTATION_CENTER:
                source_name = _("Object rotation center");
                break;
            case SNAPSOURCE_NODE_HANDLE:
            case SNAPSOURCE_OTHER_HANDLE:
                source_name = _("Handle");
                break;
            case SNAPSOURCE_PATH_INTERSECTION:
                source_name = _("Path intersection");
                break;
            case SNAPSOURCE_GUIDE:
                source_name = _("Guide");
                break;
            case SNAPSOURCE_GUIDE_ORIGIN:
                source_name = _("Guide origin");
                break;
            case SNAPSOURCE_CONVEX_HULL_CORNER:
                source_name = _("Convex hull corner");
                break;
            case SNAPSOURCE_ELLIPSE_QUADRANT_POINT:
                source_name = _("Quadrant point");
                break;
            case SNAPSOURCE_RECT_CORNER:
            case SNAPSOURCE_IMG_CORNER:
                source_name = _("Corner");
                break;
            case SNAPSOURCE_TEXT_ANCHOR:
                source_name = _("Text anchor");
                break;
            case SNAPSOURCE_GRID_PITCH:
                source_name = _("Multiple of grid spacing");
                break;
            default:
                g_warning("Snap source not in SnapSourceType enum");
                break;
        }
        //std::cout << "Snapped " << source_name << " to " << target_name << std::endl;

        remove_snapsource(); // Don't set both the source and target indicators, as these will overlap

        // Display the snap indicator (i.e. the cross)
        auto ctrl = new Inkscape::CanvasItemCtrl(_desktop->getCanvasTemp(), Inkscape::CANVAS_ITEM_CTRL_SHAPE_CROSS);
        ctrl->set_size(11);
        ctrl->set_stroke( pre_snap ? 0x7f7f7fff : 0xff0000ff);
        ctrl->set_position(p.getPoint());

        // The snap indicator will be deleted after some time-out, and sp_canvas_item_dispose
        // will be called. This will set canvas->current_item to NULL if the snap indicator was
        // the current item, after which any events will go to the root handler instead of any
        // item handler. Dragging an object which has just snapped might therefore not be possible
        // without selecting / repicking it again. To avoid this, we make sure here that the
        // snap indicator will never be picked, and will therefore never be the current item.
        // Reported bugs:
        //   - scrolling when hovering above a pre-snap indicator won't work (for example)
        //     (https://bugs.launchpad.net/inkscape/+bug/522335/comments/8)
        //   - dragging doesn't work without repicking
        //     (https://bugs.launchpad.net/inkscape/+bug/1420301/comments/15)
        ctrl->set_pickable(false);

        double timeout_val = prefs->getDouble("/options/snapindicatorpersistence/value", 2.0);
        if (timeout_val < 0.1) {
            timeout_val = 0.1; // a zero value would mean infinite persistence (i.e. until new snap occurs)
            // Besides, negatives values would ....?
        }

        _snaptarget = _desktop->add_temporary_canvasitem(ctrl, timeout_val*1000.0);
        _snaptarget_is_presnap = pre_snap;

        // Display the tooltip, which reveals the type of snap source and the type of snap target
        Glib::ustring tooltip_str;
        if ( (p.getSource() != SNAPSOURCE_GRID_PITCH) && (p.getTarget() != SNAPTARGET_UNDEFINED) ) {
            tooltip_str = source_name + _(" to ") + target_name;
        } else if (p.getSource() != SNAPSOURCE_UNDEFINED) {
            tooltip_str = source_name;
        }

        double fontsize = prefs->getDouble("/tools/measure/fontsize", 10.0);

        if (!tooltip_str.empty()) {
            Geom::Point tooltip_pos = p.getPoint();
            if (dynamic_cast<Inkscape::UI::Tools::MeasureTool *>(_desktop->event_context)) {
                // Make sure that the snap tooltips do not overlap the ones from the measure tool
                tooltip_pos += _desktop->w2d(Geom::Point(0, -3*fontsize));
            } else {
                tooltip_pos += _desktop->w2d(Geom::Point(0, -2*fontsize));
            }

            auto canvas_tooltip = new Inkscape::CanvasItemText(_desktop->getCanvasTemp(), tooltip_pos, tooltip_str);
            canvas_tooltip->set_fontsize(fontsize);
            canvas_tooltip->set_fill(0xffffffff);
            canvas_tooltip->set_background(pre_snap ? 0x33337f40 : 0x33337f7f);

            _snaptarget_tooltip = _desktop->add_temporary_canvasitem(canvas_tooltip, timeout_val*1000.0);
        }

        // Display the bounding box, if we snapped to one
        Geom::OptRect const bbox = p.getTargetBBox();
        if (bbox) {
            auto box = new Inkscape::CanvasItemRect(_desktop->getCanvasTemp(), *bbox);
            box->set_stroke(pre_snap ? 0x7f7f7fff : 0xff0000ff);
            box->set_dashed(true);
            box->set_pickable(false); // Is false by default.
            box->set_z_position(0);
            _snaptarget_bbox = _desktop->add_temporary_canvasitem(box, timeout_val*1000.0);
        }
    }
}

void
SnapIndicator::remove_snaptarget(bool only_if_presnap)
{
    if (only_if_presnap && !_snaptarget_is_presnap) {
        return;
    }

    if (_snaptarget) {
        _desktop->remove_temporary_canvasitem(_snaptarget);
        _snaptarget = nullptr;
        _snaptarget_is_presnap = false;
    }

    if (_snaptarget_tooltip) {
        _desktop->remove_temporary_canvasitem(_snaptarget_tooltip);
        _snaptarget_tooltip = nullptr;
    }

    if (_snaptarget_bbox) {
        _desktop->remove_temporary_canvasitem(_snaptarget_bbox);
        _snaptarget_bbox = nullptr;
    }

}

void
SnapIndicator::set_new_snapsource(Inkscape::SnapCandidatePoint const &p)
{
    remove_snapsource();

    g_assert(_desktop != nullptr); // If this fails, then likely setup() has not been called on the snap manager (see snap.cpp -> setup())

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool value = prefs->getBool("/options/snapindicator/value", true);

    if (value) {
        auto ctrl = new Inkscape::CanvasItemCtrl(_desktop->getCanvasTemp(), Inkscape::CANVAS_ITEM_CTRL_SHAPE_CIRCLE);
        ctrl->set_size(7);
        ctrl->set_stroke(0xff0000ff);
        ctrl->set_position(p.getPoint());
        _snapsource = _desktop->add_temporary_canvasitem(ctrl, 1000);
    }
}

void
SnapIndicator::set_new_debugging_point(Geom::Point const &p)
{
    g_assert(_desktop != nullptr);
    auto ctrl = new Inkscape::CanvasItemCtrl(_desktop->getCanvasTemp(), Inkscape::CANVAS_ITEM_CTRL_SHAPE_DIAMOND);
    ctrl->set_size(11);
    ctrl->set_stroke(0x00ff00ff);
    ctrl->set_position(p);
    _debugging_points.push_back(_desktop->add_temporary_canvasitem(ctrl, 5000));
}

void
SnapIndicator::remove_snapsource()
{
    if (_snapsource) {
        _desktop->remove_temporary_canvasitem(_snapsource);
        _snapsource = nullptr;
    }
}

void
SnapIndicator::remove_debugging_points()
{
    for (std::list<TemporaryItem *>::const_iterator i = _debugging_points.begin(); i != _debugging_points.end(); ++i) {
        _desktop->remove_temporary_canvasitem(*i);
    }
    _debugging_points.clear();
}


} //namespace Display
} /* namespace Inkscape */


/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4 :

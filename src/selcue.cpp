// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Helper object for showing selected items
 *
 * Authors:
 *   bulia byak <bulia@users.sf.net>
 *   Carl Hetherington <inkscape@carlh.net>
 *   Abhishek Sharma
 *
 * Copyright (C) 2004 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "selcue.h"

#include "desktop.h"
#include "selection.h"
#include "text-editing.h"

#include "display/control/canvas-item-ctrl.h"
#include "display/control/canvas-item-rect.h"

#include "libnrtype/Layout-TNG.h"

#include "object/sp-flowtext.h"
#include "object/sp-text.h"

Inkscape::SelCue::BoundingBoxPrefsObserver::BoundingBoxPrefsObserver(SelCue &sel_cue) :
    Observer("/tools/bounding_box"),
    _sel_cue(sel_cue)
{
}

void Inkscape::SelCue::BoundingBoxPrefsObserver::notify(Preferences::Entry const &val)
{
    _sel_cue._boundingBoxPrefsChanged(static_cast<int>(val.getBool()));
}

Inkscape::SelCue::SelCue(SPDesktop *desktop)
    : _desktop(desktop),
      _bounding_box_prefs_observer(*this)
{
    _selection = _desktop->getSelection();

    _sel_changed_connection = _selection->connectChanged(
        sigc::hide(sigc::mem_fun(*this, &Inkscape::SelCue::_newItemBboxes))
        );

    {
        void(SelCue::*modifiedSignal)() = &SelCue::_updateItemBboxes;
        _sel_modified_connection = _selection->connectModified(
            sigc::hide(sigc::hide(sigc::mem_fun(*this, modifiedSignal)))
        );
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _updateItemBboxes(prefs);
    prefs->addObserver(_bounding_box_prefs_observer);
}

Inkscape::SelCue::~SelCue()
{
    _sel_changed_connection.disconnect();
    _sel_modified_connection.disconnect();

    for (auto & item : _item_bboxes) {
        delete item;
    }
    _item_bboxes.clear();

    for (auto & item : _text_baselines) {
        delete item;
    }
    _text_baselines.clear();
}

void Inkscape::SelCue::_updateItemBboxes()
{
    _updateItemBboxes(Inkscape::Preferences::get());
}

void Inkscape::SelCue::_updateItemBboxes(Inkscape::Preferences *prefs)
{
    gint mode = prefs->getInt("/options/selcue/value", MARK);
    if (mode == NONE) {
        return;
    }

    g_return_if_fail(_selection != nullptr);

    int prefs_bbox = prefs->getBool("/tools/bounding_box");

    _updateItemBboxes(mode, prefs_bbox);
}

void Inkscape::SelCue::_updateItemBboxes(gint mode, int prefs_bbox)
{
    auto items = _selection->items();
    if (_item_bboxes.size() != (unsigned int) boost::distance(items)) {
        _newItemBboxes();
        return;
    }

    int bcount = 0;
    for (auto item : items) {

        auto canvas_item = _item_bboxes[bcount++];

        if (canvas_item) {
            Geom::OptRect const b = (prefs_bbox == 0) ?
                item->desktopVisualBounds() : item->desktopGeometricBounds();

            if (b) {
                auto ctrl = dynamic_cast<CanvasItemCtrl *>(canvas_item);
                if (ctrl) {
                    ctrl->set_position(Geom::Point(b->min().x(), b->max().y()));
                }
                auto rect = dynamic_cast<CanvasItemRect *>(canvas_item);
                if (rect) {
                    rect->set_rect(*b);
                }
                canvas_item->show();
            } else { // no bbox
                canvas_item->hide();
            }
        }
    }

    _newTextBaselines();
}


void Inkscape::SelCue::_newItemBboxes()
{
    for (auto & item : _item_bboxes) {
        delete item;
    }
    _item_bboxes.clear();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gint mode = prefs->getInt("/options/selcue/value", MARK);
    if (mode == NONE) {
        return;
    }

    g_return_if_fail(_selection != nullptr);

    int prefs_bbox = prefs->getBool("/tools/bounding_box");
    
    auto items = _selection->items();
    for (auto item : items) {

        Geom::OptRect const bbox = (prefs_bbox == 0) ?
            item->desktopVisualBounds() : item->desktopGeometricBounds();

        if (bbox) {
            Inkscape::CanvasItem *canvas_item = nullptr;

            if (mode == MARK) {
                auto ctrl = new Inkscape::CanvasItemCtrl(_desktop->getCanvasControls(),
                                                         Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER,
                                                         Geom::Point(bbox->min().x(), bbox->max().y()));
                ctrl->set_fill(0x000000ff);
                ctrl->set_stroke(0x0000000ff);
                canvas_item = ctrl;

            } else if (mode == BBOX) {
                auto rect = new Inkscape::CanvasItemRect(_desktop->getCanvasControls(), *bbox);
                rect->set_stroke(0xffffffa0);
                rect->set_shadow(0x0000c0a0, 1);
                rect->set_dashed(true);
                rect->set_inverted(true);
                canvas_item = rect;
            }

            if (canvas_item) {
                canvas_item->set_pickable(false);
                canvas_item->set_z_position(0); // Just low enough to not get in the way of other draggable knots.
                canvas_item->show();
                _item_bboxes.emplace_back(canvas_item);
            }
        }
    }

    _newTextBaselines();
}

void Inkscape::SelCue::_newTextBaselines()
{
    for (auto canvas_item : _text_baselines) {
        delete canvas_item;
    }
    _text_baselines.clear();

    auto items = _selection->items();
    for (auto item : items) {

        if (SP_IS_TEXT(item) || SP_IS_FLOWTEXT(item)) { // visualize baseline
            Inkscape::Text::Layout const *layout = te_get_layout(item);
            if (layout != nullptr && layout->outputExists()) {
                boost::optional<Geom::Point> pt = layout->baselineAnchorPoint();
                if (pt) {
                    auto canvas_item = new Inkscape::CanvasItemCtrl(_desktop->getCanvasControls(),
                                                                    Inkscape::CANVAS_ITEM_CTRL_SHAPE_SQUARE,
                                                                    (*pt) * item->i2dt_affine());
                    canvas_item->set_size(5);
                    canvas_item->set_stroke(0x000000ff);
                    canvas_item->set_fill(0x00000000);
                    canvas_item->set_z_position(0);
                    canvas_item->show();
                    _text_baselines.emplace_back(canvas_item);
                }
            }
        }
    }
}

void Inkscape::SelCue::_boundingBoxPrefsChanged(int prefs_bbox)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gint mode = prefs->getInt("/options/selcue/value", MARK);
    if (mode == NONE) {
        return;
    }

    g_return_if_fail(_selection != nullptr);

    _updateItemBboxes(mode, prefs_bbox);
}

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

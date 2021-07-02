// SPDX-License-Identifier: GPL-2.0-or-later
/* Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2009 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <iostream>

#include <gdk/gdkkeysyms.h>
#include <gdkmm.h>

#include <2geom/point.h>

#include "desktop.h"
#include "message-context.h"

#include "display/control/canvas-item-enums.h"
#include "display/control/snap-indicator.h"

#include "object/sp-namedview.h"

#include "ui/tools/tool-base.h"
#include "ui/tool/control-point.h"
#include "ui/tool/event-utils.h"
#include "ui/tool/transform-handle-set.h"

#include "ui/widget/canvas.h" // Forced redraws

namespace Inkscape {
namespace UI {


// Default colors for control points
ControlPoint::ColorSet ControlPoint::_default_color_set = {
    {0xffffff00, 0x01000000}, // normal fill, stroke
    {0xff0000ff, 0x01000000}, // mouseover fill, stroke
    {0x0000ffff, 0x01000000}, // clicked fill, stroke
    //
    {0x0000ffff, 0x000000ff}, // normal fill, stroke when selected
    {0xff000000, 0x000000ff}, // mouseover fill, stroke when selected
    {0xff000000, 0x000000ff}  // clicked fill, stroke when selected
};

ControlPoint *ControlPoint::mouseovered_point = nullptr;

sigc::signal<void, ControlPoint*> ControlPoint::signal_mouseover_change;

Geom::Point ControlPoint::_drag_event_origin(Geom::infinity(), Geom::infinity());

Geom::Point ControlPoint::_drag_origin(Geom::infinity(), Geom::infinity());

Gdk::EventMask const ControlPoint::_grab_event_mask = (Gdk::BUTTON_PRESS_MASK   |
                                                       Gdk::BUTTON_RELEASE_MASK |
                                                       Gdk::POINTER_MOTION_MASK |
                                                       Gdk::KEY_PRESS_MASK      |
                                                       Gdk::KEY_RELEASE_MASK    |
                                                       Gdk::SCROLL_MASK         |
                                                       Gdk::SMOOTH_SCROLL_MASK  );

bool ControlPoint::_drag_initiated = false;
bool ControlPoint::_event_grab = false;

ControlPoint::ColorSet ControlPoint::invisible_cset = {
    {0x00000000, 0x00000000},
    {0x00000000, 0x00000000},
    {0x00000000, 0x00000000},
    {0x00000000, 0x00000000},
    {0x00000000, 0x00000000},
    {0x00000000, 0x00000000}
};

ControlPoint::ControlPoint(SPDesktop *d, Geom::Point const &initial_pos, SPAnchorType anchor,
                           Glib::RefPtr<Gdk::Pixbuf> pixbuf,
                           ColorSet const &cset,
                           Inkscape::CanvasItemGroup *group)
    : _desktop(d)
    , _cset(cset)
    , _position(initial_pos)
{
    _canvas_item_ctrl = new Inkscape::CanvasItemCtrl(group ? group : _desktop->getCanvasControls(),
                                                Inkscape::CANVAS_ITEM_CTRL_SHAPE_BITMAP);
    _canvas_item_ctrl->set_name("CanvasItemCtrl:ControlPoint");
    _canvas_item_ctrl->set_pixbuf(pixbuf->gobj());
    _canvas_item_ctrl->set_fill(  _cset.normal.fill);
    _canvas_item_ctrl->set_stroke(_cset.normal.stroke);
    _canvas_item_ctrl->set_anchor(anchor);

    _commonInit();
}

ControlPoint::ControlPoint(SPDesktop *d, Geom::Point const &initial_pos, SPAnchorType anchor,
                           Inkscape::CanvasItemCtrlType type,
                           ColorSet const &cset,
                           Inkscape::CanvasItemGroup *group)
    : _desktop(d)
    , _cset(cset)
    , _position(initial_pos)
{
    _canvas_item_ctrl = new Inkscape::CanvasItemCtrl(group ? group : _desktop->getCanvasControls(), type);
    _canvas_item_ctrl->set_name("CanvasItemCtrl:ControlPoint");
    _canvas_item_ctrl->set_fill(  _cset.normal.fill);
    _canvas_item_ctrl->set_stroke(_cset.normal.stroke);
    _canvas_item_ctrl->set_anchor(anchor);

    _commonInit();
}

ControlPoint::~ControlPoint()
{
    // avoid storing invalid points in mouseovered_point
    if (this == mouseovered_point) {
        _clearMouseover();
    }

    //g_signal_handler_disconnect(G_OBJECT(_canvas_item_ctrl), _event_handler_connection);
    _event_handler_connection.disconnect();
    _canvas_item_ctrl->hide();
    delete _canvas_item_ctrl;
}

void ControlPoint::_commonInit()
{
    _canvas_item_ctrl->set_position(_position);
    _event_handler_connection =
        _canvas_item_ctrl->connect_event(sigc::bind(sigc::ptr_fun(_event_handler), this));
    // _event_handler_connection = g_signal_connect(G_OBJECT(_canvas_item_ctrl), "event",
    //                                              G_CALLBACK(_event_handler), this);
}

void ControlPoint::setPosition(Geom::Point const &pos)
{
    _position = pos;
    _canvas_item_ctrl->set_position(_position);
}

void ControlPoint::move(Geom::Point const &pos)
{
    setPosition(pos);
}

void ControlPoint::transform(Geom::Affine const &m) {
    move(position() * m);
}

bool ControlPoint::visible() const
{
    return _canvas_item_ctrl->is_visible();
}

void ControlPoint::setVisible(bool v)
{
    if (v) {
        _canvas_item_ctrl->show();
    } else {
        _canvas_item_ctrl->hide();
    }
}

Glib::ustring ControlPoint::format_tip(char const *format, ...)
{
    va_list args;
    va_start(args, format);
    char *dyntip = g_strdup_vprintf(format, args);
    va_end(args);
    Glib::ustring ret = dyntip;
    g_free(dyntip);
    return ret;
}


// ===== Setters =====

void ControlPoint::_setSize(unsigned int size)
{
    _canvas_item_ctrl->set_size(size);
}

void ControlPoint::_setControlType(Inkscape::CanvasItemCtrlType type)
{
    _canvas_item_ctrl->set_type(type);
}

void ControlPoint::_setAnchor(SPAnchorType anchor)
{
//     g_object_set(_canvas_item_ctrl, "anchor", anchor, nullptr);
}

void ControlPoint::_setPixbuf(Glib::RefPtr<Gdk::Pixbuf> p)
{
    _canvas_item_ctrl->set_pixbuf(Glib::unwrap(p));
}

// re-routes events into the virtual function   TODO: Refactor this nonsense.
bool ControlPoint::_event_handler(GdkEvent *event, ControlPoint *point)
{
    if ((point == nullptr) || (point->_desktop == nullptr)) {
        return false;
    }
    return point->_eventHandler(point->_desktop->event_context, event);
}

// main event callback, which emits all other callbacks.
bool ControlPoint::_eventHandler(Inkscape::UI::Tools::ToolBase *event_context, GdkEvent *event)
{
    // NOTE the static variables below are shared for all points!
    // TODO handle clicks and drags from other buttons too

    if (event == nullptr)
    {
        return false;
    }
    
    if (event_context == nullptr)
    {
        return false;
    }
    if (_desktop == nullptr)
    {
        return false;
    }
    if(event_context->getDesktop() !=_desktop)
    {
        g_warning ("ControlPoint: desktop pointers not equal!");
        //return false;
    }
    // offset from the pointer hotspot to the center of the grabbed knot in desktop coords
    static Geom::Point pointer_offset;
    // number of last doubleclicked button
    static unsigned next_release_doubleclick = 0;
    _double_clicked = false;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int drag_tolerance = prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100);

    switch(event->type)
    {   
    case GDK_BUTTON_PRESS:
        next_release_doubleclick = 0;
        if (event->button.button == 1 && !event_context->is_space_panning()) {
            // 1st mouse button click. internally, start dragging, but do not emit signals
            // or change position until drag tolerance is exceeded.
            _drag_event_origin[Geom::X] = event->button.x;
            _drag_event_origin[Geom::Y] = event->button.y;
            pointer_offset = _position - _desktop->w2d(_drag_event_origin);
            _drag_initiated = false;
            // route all events to this handler
            _canvas_item_ctrl->grab(_grab_event_mask, nullptr); // cursor is null
            _event_grab = true;
            _setState(STATE_CLICKED);
            return true;
        }
        return _event_grab;

    case GDK_2BUTTON_PRESS:
        // store the button number for next release
        next_release_doubleclick = event->button.button;
        return true;
        
    case GDK_MOTION_NOTIFY:
        if (_event_grab && ! event_context->is_space_panning()) {
            _desktop->snapindicator->remove_snaptarget(); 
            bool transferred = false;
            if (!_drag_initiated) {
                bool t = fabs(event->motion.x - _drag_event_origin[Geom::X]) <= drag_tolerance &&
                         fabs(event->motion.y - _drag_event_origin[Geom::Y]) <= drag_tolerance;
                if (t){
                    return true;
                }

                // if we are here, it means the tolerance was just exceeded.
                _drag_origin = _position;
                transferred = grabbed(&event->motion);
                // _drag_initiated might change during the above virtual call
                if (!_drag_initiated) {
                    // this guarantees smooth redraws while dragging
                    _desktop->getCanvas()->forced_redraws_start(5);
                    _drag_initiated = true;
                }
            }

            if (!transferred) {
                // dragging in progress
                Geom::Point new_pos = _desktop->w2d(event_point(event->motion)) + pointer_offset;
                // the new position is passed by reference and can be changed in the handlers.
                dragged(new_pos, &event->motion);
                move(new_pos);
                _updateDragTip(&event->motion); // update dragging tip after moving to new position

                _desktop->scroll_to_point(new_pos);
                _desktop->set_coordinate_status(_position);
                sp_event_context_snap_delay_handler(event_context, nullptr,
                    (gpointer) this, &event->motion,
                    Inkscape::UI::Tools::DelayedSnapEvent::CONTROL_POINT_HANDLER);
            }
            return true;
        }
        break;
        
    case GDK_BUTTON_RELEASE:
        if (_event_grab && event->button.button == 1) {
            // If we have any pending snap event, then invoke it now!
            // (This is needed because we might not have snapped on the latest GDK_MOTION_NOTIFY event
            // if the mouse speed was too high. This is inherent to the snap-delay mechanism.
            // We must snap at some point in time though, and this is our last chance)
            // PS: For other contexts this is handled already in sp_event_context_item_handler or
            // sp_event_context_root_handler
            //if (_desktop && _desktop->event_context && _desktop->event_context->_delayed_snap_event) {
            if (event_context->_delayed_snap_event) {
                sp_event_context_snap_watchdog_callback(event_context->_delayed_snap_event);
            }

            _canvas_item_ctrl->ungrab();
            _setMouseover(this, event->button.state);
            _event_grab = false;

            if (_drag_initiated) {
                // it is the end of a drag
                _desktop->getCanvas()->forced_redraws_stop();
                _drag_initiated = false;
                ungrabbed(&event->button);
                return true;
            } else {
                // it is the end of a click
                if (next_release_doubleclick) {
                    _double_clicked = true;
                    return doubleclicked(&event->button);
                } else {
                    return clicked(&event->button);
                }
            }
        }
        break;

    case GDK_ENTER_NOTIFY:
        _setMouseover(this, event->crossing.state);
        return true;
    case GDK_LEAVE_NOTIFY:
        _clearMouseover();
        return true;

    case GDK_GRAB_BROKEN:
        if (_event_grab && !event->grab_broken.keyboard) {
            {
                ungrabbed(nullptr);
                if (_drag_initiated) {
                    _desktop->getCanvas()->forced_redraws_stop();
                }
            }
            _setState(STATE_NORMAL);
            _event_grab = false;
            _drag_initiated = false;
            return true;
        }
        break;

    // update tips on modifier state change
    // TODO add ESC keybinding as drag cancel
    case GDK_KEY_PRESS:
        switch (Inkscape::UI::Tools::get_latin_keyval(&event->key))
        {
        case GDK_KEY_Escape: {
            // ignore Escape if this is not a drag
            if (!_drag_initiated) break;

            // temporarily disable snapping - we might snap to a different place than we were initially
            sp_event_context_discard_delayed_snap_event(event_context);
            SnapPreferences &snapprefs = _desktop->namedview->snap_manager.snapprefs;
            bool snap_save = snapprefs.getSnapEnabledGlobally();
            snapprefs.setSnapEnabledGlobally(false);

            Geom::Point new_pos = _drag_origin;

            // make a fake event for dragging
            // ASSUMPTION: dragging a point without modifiers will never prevent us from moving it
            //             to its original position
            GdkEventMotion fake;
            fake.type = GDK_MOTION_NOTIFY;
            fake.window = event->key.window;
            fake.send_event = event->key.send_event;
            fake.time = event->key.time;
            fake.x = _drag_event_origin[Geom::X]; // these two are normally not used in handlers
            fake.y = _drag_event_origin[Geom::Y]; // (and shouldn't be)
            fake.axes = nullptr;
            fake.state = 0; // unconstrained drag
            fake.is_hint = FALSE;
            fake.device = nullptr;
            fake.x_root = -1; // not used in handlers (and shouldn't be)
            fake.y_root = -1; // can be used as a flag to check for cancelled drag
            
            dragged(new_pos, &fake);

            _canvas_item_ctrl->ungrab();
            _clearMouseover(); // this will also reset state to normal
            _desktop->getCanvas()->forced_redraws_stop();
            _event_grab = false;
            _drag_initiated = false;

            ungrabbed(nullptr); // ungrabbed handlers can handle a NULL event
            snapprefs.setSnapEnabledGlobally(snap_save);
            }
            return true;
        case GDK_KEY_Tab:
            {// Downcast from ControlPoint to TransformHandle, if possible
             // This is an ugly hack; we should have the transform handle intercept the keystrokes itself
            TransformHandle *th = dynamic_cast<TransformHandle*>(this);
            if (th) {
                th->getNextClosestPoint(false);
                return true;
            }
            break;
            }
        case GDK_KEY_ISO_Left_Tab:
            {// Downcast from ControlPoint to TransformHandle, if possible
             // This is an ugly hack; we should have the transform handle intercept the keystrokes itself
            TransformHandle *th = dynamic_cast<TransformHandle*>(this);
            if (th) {
                th->getNextClosestPoint(true);
                return true;
            }
            break;
            }
        default:
            break;
        }
        // Do not break here, to allow for updating tooltips and such
    case GDK_KEY_RELEASE: 
        if (mouseovered_point != this){
            return false;
        }
        if (_drag_initiated) {
            return true; // this prevents the tool from overwriting the drag tip
        } else {
            unsigned state = state_after_event(event);
            if (state != event->key.state) {
                // we need to return true if there was a tip available, otherwise the tool's
                // handler will process this event and set the tool's message, overwriting
                // the point's message
                return _updateTip(state);
            }
        }
        break;

    default: break;
    }

    // do not propagate events during grab - it might cause problems
    return _event_grab;
}

void ControlPoint::_setMouseover(ControlPoint *p, unsigned state)
{
    bool visible = p->visible();
    if (visible) { // invisible points shouldn't get mouseovered
        p->_setState(STATE_MOUSEOVER);
    }
    p->_updateTip(state);

    if (visible && mouseovered_point != p) {
        mouseovered_point = p;
        signal_mouseover_change.emit(mouseovered_point);
    }
}

bool ControlPoint::_updateTip(unsigned state)
{
    Glib::ustring tip = _getTip(state);
    if (!tip.empty()) {
        _desktop->event_context->defaultMessageContext()->set(Inkscape::NORMAL_MESSAGE,
            tip.data());
        return true;
    } else {
        _desktop->event_context->defaultMessageContext()->clear();
        return false;
    }
}

bool ControlPoint::_updateDragTip(GdkEventMotion *event)
{
    if (!_hasDragTips()) {
        return false;
    }
    Glib::ustring tip = _getDragTip(event);
    if (!tip.empty()) {
        _desktop->event_context->defaultMessageContext()->set(Inkscape::NORMAL_MESSAGE,
            tip.data());
        return true;
    } else {
        _desktop->event_context->defaultMessageContext()->clear();
        return false;
    }
}

void ControlPoint::_clearMouseover()
{
    if (mouseovered_point) {
        mouseovered_point->_desktop->event_context->defaultMessageContext()->clear();
        mouseovered_point->_setState(STATE_NORMAL);
        mouseovered_point = nullptr;
        signal_mouseover_change.emit(mouseovered_point);
    }
}

void ControlPoint::transferGrab(ControlPoint *prev_point, GdkEventMotion *event)
{
    if (!_event_grab) return;

    grabbed(event);
    prev_point->_canvas_item_ctrl->ungrab();
    _canvas_item_ctrl->grab(_grab_event_mask, nullptr); // cursor is null

    if (!_drag_initiated) {
        _desktop->getCanvas()->forced_redraws_start(5);
        _drag_initiated = true;
    }

    prev_point->_setState(STATE_NORMAL);
    _setMouseover(this, event->state);
}

void ControlPoint::_setState(State state)
{
    ColorEntry current = {0, 0};
    ColorSet const &activeCset = (_isLurking()) ? invisible_cset : _cset;
    switch(state) {
        case STATE_NORMAL:
            current = activeCset.normal;
            break;
        case STATE_MOUSEOVER:
            current = activeCset.mouseover;
            break;
        case STATE_CLICKED:
            current = activeCset.clicked;
            break;
    };
    _setColors(current);
    _state = state;
}

// TODO: RENAME
void ControlPoint::_handleControlStyling()
{
    _canvas_item_ctrl->set_size_default();
}

void ControlPoint::_setColors(ColorEntry colors)
{
    _canvas_item_ctrl->set_fill(colors.fill);
    _canvas_item_ctrl->set_stroke(colors.stroke);
}

bool ControlPoint::_isLurking()
{
    return _lurking;
}

void ControlPoint::_setLurking(bool lurking)
{
    if (lurking != _lurking) {
        _lurking = lurking;
        _setState(_state); // TODO refactor out common part
    }
}


bool ControlPoint::_is_drag_cancelled(GdkEventMotion *event)
{
    return !event || event->x_root == -1;
}

// dummy implementations for handlers

bool ControlPoint::grabbed(GdkEventMotion * /*event*/)
{
    return false;
}

void ControlPoint::dragged(Geom::Point &/*new_pos*/, GdkEventMotion * /*event*/)
{
}

void ControlPoint::ungrabbed(GdkEventButton * /*event*/)
{
}

bool ControlPoint::clicked(GdkEventButton * /*event*/)
{
    return false;
}

bool ControlPoint::doubleclicked(GdkEventButton * /*event*/)
{
    return false;
}

} // namespace UI
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

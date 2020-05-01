// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * A dockable dialog implementation.
 */
/* Author:
 *   Gustav Broberg <broberg@kth.se>
 *
 * Copyright (C) 2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "dock-behavior.h"
#include "inkscape.h"
#include "desktop.h"
#include "ui/widget/dock.h"
#include "verbs.h"
#include "dialog.h"
#include "ui/dialog-events.h"

namespace Inkscape {
namespace UI {
namespace Dialog {
namespace Behavior {


DockBehavior::DockBehavior(Dialog &dialog) :
    Behavior(dialog),
    _dock_item(*SP_ACTIVE_DESKTOP->getDock(),
               Inkscape::Verb::get(dialog._verb_num)->get_id(), dialog._title.c_str(),
               (Inkscape::Verb::get(dialog._verb_num)->get_image() ?
                Inkscape::Verb::get(dialog._verb_num)->get_image() : ""),
               static_cast<Widget::DockItem::State>(
                   Inkscape::Preferences::get()->getInt(_dialog._prefs_path + "/state",
                                            UI::Widget::DockItem::DOCKED_STATE)),
                static_cast<GdlDockPlacement>(
                    Inkscape::Preferences::get()->getInt(_dialog._prefs_path + "/placement",
                                             GDL_DOCK_TOP)))

{
    // Connect signals
    _signal_hide_connection = signal_hide().connect(sigc::mem_fun(*this, &Inkscape::UI::Dialog::Behavior::DockBehavior::_onHide));

    _connections.emplace_back(_signal_hide_connection);
    _connections.emplace_back(
        signal_show().connect(sigc::mem_fun(*this, &Inkscape::UI::Dialog::Behavior::DockBehavior::_onShow)));
}

DockBehavior::~DockBehavior() {
    for (auto &conn : _connections) {
        conn.disconnect();
    }
}


Behavior *
DockBehavior::create(Dialog &dialog)
{
    return new DockBehavior(dialog);
}


DockBehavior::operator Gtk::Widget &()
{
    return _dock_item.getWidget();
}

GtkWidget *
DockBehavior::gobj()
{
    return _dock_item.gobj();
}

Gtk::VBox *
DockBehavior::get_vbox()
{
    return _dock_item.get_vbox();
}

void
DockBehavior::present()
{
    bool was_attached = _dock_item.isAttached();

    _dock_item.present();

    if (!was_attached)
        _dialog.read_geometry();
}

void
DockBehavior::hide()
{
    _signal_hide_connection.block();
    _dock_item.hide();
    _signal_hide_connection.unblock();
}

void
DockBehavior::show()
{
    _dock_item.show();
}

void
DockBehavior::show_all_children()
{
    get_vbox()->show_all_children();
}

void
DockBehavior::get_position(int &x, int &y)
{
    _dock_item.get_position(x, y);
}

void
DockBehavior::get_size(int &width, int &height)
{
    _dock_item.get_size(width, height);
}

void
DockBehavior::resize(int width, int height)
{
    _dock_item.resize(width, height);
}

void
DockBehavior::move(int x, int y)
{
    _dock_item.move(x, y);
}

void
DockBehavior::set_position(Gtk::WindowPosition position)
{
    _dock_item.set_position(position);
}

void
DockBehavior::set_size_request(int width, int height)
{
    _dock_item.set_size_request(width, height);
}

void
DockBehavior::size_request(Gtk::Requisition &requisition)
{
    _dock_item.size_request(requisition);
}

void
DockBehavior::set_title(Glib::ustring title)
{
    _dock_item.set_title(title);
}

void DockBehavior::set_sensitive(bool sensitive)
{
    // TODO check this. Seems to be bad that we ignore the parameter
    get_vbox()->set_sensitive();
}


void
DockBehavior::_onHide()
{
    _dialog.save_geometry();
    _dialog._user_hidden = true;
}

void
DockBehavior::_onShow()
{
    _dialog._user_hidden = false;
}

void
DockBehavior::_onStateChanged(Widget::DockItem::State /*prev_state*/,
                              Widget::DockItem::State new_state)
{
    // TODO remove method
    g_assert_not_reached();
}

void
DockBehavior::onHideF12()
{
    _dialog.save_geometry();
    hide();
}

void
DockBehavior::onShowF12()
{
    present();
}

void
DockBehavior::onShutdown()
{
    int visible = _dock_item.isIconified() || !_dialog._user_hidden;
    int status = (_dock_item.getState() == Inkscape::UI::Widget::DockItem::UNATTACHED) ? _dock_item.getPrevState() : _dock_item.getState();
    _dialog.save_status( visible, status, _dock_item.getPlacement() );
}

void
DockBehavior::onDesktopActivated(SPDesktop *desktop)
{
}


/* Signal wrappers */

Glib::SignalProxy0<void>
DockBehavior::signal_show() { return _dock_item.signal_show(); }

Glib::SignalProxy0<void>
DockBehavior::signal_hide() { return _dock_item.signal_hide(); }

Glib::SignalProxy1<bool, GdkEventAny *>
DockBehavior::signal_delete_event() { return _dock_item.signal_delete_event(); }

Glib::SignalProxy0<void>
DockBehavior::signal_drag_begin() { return _dock_item.signal_drag_begin(); }

Glib::SignalProxy1<void, bool>
DockBehavior::signal_drag_end() { return _dock_item.signal_drag_end(); }


} // namespace Behavior
} // namespace Dialog
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

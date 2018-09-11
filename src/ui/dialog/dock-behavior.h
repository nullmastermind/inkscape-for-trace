// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief A dockable dialog implementation.
 */
/* Author:
 *   Gustav Broberg <broberg@kth.se>
 *
 * Copyright (C) 2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#ifndef INKSCAPE_UI_DIALOG_DOCK_BEHAVIOR_H
#define INKSCAPE_UI_DIALOG_DOCK_BEHAVIOR_H

#include "ui/widget/dock-item.h"
#include "behavior.h"

namespace Gtk {
	class Paned;
}

namespace Inkscape {
namespace UI {
namespace Dialog {
namespace Behavior {

class DockBehavior : public Behavior {

public:
    static Behavior *create(Dialog& dialog);

    ~DockBehavior() override;

    /** Gtk::Dialog methods */
    operator Gtk::Widget&() override;
    GtkWidget *gobj() override;
    void present() override;
    Gtk::VBox *get_vbox() override;
    void show() override;
    void hide() override;
    void show_all_children() override;
    void resize(int width, int height) override;
    void move(int x, int y) override;
    void set_position(Gtk::WindowPosition) override;
    void set_size_request(int width, int height) override;
    void size_request(Gtk::Requisition& requisition) override;
    void get_position(int& x, int& y) override;
    void get_size(int& width, int& height) override;
    void set_title(Glib::ustring title) override;
    void set_sensitive(bool sensitive) override;

    /** Gtk::Dialog signal proxies */
    Glib::SignalProxy0<void> signal_show() override;
    Glib::SignalProxy0<void> signal_hide() override;
    Glib::SignalProxy1<bool, GdkEventAny *> signal_delete_event() override;
    Glib::SignalProxy0<void> signal_drag_begin();
    Glib::SignalProxy1<void, bool> signal_drag_end();

    /** Custom signal handlers */
    void onHideF12() override;
    void onShowF12() override;
    void onDesktopActivated(SPDesktop *desktop) override;
    void onShutdown() override;

private:
    Widget::DockItem _dock_item;

    DockBehavior(Dialog& dialog);

    /** Internal helpers */
    Gtk::Paned *_getPaned();              //< gives the parent pane, if the dock item has one
    void _requestHeight(int height);      //< tries to resize the dock item to the requested height

    /** Internal signal handlers */
    void _onHide();
    void _onShow();
    bool _onDeleteEvent(GdkEventAny *event);
    void _onStateChanged(Widget::DockItem::State prev_state, Widget::DockItem::State new_state);
    bool _onKeyPress(GdkEventKey *event);

    sigc::connection _signal_hide_connection;
    sigc::connection _signal_key_press_event_connection;

};

} // namespace Behavior
} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_DIALOG_DOCK_BEHAVIOR_H

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

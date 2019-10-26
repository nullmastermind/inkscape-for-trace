// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief A floating dialog implementation.
 */
/* Author:
 *   Gustav Broberg <broberg@kth.se>
 *
 * Copyright (C) 2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#ifndef INKSCAPE_UI_DIALOG_FLOATING_BEHAVIOR_H
#define INKSCAPE_UI_DIALOG_FLOATING_BEHAVIOR_H

#include <glibmm/property.h>
#include "behavior.h"

namespace Gtk {
class Dialog;
}

namespace Inkscape {
namespace UI {
namespace Dialog {
namespace Behavior {

class FloatingBehavior : public Behavior {

public:
    static Behavior *create(Dialog &dialog);

    ~FloatingBehavior() override;

    /** Gtk::Dialog methods */
    operator Gtk::Widget &() override;
    GtkWidget *gobj() override;
    void present() override;
    Gtk::Box *get_vbox() override;
    void show() override;
    void hide() override;
    void show_all_children() override;
    void resize(int width, int height) override;
    void move(int x, int y) override;
    void set_position(Gtk::WindowPosition) override;
    void set_size_request(int width, int height) override;
    void size_request(Gtk::Requisition &requisition) override;
    void get_position(int &x, int &y) override;
    void get_size(int& width, int &height) override;
    void set_title(Glib::ustring title) override;
    void set_sensitive(bool sensitive) override;

    /** Gtk::Dialog signal proxies */
    Glib::SignalProxy0<void> signal_show() override;
    Glib::SignalProxy0<void> signal_hide() override;
    Glib::SignalProxy1<bool, GdkEventAny *> signal_delete_event() override;

    /** Custom signal handlers */
    void onHideF12() override;
    void onShowF12() override;
    void onDesktopActivated(SPDesktop *desktop) override;
    void onShutdown() override;

private:
    FloatingBehavior(Dialog& dialog);

    Gtk::Dialog *_d;   //< the actual dialog

    Glib::PropertyProxy_ReadOnly<bool> _dialog_active;  //< Variable proxy to track whether the dialog is the active window
    float _trans_focus;  //< The percentage opacity when the dialog is focused
    float _trans_blur;   //< The percentage opactiy when the dialog is not focused
    int _trans_time;     //< The amount of time (in ms) for the dialog to change it's transparency
};

} // namespace Behavior
} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_DIALOG_FLOATING_BEHAVIOR_H

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

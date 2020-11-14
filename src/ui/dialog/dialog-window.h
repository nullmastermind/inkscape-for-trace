// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INK_WINDOW_DOCK_H
#define INK_WINDOW_DOCK_H

/** @file
 * @brief A window for floating docks.
 *
 * Authors: see git history
 *   Tavmjong Bah
 *
 * Copyright (c) 2018 Tavmjong Bah, Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/applicationwindow.h>

#include "inkscape-application.h"

class SPDesktop;
using Gtk::Label;
using Gtk::Widget;

namespace Inkscape {
namespace UI {
namespace Dialog {

class DialogContainer;
class DialogMultipaned;

/**
 * DialogWindow holds DialogContainer instances for undocked dialogs.
 *
 * It watches the last active InkscapeWindow and updates its inner dialogs, if any.
 */
class DialogWindow : public Gtk::ApplicationWindow
{
public:
    DialogWindow(Gtk::Widget *page = nullptr);
    ~DialogWindow() override{};

    void update_dialogs();
    void on_close();
    void on_new_dialog(Glib::ustring value);

    // Getters
    Gtk::Label *get_label() { return _label; }
    DialogContainer *get_container() { return _container; }

private:
    ConcreteInkscapeApplication<Gtk::Application> *_app;
    Gtk::Label *_label;
    DialogContainer *_container;
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INK_WINDOW_DOCK_H

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

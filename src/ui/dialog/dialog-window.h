// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INKSCAPE_UI_DIALOG_WINDOW_H
#define INKSCAPE_UI_DIALOG_WINDOW_H

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
    void update_window_size_to_fit_children();

    // Getters
    DialogContainer *get_container() { return _container; }

private:
    InkscapeApplication *_app;
    DialogContainer *_container;
    Glib::ustring _title;
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_DIALOG_WINDOW_H

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

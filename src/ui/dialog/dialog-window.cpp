// SPDX-License-Identifier: GPL-2.0-or-later

/** @file
 * @brief A window for floating dialogs.
 *
 * Authors: see git history
 *   Tavmjong Bah
 *
 * Copyright (c) 2018 Tavmjong Bah, Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/dialog/dialog-window.h"

#include <glibmm/i18n.h>
#include <gtkmm/application.h>
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <iostream>

#include "dialog-container.h"
#include "dialog-multipaned.h"
#include "dialog-notebook.h"
#include "inkscape-application.h"
#include "preferences.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

class DialogNotebook;
class DialogContainer;

// Create a dialog window and move page from old notebook.
DialogWindow::DialogWindow(Gtk::Widget *page)
    : Gtk::ApplicationWindow()
    , _app(&ConcreteInkscapeApplication<Gtk::Application>::get_instance())
{
    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG); // Make DialogWindow stay above InkscapeWindow
    if (!_app) {
        std::cerr << "DialogWindow::DialogWindow(): _app is null" << std::endl;
        return;
    }
    _app->add_window(*this);

    // ============ Theming: icons ==============

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    if (prefs->getBool("/theme/symbolicIcons", false)) {
        get_style_context()->add_class("symbolic");
        get_style_context()->remove_class("regular");
    } else {
        get_style_context()->add_class("regular");
        get_style_context()->remove_class("symbolic");
    }

    // ========================  Actions ==========================

    add_action_radio_string("new_dialog", sigc::mem_fun(*this, &DialogWindow::on_new_dialog), "Preferences");
    add_action("close", sigc::mem_fun(*this, &DialogWindow::close));

    // ========================= Widgets ==========================

    // ================ Window ==================
    static int i = 0;
    set_title(_("Dialog Window ") + Glib::ustring::format(++i));
    set_name(_("Dialog Window ") + Glib::ustring::format(i));
    set_default_size(300, 400);

    // =============== Outer Box ================
    Gtk::Box *box_outer = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    box_outer->set_name("Outer Box");
    add(*box_outer);

    // ================ Label ===================
    _label = Gtk::manage(new Gtk::Label(_("Dialog Window")));
    box_outer->pack_start(*_label, false, false);

    // =============== Container ================
    _container = Gtk::manage(new DialogContainer());
    box_outer->pack_end(*_container);

    // ============= Initial Column =============
    DialogMultipaned *column = _container->create_column();
    _container->get_columns()->append(column);

    // ============== New Notebook ==============
    DialogNotebook *ink_notebook = Gtk::manage(new DialogNotebook(_container));
    column->append(ink_notebook);
    ink_notebook->move_page(*page);
}

/**
 * Update all dialogs that are owned by the DialogWindow's _container.
 */
void DialogWindow::update_dialogs()
{
    _container->update_dialogs(); // Updating dialogs is not using the _app reference here.

    if (!_app) {
        std::cerr << "DialogWindow::update_dialogs(): _app is null" << std::endl;
        return;
    }
    _label->set_text(_app->get_active_document()->getDocumentName());
}

// =====================  Callbacks ======================

/**
 * Close callback.
 * Can't override window->close()
 */
void DialogWindow::on_close()
{
    // Check if saved.
    delete this;
}

/**
 * Callback on adding new dialog. Unused.
 */
void DialogWindow::on_new_dialog(Glib::ustring value) {}

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

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

#include "enums.h"
#include "inkscape-application.h"
#include "preferences.h"
#include "ui/dialog/dialog-base.h"
#include "ui/dialog/dialog-container.h"
#include "ui/dialog/dialog-multipaned.h"
#include "ui/dialog/dialog-notebook.h"

// Sizing constants
const int MINIMUM_WINDOW_WIDTH = 210;
const int MINIMUM_WINDOW_HEIGHT = 320;
const int INITIAL_WINDOW_WIDTH = 360;
const int INITIAL_WINDOW_HEIGHT = 520;
const int WINDOW_DROPZONE_SIZE = 10;
const int WINDOW_DROPZONE_SIZE_LARGE = 16;
const int NOTEBOOK_TAB_HEIGHT = 36;

namespace Inkscape {
namespace UI {
namespace Dialog {

class DialogNotebook;
class DialogContainer;

// Create a dialog window and move page from old notebook.
DialogWindow::DialogWindow(Gtk::Widget *page)
    : Gtk::ApplicationWindow()
    , _app(InkscapeApplication::instance())
    , _title(_("Dialog Window"))
{
    // ============ Intialization ===============
    // Setting the window type
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool window_above = true;
    if (prefs) {
        window_above =
            prefs->getInt("/options/transientpolicy/value", PREFS_DIALOGS_WINDOWS_NORMAL) != PREFS_DIALOGS_WINDOWS_NONE;
    }

    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

    if (page && window_above) {
        if (auto top_win = dynamic_cast<Gtk::Window*>(page->get_toplevel())) {
            set_transient_for(*top_win);
        }
    }

    // Add the window to our app
    if (!_app) {
        std::cerr << "DialogWindow::DialogWindow(): _app is null" << std::endl;
        return;
    }
    _app->gtk_app()->add_window(*this);

    // ============ Theming: icons ==============

    if (prefs->getBool("/theme/symbolicIcons", false)) {
        get_style_context()->add_class("symbolic");
        get_style_context()->remove_class("regular");
    } else {
        get_style_context()->add_class("regular");
        get_style_context()->remove_class("symbolic");
    }

    // ================ Window ==================
    set_title(_title);
    set_name(_title);
    int window_width = INITIAL_WINDOW_WIDTH;
    int window_height = INITIAL_WINDOW_HEIGHT;

    // =============== Outer Box ================
    Gtk::Box *box_outer = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    add(*box_outer);

    // =============== Container ================
    _container = Gtk::manage(new DialogContainer());
    DialogMultipaned *columns = _container->get_columns();
    columns->set_dropzone_sizes(WINDOW_DROPZONE_SIZE, WINDOW_DROPZONE_SIZE);
    box_outer->pack_end(*_container);

    // If there is no page, create an empty Dialogwindow to be populated later
    if (page) {
        // ============= Initial Column =============
        DialogMultipaned *column = _container->create_column();
        columns->append(column);

        // ============== New Notebook ==============
        DialogNotebook *dialog_notebook = Gtk::manage(new DialogNotebook(_container));
        column->append(dialog_notebook);
        column->set_dropzone_sizes(WINDOW_DROPZONE_SIZE, WINDOW_DROPZONE_SIZE);
        dialog_notebook->move_page(*page);

        // Set window title
        DialogBase *dialog = dynamic_cast<DialogBase *>(page);
        if (dialog) {
            _title = dialog->get_name();
            set_title(_title);
        }

        // Set window size considering what the dialog needs
        Gtk::Requisition minimum_size, natural_size;
        dialog->get_preferred_size(minimum_size, natural_size);
        int overhead = 2 * (WINDOW_DROPZONE_SIZE + dialog->property_margin().get_value());
        int width = natural_size.width + overhead;
        int height = natural_size.height + overhead + NOTEBOOK_TAB_HEIGHT;
        window_width = std::max(width, window_width);
        window_height = std::max(height, window_height);
    }

    // Set window sizing
    set_size_request(MINIMUM_WINDOW_WIDTH, MINIMUM_WINDOW_HEIGHT);
    set_default_size(window_width, window_height);

    if (page) {
        update_dialogs();
    }

    show();
    show_all();
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

    if (_container) {
        const std::multimap<int, DialogBase *> *dialogs = _container->get_dialogs();
        if (dialogs->size() > 1) {
            _title = "Multiple dialogs";
        } else if (dialogs->size() == 1) {
            _title = dialogs->begin()->second->get_name();
        }
    }

    if (_app->get_active_document()) {
        auto document_name = _app->get_active_document()->getDocumentName();
        if (document_name) {
            set_title(_title + " - " + Glib::ustring(document_name));
        }
    }
}

/**
 * Update window width and height in order to fit all dialogs inisde its container.
 *
 * The intended use of this function is at initialization.
 */
void DialogWindow::update_window_size_to_fit_children()
{
    // Declare variables
    int pos_x = 0, pos_y = 0;
    int width = 0, height = 0;
    int overhead = 0, baseline;
    Gtk::Allocation allocation;
    Gtk::Requisition minimum_size, natural_size;

    // Read needed data
    get_position(pos_x, pos_y);
    get_allocated_size(allocation, baseline);
    const std::multimap<int, DialogBase *> *dialogs = _container->get_dialogs();

    // Get largest sizes for dialogs
    for (auto dialog : *dialogs) {
        dialog.second->get_preferred_size(minimum_size, natural_size);
        width = std::max(natural_size.width, width);
        height = std::max(natural_size.height, height);
        overhead = std::max(overhead, dialog.second->property_margin().get_value());
    }

    // Compute sizes including overhead
    overhead = 2 * (WINDOW_DROPZONE_SIZE_LARGE + overhead);
    width = width + overhead;
    height = height + overhead + NOTEBOOK_TAB_HEIGHT;

    // If sizes are lower then current, don't change them
    if (allocation.get_width() >= width && allocation.get_height() >= height) {
        return;
    }

    // Compute largest sizes on both axis
    width = std::max(width, allocation.get_width());
    height = std::max(height, allocation.get_height());

    // Compute new positions to keep window centered
    pos_x = pos_x - (width - allocation.get_width()) / 2;
    pos_y = pos_y - (height - allocation.get_height()) / 2;

    // Keep window inside the screen
    pos_x = std::max(pos_x, 0);
    pos_y = std::max(pos_y, 0);

    // Resize window
    move(pos_x, pos_y);
    resize(width, height);
}

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

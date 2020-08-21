// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INKSCAPE_UI_DIALOG_CONTAINER_H
#define INKSCAPE_UI_DIALOG_CONTAINER_H

/** @file
 * @brief A widget that manages DialogNotebook's and other widgets inside a horizontal DialogMultipaned.
 *
 * Authors: see git history
 *   Tavmjong Bah
 *
 * Copyright (c) 2018 Tavmjong Bah, Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gdkmm/dragcontext.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/accelkey.h>
#include <gtkmm/box.h>
#include <gtkmm/targetentry.h>
#include <iostream>

class SPDesktop;

namespace Inkscape {
namespace UI {
namespace Dialog {

class DialogBase;
class DialogNotebook;
class DialogMultipaned;

/**
 * A widget that manages DialogNotebook's and other widgets inside a
 * horizontal DialogMultipaned containing vertical DialogMultipaned's or other widgets.
 */
class DialogContainer : public Gtk::Box
{
public:
    DialogContainer();

    // Columns-related functions
    DialogMultipaned *get_columns() { return columns; }
    DialogMultipaned *create_column();

    // Dialog-related functions
    void new_dialog(unsigned int code);
    void new_dialog(unsigned int code, DialogNotebook *notebook);
    void new_floating_dialog(unsigned int code);
    bool has_dialog_of_type(DialogBase *dialog);
    DialogBase *get_dialog(unsigned int code);
    void link_dialog(DialogBase *dialog);
    void unlink_dialog(DialogBase *dialog);
    const std::multimap<int, DialogBase *> *get_dialogs() { return &dialogs; };
    void toggle_dialogs();
    void update_dialogs(); // Update all linked dialogs

    // State saving functionality
    void save_container_state();
    void load_container_state();
    void load_container_state(Glib::ustring filename);

private:
    DialogMultipaned *columns;                    // The main widget inside which other children are kept.
    std::vector<Gtk::TargetEntry> target_entries; // What kind of object can be dropped.

    /**
     * Due to the way Gtk handles dragging between notebooks, one can
     * either allow multiple instances of the same dialog in a notebook
     * or restrict dialogs to docks tied to a particular document
     * window. (More explicitly, use one group name for all notebooks or
     * use a unique group name for each document window with related
     * floating docks.) For the moment we choose the former which
     * requires a multimap here as we use the dialog verb code as a key.
     */
    std::multimap<int, DialogBase *> dialogs;

    DialogBase *dialog_factory(unsigned int code);
    Gtk::Widget *create_notebook_tab(Glib::ustring label, Glib::ustring image, Gtk::AccelKey shortcut);

    // Signal connections
    std::vector<sigc::connection> connections;

    // Handlers
    void on_unmap() override;
    DialogNotebook *prepare_drop(const Glib::RefPtr<Gdk::DragContext> context);
    void prepend_drop(const Glib::RefPtr<Gdk::DragContext> context, DialogMultipaned *column);
    void append_drop(const Glib::RefPtr<Gdk::DragContext> context, DialogMultipaned *column);
    void column_empty(DialogMultipaned *column);
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_DIALOG_CONTAINER_H

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

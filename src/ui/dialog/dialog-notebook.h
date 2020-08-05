// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INKSCAPE_UI_DIALOG_NOTEBOOK_H
#define INKSCAPE_UI_DIALOG_NOTEBOOK_H

/** @file
 * @brief A wrapper for Gtk::Notebook.
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
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/notebook.h>
#include <gtkmm/widget.h>

namespace Inkscape {
namespace UI {
namespace Dialog {

class DialogContainer;

/**
 * A widget that wraps a Gtk::Notebook with dialogs as pages.
 *
 * A notebook is fixed to a specific DialogContainer which manages the dialogs inside the notebook.
 *
 * The notebook is inside a Gtk::Expander to be able to be hidden easily.
 */
class DialogNotebook : public Gtk::ScrolledWindow
{
public:
    DialogNotebook(DialogContainer *container);
    ~DialogNotebook() override{};

    void add_page(Gtk::Widget &page, Gtk::Widget &tab, Glib::ustring label);
    void move_page(Gtk::Widget &page);

    // Getters
    Gtk::Notebook *get_notebook() { return &_notebook; }
    DialogContainer *get_container() { return _container; }

    // Signal handlers - Notebook
    void on_drag_end(const Glib::RefPtr<Gdk::DragContext> context);
    bool on_drag_failed(const Glib::RefPtr<Gdk::DragContext> context, Gtk::DragResult result);

    void on_page_added(Gtk::Widget *page, int page_num);
    void on_page_removed(Gtk::Widget *page, int page_num);
    void on_page_switched(Gtk::Widget *page, int page_num);

    // Signal handlers - Notebook menu (action)
    void close_tab_callback();
    void hide_tab_label_callback();
    void show_tab_label_callback();
    void hide_all_tab_labels_callback();
    void show_all_tab_labels_callback();
    void move_tab_callback();
    void close_notebook_callback();

private:
    void open_dialog_from_notebook(Glib::ustring);
    void toggle_tab_labels_callback();
    void handle_scrolling(Gtk::Allocation &allocation);

    DialogContainer *_container;
    Gtk::MenuButton _menu_button;
    Gtk::Menu _menu;
    Gtk::MenuItem *_toggle_all_labels_menuitem;
    int _dialog_menu_items;
    bool _labels_shown;
    Gtk::Notebook _notebook;
    std::vector<sigc::connection> _scrolling_connections;
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_DIALOG_NOTEBOOK_H

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

// SPDX-License-Identifier: GPL-2.0-or-later

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

#include "dialog-notebook.h"

#include <glibmm/i18n.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/separatormenuitem.h>

#include "enums.h"
#include "ui/dialog/dialog-base.h"
#include "ui/dialog/dialog-container.h"
#include "ui/dialog/dialog-multipaned.h"
#include "ui/dialog/dialog-window.h"
#include "verbs.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

/**
 * DialogNotebook constructor.
 *
 * @param container the parent DialogContainer of the notebook.
 */
DialogNotebook::DialogNotebook(DialogContainer *container)
    : Gtk::ScrolledWindow()
    , _container(container)
    , _labels_auto(true)
    , _detaching_duplicate(false)
    , _selected_page(nullptr)
{
    set_name("DialogNotebook");
    set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    set_shadow_type(Gtk::SHADOW_NONE);
    set_vexpand(true);
    set_hexpand(true);

    // =========== Getting preferences ==========
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs == nullptr) {
        return;
    }

    _labels_auto =
        prefs->getInt("/options/notebooklabels/value", PREFS_NOTEBOOK_LABELS_AUTO) == PREFS_NOTEBOOK_LABELS_AUTO;

    // ============= Notebook menu ==============
    _menu.set_title("NotebookOptions");
    _notebook.set_group_name("InkscapeDialogGroup");
    _notebook.set_scrollable(true);

    Gtk::MenuItem *new_menu_item = nullptr;

    // Close tab
    new_menu_item = Gtk::manage(new Gtk::MenuItem(_("Close Tab")));
    _conn.emplace_back(
        new_menu_item->signal_activate().connect(sigc::mem_fun(*this, &DialogNotebook::close_tab_callback)));
    _menu.append(*new_menu_item);

    // Close notebook
    new_menu_item = Gtk::manage(new Gtk::MenuItem(_("Close Notebook")));
    _conn.emplace_back(
        new_menu_item->signal_activate().connect(sigc::mem_fun(*this, &DialogNotebook::close_notebook_callback)));
    _menu.append(*new_menu_item);

    // Move to new window
    new_menu_item = Gtk::manage(new Gtk::MenuItem(_("Move Tab to New Window")));
    _conn.emplace_back(
        new_menu_item->signal_activate().connect(sigc::mem_fun(*this, &DialogNotebook::pop_tab_callback)));
    _menu.append(*new_menu_item);

    // Separator menu item
    new_menu_item = Gtk::manage(new Gtk::SeparatorMenuItem());
    _menu.append(*new_menu_item);

    // Labels radio menu
    _labels_auto_button.set_label(_("Labels: automatic"));
    _menu.append(_labels_auto_button);

    Gtk::RadioMenuItem *labels_off_button = Gtk::manage(new Gtk::RadioMenuItem(_("Labels: always off")));
    _menu.append(*labels_off_button);
    labels_off_button->join_group(_labels_auto_button);

    _labels_auto ? _labels_auto_button.set_active() : labels_off_button->set_active();
    _conn.emplace_back(
        _labels_auto_button.signal_toggled().connect(sigc::mem_fun(*this, &DialogNotebook::on_labels_toggled)));

    _menu.show_all_children();

    // =============== Signals ==================
    _conn.emplace_back(signal_size_allocate().connect(sigc::mem_fun(*this, &DialogNotebook::on_size_allocate_scroll)));
    _conn.emplace_back(_notebook.signal_drag_end().connect(sigc::mem_fun(*this, &DialogNotebook::on_drag_end)));
    _conn.emplace_back(_notebook.signal_page_added().connect(sigc::mem_fun(*this, &DialogNotebook::on_page_added)));
    _conn.emplace_back(_notebook.signal_page_removed().connect(sigc::mem_fun(*this, &DialogNotebook::on_page_removed)));

    // ============= Finish setup ===============
    add(_notebook);
    show_all();
}

DialogNotebook::~DialogNotebook()
{
    // Unlink and remove pages
    for (int i = _notebook.get_n_pages(); i >= 0; --i) {
        DialogBase *dialog = dynamic_cast<DialogBase *>(_notebook.get_nth_page(i));
        _container->unlink_dialog(dialog);
        _notebook.remove_page(i);
    }

    for_each(_conn.begin(), _conn.end(), [&](auto c) { c.disconnect(); });
    for_each(_tab_connections.begin(), _tab_connections.end(), [&](auto it) { it.second.disconnect(); });

    _conn.clear();
    _tab_connections.clear();
}

/**
 * Adds a widget as a new page with a tab.
 */
void DialogNotebook::add_page(Gtk::Widget &page, Gtk::Widget &tab, Glib::ustring label)
{
    page.set_vexpand();

    int page_number = _notebook.append_page(page, tab);
    _notebook.set_tab_reorderable(page);
    _notebook.set_tab_detachable(page);
    _notebook.show_all();
    _notebook.set_current_page(page_number);
}

/**
 * Moves a page from a different notebook to this one.
 */
void DialogNotebook::move_page(Gtk::Widget &page)
{
    // Find old notebook
    Gtk::Notebook *old_notebook = dynamic_cast<Gtk::Notebook *>(page.get_parent());
    if (!old_notebook) {
        std::cerr << "DialogNotebook::move_page: page not in notebook!" << std::endl;
        return;
    }

    Gtk::Widget *tab = old_notebook->get_tab_label(page);
    Glib::ustring text = old_notebook->get_menu_label_text(page);

    // Keep references until re-attachment
    tab->reference();
    page.reference();

    old_notebook->detach_tab(page);
    _notebook.append_page(page, *tab);
    // Remove unnecessary references
    tab->unreference();
    page.unreference();

    // Set default settings for a new page
    _notebook.set_tab_reorderable(page);
    _notebook.set_tab_detachable(page);
    _notebook.show_all();
}

// ============ Notebook callbacks ==============

/**
 * Callback to close the current active tab.
 */
void DialogNotebook::close_tab_callback()
{
    int page_number = _notebook.get_current_page();

    if (_selected_page) {
        page_number = _notebook.page_num(*_selected_page);
        _selected_page = nullptr;
    }

    // Remove page from notebook
    _notebook.remove_page(page_number);

    // Delete the signal connection
    remove_close_tab_callback(_selected_page);

    if (_notebook.get_n_pages() == 0) {
        close_notebook_callback();
        return;
    }

    // Update tab labels by comparing the sum of their widths to the allocation
    Gtk::Allocation allocation = get_allocation();
    on_size_allocate_scroll(allocation);
}

/**
 * Shutdown callback - delete the parent DialogMultipaned before destructing.
 */
void DialogNotebook::close_notebook_callback()
{
    // Search for DialogMultipaned
    DialogMultipaned *multipaned = dynamic_cast<DialogMultipaned *>(get_parent());
    if (multipaned) {
        multipaned->remove(*this);
    } else if (get_parent()) {
        std::cerr << "DialogNotebook::close_notebook_callback: Unexpected parent!" << std::endl;
        get_parent()->remove(*this);
    }
    delete this;
}

/**
 * Callback to move the current active tab.
 */
void DialogNotebook::pop_tab_callback()
{
    // Find page.
    Gtk::Widget *page = _notebook.get_nth_page(_notebook.get_current_page());

    if (_selected_page) {
        page = _selected_page;
        _selected_page = nullptr;
    }

    if (!page) {
        std::cerr << "DialogNotebook::pop_tab_callback: page not found!" << std::endl;
        return;
    }

    // Move page to notebook in new dialog window
    auto window = new DialogWindow(page);
    window->show_all();

    if (_notebook.get_n_pages() == 0) {
        close_notebook_callback();
        return;
    }

    // Update tab labels by comparing the sum of their widths to the allocation
    Gtk::Allocation allocation = get_allocation();
    on_size_allocate_scroll(allocation);
}

// ========= Signal handlers - notebook =========

/**
 * Signal handler to pop a dragged tab into its own DialogWindow.
 *
 * A failed drag means that the page was not dropped on an existing notebook.
 * Thus create a new window with notebook to move page to.
 *
 * BUG: this has inconsistent behavior on Wayland.
 */
void DialogNotebook::on_drag_end(const Glib::RefPtr<Gdk::DragContext> context)
{
    bool set_floating = !context->get_dest_window();
    if (!set_floating && context->get_dest_window()->get_window_type() == Gdk::WINDOW_FOREIGN) {
        set_floating = true;
    }

    if (set_floating) {
        Gtk::Widget *source = Gtk::Widget::drag_get_source_widget(context);

        // Find source notebook and page
        Gtk::Notebook *old_notebook = dynamic_cast<Gtk::Notebook *>(source);
        if (!old_notebook) {
            std::cerr << "DialogNotebook::on_drag_end: notebook not found!" << std::endl;
        } else {
            // Find page
            Gtk::Widget *page = old_notebook->get_nth_page(old_notebook->get_current_page());
            if (page) {
                // Move page to notebook in new dialog window
                auto window = new DialogWindow(page);

                // Move window to mouse pointer
                if (auto device = context->get_device()) {
                    int x = 0, y = 0;
                    device->get_position(x, y);
                    window->move(std::max(0, x - 50), std::max(0, y - 50));
                }

                window->show_all();
            }
        }
    }

    // Closes the notebook if empty.
    if (_notebook.get_n_pages() == 0) {
        close_notebook_callback();
        return;
    }

    // Update tab labels by comparing the sum of their widths to the allocation
    Gtk::Allocation allocation = get_allocation();
    on_size_allocate_scroll(allocation);
}

/**
 * Signal handler to update dialog list when adding a page.
 */
void DialogNotebook::on_page_added(Gtk::Widget *page, int page_num)
{
    DialogBase *dialog = dynamic_cast<DialogBase *>(page);

    // Does current container/window already have such a dialog?
    if (dialog && _container->has_dialog_of_type(dialog)) {
        // We already have a dialog of the same type

        // Highlight first dialog
        DialogBase *other_dialog = _container->get_dialog(dialog->getVerb());
        other_dialog->blink();

        // Remove page from notebook
        _detaching_duplicate = true; // HACK: prevent removing the initial dialog of the same type
        _notebook.detach_tab(*page);
        return;
    } else if (dialog) {
        // We don't have a dialog of this type

        // Add to dialog list
        _container->link_dialog(dialog);
    } else {
        // This is not a dialog
        return;
    }

    // add close tab signal
    add_close_tab_callback(page);

    // Switch tab labels if needed
    if (!_labels_auto) {
        toggle_tab_labels_callback(false);
    }

    // Update tab labels by comparing the sum of their widths to the allocation
    Gtk::Allocation allocation = get_allocation();
    on_size_allocate_scroll(allocation);
}

/**
 * Signal handler to update dialog list when removing a page.
 */
void DialogNotebook::on_page_removed(Gtk::Widget *page, int page_num)
{
    /**
     * When adding a dialog in a notebooks header zone of the same type as an existing one,
     * we remove it immediately, which triggers a call to this method. We use `_detaching_duplicate`
     * to prevent reemoving the initial dialog.
     */
    if (_detaching_duplicate) {
        _detaching_duplicate = false;
        return;
    }

    // Remove from dialog list
    DialogBase *dialog = dynamic_cast<DialogBase *>(page);
    if (dialog) {
        _container->unlink_dialog(dialog);
    }

    // remove old close tab signal
    remove_close_tab_callback(page);
}

/**
 * We need to remove the scrollbar to snap a whole DialogNotebook to width 0.
 *
 * This function also hides the tab labels if necessary (and _labels_auto == true)
 */
void DialogNotebook::on_size_allocate_scroll(Gtk::Allocation &a)
{
    // magic numbers
    const int MIN_HEIGHT = 60;
    const int ICON_SIZE = 50;

    // set or unset scrollbars to completely hide a notebook
    property_vscrollbar_policy().set_value(a.get_height() >= MIN_HEIGHT ? Gtk::POLICY_AUTOMATIC : Gtk::POLICY_EXTERNAL);

    set_allocation(a);

    if (!_labels_auto) {
        return;
    }

    // hide tab labels under a size
    int size = 0;
    int min_width, nat_width;

    for (auto const &page : _notebook.get_children()) {
        Gtk::EventBox *cover = dynamic_cast<Gtk::EventBox *>(_notebook.get_tab_label(*page));
        if (!cover) {
            continue;
        }

        Gtk::Box *box = dynamic_cast<Gtk::Box *>(cover->get_child());
        if (!box) {
            continue;
        }

        Gtk::Label *label = dynamic_cast<Gtk::Label *>(box->get_children()[1]);

        if (label) {
            label->show();
            label->get_preferred_width(min_width, nat_width);
            size += (ICON_SIZE + min_width);
        }
    }

    (a.get_width() < size) ? toggle_tab_labels_callback(false) : toggle_tab_labels_callback(true);
}

/**
 * Signal handler to toggle the tab labels internal state.
 */
void DialogNotebook::on_labels_toggled() {
    bool previous = _labels_auto;
    _labels_auto = _labels_auto_button.get_active();

    if (previous && !_labels_auto) {
        toggle_tab_labels_callback(false);
    } else if (!previous && _labels_auto) {
        toggle_tab_labels_callback(true);
    }
}

/**
 * Signal handler to close a tab when middle-clicking.
 */
bool DialogNotebook::on_tab_click_event(GdkEventButton *event, Gtk::Widget *page)
{
    if (event->type == GDK_BUTTON_PRESS) {
        if (event->button == 2) { // Close tab
            _selected_page = page;
            close_tab_callback();
        } else if (event->button == 3) { // Show menu
            _selected_page = page;
            _menu.popup_at_pointer((GdkEvent *)event);
        }
    }

    return false;
}

// ================== Helpers ===================

/**
 * Callback to toggle all tab labels to the selected state.
 * @param show: wether you want the labels to show or not
 */
void DialogNotebook::toggle_tab_labels_callback(bool show)
{
    for (auto const &page : _notebook.get_children()) {
        Gtk::EventBox *cover = dynamic_cast<Gtk::EventBox *>(_notebook.get_tab_label(*page));
        if (!cover) {
            continue;
        }

        Gtk::Box *box = dynamic_cast<Gtk::Box *>(cover->get_child());
        if (!box) {
            continue;
        }

        Gtk::Label *label = dynamic_cast<Gtk::Label *>(box->get_children()[1]);
        if (label) {
            show ? label->show() : label->hide();
        }
    }
}

/**
 * Helper method that adds the close tab signal connection for the page given.
 */
void DialogNotebook::add_close_tab_callback(Gtk::Widget *page)
{
    Gtk::Widget *tab = _notebook.get_tab_label(*page);
    sigc::connection tab_connection = tab->signal_button_press_event().connect(
        sigc::bind<Gtk::Widget *>(sigc::mem_fun(*this, &DialogNotebook::on_tab_click_event), page), true);

    _tab_connections.insert(std::pair<Gtk::Widget *, sigc::connection>(page, tab_connection));
}

/**
 * Helper method that removes the close tab signal connection for the page given.
 */
void DialogNotebook::remove_close_tab_callback(Gtk::Widget *page)
{
    auto tab_connection_it = _tab_connections.find(page);

    if (tab_connection_it != _tab_connections.end()) {
        (*tab_connection_it).second.disconnect();
        _tab_connections.erase(tab_connection_it);
    }
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

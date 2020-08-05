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
#include <gtkmm/separatormenuitem.h>
#include <iostream>

#include "dialog-base.h"
#include "dialog-container.h"
#include "dialog-multipaned.h"
#include "src/helper/action-context.h"
#include "src/helper/action.h"
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
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL)
    , _container(container)
    , _dialog_menu_items(0)
{
    set_name("DialogNotebook");
    set_vexpand();

    // ============= Notebook menu ==============
    _action_menu.set_title("NotebookOptions");

    Gtk::MenuItem *new_menu_item = nullptr;

    // Close tab
    new_menu_item = Gtk::manage(new Gtk::MenuItem(_("Close Tab")));
    new_menu_item->signal_activate().connect(sigc::mem_fun(*this, &DialogNotebook::close_tab_callback));
    _action_menu.append(*new_menu_item);

    // Hide tab label
    new_menu_item = Gtk::manage(new Gtk::MenuItem(_("Hide Tab Label")));
    new_menu_item->signal_activate().connect(sigc::mem_fun(*this, &DialogNotebook::hide_tab_label_callback));
    _action_menu.append(*new_menu_item);

    // Show tab label
    new_menu_item = Gtk::manage(new Gtk::MenuItem(_("Show Tab Label")));
    new_menu_item->signal_activate().connect(sigc::mem_fun(*this, &DialogNotebook::show_tab_label_callback));
    _action_menu.append(*new_menu_item);

    // Hide all tab labels
    new_menu_item = Gtk::manage(new Gtk::MenuItem(_("Hide All Tab Labels")));
    new_menu_item->signal_activate().connect(sigc::mem_fun(*this, &DialogNotebook::hide_all_tab_labels_callback));
    _action_menu.append(*new_menu_item);

    // Show all tab labels
    new_menu_item = Gtk::manage(new Gtk::MenuItem(_("Show All Tab Labels")));
    new_menu_item->signal_activate().connect(sigc::mem_fun(*this, &DialogNotebook::show_all_tab_labels_callback));
    _action_menu.append(*new_menu_item);

    // Move to new window
    new_menu_item = Gtk::manage(new Gtk::MenuItem(_("Move Tab to New Window")));
    new_menu_item->signal_activate().connect(sigc::mem_fun(*this, &DialogNotebook::move_tab_callback));
    _action_menu.append(*new_menu_item);

    // Close notebook
    new_menu_item = Gtk::manage(new Gtk::MenuItem(_("Close Notebook")));
    new_menu_item->signal_activate().connect(sigc::mem_fun(*this, &DialogNotebook::close_notebook_callback));
    _action_menu.append(*new_menu_item);

    _action_menu.show_all_children();

    // ============ Notebook setup ==============
    _notebook.set_group_name("InkscapeDialogGroup"); // Could be param.
    _notebook.popup_enable();                        // Doesn't hurt.

    // Add notebook action menu
    _action_button.set_image_from_icon_name("pan-start-symbolic");
    _action_button.set_popup(_action_menu);
    _action_button.show(); // show_all() below doesn't show this.
    _notebook.set_action_widget(&_action_button, Gtk::PACK_END);

    _notebook.signal_drag_end().connect(sigc::mem_fun(*this, &DialogNotebook::on_drag_end));
    _notebook.signal_drag_failed().connect(sigc::mem_fun(*this, &DialogNotebook::on_drag_failed));

    _notebook.signal_page_added().connect(sigc::mem_fun(*this, &DialogNotebook::on_page_added));
    _notebook.signal_page_removed().connect(sigc::mem_fun(*this, &DialogNotebook::on_page_removed));

    _expander.property_expanded().signal_changed().connect(sigc::mem_fun(*this, &DialogNotebook::on_expandeder_resize));

    // ============= Finish setup ===============
    _expander.add(_notebook);
    _expander.set_expanded();
    add(_expander);
    show_all();
}

// Adds a widget as a new page with a tab.
void DialogNotebook::add_page(Gtk::Widget &page, Gtk::Widget &tab, Glib::ustring label)
{
    page.set_vexpand();

    _notebook.append_page(page, tab);
    _notebook.set_menu_label_text(page, label);
    _notebook.set_tab_reorderable(page);
    _notebook.set_tab_detachable(page);
    _notebook.show_all();

    // Switch notebook to new page.
    int page_number = _notebook.page_num(page);
    _notebook.set_current_page(page_number);
}

// Moves a page from a different notebook to this one.
void DialogNotebook::move_page(Gtk::Widget &page)
{
    // Find old notebook
    Gtk::Notebook *old_notebook = dynamic_cast<Gtk::Notebook *>(page.get_parent());
    if (!old_notebook) {
        std::cerr << "DialogNotebook::move_page: page not in notebook!" << std::endl;
        return;
    }

    Gtk::Widget *tab = old_notebook->get_tab_label(page);

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

// Signal handlers - Notebook

// Closes the notebook if empty.
void DialogNotebook::on_drag_end(const Glib::RefPtr<Gdk::DragContext> context)
{
    if (_notebook.get_n_pages() == 0) {
        close_notebook_callback();
    }
}

/**
 * A failed drag means that the page was not dropped on an existing notebook.
 * Thus create a new window with notebook to move page to.
 * WARNING: this only triggers in Wayland, not X11.
 */
bool DialogNotebook::on_drag_failed(const Glib::RefPtr<Gdk::DragContext> context, Gtk::DragResult result)
{
    Gtk::Widget *source = Gtk::Widget::drag_get_source_widget(context);

    // Find source notebook and page
    Gtk::Notebook *old_notebook = dynamic_cast<Gtk::Notebook *>(source);
    if (!old_notebook) {
        std::cerr << "DialogNotebook::on_drag_failed: notebook not found!" << std::endl;
        return false;
    }

    // Find page
    Gtk::Widget *page = old_notebook->get_nth_page(old_notebook->get_current_page());
    if (!page) {
        std::cerr << "DialogNotebook::on_drag_failed: page not found!" << std::endl;
        return false;
    }

    // Move page to notebook in new dialog window
    auto window = new DialogWindow(page);
    window->show_all();

    return true;
}

// Update dialog list on adding a page.
void DialogNotebook::on_page_added(Gtk::Widget *page, int page_num)
{
    // Add to dialog list
    DialogBase *dialog = dynamic_cast<DialogBase *>(page);
    if (dialog) {
        _container->link_dialog(dialog);
    }

    // Add notebook menu item
    Gtk::MenuItem *new_menu_item = nullptr;
    if (_dialog_menu_items == 0) {
        new_menu_item = Gtk::manage(new Gtk::SeparatorMenuItem());
        _action_menu.append(*new_menu_item);
    }

    new_menu_item = Gtk::manage(new Gtk::MenuItem(dialog->get_name()));

    // Add a signal in order to focus on the dialog from the notebook menu
    Verb *verb = Verb::get(dialog->getVerb());
    if (verb) {
        new_menu_item->signal_activate().connect(sigc::bind<DialogContainer *, Glib::ustring>(
            sigc::mem_fun(&DialogContainer::new_dialog), _container, Glib::ustring(verb->get_id())));
    }

    _action_menu.append(*new_menu_item);
    _action_menu.show_all_children();
    _dialog_menu_items++; // remember how many dialogs are in the notebook
}

// Update dialog list on removing a page.
void DialogNotebook::on_page_removed(Gtk::Widget *page, int page_num)
{
    // Remove from dialog list
    DialogBase *dialog = dynamic_cast<DialogBase *>(page);
    if (dialog) {
        _container->unlink_dialog(dialog);
    }

    // Remove the menu item for the removed page
    std::vector<Gtk::Widget *> actions = _action_menu.get_children();
    int n_children = (int)actions.size();
    Glib::ustring label = dialog->get_name();
    for (int i = n_children - 1; i >= 0; i--) {
        Gtk::MenuItem *menuitem = dynamic_cast<Gtk::MenuItem *>(actions[i]);
        if (menuitem && menuitem->get_label() == label) {
            _action_menu.remove(*menuitem);
            break;
        }
    }
    _dialog_menu_items--;

    // Remove extra separator menu item
    if (_dialog_menu_items == 0) {
        _action_menu.remove(*actions[n_children - 2]);
    }
}

// Signal handlers - Notebook menu (action)

void DialogNotebook::close_tab_callback()
{
    _notebook.remove_page(_notebook.get_current_page());

    if (_notebook.get_n_pages() == 0) {
        close_notebook_callback();
    }
}

void DialogNotebook::hide_tab_label_callback()
{
    // Find page and label.
    Gtk::Widget *page = _notebook.get_nth_page(_notebook.get_current_page());
    Gtk::Widget *tab = _notebook.get_tab_label(*page);

    Gtk::Box *box = dynamic_cast<Gtk::Box *>(tab);
    if (!box)
        return;

    std::vector<Gtk::Widget *> children = box->get_children();
    Gtk::Label *label = dynamic_cast<Gtk::Label *>(children[1]);
    if (label) {
        label->hide();
    }
}

void DialogNotebook::show_tab_label_callback()
{
    // Find page and label.
    Gtk::Widget *page = _notebook.get_nth_page(_notebook.get_current_page());
    Gtk::Widget *tab = _notebook.get_tab_label(*page);

    Gtk::Box *box = dynamic_cast<Gtk::Box *>(tab);
    if (!box)
        return;

    std::vector<Gtk::Widget *> children = box->get_children();
    Gtk::Label *label = dynamic_cast<Gtk::Label *>(children[1]);
    if (label) {
        label->show();
    }
}

void DialogNotebook::hide_all_tab_labels_callback()
{
    std::vector<Gtk::Widget *> pages = _notebook.get_children();

    for (auto page : pages) {
        Gtk::Widget *tab = _notebook.get_tab_label(*page);

        Gtk::Box *box = dynamic_cast<Gtk::Box *>(tab);
        if (!box)
            continue;

        std::vector<Gtk::Widget *> children = box->get_children();
        Gtk::Label *label = dynamic_cast<Gtk::Label *>(children[1]);
        if (label) {
            label->hide();
        }
    }
}

void DialogNotebook::show_all_tab_labels_callback()
{
    std::vector<Gtk::Widget *> pages = _notebook.get_children();

    for (auto page : pages) {
        Gtk::Widget *tab = _notebook.get_tab_label(*page);

        Gtk::Box *box = dynamic_cast<Gtk::Box *>(tab);
        if (!box)
            continue;

        std::vector<Gtk::Widget *> children = box->get_children();
        Gtk::Label *label = dynamic_cast<Gtk::Label *>(children[1]);
        if (label) {
            label->show();
        }
    }
}

void DialogNotebook::move_tab_callback()
{
    // Find page.
    Gtk::Widget *page = _notebook.get_nth_page(_notebook.get_current_page());
    if (!page) {
        std::cerr << "DialogNotebook::move_tab_callback: page not found!" << std::endl;
        return;
    }

    // Move page to notebook in new dialog window
    auto window = new DialogWindow(page);
    window->show_all();

    if (_notebook.get_n_pages() == 0) {
        close_notebook_callback();
    }
}

// Signal handlers - Other

/**
 * Shutdown callback - delete the parent DialogMultipaned before destructing.
 */
void DialogNotebook::close_notebook_callback()
{
    // Search for DialogMultipaned
    DialogMultipaned *multipaned = dynamic_cast<DialogMultipaned *>(get_parent());
    if (multipaned) {
        multipaned->remove(*this);
    } else {
        std::cerr << "DialogNotebook::close_notebook_callback: Unexpected parent!" << std::endl;
        get_parent()->remove(*this);
    }
    delete this;
}

void DialogNotebook::on_expandeder_resize()
{
    _expander.get_expanded() ? set_vexpand(true) : set_vexpand(false);
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

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "toolbar.h"

#include <gtkmm/label.h>
#include <gtkmm/separatortoolitem.h>
#include <gtkmm/toggletoolbutton.h>

#include "desktop.h"

#include "helper/action.h"

namespace Inkscape {
namespace UI {
namespace Toolbar {

Gtk::ToolItem *
Toolbar::add_label(const Glib::ustring &label_text)
{
    auto ti = Gtk::manage(new Gtk::ToolItem());

    // For now, we always enable mnemonic
    auto label = Gtk::manage(new Gtk::Label(label_text, true));

    ti->add(*label);
    add(*ti);

    return ti;
}

/**
 * \brief Add a toggle toolbutton to the toolbar
 *
 * \param[in] label_text   The text to display in the toolbar
 * \param[in] tooltip_text The tooltip text for the toolitem
 *
 * \returns The toggle button
 */
Gtk::ToggleToolButton *
Toolbar::add_toggle_button(const Glib::ustring &label_text,
                           const Glib::ustring &tooltip_text)
{
    auto btn = Gtk::manage(new Gtk::ToggleToolButton(label_text));
    btn->set_tooltip_text(tooltip_text);
    add(*btn);
    return btn;
}

/**
 * \brief Add a toolbutton that performs a given verb
 *
 * \param[in] verb_code The code for the verb (e.g., SP_VERB_EDIT_SELECT_ALL)
 *
 * \returns a pointer to the toolbutton
 */
Gtk::ToolButton *
Toolbar::add_toolbutton_for_verb(unsigned int verb_code)
{
    auto context = Inkscape::ActionContext(_desktop);
    auto button  = SPAction::create_toolbutton_for_verb(verb_code, context);
    add(*button);
    return button;
}

/**
 * \brief Add a separator line to the toolbar
 *
 * \details This is just a convenience wrapper for the
 *          standard GtkMM functionality
 */
void
Toolbar::add_separator()
{
    add(* Gtk::manage(new Gtk::SeparatorToolItem()));
}

GtkWidget *
Toolbar::create(SPDesktop *desktop)
{
    auto toolbar = Gtk::manage(new Toolbar(desktop));
    return GTK_WIDGET(toolbar->gobj());
}
}
}
}
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

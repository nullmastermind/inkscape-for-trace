// SPDX-License-Identifier: GPL-2.0-or-later

/** @file
 * @brief A base class for all dialogs.
 *
 * Authors: see git history
 *   Tavmjong Bah
 *
 * Copyright (c) 2018 Tavmjong Bah, Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "dialog-base.h"

#include <glibmm/i18n.h>
#include <glibmm/main.h>
#include <glibmm/refptr.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/notebook.h>
#include <iostream>

#include "desktop.h"
#include "ui/dialog/dialog-notebook.h"
#include "ui/dialog-events.h"
// get_latin_keyval
#include "ui/tools/tool-base.h"
#include "widgets/spw-utilities.h"
#include "ui/widget/canvas.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

/**
 * DialogBase constructor.
 *
 * @param prefs_path characteristic path to load/save dialog position.
 * @param verb_num the dialog verb.
 */
DialogBase::DialogBase(gchar const *prefs_path, int verb_num)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL)
    , _name("DialogBase")
    , _prefs_path(prefs_path)
    , _verb_num(verb_num)
    , _app(InkscapeApplication::instance())
{
    // Derive a pretty display name for the dialog based on the verbs name.
    // TODO: This seems fragile. Should verbs have a proper display name?
    Verb *verb = Verb::get(verb_num);
    if (verb) {
        // get translated verb name
        _name = _(verb->get_name());

        // remove ellipsis and mnemonics
        int pos = _name.find("...", 0);
        if (pos >= 0 && pos < _name.length() - 2) {
            _name.erase(pos, 3);
        }
        pos = _name.find("…", 0);
        if (pos >= 0 && pos < _name.length()) {
            _name.erase(pos, 1);
        }
        pos = _name.find("_", 0);
        if (pos >= 0 && pos < _name.length()) {
            _name.erase(pos, 1);
        }
    }

    set_name(_name); // Essential for dialog functionality
    property_margin().set_value(1); // Essential for dialog UI
}

bool DialogBase::on_key_press_event(GdkEventKey* key_event) {
    switch (Inkscape::UI::Tools::get_latin_keyval(key_event)) {
        case GDK_KEY_Escape:
            defocus_dialog();
            return true;
    }

    return parent_type::on_key_press_event(key_event);
}


/**
 * Highlight notebook where dialog already exists.
 */
void DialogBase::blink()
{
    Gtk::Notebook *notebook = dynamic_cast<Gtk::Notebook *>(get_parent());
    if (notebook && notebook->get_is_drawable()) {
        // Switch notebook to this dialog.
        notebook->set_current_page(notebook->page_num(*this));
        notebook->get_style_context()->add_class("blink");

        // Add timer to turn off blink.
        sigc::slot<bool> slot = sigc::mem_fun(*this, &DialogBase::blink_off);
        sigc::connection connection = Glib::signal_timeout().connect(slot, 1000); // msec
    }
}

void DialogBase::focus_dialog() {
    if (auto window = dynamic_cast<Gtk::Window*>(get_toplevel())) {
        window->present();
    }

    // widget that had focus, if any
    if (auto child = get_focus_child()) {
        child->grab_focus();
    }
    else {
        // find first focusable widget
        if (auto child = sp_find_focusable_widget(this)) {
            child->grab_focus();
        }
    }
}

void DialogBase::defocus_dialog() {
    if (auto wnd = dynamic_cast<Gtk::Window*>(get_toplevel())) {
        // defocus floating dialog:
        sp_dialog_defocus_cpp(wnd);

        // for docked dialogs, move focus to canvas
        if (auto desktop = getDesktop()) {
            desktop->getCanvas()->grab_focus();
        }
    }
}

/**
 * Callback to reset the dialog highlight.
 */
bool DialogBase::blink_off()
{
    Gtk::Notebook *notebook = dynamic_cast<Gtk::Notebook *>(get_parent());
    if (notebook && notebook->get_is_drawable()) {
        notebook->get_style_context()->remove_class("blink");
    }
    return false;
}

/**
 * Get the active desktop.
 */
SPDesktop *DialogBase::getDesktop()
{
    return dynamic_cast<SPDesktop *>(_app->get_active_view());
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

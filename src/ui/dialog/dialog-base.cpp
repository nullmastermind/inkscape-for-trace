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

#include <glibmm/main.h>
#include <glibmm/refptr.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/notebook.h>
#include <iostream>

#include "desktop.h"
#include "ui/dialog/dialog-notebook.h"

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
    // Get translatable name for the dialog based on the verb
    Verb *verb = Verb::get(verb_num);
    if (verb) {
        _name = verb->get_name();
        int pos = _name.find("...", 0);
        if (pos >= 0 && pos < _name.length() - 2) {
            _name.replace(pos, 3, "");
        }
        pos = _name.find("_", 0);
        if (pos >= 0 && pos < _name.length()) {
            _name.replace(pos, 1, "");
        }
    }

    set_name(_name); // Essential for dialog functionality
    property_margin().set_value(1); // Essential for dialog UI
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

        Glib::RefPtr<Gtk::StyleContext> style = notebook->get_style_context();

        Glib::RefPtr<Gtk::CssProvider> provider = Gtk::CssProvider::create();
        provider->load_from_data(" *.blink {border: 3px solid @selected_bg_color;}");
        style->add_provider(provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        style->add_class("blink");

        // Add timer to turn off blink.
        sigc::slot<bool> slot = sigc::mem_fun(*this, &DialogBase::blink_off);
        sigc::connection connection = Glib::signal_timeout().connect(slot, 500); // msec
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

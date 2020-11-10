// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Help/About window
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 1999-2005 authors
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "help.h"

#include <glibmm.h>
#include <glibmm/i18n.h>
#include <gtkmm.h>

#include "inkscape-application.h"

#include "io/resource.h"
#include "ui/interface.h" // sp_ui_error_dialog
#include "ui/dialog/about.h"

using namespace Inkscape::IO::Resource;

void sp_help_about()
{
    Inkscape::UI::Dialog::AboutDialog::show_about();
}

/** Open an URL in the the default application
 *
 * See documentation of gtk_show_uri_on_window() for details
 *
 * @param url    URL to be opened
 * @param window Parent window for which the URL is opened
 */
// TODO: Do we really need a window reference here? It's the way recommended by gtk, though.
void sp_help_open_url(const Glib::ustring &url, Gtk::Window *window)
{
    try {
        window->show_uri(url, GDK_CURRENT_TIME);
    } catch (const Glib::Error &e) {
        g_warning("Unable to show '%s': %s", url.c_str(), e.what().c_str());
    }
}

void sp_help_open_tutorial(Glib::ustring name)
{
    Glib::ustring filename = name + ".svg";

    filename = Inkscape::IO::Resource::get_filename(Inkscape::IO::Resource::TUTORIALS, filename.c_str(), true);
    if (!filename.empty()) {
        auto *app = InkscapeApplication::instance();
        SPDocument* doc = app->document_new(filename);
        app->window_open(doc);
    } else {
        // TRANSLATORS: Please don't translate link unless the page exists in your language. Add your language code to
        // the link this way: https://inkscape.org/[lang]/learn/tutorials/
        sp_ui_error_dialog(_("The tutorial files are not installed.\nFor Linux, you may need to install "
                             "'inkscape-tutorials'; for Windows, please re-run the setup and select 'Tutorials'.\nThe "
                             "tutorials can also be found online at https://inkscape.org/en/learn/tutorials/"));
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

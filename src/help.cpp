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

#include <glibmm.h>
#include <glibmm/i18n.h>

#include "inkscape-application.h"

#include "help.h"
#include "io/resource.h"
#include "io/sys.h"
#include "path-prefix.h"
#include "ui/dialog/aboutbox.h"
#include "ui/interface.h"


using namespace Inkscape::IO::Resource;


void sp_help_about()
{
    Inkscape::UI::Dialog::AboutBox::show_about();
}

/** Open an URL in the the default application
*
* See documentation of gtk_show_uri_on_window() for details
*
* @param url    URL to be opened
* @param window Parent window for which the URL is opened
*/
void sp_help_open_url(Glib::ustring url, Gtk::Window *window)
{
    // TODO: Find gtkmm/glibmm alternative for gtk_show_uri_on_window (currently not wrapped)
    //       Do we really need a window reference here? It's the way recommended by gtk, though.
    GError *error = nullptr;
    gtk_show_uri_on_window(window->gobj(), url.c_str(), GDK_CURRENT_TIME, &error);
    if (error) {
        g_warning ("Unable to show '%s': %s", url.c_str(), error->message);
        g_error_free (error);
    }
}

void sp_help_open_tutorial(Glib::ustring name)
{
    Glib::ustring filename = name + ".svg";

    filename = get_filename(TUTORIALS, filename.c_str(), true);
    if (!filename.empty()) {
        Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(filename);
        ConcreteInkscapeApplication<Gtk::Application>* app = &(ConcreteInkscapeApplication<Gtk::Application>::get_instance());
        app->create_window(file, false, false);
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

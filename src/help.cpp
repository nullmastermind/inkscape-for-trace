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

void sp_help_open_tutorial(Glib::ustring name)
{
    Glib::ustring filename = name + ".svg";

    filename = get_filename(TUTORIALS, filename.c_str(), true);
    if (!filename.empty()) {
        Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(filename);
        ConcreteInkscapeApplication<Gtk::Application>* app = &(ConcreteInkscapeApplication<Gtk::Application>::get_instance());
        app->create_window(file, false, false);
    } else {
        sp_ui_error_dialog(_("The tutorial files are not installed.\nFor Linux, you may need to install "
                             "'inkscape-tutorials'; for Windows, please re-run the setup and select 'Tutorials'.\nThe "
                             "tutorials can also be found online at https://inkscape.org/learn/tutorials/"));
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

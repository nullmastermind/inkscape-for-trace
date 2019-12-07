// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_DESKTOP_MENUBAR_H
#define SEEN_DESKTOP_MENUBAR_H

/**
 * @file
 * Desktop main menu bar code.
 */
/*
 * Authors:
 *   Tavmjong Bah
 *
 * Copyright (C) 2018 Authors
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 * Read the file 'COPYING' for more information.
 *
 */

namespace Gtk {
  class MenuBar;
  class MenuItem;
}

namespace Inkscape {
namespace UI {
namespace View {
  class View;
}
}
}

bool getStateFromPref(SPDesktop *dt, Glib::ustring item);
Gtk::MenuBar* build_menubar(Inkscape::UI::View::View* view);
Gtk::MenuItem *get_menu_item_for_verb(unsigned int verb, Inkscape::UI::View::View *);

#endif // SEEN_DESKTOP_MENUBAR_H

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

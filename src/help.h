// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2003 authors
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_HELP_H
#define SEEN_HELP_H

namespace Glib {
class ustring;
}

namespace Gtk {
class Window;
}

/**
 * Help/About window.
 */
void sp_help_about();
void sp_help_open_url(const Glib::ustring &url, Gtk::Window *window);
void sp_help_open_tutorial(Glib::ustring name);

#endif // !SEEN_HELP_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :

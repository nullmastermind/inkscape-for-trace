// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Cursor utilities
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#ifndef INK_CURSOR_UTILITIES_H
#define INK_CURSOR_UTILITIES_H

#include <gtkmm.h>
#include <string>

namespace Inkscape {

Glib::RefPtr<Gdk::Cursor> load_svg_cursor(Glib::RefPtr<Gdk::Display> display,
                                          Glib::RefPtr<Gdk::Window> window,
                                          std::string const &file_name,
                                          guint32 fill = 0xffffffff,
                                          guint32 stroke = 0x000000ff,
                                          double fill_opacity = 1.0,
                                          double stroke_opacity = 1.0);

} // Namespace Inkscape

#endif // INK_CURSOR_UTILITIES_H

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

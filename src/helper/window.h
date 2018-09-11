// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors:
 * see git history
 * Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2014 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_WINDOW_H
#define SEEN_SP_WINDOW_H

struct _GtkWidget;
typedef _GtkWidget GtkWidget;

namespace Gtk {
class Window;
}

// Can we just get rid of this altogether?
#if defined(GCC_VERSION) || defined(__clang__)
__attribute__((deprecated))
#endif
GtkWidget * sp_window_new (const gchar *title, unsigned int resizeable);

namespace Inkscape {
namespace UI {

Gtk::Window *window_new (const gchar *title, unsigned int resizeable);

}
}

#endif // !SEEN_SP_WINDOW_H

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

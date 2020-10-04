// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_INKSCAPE_UI_WIDGET_SCROLL_UTILS_H
#define SEEN_INKSCAPE_UI_WIDGET_SCROLL_UTILS_H

/* Authors:
 *   Thomas Holder
 *
 * Copyright (C) 2020 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gdk/gdk.h>

namespace Gtk {
class Widget;
}

namespace Inkscape {
namespace UI {
namespace Widget {

Gtk::Widget *get_scrollable_ancestor(Gtk::Widget *widget);

bool scrolling_allowed(Gtk::Widget *widget, GdkEventScroll *event = nullptr);

} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif
// vim: filetype=cpp:expandtab:shiftwidth=4:softtabstop=4:fileencoding=utf-8:textwidth=99 :

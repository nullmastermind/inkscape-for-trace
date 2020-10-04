// SPDX-License-Identifier: GPL-2.0-or-later
/* Authors:
 *   Thomas Holder
 *
 * Copyright (C) 2020 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "scroll-utils.h"

#include <gtkmm/scrolledwindow.h>

namespace Inkscape {
namespace UI {
namespace Widget {

/**
 * Get the first ancestor which is scrollable.
 */
Gtk::Widget *get_scrollable_ancestor(Gtk::Widget *widget)
{
    auto parent = widget->get_parent();
    if (!parent) {
        return nullptr;
    }
    if (auto scrollable = dynamic_cast<Gtk::ScrolledWindow *>(parent)) {
        return scrollable;
    }
    return get_scrollable_ancestor(parent);
}

/**
 * Return true if scrolling is allowed.
 *
 * Scrolling is allowed for any of:
 * - Shift modifier is pressed
 * - Widget has focus
 * - Widget has no scrollable ancestor
 */
bool scrolling_allowed(Gtk::Widget *widget, GdkEventScroll *event)
{
    bool const shift = event && (event->state & GDK_SHIFT_MASK);
    return shift ||               //
           widget->has_focus() || //
           get_scrollable_ancestor(widget) == nullptr;
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape

// vim: filetype=cpp:expandtab:shiftwidth=4:softtabstop=4:fileencoding=utf-8:textwidth=99 :

// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_INKSCAPE_UI_WIDGET_SCROLLPROTECTED_H
#define SEEN_INKSCAPE_UI_WIDGET_SCROLLPROTECTED_H

/* Authors:
 *   Thomas Holder
 *
 * Copyright (C) 2020 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "scroll-utils.h"

namespace Inkscape {
namespace UI {
namespace Widget {

/**
 * A class decorator which blocks the scroll event if the widget does not have
 * focus and any ancestor is a scrollable window, and SHIFT is not pressed.
 *
 * For custom scroll event handlers, derived classes must implement
 * on_safe_scroll_event instead of on_scroll_event. Directly connecting to
 * signal_scroll_event() will bypass the scroll protection.
 *
 * @tparam Base A subclass of Gtk::Widget
 */
template <typename Base>
class ScrollProtected : public Base
{
public:
    using Base::Base;

protected:
    /**
     * Event handler for "safe" scroll events which are only triggered if:
     * - the widget has focus
     * - or the widget has no scrolled window ancestor
     * - or the Shift key is pressed
     */
    virtual bool on_safe_scroll_event(GdkEventScroll *event)
    { //
        return Base::on_scroll_event(event);
    }

    bool on_scroll_event(GdkEventScroll *event) final
    {
        if (!scrolling_allowed(this, event)) {
            return false;
        }
        return on_safe_scroll_event(event);
    }
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif
// vim: filetype=cpp:expandtab:shiftwidth=4:softtabstop=4:fileencoding=utf-8:textwidth=99 :

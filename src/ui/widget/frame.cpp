// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Murray C
 *
 * Copyright (C) 2012 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "frame.h"


// Inkscape::UI::Widget::Frame

namespace Inkscape {
namespace UI {
namespace Widget {

Frame::Frame(Glib::ustring const &label_text /*= ""*/, gboolean label_bold /*= TRUE*/ )
    : _label(label_text, Gtk::ALIGN_END, Gtk::ALIGN_CENTER, true)
{
    set_shadow_type(Gtk::SHADOW_NONE);

    set_label_widget(_label);
    set_label(label_text, label_bold);
}

void
Frame::add(Widget& widget)
{
    Gtk::Frame::add(widget);
    set_padding(4, 0, 8, 0);
    show_all_children();
}

void
Frame::set_label(const Glib::ustring &label_text, gboolean label_bold /*= TRUE*/)
{
    if (label_bold) {
        _label.set_markup(Glib::ustring("<b>") + label_text + "</b>");
    } else {
        _label.set_text(label_text);
    }
}

void
Frame::set_padding (guint padding_top, guint padding_bottom, guint padding_left, guint padding_right)
{
    auto child = get_child();

    if(child)
    {
        child->set_margin_top(padding_top);
        child->set_margin_bottom(padding_bottom);
        child->set_margin_start(padding_left);
        child->set_margin_end(padding_right);
    }
}

Gtk::Label const *
Frame::get_label_widget() const
{
    return &_label;
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape

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

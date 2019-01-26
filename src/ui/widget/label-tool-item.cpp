// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * A label that can be added to a toolbar
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "label-tool-item.h"

#include <gtkmm/label.h>

namespace Inkscape {
namespace UI {
namespace Widget {

/**
 * \brief Create a tool-item containing a label
 *
 * \param[in] label    The text to display in the label
 * \param[in] mnemonic True if text should use a mnemonic
 */
LabelToolItem::LabelToolItem(const Glib::ustring& label, bool mnemonic)
    : _label(Gtk::manage(new Gtk::Label(label, mnemonic)))
{
    add(*_label);
    show_all();
}

/**
 * \brief Set the markup text in the label
 *
 * \param[in] str The markup text
 */
void
LabelToolItem::set_markup(const Glib::ustring& str)
{
    _label->set_markup(str);
}

/**
 * \brief Sets whether label uses Pango markup
 *
 * \param[in] setting true if the label text should be parsed for markup
 */
void
LabelToolItem::set_use_markup(bool setting)
{
    _label->set_use_markup(setting);
}

}
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

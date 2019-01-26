// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * A label that can be added to a toolbar
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_LABEL_TOOL_ITEM_H
#define SEEN_LABEL_TOOL_ITEM_H

#include <gtkmm/toolitem.h>

namespace Gtk {
class Label;
}

namespace Inkscape {
namespace UI {
namespace Widget {

/**
 * \brief A label that can be added to a toolbar
 */
class LabelToolItem : public Gtk::ToolItem {
private:
    Gtk::Label *_label;

public:
    LabelToolItem(const Glib::ustring& label, bool mnemonic = false);

    void set_markup(const Glib::ustring& str);
    void set_use_markup(bool setting = true);
};
}
}
}

#endif
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

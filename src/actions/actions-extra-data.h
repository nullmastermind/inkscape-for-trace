// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Extra data associated with actions: Label, Section, Tooltip.
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#ifndef INK_ACTIONS_EXTRA_DATA_H
#define INK_ACTIONS_EXTRA_DATA_H

#include <map>
#include <vector>

#include <glibmm/ustring.h>

class InkActionExtraDatum {
 public:
  InkActionExtraDatum(Glib::ustring& label, Glib::ustring& section, Glib::ustring& tooltip)
    : action_label(label)
    , action_section(section)
    , action_tooltip(tooltip)
    {
    }

  Glib::ustring get_label()   { return action_label; }
  Glib::ustring get_section() { return action_section; }
  Glib::ustring get_tooltip() { return action_tooltip; }

 private:
  Glib::ustring action_label;
  Glib::ustring action_section;
  Glib::ustring action_tooltip;
};

class InkActionExtraData {

 public:
  InkActionExtraData() = default;

  void add_data(std::vector<std::vector<Glib::ustring>> &raw_data);

  Glib::ustring get_label_for_action(Glib::ustring& action_name);
  Glib::ustring get_section_for_action(Glib::ustring& action_name);
  Glib::ustring get_tooltip_for_action(Glib::ustring& action_name);

 private:
  std::map<Glib::ustring, InkActionExtraDatum> data;
};

#endif // INK_ACTIONS_EXTRA_DATA_H

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

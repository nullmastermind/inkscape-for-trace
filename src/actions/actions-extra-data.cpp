// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Extra data associated with actions: Label, Section, Tooltip.
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include "actions-extra-data.h"

#include <iostream>

Glib::ustring
InkActionExtraData::get_label_for_action(Glib::ustring& action_name)
{
  Glib::ustring value;
  auto search = data.find(action_name);
  if (search != data.end()) {
    value = search->second.get_label();
  }
  return value;
}

Glib::ustring
InkActionExtraData::get_section_for_action(Glib::ustring& action_name) {

  Glib::ustring value;
  auto search = data.find(action_name);
  if (search != data.end()) {
    value = search->second.get_section();
  }
  return value;
}

Glib::ustring
InkActionExtraData::get_tooltip_for_action(Glib::ustring& action_name) {

  Glib::ustring value;
  auto search = data.find(action_name);
  if (search != data.end()) {
    value = search->second.get_tooltip();
  }
  return value;
}

void
InkActionExtraData::add_data(std::vector<std::vector<Glib::ustring>> &raw_data)
{
  for (auto raw : raw_data) {
    InkActionExtraDatum datum(raw[1], raw[2], raw[3]);
    data.emplace(raw[0], datum);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

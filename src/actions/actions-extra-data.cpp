// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Extra data associated with actions: Label, Section, Tooltip.
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Extra data is indexed by "detailed action names", that is an action
 * with prefix and value (if statefull). For example:
 *   "win.canvas-display-mode(1)"
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include "actions-extra-data.h"

#include <iostream>

#include <glibmm/i18n.h>

std::vector<Glib::ustring>
InkActionExtraData::get_actions()
{
    std::vector<Glib::ustring> action_names;
    for (auto datum : data) {
        action_names.emplace_back(datum.first);
    }
    return action_names;
}

Glib::ustring
InkActionExtraData::get_label_for_action(Glib::ustring const &action_name, bool translated)
{
    Glib::ustring value;
    auto search = data.find(action_name);
    if (search != data.end()) {
        value = translated ? _(search->second.get_label().c_str())
                           :   search->second.get_label();
    }
    return value;
}

// TODO: Section should be translatable, too
Glib::ustring
InkActionExtraData::get_section_for_action(Glib::ustring const &action_name) {

    Glib::ustring value;
    auto search = data.find(action_name);
    if (search != data.end()) {
        value = search->second.get_section();
    }
    return value;
}

Glib::ustring
InkActionExtraData::get_tooltip_for_action(Glib::ustring const &action_name, bool translated) {

    Glib::ustring value;
    auto search = data.find(action_name);
    if (search != data.end()) {
        value = translated ? _(search->second.get_tooltip().c_str())
                           :   search->second.get_tooltip();
    }
    return value;
}

void
InkActionExtraData::add_data(std::vector<std::vector<Glib::ustring>> &raw_data) {
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

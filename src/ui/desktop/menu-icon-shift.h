// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_DESKTOP_MENU_ITEM_SHIFT_H
#define SEEN_DESKTOP_MENU_ITEM_SHIFT_H

/**
 * @file
 * Shift Gtk::MenuItems with icons to align with Toggle and Radio buttons.
 */
/*
 * Authors:
 *   Tavmjong Bah       <tavmjong@free.fr>
 *   Patrick Storz      <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2020 Authors
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 * Read the file 'COPYING' for more information.
 *
 */

namespace Gtk {
  class MenuShell;
}

/**
 * Call back to shift icons into place reserved for toggles (i.e. check and radio items).
 */
void shift_icons(Gtk::MenuShell *menu);

/**
 * Add callbacks recursively to menus.
 */
void shift_icons_recursive(Gtk::MenuShell *menu);

#endif // SEEN_DESKTOP_MENU_ITEM_SHIFT_H

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

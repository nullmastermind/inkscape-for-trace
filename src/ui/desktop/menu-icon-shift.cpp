// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Shift Gtk::MenuItems with icons to allign with Toggle and Radio buttons.
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

#include "menu-icon-shift.h"

#include <iostream>
#include <gtkmm.h>

/*
 *  Install CSS to shift icons into the space reserved for toggles (i.e. check and radio items).
 */
void
shift_icons(Gtk::MenuShell* menu)
{
    static Glib::RefPtr<Gtk::CssProvider> provider;
    if (!provider) {
        provider = Gtk::CssProvider::create();
        auto const screen = Gdk::Screen::get_default();
        Gtk::StyleContext::add_provider_for_screen(screen, provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    // Calculate required shift. We need an example!
    // Search for Gtk::MenuItem -> Gtk::Box -> Gtk::Image
    for (auto child : menu->get_children()) {

        auto menuitem = dynamic_cast<Gtk::MenuItem *>(child);
        if (menuitem) {

            auto box = dynamic_cast<Gtk::Box *>(menuitem->get_child());
            if (box) {

                box->set_spacing(0); // Match ImageMenuItem

                auto children = box->get_children();
                if (children.size() == 2) { // Image + Label

                    auto image = dynamic_cast<Gtk::Image *>(box->get_children()[0]);
                    if (image) {

                        // OK, we have an example, do calculation.
                        auto allocation_menuitem = menuitem->get_allocation();
                        auto allocation_image = image->get_allocation();

                        int shift = -allocation_image.get_x();
                        if (menuitem->get_direction() == Gtk::TEXT_DIR_RTL) {
                            shift += (allocation_menuitem.get_width() - allocation_image.get_width());
                        }

                        static int current_shift = 0;
                        if (std::abs(current_shift - shift) > 2) {
                            // Only do this once per menu, and only if there is a large change.
                            current_shift = shift;
                            std::string css_str;
                            if (menuitem->get_direction() == Gtk::TEXT_DIR_RTL) {
                                css_str = "menuitem box image {margin-right:" + std::to_string(shift) + "px;}";
                            } else {
                                css_str = "menuitem box image {margin-left:" + std::to_string(shift) + "px;}";
                            }
                            provider->load_from_data(css_str);
                        }
                    }
                }
            }
        }
    }
    // If we get here, it means there were no examples... but we don't care as there are no icons to shift anyway.
}

/*
 * Find all submenus to add "shift_icons" callback. We need to do this for
 * all submenus as some submenus are children of other submenus without icons.
 */
void
shift_icons_recursive(Gtk::MenuShell *menu)
{
    if (menu) {

        // Connect signal
        menu->signal_map().connect(sigc::bind<Gtk::MenuShell *>(sigc::ptr_fun(&shift_icons), menu));

        // Look for descendent menus.
        auto children = menu->get_children(); // Should be Gtk::MenuItem's
        for (auto child : children) {
            auto menuitem = dynamic_cast<Gtk::MenuItem *>(child);
            if (menuitem) {
                auto submenu = menuitem->get_submenu();
                if (submenu) {
                    shift_icons_recursive(submenu);
                }
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

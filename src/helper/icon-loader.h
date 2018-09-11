// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Icon Loader
 *//*
 * Authors:
 * see git history
 * Jabiertxo Arraiza <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_INK_ICON_LOADER_H
#define SEEN_INK_ICON_LOADER_H

#include <gdkmm/pixbuf.h>
#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/radioaction.h>

Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, gint size, bool negative = false);
Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, Gtk::BuiltinIconSize icon_size, bool negative = false);
Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, GtkIconSize icon_size, bool negative = false);
Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, gchar const *prefs_sice, bool negative = false);
Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, gint size, bool negative = false);
Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, Gtk::BuiltinIconSize icon_size, bool negative = false);
Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, GtkIconSize icon_size, bool negative = false);
Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, gchar const *prefs_sice, bool negative = false);
std::pair<Glib::RefPtr<Gtk::RadioAction>, Gdk::RGBA> sp_set_radioaction_icon(Gtk::RadioAction::Group group,
                                                                             Glib::ustring icon_name,
                                                                             Glib::ustring label,
                                                                             Glib::ustring tooltip,
                                                                             bool negative = false);
#endif // SEEN_INK_STOCK_ITEMS_H

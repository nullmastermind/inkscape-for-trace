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
#include <gtkmm/image.h>

Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, gint size);
Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, Gtk::BuiltinIconSize icon_size);
Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, Gtk::IconSize icon_size);
Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, gchar const *prefs_sice);
GtkWidget  *sp_get_icon_image(Glib::ustring icon_name, GtkIconSize icon_size);
Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, gint size);
Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, Gtk::IconSize icon_size);
Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, Gtk::BuiltinIconSize icon_size);
Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, GtkIconSize icon_size);
Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, gchar const *prefs_sice);

#endif // SEEN_INK_ICON_LOADER_H

#ifndef SEEN_INK_ICON_LOADER_H
#define SEEN_INK_ICON_LOADER_H

/*
 * Icon Loader
 *
 *
 * Authors:
 *  Jabiertxo Arraiza <jabier.arraiza@marker.es>
 *
 *
 */
#include <gdkmm/pixbuf.h>
#include <gtkmm/image.h>

Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, gint size);
Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, Gtk::BuiltinIconSize icon_size);
Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, GtkIconSize icon_size);
Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, gchar const *prefs_sice);
Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, gint size);
Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, Gtk::BuiltinIconSize icon_sice);
Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, GtkIconSize icon_sice);
Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, gchar const *prefs_sice);

#endif // SEEN_INK_STOCK_ITEMS_H

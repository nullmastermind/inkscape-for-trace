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
#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/radioaction.h>

Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, gint size);
Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, Gtk::BuiltinIconSize icon_size);
Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, GtkIconSize icon_size);
Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, gchar const *prefs_sice);
Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, gint size);
Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, Gtk::BuiltinIconSize icon_sice);
Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, GtkIconSize icon_sice);
Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, gchar const *prefs_sice);
std::pair<Glib::RefPtr<Gtk::RadioAction>, Gdk::RGBA> sp_set_radioaction_icon(Gtk::RadioAction::Group group,
                                                                             Glib::ustring icon_name,
                                                                             Glib::ustring label,
                                                                             Glib::ustring tooltip);
#endif // SEEN_INK_STOCK_ITEMS_H

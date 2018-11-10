// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Icon Loader
 *
 * Icon Loader management code
 *
 * Authors:
 *  Jabiertxo Arraiza <jabier.arraiza@marker.es>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "icon-loader.h"
#include "inkscape.h"
#include "io/resource.h"
#include "preferences.h"
#include "svg/svg-color.h"
#include "widgets/toolbox.h"
#include <gdkmm/display.h>
#include <gdkmm/screen.h>
#include <gtkmm/iconinfo.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/toolitem.h>

void sp_load_theme() {}

Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, gint size)
{
    Glib::RefPtr<Gdk::Display> display = Gdk::Display::get_default();
    Glib::RefPtr<Gdk::Screen>  screen = display->get_default_screen();
    Glib::RefPtr<Gtk::IconTheme> icon_theme = Gtk::IconTheme::get_for_screen(screen);
    // TODO all calls to "sp_get_icon_pixbuf" need to be removed in thew furture
    // Put here temporary for allow use symbolic in a few icons require pixbug instead Gtk::Image
    // We coulden't acces to pixbuf of a symbolic ones with the next order
    // icon_theme->load_icon(icon_name, size, Gtk::ICON_LOOKUP_FORCE_SIZE);
    // Maybe we can do with Gio, but not sure.  Also can render a icon to pixbuf but need to be
    // a stock-icon not on named ones I think or access directly to the icon.svg file
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::RefPtr<Gdk::Pixbuf> _icon_pixbuf;
    try {
        if (prefs->getBool("/theme/symbolicIcons", false)) {
            gchar colornamed[64];
            int colorset = prefs->getInt("/theme/symbolicColor", 0x000000ff);
            // Use in case the special widgets have inverse theme background and symbolic
            sp_svg_write_color(colornamed, sizeof(colornamed), colorset);
            Gdk::RGBA color;
            color.set(colornamed);
            Gtk::IconInfo iconinfo =
                icon_theme->lookup_icon(icon_name + Glib::ustring("-symbolic"), size, Gtk::ICON_LOOKUP_FORCE_SIZE);
            if (bool(iconinfo)) {
                bool was_symbolic = false;
                _icon_pixbuf = iconinfo.load_symbolic(color, color, color, color, was_symbolic);
            }
            else {
                _icon_pixbuf = icon_theme->load_icon(icon_name, size, Gtk::ICON_LOOKUP_FORCE_SIZE);
            }
            g_warning("Icon Loader using a future dead function in this icon: %s", icon_name.c_str());
        }
        else {
            _icon_pixbuf = icon_theme->load_icon(icon_name, size, Gtk::ICON_LOOKUP_FORCE_SIZE);
        }
    }
    catch (const Gtk::IconThemeError &e) {
        g_warning("Icon Loader error loading icon file: %s", e.what().c_str());
    }
    return _icon_pixbuf;
}

Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, Gtk::IconSize icon_size)
{
    int width, height;
    Gtk::IconSize::lookup(icon_size, width, height);
    return sp_get_icon_pixbuf(icon_name, width);
}

Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, Gtk::BuiltinIconSize icon_size)
{
    int width, height;
    Gtk::IconSize::lookup(Gtk::IconSize(icon_size), width, height);
    return sp_get_icon_pixbuf(icon_name, width);
}

Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, GtkIconSize icon_size)
{
    gint width, height;
    gtk_icon_size_lookup(icon_size, &width, &height);
    return sp_get_icon_pixbuf(icon_name, width);
}

Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, gchar const *prefs_size)
{
    // Load icon based in preference size defined allowed values are:
    //"/toolbox/tools/small" Toolbox icon size
    //"/toolbox/small" Control bar icon size
    //"/toolbox/secondary" Secondary toolbar icon size
    GtkIconSize icon_size = Inkscape::UI::ToolboxFactory::prefToSize(prefs_size);
    return sp_get_icon_pixbuf(icon_name, icon_size);
}


Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, Gtk::BuiltinIconSize icon_size)
{

    Gtk::Image *icon = new Gtk::Image();
    icon->set_from_icon_name(icon_name, Gtk::IconSize(icon_size));
    return icon;
}

Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, Gtk::IconSize icon_size)
{

    Gtk::Image *icon = new Gtk::Image();
    icon->set_from_icon_name(icon_name, icon_size);
    return icon;
}

Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, gchar const *prefs_size)
{

    Gtk::IconSize icon_size = Inkscape::UI::ToolboxFactory::prefToSize_mm(prefs_size);
    Gtk::Image *icon = new Gtk::Image();
    icon->set_from_icon_name(icon_name, icon_size);
    return icon;
}


GtkWidget *sp_get_icon_image(Glib::ustring icon_name, GtkIconSize icon_size)
{

    return gtk_image_new_from_icon_name(icon_name.c_str(), icon_size);
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

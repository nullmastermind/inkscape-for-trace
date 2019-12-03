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
#include "svg/svg-color.h"
#include "widgets/toolbox.h"
#include <fstream>
#include <gdkmm/display.h>
#include <gdkmm/screen.h>
#include <gtkmm/iconinfo.h>
#include <gtkmm/icontheme.h>

Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, gint size)
{
    Gtk::Image *icon = new Gtk::Image();
    icon->set_from_icon_name(icon_name, Gtk::IconSize(Gtk::ICON_SIZE_BUTTON));
    icon->set_pixel_size(size);
    return icon;
}

Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, Gtk::IconSize icon_size)
{
    Gtk::Image *icon = new Gtk::Image();
    icon->set_from_icon_name(icon_name, icon_size);
    return icon;
}

Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, Gtk::BuiltinIconSize icon_size)
{
    Gtk::Image *icon = new Gtk::Image();
    icon->set_from_icon_name(icon_name, icon_size);
    return icon;
}

Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, gchar const *prefs_size)
{
    Gtk::IconSize icon_size = Inkscape::UI::ToolboxFactory::prefToSize_mm(prefs_size);
    return sp_get_icon_image(icon_name, icon_size);
}

GtkWidget *sp_get_icon_image(Glib::ustring icon_name, GtkIconSize icon_size)
{
    return gtk_image_new_from_icon_name(icon_name.c_str(), icon_size);
}

Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, gint size)
{
    Glib::RefPtr<Gdk::Display> display = Gdk::Display::get_default();
    Glib::RefPtr<Gdk::Screen>  screen = display->get_default_screen();
    Glib::RefPtr<Gtk::IconTheme> icon_theme = Gtk::IconTheme::get_for_screen(screen);
    Glib::RefPtr<Gdk::Pixbuf> _icon_pixbuf;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/theme/symbolicIcons", false)) {
        Gtk::IconInfo iconinfo = icon_theme->lookup_icon(icon_name + Glib::ustring("-symbolic"), size, Gtk::ICON_LOOKUP_FORCE_SIZE);
        if (iconinfo && SP_ACTIVE_DESKTOP->getToplevel()) {
            bool was_symbolic = false;
            Glib::ustring css_str = "";
            Glib::ustring themeiconname = prefs->getString("/theme/iconTheme");
            guint32 colorsetbase = prefs->getUInt("/theme/" + themeiconname + "/symbolicBaseColor", 0x2E3436ff);
            guint32 colorsetsuccess = prefs->getUInt("/theme/" + themeiconname + "/symbolicSuccessColor", 0x4AD589ff);
            guint32 colorsetwarning = prefs->getUInt("/theme/" + themeiconname + "/symbolicWarningColor", 0xF57900ff);
            guint32 colorseterror = prefs->getUInt("/theme/" + themeiconname + "/symbolicErrorColor", 0xCC0000ff);
            gchar colornamed[64];
            gchar colornamedsuccess[64];
            gchar colornamedwarning[64];
            gchar colornamederror[64];
            sp_svg_write_color(colornamed, sizeof(colornamed), colorsetbase);
            sp_svg_write_color(colornamedsuccess, sizeof(colornamedsuccess), colorsetsuccess);
            sp_svg_write_color(colornamedwarning, sizeof(colornamedwarning), colorsetwarning);
            sp_svg_write_color(colornamederror, sizeof(colornamederror), colorseterror);
            _icon_pixbuf =
                iconinfo.load_symbolic(Gdk::RGBA(colornamed), Gdk::RGBA(colornamedsuccess),
                                       Gdk::RGBA(colornamedwarning), Gdk::RGBA(colornamederror), was_symbolic);
        } else {
            Gtk::IconInfo iconinfo = icon_theme->lookup_icon(icon_name, size, Gtk::ICON_LOOKUP_FORCE_SIZE);
            _icon_pixbuf = iconinfo.load_icon();
        }
    } else {
        Gtk::IconInfo iconinfo = icon_theme->lookup_icon(icon_name, size, Gtk::ICON_LOOKUP_FORCE_SIZE);
        _icon_pixbuf = iconinfo.load_icon();
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

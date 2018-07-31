/*
 * Icon Loader
 *
 * Icon Loader management code
 *
 * Authors:
 *  Jabiertxo Arraiza <jabier.arraiza@marker.es>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "icon-loader.h"
#include "inkscape.h"
#include "io/resource.h"
#include "preferences.h"
#include "svg/svg-color.h"
#include "widgets/toolbox.h"
#include <gtkmm/iconinfo.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/toolitem.h>

Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, gint size)
{
    using namespace Inkscape::IO::Resource;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::RefPtr<Gdk::Pixbuf> _icon_pixbuf;
    static auto icon_theme = Gtk::IconTheme::get_default();
    static bool icon_theme_set;
    if (!icon_theme_set) {
        icon_theme_set = true;
        icon_theme->prepend_search_path(get_path_ustring(SYSTEM, ICONS));
        icon_theme->prepend_search_path(get_path_ustring(USER, ICONS));
    }
    try {
        if (prefs->getBool("/theme/symbolicIcons", false)) {
            gchar colornamed[64];
            if (icon_name == "gtk-preferences") {
                icon_name = "preferences-system";
            }
            sp_svg_write_color(colornamed, sizeof(colornamed), prefs->getInt("/theme/symbolicColor", 0x000000ff));
            Gdk::RGBA color;
            color.set(colornamed);
            Gtk::IconInfo iconinfo =
                icon_theme->lookup_icon(icon_name + Glib::ustring("-symbolic"), size, Gtk::ICON_LOOKUP_FORCE_SIZE);
            if (bool(iconinfo)) {
                // TODO: view if we need parametrice other colors
                bool was_symbolic = false;
                _icon_pixbuf = iconinfo.load_symbolic(color, color, color, color, was_symbolic);
            }
            else {
                _icon_pixbuf = icon_theme->load_icon(icon_name, size, Gtk::ICON_LOOKUP_FORCE_SIZE);
            }
        }
        else {
            _icon_pixbuf = icon_theme->load_icon(icon_name, size, Gtk::ICON_LOOKUP_FORCE_SIZE);
        }
    }
    catch (const Gtk::IconThemeError &e) {
        std::cout << "Icon Loader: " << e.what() << std::endl;
    }
    return _icon_pixbuf;
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

Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, gint size)
{
    auto icon = sp_get_icon_pixbuf(icon_name, size);
    Gtk::Image *image = new Gtk::Image(icon);
    return image;
}

Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, Gtk::BuiltinIconSize icon_size)
{
    auto icon = sp_get_icon_pixbuf(icon_name, icon_size);
    Gtk::Image *image = new Gtk::Image(icon);
    return image;
}

Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, GtkIconSize icon_size)
{
    auto icon = sp_get_icon_pixbuf(icon_name, icon_size);
    Gtk::Image *image = new Gtk::Image(icon);
    return image;
}

Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, gchar const *prefs_size)
{
    auto icon = sp_get_icon_pixbuf(icon_name, prefs_size);
    Gtk::Image *image = new Gtk::Image(icon);
    return image;
}

std::pair<Glib::RefPtr<Gtk::RadioAction>, Gdk::RGBA> sp_set_radioaction_icon(Gtk::RadioAction::Group group,
                                                                             Glib::ustring icon_name,
                                                                             Glib::ustring label, Glib::ustring tooltip)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/theme/symbolicIcons", false)) {
        icon_name = icon_name + Glib::ustring("-symbolic");
    }

    Glib::RefPtr<Gtk::RadioAction> action =
        Gtk::RadioAction::create_with_icon_name(group, "Anonymous", icon_name.c_str(), label.c_str(), tooltip.c_str());
    Gtk::ToolItem *item = action->create_tool_item();
    Gdk::RGBA color;
    gchar colornamed[64];
    sp_svg_write_color(colornamed, sizeof(colornamed), prefs->getInt("/theme/symbolicColor", 0x000000ff));
    color.set(colornamed);
    return std::make_pair(action, color);
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

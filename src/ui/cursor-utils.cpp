// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Cursor utilities
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include <iomanip>
#include <sstream>

#include "cursor-utils.h"

#include "document.h"
#include "preferences.h"

#include "display/cairo-utils.h"

#include "helper/pixbuf-ops.h"

#include "io/file.h"
#include "io/resource.h"

#include "object/sp-object.h"
#include "object/sp-root.h"

#include "util/units.h"

using Inkscape::IO::Resource::SYSTEM;
using Inkscape::IO::Resource::ICONS;

namespace Inkscape {

/**
 * Loads and sets an SVG cursor from the specified file name.
 * Returns pointer to cursor (or null cursor if we could not load a cursor).
 */
Glib::RefPtr<Gdk::Cursor>
load_svg_cursor(Glib::RefPtr<Gdk::Display> display,
                Glib::RefPtr<Gdk::Window> window,
                std::string const &file_name,
                guint32 fill,
                guint32 stroke,
                double fill_opacity,
                double stroke_opacity)
{
    // GTK puts cursors in a "cursors" subdirectory of icon themes. We'll do the same... but
    // note that we cannot use the normal GTK method for loading cursors as GTK knows nothing
    // about scalable SVG cursors. We must locate and load the files ourselves. (Even if
    // GTK could handle scalable cursors, we would need to load the files ourselves inorder
    // to modify CSS 'fill' and 'stroke' properties.)

    Glib::RefPtr<Gdk::Cursor> cursor;

    // Make list of icon themes, highest priority first.
    std::vector<std::string> theme_names;

    // Set in preferences
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring theme_name = prefs->getString("/theme/iconTheme");
    if (!theme_name.empty()) {
        theme_names.push_back(theme_name);
    }

    // System
    theme_name = Gtk::Settings::get_default()->property_gtk_icon_theme_name();
    theme_names.push_back(theme_name);

    // Our default
    theme_names.emplace_back("hicolor");


    // Find theme paths.
    auto screen = display->get_default_screen();
    auto icon_theme = Gtk::IconTheme::get_for_screen(screen);
    auto theme_paths = icon_theme->get_search_path();

    // Loop over theme names and paths, looking for file.
    Glib::RefPtr<Gio::File> file;
    std::string full_file_path;
    for (auto theme_name : theme_names) {
        for (auto theme_path : theme_paths) {
            full_file_path = Glib::build_filename(theme_path, theme_name, "cursors", file_name);
            // std::cout << "Checking: " << full_file_path << std::endl;
            file = Gio::File::create_for_path(full_file_path);
            if (file->query_exists()) break;
        }
        if (file->query_exists()) break;
    }

    if (!file->query_exists()) {
        std::cerr << "load_svg_cursor: Cannot locate cursor file: " << file_name << std::endl;
        return cursor;
    }

    bool cancelled = false;
    std::unique_ptr<SPDocument> document;
    document.reset(ink_file_open(file, &cancelled));

    if (!document) {
        std::cerr << "load_svg_cursor: Could not open document: " << full_file_path << std::endl;
        return cursor;
    }

    SPRoot *root = document->getRoot();
    if (!root) {
        std::cerr << "load_svg_cursor: Could not find SVG element: " << full_file_path << std::endl;
        return cursor;
    }

    // Set the CSS 'fill' and 'stroke' properties on the SVG element (for cascading).
    SPCSSAttr *css = sp_repr_css_attr(root->getRepr(), "style");

    std::stringstream fill_stream;
    fill_stream << "#" 
                << std::setfill ('0') << std::setw(6)
                << std::hex << (fill >> 8);
    std::stringstream stroke_stream;
    stroke_stream << "#" 
                  << std::setfill ('0') << std::setw(6)
                  << std::hex << (stroke >> 8);

    std::string fill_opacity_string = std::to_string(fill_opacity);
    std::string stroke_opacity_string = std::to_string(stroke_opacity);

    sp_repr_css_set_property(css, "fill",   fill_stream.str().c_str());
    sp_repr_css_set_property(css, "stroke", stroke_stream.str().c_str());
    sp_repr_css_set_property(css, "fill-opacity",   fill_opacity_string.c_str());
    sp_repr_css_set_property(css, "stroke-opacity", stroke_opacity_string.c_str());
    root->changeCSS(css, "style");
    sp_repr_css_attr_unref(css);

    // Find the rendered size of the icon.
    int scale = 1;
#ifndef GDK_WINDOWING_QUARTZ
    // Default cursor size (get_default_cursor_size()) fixed to 32 on Quartz. Cursor scaling handled elsewhere.

    bool cursor_scaling = prefs->getBool("/options/cursorscaling"); // Fractional scaling is broken but we can't detect it.
    if (cursor_scaling) {
        scale = window->get_scale_factor(); // Adjust for HiDPI screens.
    }
#endif

    // Check for maximum size
    // int mwidth = 0;
    // int mheight = 0;
    // display->get_maximal_cursor_size(mwidth, mheight);
    // int normal_size = display->get_default_cursor_size();

    auto w = document->getWidth().value("px");
    auto h = document->getHeight().value("px");
    int sw = w * scale;
    int sh = h * scale;
    int dpix = 96 * scale; // DPI
    int dpiy = 96 * scale;

    // Calculate the hotspot.
    int hotspot_x = root->getIntAttribute("inkscape:hotspot_x", 0); // Do not include window scale factor!
    int hotspot_y = root->getIntAttribute("inkscape:hotspot_y", 0);

    auto ink_pixbuf = sp_generate_internal_bitmap(document.get(), nullptr, 0, 0, w, h, sw, sh, dpix, dpiy, 0, nullptr);
    auto pixbuf = Glib::wrap(ink_pixbuf->getPixbufRaw());

    if (pixbuf) {
        cursor = Gdk::Cursor::create(display, pixbuf, hotspot_x, hotspot_y);
        window->set_cursor(cursor);
    } else {
        std::cerr << "load_svg_cursor: failed to create pixbuf for: " << full_file_path << std::endl;
    }

    return cursor;
}

} // Namespace

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

// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * Gtk <themes> helper code.
 */
/*
 * Authors:
 *   Jabiertxof
 *   Martin Owens
 *
 * Copyright (C) 2017-2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "themes.h"

#include <cstring>
#include <gio/gio.h>
#include <glibmm.h>
#include <gtkmm.h>
#include <map>
#include <utility>
#include <vector>

#include "preferences.h"

/**
 * Inkscape fill gtk, taken from glib/gtk code with our own checks.
 */
static void inkscape_fill_gtk(const gchar *path, gtkThemeList &themes)
{
    const gchar *dir_entry;
    GDir *dir = g_dir_open(path, 0, nullptr);
    if (!dir)
        return;
    while ((dir_entry = g_dir_read_name(dir))) {
        gchar *filename = g_build_filename(path, dir_entry, "gtk-3.0", "gtk.css", nullptr);
        bool has_prefer_dark = false;
  
        Glib::ustring theme = dir_entry;
        gchar *filenamedark = g_build_filename(path, dir_entry, "gtk-3.0", "gtk-dark.css", nullptr);
        if (g_file_test(filenamedark, G_FILE_TEST_IS_REGULAR))
            has_prefer_dark = true;
        if (themes.find(theme) != themes.end() && !has_prefer_dark) {
            continue;
        }
        if (g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
            themes[theme] = has_prefer_dark;
        }
        g_free(filename);
        g_free(filenamedark);
    }
  
    g_dir_close(dir);
}

/**
 * Get available themes based on locations of gtk directories.
 */
std::map<Glib::ustring, bool> get_available_themes()
{
    gtkThemeList themes;
    Glib::ustring theme = "";
    gchar *path;
    gchar **builtin_themes;
    guint i, j;
    const gchar *const *dirs;
  
    /* Builtin themes */
    builtin_themes = g_resources_enumerate_children("/org/gtk/libgtk/theme", G_RESOURCE_LOOKUP_FLAGS_NONE, nullptr);
    for (i = 0; builtin_themes[i] != NULL; i++) {
        if (g_str_has_suffix(builtin_themes[i], "/")) {
            theme = builtin_themes[i];
            theme.resize(theme.size() - 1);
            Glib::ustring theme_path = "/org/gtk/libgtk/theme";
            theme_path += "/" + theme;
            gchar **builtin_themes_files =
                g_resources_enumerate_children(theme_path.c_str(), G_RESOURCE_LOOKUP_FLAGS_NONE, nullptr);
            bool has_prefer_dark = false;
            if (builtin_themes_files != NULL) {
                for (j = 0; builtin_themes_files[j] != NULL; j++) {
                    Glib::ustring file = builtin_themes_files[j];
                    if (file == "gtk-dark.css") {
                        has_prefer_dark = true;
                    }
                }
            }
            g_strfreev(builtin_themes_files);
            themes[theme] = has_prefer_dark;
        }
    }

    g_strfreev(builtin_themes);

    path = g_build_filename(g_get_user_data_dir(), "themes", nullptr);
    inkscape_fill_gtk(path, themes);
    g_free(path);
  
    path = g_build_filename(g_get_home_dir(), ".themes", nullptr);
    inkscape_fill_gtk(path, themes);
    g_free(path);
  
    dirs = g_get_system_data_dirs();
    for (i = 0; dirs[i]; i++) {
        path = g_build_filename(dirs[i], "themes", nullptr);
        inkscape_fill_gtk(path, themes);
        g_free(path);
    }
    return themes;
}

/**
 * Check if current applied theme is dark or not by looking at style context.
 * This is important to check system default theme is dark or not
 * It only return True for dark and False for Bright. It does not apply any
 * property other than preferDarkTheme, so theme should be set before calling
 * this function as it may otherwise return outdated result.
 */
bool isCurrentThemeDark(Gtk::Container *window)
{
    bool dark = false;
    if (window) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        Glib::ustring current_theme =
            prefs->getString("/theme/gtkTheme", prefs->getString("/theme/defaultGtkTheme", ""));
        auto settings = Gtk::Settings::get_default();
        if (settings) {
            settings->property_gtk_application_prefer_dark_theme() = prefs->getBool("/theme/preferDarkTheme", false);
        }
        dark = current_theme.find(":dark") != std::string::npos;
        if (!dark) {
            Glib::RefPtr<Gtk::StyleContext> stylecontext = window->get_style_context();
            Gdk::RGBA rgba;
            bool background_set = stylecontext->lookup_color("theme_bg_color", rgba);
            if (background_set && (0.299 * rgba.get_red() + 0.587 * rgba.get_green() + 0.114 * rgba.get_blue()) < 0.5) {
                dark = true;
            }
        }
    }
    return dark;
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

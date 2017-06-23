/*
 * Inkscape::IO::Resource - simple resource API
 *
 * Copyright 2006 MenTaLguY <mental@rydia.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * See the file COPYING for details.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h> // g_assert()
#include "path-prefix.h"
#include "inkscape.h"
#include "io/sys.h"
#include "io/resource.h"

using Inkscape::IO::file_test;

namespace Inkscape {

namespace IO {

namespace Resource {

gchar *_get_path(Domain domain, Type type, char const *filename)
{
    gchar *path=NULL;
    switch (domain) {
        case SYSTEM: {
            gchar const* temp = 0;
            switch (type) {
                case APPICONS: temp = INKSCAPE_APPICONDIR; break;
                case EXTENSIONS: temp = INKSCAPE_EXTENSIONDIR; break;
                case GRADIENTS: temp = INKSCAPE_GRADIENTSDIR; break;
                case ICONS: temp = INKSCAPE_PIXMAPDIR; break;
                case KEYS: temp = INKSCAPE_KEYSDIR; break;
                case MARKERS: temp = INKSCAPE_MARKERSDIR; break;
                case PALETTES: temp = INKSCAPE_PALETTESDIR; break;
                case PATTERNS: temp = INKSCAPE_PATTERNSDIR; break;
                case SCREENS: temp = INKSCAPE_SCREENSDIR; break;
                case TEMPLATES: temp = INKSCAPE_TEMPLATESDIR; break;
                case TUTORIALS: temp = INKSCAPE_TUTORIALSDIR; break;
                case UI: temp = INKSCAPE_UIDIR; break;
                default: g_assert_not_reached();
            }
            path = g_strdup(temp);
        } break;
        case CREATE: {
            gchar const* temp = 0;
            switch (type) {
                case GRADIENTS: temp = CREATE_GRADIENTSDIR; break;
                case PALETTES: temp = CREATE_PALETTESDIR; break;
                case PATTERNS: temp = CREATE_PATTERNSDIR; break;
                default: g_assert_not_reached();
            }
            path = g_strdup(temp);
        } break;
        case USER: {
            char const *name=NULL;
            switch (type) {
                case EXTENSIONS: name = "extensions"; break;
                case GRADIENTS: name = "gradients"; break;
                case ICONS: name = "icons"; break;
                case KEYS: name = "keys"; break;
                case MARKERS: name = "markers"; break;
                case PALETTES: name = "palettes"; break;
                case PATTERNS: name = "patterns"; break;
                case TEMPLATES: name = "templates"; break;
                case UI: name = "ui"; break;
                default: return _get_path(SYSTEM, type, filename);
            }
            path = Inkscape::Application::profile_path(name);
        } break;
    }

    if (filename) {
        gchar *temp=g_build_filename(path, filename, NULL);
        g_free(path);
        path = temp;
    }

    return path;
}

Util::ptr_shared<char> get_path(Domain domain, Type type, char const *filename)
{
    char *path = _get_path(domain, type, filename);
    Util::ptr_shared<char> result=Util::share_string(path);
    g_free(path);
    return result;
}

/*
 * Same as get_path, but checks for file's existance and falls back
 * from USER to SYSTEM modes.
 */
Util::ptr_shared<char> get_filename(Type type, char const *filename)
{
    Util::ptr_shared<char> result;
    char *user_filename = _get_path(USER, type, filename);
    char *sys_filename = _get_path(SYSTEM, type, filename);

    if (file_test(user_filename, G_FILE_TEST_EXISTS)) {
        result = Util::share_string(user_filename);
    } else if(file_test(sys_filename, G_FILE_TEST_EXISTS)) {
        result = Util::share_string(sys_filename);
    } else {
        g_warning("Failed to load resource: %s", filename);
    }
    g_free(user_filename);
    g_free(sys_filename);
    return result;
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

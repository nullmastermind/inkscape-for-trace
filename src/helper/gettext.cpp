/**
 * \file
 * \brief helper functions for gettext
 *//*
 * Authors:
 *   Eduard Braun <eduard.braun2@gmx.de>
 *
 * Copyright 2017 Authors
 *
 * This file is part of Inkscape.
 *
 * Inkscape is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Inkscape is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Inkscape.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include <string>
#include <glibmm.h>
#include <glibmm/i18n.h>

namespace Inkscape {

/** does all required gettext initialization and takes care of the respective locale directory paths */
void initialize_gettext() {
#ifdef WIN32
    gchar *datadir = g_win32_get_package_installation_directory_of_module(NULL);

    // obtain short path to executable dir and pass it
    // to bindtextdomain (it doesn't understand UTF-8)
    gchar *shortdatadir = g_win32_locale_filename_from_utf8(datadir);
    gchar *localepath = g_build_filename(shortdatadir, PACKAGE_LOCALE_DIR, NULL);
    bindtextdomain(GETTEXT_PACKAGE, localepath);
    g_free(shortdatadir);
    g_free(localepath);

    g_free(datadir);
#else
# ifdef ENABLE_BINRELOC
    bindtextdomain(GETTEXT_PACKAGE, BR_LOCALEDIR(""));
# else
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    // needed by Python/Gettext
    g_setenv("PACKAGE_LOCALE_DIR", PACKAGE_LOCALE_DIR, TRUE);
# endif
#endif

    // Allow the user to override the locale directory by setting
    // the environment variable INKSCAPE_LOCALEDIR.
    char const *inkscape_localedir = g_getenv("INKSCAPE_LOCALEDIR");
    if (inkscape_localedir != NULL) {
        bindtextdomain(GETTEXT_PACKAGE, inkscape_localedir);
    }

    // common setup
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
}

/** set gettext codeset to UTF8 */
void bind_textdomain_codeset_utf8() {
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
}

/** set gettext codeset to codeset of the system console
 *   - on *nix this is typically the current locale,
 *   - on Windows we don't care and simply use UTF8
 *       any conversion would need to happen in our console wrappers (see winconsole.cpp) anyway
 *       as we have no easy way of determining console encoding from inkscape/inkview.exe process;
 *       for now do something even easier - switch console encoding to UTF8 and be done with it!
 *     this also works nicely on MSYS consoles where UTF8 encoding is used by default, too  */
void bind_textdomain_codeset_console() {
#ifdef WIN32
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#else
    std::string charset;
    Glib::get_charset(charset);
    bind_textdomain_codeset(GETTEXT_PACKAGE, charset.c_str());
#endif
}

}


/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace .0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99:
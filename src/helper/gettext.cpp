// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * helper functions for gettext
 *//*
 * Authors:
 * see git history
 *   Patrick Storz <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef ENABLE_BINRELOC
#include "prefix.h"
#endif

#include <string>
#include <glibmm.h>
#include <glibmm/i18n.h>

namespace Inkscape {

/** does all required gettext initialization and takes care of the respective locale directory paths */
void initialize_gettext() {
#ifdef _WIN32
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
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR_ABSOLUTE);
# endif
#endif

    // Allow the user to override the locale directory by setting
    // the environment variable INKSCAPE_LOCALEDIR.
    char const *inkscape_localedir = g_getenv("INKSCAPE_LOCALEDIR");
    if (inkscape_localedir != nullptr) {
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
#ifdef _WIN32
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

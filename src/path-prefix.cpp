// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * path-prefix.cpp - Inkscape specific prefix handling *//*
 * Authors:
 *   Patrick Storz <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#ifdef _WIN32
#include <windows.h> // for GetModuleFileNameW
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h> // for _NSGetExecutablePath
#endif

#include <glib.h>

#include "path-prefix.h"

/**
 * Determine the location of the Inkscape data directory (typically the share/ folder
 * from where Inkscape should be loading resources) and append a relative path
 *
 *  - by default use the compile time value of INKSCAPE_DATADIR
 *  - on Windows inkscape_datadir will be relative to the called executable by default
 *    (typically inkscape/share but also handles the case where the executable is in a /bin subfolder)
 *  - if the environment variable INKSCAPE_DATADIR is set it will override all of the above
 */
char *append_inkscape_datadir(const char *relative_path)
{
    static gchar const *inkscape_datadir;
    if (!inkscape_datadir) {
        gchar *datadir;
        gchar const *datadir_env = g_getenv("INKSCAPE_DATADIR");
        if (datadir_env) {
            datadir = g_strdup(datadir_env);
        } else {
#ifdef _WIN32
            gchar *module_path = g_win32_get_package_installation_directory_of_module(NULL);
            datadir = g_build_filename(module_path, "share", NULL);
            g_free(module_path);
#elif defined(__APPLE__)
            gchar *program_dir = get_program_dir();
            if (g_str_has_suffix(program_dir, "Contents/MacOS")) {
                datadir = g_build_filename(program_dir, "../Resources/share", nullptr);
            } else {
                datadir = g_strdup(INKSCAPE_DATADIR);
            }
            g_free(program_dir);
#else
            datadir = g_strdup(INKSCAPE_DATADIR);
#endif
        }

#if GLIB_CHECK_VERSION(2,58,0)
        inkscape_datadir = g_canonicalize_filename(datadir, NULL);
        g_free(datadir);
#else
        inkscape_datadir = datadir;
#endif
    }

    if (!relative_path) {
        relative_path = "";
    }

#if GLIB_CHECK_VERSION(2,58,0)
    return g_canonicalize_filename(relative_path, inkscape_datadir);
#else
    return g_build_filename(inkscape_datadir, relative_path, NULL);
#endif
}

/**
 * Gets the the currently running program's executable name (including full path)
 *
 * @return executable name (including full path) encoded as UTF-8
 *         or NULL if it can't be determined
 */
gchar *get_program_name()
{
    static gchar *program_name = NULL;

    if (program_name == NULL) {
        // There is no portable way to get an executable's name including path, so we need to do it manually.
        // TODO: Re-evaluate boost::dll::program_location() once we require Boost >= 1.61
        //
        // The following platform-specific code is partially based on GdxPixbuf's get_toplevel()
        // See also https://stackoverflow.com/a/1024937
#if defined(_WIN32)
        wchar_t module_file_name[MAX_PATH];
        if (GetModuleFileNameW(NULL, module_file_name, MAX_PATH)) {
            program_name = g_utf16_to_utf8((gunichar2 *)module_file_name, -1, NULL, NULL, NULL);
        } else {
            g_warning("get_program_name() - GetModuleFileNameW failed");
        }
#elif defined(__APPLE__)
        char pathbuf[PATH_MAX + 1];
        uint32_t bufsize = sizeof(pathbuf);
        if (_NSGetExecutablePath(pathbuf, &bufsize) == 0) {
            program_name = realpath(pathbuf, nullptr);
        } else {
            g_warning("get_program_name() - _NSGetExecutablePath failed");
        }
#elif defined(__linux__)
        program_name = g_file_read_link("/proc/self/exe", NULL);
        if (!program_name) {
            g_warning("get_program_name() - g_file_read_link failed");
        }
#else
#warning get_program_name() - no known way to obtain executable name on this platform
        g_info("get_program_name() - no known way to obtain executable name on this platform");
#endif
    }

    return program_name;
}

/**
 * Gets the the full path to the directory containing the currently running program's executable
 *
 * @return full path to directory encoded as UTF-8
 *         or NULL if it can't be determined
 */
gchar *get_program_dir()
{
    return g_path_get_dirname(get_program_name());
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

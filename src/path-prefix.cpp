/*
 * path-prefix.cpp - Inkscape specific prefix handling
 *
 * Authors:
 *   Eduard Braun <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2017 Authors
 *
 * This file is part of Inkscape.
 *
 * Inkscape is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * See the file COPYING for details.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "io/resource.h"
#include "path-prefix.h"
#include <glib.h>


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
        gchar const *datadir_env = g_getenv("INKSCAPE_DATADIR");
        if (datadir_env) {
#if GLIB_CHECK_VERSION(2,58,0)
            inkscape_datadir = g_canonicalize_filename(datadir_env, NULL);
#else
            inkscape_datadir = g_strdup(datadir_env);
#endif
        } else {
#ifdef _WIN32
            gchar *module_path = g_win32_get_package_installation_directory_of_module(NULL);
            inkscape_datadir = g_build_filename(module_path, "share", NULL);
            g_free(module_path);
#else
            inkscape_datadir = INKSCAPE_DATADIR;
#endif
        }
    }

    if (!relative_path) {
        relative_path = "";
    }

    return g_build_filename(inkscape_datadir, relative_path, NULL);
}

gchar *get_extensions_path()
{
    using namespace Inkscape::IO::Resource;
    gchar const *pythonpath = g_getenv("PYTHONPATH");
    gchar *extdir;
    gchar *new_pythonpath;

#ifdef _WIN32
    extdir = g_win32_locale_filename_from_utf8(INKSCAPE_EXTENSIONDIR);
#else
    extdir = g_strdup(INKSCAPE_EXTENSIONDIR);
#endif

    // On some platforms, INKSCAPE_EXTENSIONDIR is not absolute,
    // but relative to the directory that contains the Inkscape executable.
    // Since we spawn Python chdir'ed into the script's directory,
    // we need to obtain the absolute path here.
    if (!g_path_is_absolute(extdir)) {
        gchar *curdir = g_get_current_dir();
        gchar *extdir_new = g_build_filename(curdir, extdir, NULL);
        g_free(extdir);
        g_free(curdir);
        extdir = extdir_new;
    }

    if (pythonpath) {
        new_pythonpath = g_strdup_printf("%s" G_SEARCHPATH_SEPARATOR_S "%s", extdir, pythonpath);
        g_free(extdir);
    }
    else {
        new_pythonpath = extdir;
    }

    return new_pythonpath;
}

gchar *get_datadir_path()
{
    using namespace Inkscape::IO::Resource;
    gchar *datadir;

#ifdef _WIN32
    datadir = g_win32_locale_filename_from_utf8(profile_path(""));
#else
    datadir = profile_path("");
#endif

    // On some platforms, INKSCAPE_EXTENSIONDIR is not absolute,
    // but relative to the directory that contains the Inkscape executable.
    // Since we spawn Python chdir'ed into the script's directory,
    // we need to obtain the absolute path here.
    if (!g_path_is_absolute(datadir)) {
        gchar *curdir = g_get_current_dir();
        gchar *datadir_new = g_build_filename(curdir, datadir, NULL);
        g_free(datadir);
        g_free(curdir);
        datadir = datadir_new;
    }

    return datadir;
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

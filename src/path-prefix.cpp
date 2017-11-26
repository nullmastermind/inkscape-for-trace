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


#ifdef __WIN32__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <glib.h>
#include "path-prefix.h"


/**
 * Provide a similar mechanism for Win32.  Enable a macro,
 * WIN32_DATADIR, that can look up subpaths for inkscape resources
 */

/**
 * Get the Windows-equivalent of INKSCAPE_DATADIR and append a relative path
 *
 *  - by default INKSCAPE_DATADIR will be relative to the called executable
 *    (typically inkscape/share but also handles the case where the executable is in a /bin subfolder)
 *  - to override set the INKSCAPE_DATADIR environment variable
 */
char *win32_append_datadir(const char *relative_path)
{
    static gchar *datadir;
    if (!datadir) {
        gchar const *inkscape_datadir = g_getenv("INKSCAPE_DATADIR");
        if (inkscape_datadir) {
            datadir = g_strdup(inkscape_datadir);
        } else {
            gchar *module_path = g_win32_get_package_installation_directory_of_module(NULL);
            datadir = g_build_filename(module_path, "share", NULL);
            g_free(module_path);
        }
    }

    if (!relative_path) {
        relative_path = "";
    }

    return g_build_filename(datadir, relative_path, NULL);
}
#endif /* __WIN32__ */


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

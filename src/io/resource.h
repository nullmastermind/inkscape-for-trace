// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Martin Owens <doctormo@gmail.com>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_IO_RESOURCE_H
#define SEEN_INKSCAPE_IO_RESOURCE_H

#include <vector>
#include <glibmm/ustring.h>
#include "util/share.h"

namespace Inkscape {

namespace IO {

/**
 * simple resource API
 */
namespace Resource {

enum Type {
    EXTENSIONS,
    FONTS,
    ICONS,
    KEYS,
    MARKERS,
    NONE,
    PAINT,
    PALETTES,
    SCREENS,
    TEMPLATES,
    TUTORIALS,
    SYMBOLS,
    FILTERS,
    THEMES,
    UIS,
    PIXMAPS,
};

enum Domain {
    SYSTEM,
    CREATE,
    CACHE,
    USER
};

Util::ptr_shared get_path(Domain domain, Type type,
                                char const *filename=nullptr);

Glib::ustring get_path_ustring(Domain domain, Type type,
                                char const *filename=nullptr);

Glib::ustring get_filename(Type type, char const *filename, bool localized = false, bool silent = false);
Glib::ustring get_filename(Glib::ustring path, Glib::ustring filename);

std::vector<Glib::ustring> get_filenames(Type type,
                                std::vector<const char *> extensions={},
                                std::vector<const char *> exclusions={});

std::vector<Glib::ustring> get_filenames(Domain domain, Type type,
                                std::vector<const char *> extensions={},
                                std::vector<const char *> exclusions={});

std::vector<Glib::ustring> get_filenames(Glib::ustring path,
                                std::vector<const char *> extensions={},
                                std::vector<const char *> exclusions={});

std::vector<Glib::ustring> get_foldernames(Type type, std::vector<const char *> exclusions = {});

std::vector<Glib::ustring> get_foldernames(Domain domain, Type type, std::vector<const char *> exclusions = {});

std::vector<Glib::ustring> get_foldernames(Glib::ustring path, std::vector<const char *> exclusions = {});

void get_filenames_from_path(std::vector<Glib::ustring> &files,
                              Glib::ustring path,
                              std::vector<const char *> extensions={},
                              std::vector<const char *> exclusions={});

void get_foldernames_from_path(std::vector<Glib::ustring> &files, Glib::ustring path,
                               std::vector<const char *> exclusions = {});


char *profile_path(const char *filename);
char *homedir_path(const char *filename);
char *log_path(const char *filename);

}

}

}

#endif
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

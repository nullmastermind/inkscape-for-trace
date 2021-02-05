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
#ifndef UI_THEMES_H_SEEN
#define UI_THEMES_H_SEEN

#include <map>
#include <cstring>
#include <utility>
#include <vector>

#include <glibmm.h>

// Name of theme -> has dark theme
typedef std::map<Glib::ustring, bool> gtkThemeList;

gtkThemeList get_available_themes();

#endif /* !UI_THEMES_H_SEEN */

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

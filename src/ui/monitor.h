// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * \brief helper functions for retrieving monitor geometry, etc.
 *//*
 * Authors:
 * see git history
 *   Patrick Storz <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_MONITOR_H
#define SEEN_MONITOR_H

#include <gdkmm/rectangle.h>
#include <gdkmm/window.h>

namespace Inkscape {
namespace UI {
    Gdk::Rectangle get_monitor_geometry_primary();
    Gdk::Rectangle get_monitor_geometry_at_window(const Glib::RefPtr<Gdk::Window>& window);
    Gdk::Rectangle get_monitor_geometry_at_point(int x, int y);
}
}

#endif // SEEN_MONITOR_H

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

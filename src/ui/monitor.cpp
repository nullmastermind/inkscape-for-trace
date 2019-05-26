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

#include <gdkmm/monitor.h>
#include <gdkmm/rectangle.h>
#include <gdkmm/window.h>

#include "include/gtkmm_version.h"

namespace Inkscape {
namespace UI {

/** get monitor geometry of primary monitor */
Gdk::Rectangle get_monitor_geometry_primary() {
    Gdk::Rectangle monitor_geometry;
    auto const display = Gdk::Display::get_default();
    auto monitor = display->get_primary_monitor();

    // Fallback to monitor number 0 if the user hasn't configured a primary monitor
    if (!monitor) {
        monitor = display->get_monitor(0);
    }

    monitor->get_geometry(monitor_geometry);
    return monitor_geometry;
}

/** get monitor geometry of monitor containing largest part of window */
Gdk::Rectangle get_monitor_geometry_at_window(const Glib::RefPtr<Gdk::Window>& window) {
    Gdk::Rectangle monitor_geometry;
    auto const display = Gdk::Display::get_default();
    auto const monitor = display->get_monitor_at_window(window);
    monitor->get_geometry(monitor_geometry);
    return monitor_geometry;
}

/** get monitor geometry of monitor at (or closest to) point on combined screen area */
Gdk::Rectangle get_monitor_geometry_at_point(int x, int y) {
    Gdk::Rectangle monitor_geometry;
    auto const display = Gdk::Display::get_default();
    auto const monitor = display->get_monitor_at_point(x ,y);
    monitor->get_geometry(monitor_geometry);
    return monitor_geometry;
}

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

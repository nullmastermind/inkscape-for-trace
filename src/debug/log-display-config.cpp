// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::Debug::log_display_config - log display configuration
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2007 MenTaLguY
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <iostream>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include "debug/event-tracker.h"
#include "debug/logger.h"
#include "debug/simple-event.h"
#include "debug/log-display-config.h"

namespace Inkscape {

namespace Debug {

namespace {

typedef SimpleEvent<Event::CONFIGURATION> ConfigurationEvent;

class Monitor : public ConfigurationEvent {
public:
    Monitor(GdkMonitor *monitor)
        : ConfigurationEvent("monitor")
    {
        GdkRectangle area;
        gdk_monitor_get_geometry(monitor, &area);

        _addProperty("x", area.x);
        _addProperty("y", area.y);
        _addProperty("width", area.width);
        _addProperty("height", area.height);
    }
};

class Display : public ConfigurationEvent {
public:
    Display() : ConfigurationEvent("display") {}
    void generateChildEvents() const override {
        GdkDisplay *display=gdk_display_get_default();

        gint const n_monitors = gdk_display_get_n_monitors(display);

        // Loop through all monitors and log their details
        for (gint i_monitor = 0; i_monitor < n_monitors; ++i_monitor) {
            GdkMonitor *monitor = gdk_display_get_monitor(display, i_monitor);
            Logger::write<Monitor>(monitor);
        }
    }
};

}

void log_display_config() {
    Logger::write<Display>();
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

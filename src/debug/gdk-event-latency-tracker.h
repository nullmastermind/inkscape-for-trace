// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::Debug::GdkEventLatencyTracker - tracks backlog of GDK events
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2008 MenTaLguY
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_DEBUG_GDK_EVENT_LATENCY_TRACKER_H
#define SEEN_INKSCAPE_DEBUG_GDK_EVENT_LATENCY_TRACKER_H

typedef union _GdkEvent GdkEvent;
#include <glibmm/timer.h>
#include <optional>

namespace Inkscape {
namespace Debug {

class GdkEventLatencyTracker {
public:
    GdkEventLatencyTracker();
    std::optional<double> process(GdkEvent const *e);
    double maxLatency() const { return max_latency; }
    double getSkew();

    static GdkEventLatencyTracker &default_tracker();

private:
    GdkEventLatencyTracker(GdkEventLatencyTracker const &) = delete; // no copy
    void operator=(GdkEventLatencyTracker const &) = delete; // no assign

    double start_seconds;
    double max_latency;
    double skew;
    double last_elapsed;
    double last_seconds;
    Glib::Timer elapsed;
};

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

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * This is the C++ glue between Inkscape and Autotrace
 *//*
 *
 * Authors:
 *   Marc Jeanmougin
 *
 * Copyright (C) 2018 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 */

#include "inkscape-autotrace.h"

extern "C" {
#include "3rdparty/autotrace/autotrace.h"
#include "3rdparty/autotrace/output.h"
#include "3rdparty/autotrace/spline.h"
}

#include <glibmm/i18n.h>
#include <gtkmm/main.h>
#include <iomanip>

#include "trace/filterset.h"
#include "trace/imagemap-gdk.h"
#include "trace/quantize.h"

#include "desktop.h"
#include "message-stack.h"
#include <inkscape.h>

#include "object/sp-path.h"

#include <svg/path-string.h>

using Glib::ustring;

static void updateGui()
{
    //## Allow the GUI to update
    Gtk::Main::iteration(false); // at least once, non-blocking
    while (Gtk::Main::events_pending())
        Gtk::Main::iteration();
}

namespace Inkscape {

namespace Trace {

namespace Autotrace {


/**
 *
 */
AutotraceTracingEngine::AutotraceTracingEngine()
    : keepGoing(1)
    , traceType(TRACE_OUTLINE)
    , invert(false)
{
    /* get default parameters */
    opts = at_fitting_opts_new();
}

AutotraceTracingEngine::~AutotraceTracingEngine() { at_fitting_opts_free(opts); }



// TODO
Glib::RefPtr<Gdk::Pixbuf> AutotraceTracingEngine::preview(Glib::RefPtr<Gdk::Pixbuf> thePixbuf) { return thePixbuf; }

/**
 *  This is the working method of this interface, and all
 *  implementing classes.  Take a GdkPixbuf, trace it, and
 *  return the path data that is compatible with the d="" attribute
 *  of an SVG <path> element.
 */
std::vector<TracingEngineResult> AutotraceTracingEngine::trace(Glib::RefPtr<Gdk::Pixbuf> pixbuf)
{
    GdkPixbuf *pb = pixbuf->gobj();
    
    at_bitmap *bitmap =
        at_bitmap_new(gdk_pixbuf_get_width(pb), gdk_pixbuf_get_height(pb), gdk_pixbuf_get_n_channels(pb));
    bitmap->bitmap = gdk_pixbuf_get_pixels(pb);
    
    at_splines_type *splines = at_splines_new_full(bitmap, opts, NULL, NULL, NULL, NULL, NULL, NULL);
    // at_output_write_func wfunc = at_output_get_handler_by_suffix("svg");
    at_spline_writer *wfunc = at_output_get_handler_by_suffix("svg");


    int height = splines->height;
    // const at_splines_type spline = *splines;
    at_spline_list_array_type spline = *splines;

    unsigned this_list;
    at_spline_list_type list;
    at_color last_color = { 0, 0, 0 };

    std::stringstream theStyle;
    std::stringstream thePath;
    char color[10];
    int nNodes = 0;

    std::vector<TracingEngineResult> res;

    // at_splines_write(wfunc, stdout, "", NULL, splines, NULL, NULL);

    for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(spline); this_list++) {
        unsigned this_spline;
        at_spline_type first;

        list = SPLINE_LIST_ARRAY_ELT(spline, this_list);
        first = SPLINE_LIST_ELT(list, 0);

        if (this_list == 0 || !at_color_equal(&list.color, &last_color)) {
            if (this_list > 0) {
                if (!(spline.centerline || list.open)) {
                    thePath << "z";
                    nNodes++;
                }
                TracingEngineResult ter(theStyle.str(), thePath.str(), nNodes);
                res.push_back(ter);
                theStyle.clear();
                thePath.clear();
                nNodes = 0;
            }
            sprintf(color, "#%02x%02x%02x;", list.color.r, list.color.g, list.color.b);

            theStyle << ((spline.centerline || list.open) ? "stroke:" : "fill:") << color
                     << ((spline.centerline || list.open) ? "fill:" : "stroke:") << "none";
        }
        thePath << "M" << START_POINT(first).x << " " << height - START_POINT(first).y;
        nNodes++;
        for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(list); this_spline++) {
            at_spline_type s = SPLINE_LIST_ELT(list, this_spline);

            if (SPLINE_DEGREE(s) == AT_LINEARTYPE) {
                thePath << "L" << END_POINT(s).x << " " << height - END_POINT(s).y;
                nNodes++;
            }
            else {
                thePath << "C" << CONTROL1(s).x << " " << height - CONTROL1(s).y << " " << CONTROL2(s).x << " "
                        << height - CONTROL2(s).y << " " << END_POINT(s).x << " " << height - END_POINT(s).y;
                nNodes++;
            }
            last_color = list.color;
        }
    }
    if (!(spline.centerline || list.open))
        thePath << "z";
    nNodes++;
    if (SPLINE_LIST_ARRAY_LENGTH(spline) > 0) {
        TracingEngineResult ter(theStyle.str(), thePath.str(), nNodes);
        res.push_back(ter);
        theStyle.clear();
        thePath.clear();
        nNodes = 0;
    }

    return res;
}


/**
 *  Abort the thread that is executing getPathDataFromPixbuf()
 */
void AutotraceTracingEngine::abort()
{
    // g_message("PotraceTracingEngine::abort()\n");
    keepGoing = 0;
}



} // namespace Autotrace
} // namespace Trace
} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

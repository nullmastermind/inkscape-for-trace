// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This is the C++ glue between Inkscape and Potrace
 *
 * Copyright (C) 2019 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 * Potrace, the wonderful tracer located at http://potrace.sourceforge.net,
 * is provided by the generosity of Peter Selinger, to whom we are grateful.
 *
 */

#ifndef __INKSCAPE_DEPIXTRACE_H__
#define __INKSCAPE_DEPIXTRACE_H__

#include <trace/trace.h>
#include "3rdparty/libdepixelize/kopftracer2011.h"

struct GrayMap_def;
typedef GrayMap_def GrayMap;

namespace Inkscape {

namespace Trace {

namespace Depixelize {

enum TraceType
    {
    TRACE_VORONOI,
    TRACE_BSPLINES
    };


class DepixelizeTracingEngine : public TracingEngine
{

    public:

    /**
     *
     */
    DepixelizeTracingEngine();
    DepixelizeTracingEngine(TraceType traceType, double curves, int islands, int sparsePixels, double sparseMultiplier, bool optimize);

    /**
     *
     */
    ~DepixelizeTracingEngine() override;

    /**
     *  This is the working method of this implementing class, and all
     *  implementing classes.  Take a GdkPixbuf, trace it, and
     *  return the path data that is compatible with the d="" attribute
     *  of an SVG <path> element.
     */
    std::vector<TracingEngineResult> trace(
                        Glib::RefPtr<Gdk::Pixbuf> pixbuf) override;

    /**
     *  Abort the thread that is executing getPathDataFromPixbuf()
     */
    void abort() override;

    /**
     *
     */
    Glib::RefPtr<Gdk::Pixbuf> preview(Glib::RefPtr<Gdk::Pixbuf> pixbuf);

    /**
     *
     */
    int keepGoing;

    ::Tracer::Kopf2011::Options *params;
    TraceType traceType;

};//class PotraceTracingEngine



}  // namespace Depixelize
}  // namespace Trace
}  // namespace Inkscape


#endif  //__INKSCAPE_TRACE_H__


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

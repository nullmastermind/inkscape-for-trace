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
 * Autotrace is available at http://github.com/autotrace/autotrace.
 *
 */

#ifndef __INKSCAPE_AUTOTRACE_H__
#define __INKSCAPE_AUTOTRACE_H__

#include "3rdparty/autotrace/autotrace.h"
#include <trace/trace.h>

namespace Inkscape {

namespace Trace {

namespace Autotrace {
enum TraceType { TRACE_CENTERLINE, TRACE_OUTLINE };

class AutotraceTracingEngine : public TracingEngine {

  public:
    /**
     *
     */
    AutotraceTracingEngine();

    /**
     *
     */
    ~AutotraceTracingEngine() override;


    /**
     * Sets/gets parameters
     */
    // TODO

    /**
     *  This is the working method of this implementing class, and all
     *  implementing classes.  Take a GdkPixbuf, trace it, and
     *  return the path data that is compatible with the d="" attribute
     *  of an SVG <path> element.
     */
    std::vector<TracingEngineResult> trace(Glib::RefPtr<Gdk::Pixbuf> pixbuf) override;

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

  //private:
    // autotrace_param_t *autotraceParams;
    TraceType traceType;
    at_fitting_opts_type *opts;

    //## do I invert at the end?
    bool invert;

}; // class AutotraceTracingEngine



} // namespace Autotrace
} // namespace Trace
} // namespace Inkscape


#endif //__INKSCAPE_POTRACE_H__


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

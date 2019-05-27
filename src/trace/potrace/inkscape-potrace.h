// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This is the C++ glue between Inkscape and Potrace
 *
 * Authors:
 *   Bob Jamison <rjamison@titan.com>
 *   Stéphane Gimenez <dev@gim.name>
 *
 * Copyright (C) 2004-2006 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 * Potrace, the wonderful tracer located at http://potrace.sourceforge.net,
 * is provided by the generosity of Peter Selinger, to whom we are grateful.
 *
 */

#ifndef __INKSCAPE_POTRACE_H__
#define __INKSCAPE_POTRACE_H__

#include <trace/trace.h>
#include <potracelib.h>

struct GrayMap_def;
typedef GrayMap_def GrayMap;

namespace Inkscape {

namespace Trace {

namespace Potrace {

enum TraceType
    {
    TRACE_BRIGHTNESS,
    TRACE_BRIGHTNESS_MULTI,
    TRACE_CANNY,
    TRACE_QUANT,
    TRACE_QUANT_COLOR,
    TRACE_QUANT_MONO
    };


class PotraceTracingEngine : public TracingEngine
{

    public:

    /**
     *
     */
    PotraceTracingEngine();
    PotraceTracingEngine(TraceType traceType, bool invert, int quantizationNrColors, double brightnessThreshold, double brightnessFloor, double cannyHighThreshold, int multiScanNrColors, bool multiScanStack, bool multiScanSmooth, bool multiScanRemoveBackground);

    /**
     *
     */
    ~PotraceTracingEngine() override;

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

    std::vector<TracingEngineResult>traceGrayMap(GrayMap *grayMap);

    potrace_param_t *potraceParams;
    TraceType traceType;

    //## do I invert at the end?
    bool invert;

    //## Color-->b&w quantization
    int quantizationNrColors;

    //## brightness items
    double brightnessThreshold;
    double brightnessFloor; //usually 0.0

    //## canny items
    double cannyHighThreshold;

    //## Color-->multiscan quantization
    int multiScanNrColors;
    bool multiScanStack; //do we tile or stack?
    bool multiScanSmooth;//do we use gaussian filter?
    bool multiScanRemoveBackground; //do we remove the bottom trace?
    
    private:
    /**
     * This is the actual wrapper of the call to Potrace.  nodeCount
     * returns the count of nodes created.  May be NULL if ignored.
     */
    std::string grayMapToPath(GrayMap *gm, long *nodeCount);

    std::vector<TracingEngineResult>traceBrightnessMulti(GdkPixbuf *pixbuf);
    std::vector<TracingEngineResult>traceQuant(GdkPixbuf *pixbuf);
    std::vector<TracingEngineResult>traceSingle(GdkPixbuf *pixbuf);


};//class PotraceTracingEngine



}  // namespace Potrace
}  // namespace Trace
}  // namespace Inkscape


#endif  //__INKSCAPE_POTRACE_H__


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

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

#include "inkscape-depixelize.h"

#include <glibmm/i18n.h>
#include <gtkmm/main.h>
#include <gtkmm.h>
#include <iomanip>

#include "desktop.h"
#include "message-stack.h"
#include "helper/geom.h"
#include "object/sp-path.h"

#include <svg/path-string.h>
#include <svg/svg.h>
#include <svg/svg-color.h>
#include "svg/css-ostringstream.h"

using Glib::ustring;



namespace Inkscape {

namespace Trace {

namespace Depixelize {


/**
 *
 */
DepixelizeTracingEngine::DepixelizeTracingEngine()
    : keepGoing(1)
    , traceType(TRACE_VORONOI)
{
    params = new ::Tracer::Kopf2011::Options();
}



DepixelizeTracingEngine::DepixelizeTracingEngine(TraceType traceType, double curves, int islands, int sparsePixels,
                                                 double sparseMultiplier)
    : keepGoing(1)
    , traceType(traceType)
{
    params = new ::Tracer::Kopf2011::Options();
    params->curvesMultiplier = curves;
    params->islandsWeight = islands;
    params->sparsePixelsRadius = sparsePixels;
    params->sparsePixelsMultiplier = sparseMultiplier;
    params->nthreads = Inkscape::Preferences::get()->getIntLimited("/options/threading/numthreads",
#ifdef HAVE_OPENMP
                                                                   omp_get_num_procs(),
#else
                                                                   1,
#endif // HAVE_OPENMP
                                                                   1, 256);
}

DepixelizeTracingEngine::~DepixelizeTracingEngine() { delete params; }

std::vector<TracingEngineResult> DepixelizeTracingEngine::trace(Glib::RefPtr<Gdk::Pixbuf> pixbuf)
{
    if (pixbuf->get_width() > 256 || pixbuf->get_height() > 256) {
        char *msg = _("Image looks too big. Process may take a while and it is"
                      " wise to save your document before continuing."
                      "\n\nContinue the procedure (without saving)?");
        Gtk::MessageDialog dialog(msg, false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL, true);

//        if (dialog.run() != Gtk::RESPONSE_OK)
//            return;
//            TODO
    }

    ::Tracer::Splines splines;

    if (traceType == TRACE_VORONOI)
        splines = ::Tracer::Kopf2011::to_voronoi(pixbuf, *params);
    else
        splines = ::Tracer::Kopf2011::to_splines(pixbuf, *params);

    std::vector<TracingEngineResult> res;

    for (::Tracer::Splines::const_iterator it = splines.begin(), end = splines.end(); it != end; ++it) {
                gchar b[64];
                sp_svg_write_color(b, sizeof(b),
                                   SP_RGBA32_U_COMPOSE(unsigned(it->rgba[0]),
                                                       unsigned(it->rgba[1]),
                                                       unsigned(it->rgba[2]),
                                                       unsigned(it->rgba[3])));
        Inkscape::CSSOStringStream osalpha;
        osalpha << float(it->rgba[3]) / 255.;
        gchar* style = g_strdup_printf("fill:%s;fill-opacity:%s;", b, osalpha.str().c_str());
        printf("%s\n", style);
        TracingEngineResult r(style, sp_svg_write_path(it->pathVector), count_pathvector_nodes(it->pathVector));
        res.push_back(r);
        g_free(style);
    }
    return res;
}

void DepixelizeTracingEngine::abort() { keepGoing = 0; }

Glib::RefPtr<Gdk::Pixbuf> DepixelizeTracingEngine::preview(Glib::RefPtr<Gdk::Pixbuf> pixbuf) { return pixbuf; }


} // namespace Depixelize
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

/*
 * Author(s):
 *   Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2014 Author(s)

 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#include "live_effects/lpe-measure-line.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

using namespace Geom;
namespace Inkscape {
namespace LivePathEffect {

LPEMeasureLine::LPEMeasureLine(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    text(_("text:"), _("text"), "text", &wr, this, "This is a labelXXX")
{
    registerParameter(&text);
}

LPEMeasureLine::~LPEMeasureLine() {}

Geom::PathVector
LPEMeasureLine::doEffect_path(Geom::PathVector const &path_in)
{
    return path_in;
}

}; //namespace LivePathEffect
}; /* namespace Inkscape */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offset:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

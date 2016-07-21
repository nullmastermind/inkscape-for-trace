#ifndef INKSCAPE_LPE_MEASURE_LINE_H
#define INKSCAPE_LPE_MEASURE_LINE_H

/*
 * Author(s):
 *     Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2014 Author(s)
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#include "live_effects/parameter/texttopath.h"
#include "live_effects/effect.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEMeasureLine : public Effect {
public:
    LPEMeasureLine(LivePathEffectObject *lpeobject);
    virtual ~LPEMeasureLine();

    virtual Geom::PathVector doEffect_path(Geom::PathVector const &path_in);

private:

    TextToPathParam text;

    LPEMeasureLine(const LPEMeasureLine &);
    LPEMeasureLine &operator=(const LPEMeasureLine &);

};

} //namespace LivePathEffect
} //namespace Inkscape

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

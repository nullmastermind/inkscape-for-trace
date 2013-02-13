#ifndef INKSCAPE_LPE_BSPLINE_H
#define INKSCAPE_LPE_BSPLINE_H

/*
 * Inkscape::LPEBSpline
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/effect.h"


namespace Inkscape {
namespace LivePathEffect {

class LPEBSpline : public Effect {
public:
    LPEBSpline(LivePathEffectObject *lpeobject);
    virtual ~LPEBSpline();

    virtual LPEPathFlashType pathFlashType() const { return SUPPRESS_FLASH; }

    virtual void doEffect(SPCurve * curve);

private:
    LPEBSpline(const LPEBSpline&);
    LPEBSpline& operator=(const LPEBSpline&);
};

}; //namespace LivePathEffect
}; //namespace Inkscape

#endif

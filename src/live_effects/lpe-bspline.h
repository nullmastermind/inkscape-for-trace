#ifndef INKSCAPE_LPE_BSPLINE_H
#define INKSCAPE_LPE_BSPLINE_H

/*
 * Inkscape::LPEBSpline
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/effect.h"
#include "live_effects/parameter/parameter.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEBSpline : public Effect {
public:
    LPEBSpline(LivePathEffectObject *lpeobject);
    virtual ~LPEBSpline();

    virtual std::vector<Geom::Path> doEffect_path (std::vector<Geom::Path> const & input_path);

    virtual LPEPathFlashType pathFlashType() const { return SUPPRESS_FLASH; }

    virtual void doEffect(SPCurve * curve);
    
    virtual void doEffect(SPCurve * curve, int value);
    
    virtual void updateAllHandles(int value);
    
    virtual void updateHandles(SPItem * item , int value);

private:
    ScalarParam unify_weights;
    LPEBSpline(const LPEBSpline&);
    LPEBSpline& operator=(const LPEBSpline&);
};

}; //namespace LivePathEffect
}; //namespace Inkscape

#endif

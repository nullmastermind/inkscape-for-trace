#ifndef INKSCAPE_LPE_BSPLINE_H
#define INKSCAPE_LPE_BSPLINE_H

/*
 * Inkscape::LPEBSpline
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/effect.h"
#include "live_effects/parameter/bool.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEBSpline : public Effect {

private:
    BoolParam ignoreCusp;
    ScalarParam weight;

    LPEBSpline(const LPEBSpline&);
    LPEBSpline& operator=(const LPEBSpline&);

public:
    LPEBSpline(LivePathEffectObject *lpeobject);
    virtual ~LPEBSpline();

    virtual LPEPathFlashType pathFlashType() const { return SUPPRESS_FLASH; }

    virtual void doOnApply(SPLPEItem const* lpeitem);

    virtual void doEffect(SPCurve * curve);

    virtual void doBSplineFromWidget(SPCurve * curve, double value, bool noCusp);

    virtual Gtk::Widget * newWidget();

    virtual void changeWeight(double weightValue);

    virtual void toDefaultWeight();

    virtual void toWeight();

    ScalarParam steps;

};

}; //namespace LivePathEffect
}; //namespace Inkscape
#endif

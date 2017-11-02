#ifndef INKSCAPE_LPE_POWERMASK_H
#define INKSCAPE_LPE_POWERMASK_H

/*
 * Inkscape::LPEPowerMask
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/effect.h"
#include "live_effects/parameter/text.h"
#include "live_effects/parameter/hidden.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEPowerMask : public Effect {
public:
    LPEPowerMask(LivePathEffectObject *lpeobject);
    virtual ~LPEPowerMask();
    virtual void doBeforeEffect (SPLPEItem const* lpeitem);
    virtual void doEffect (SPCurve * curve);
    virtual void doOnRemove (SPLPEItem const* /*lpeitem*/);
    virtual void doOnVisibilityToggled(SPLPEItem const* lpeitem);
    //virtual void transform_multiply(Geom::Affine const& postmul, bool set);
    void toggleMaskVisibility();
    void setMask();
private:
    HiddenParam uri;
    BoolParam invert;
    BoolParam wrap;
    BoolParam hide_mask;
    BoolParam background;
    TextParam background_style;
    Geom::Path mask_box;
};

void sp_inverse_powermask(Inkscape::Selection *sel);

} //namespace LivePathEffect
} //namespace Inkscape

#endif

// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LPE_POWERCLIP_H
#define INKSCAPE_LPE_POWERCLIP_H

/*
 * Inkscape::LPEPowerClip
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/effect.h"
#include "live_effects/parameter/message.h"
#include "live_effects/parameter/hidden.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEPowerClip : public Effect {
public:
    LPEPowerClip(LivePathEffectObject *lpeobject);
    ~LPEPowerClip() override;
    void doOnApply (SPLPEItem const * lpeitem);
    void doBeforeEffect (SPLPEItem const* lpeitem) override;
    Geom::PathVector doEffect_path (Geom::PathVector const & path_in) override;
    void doOnRemove (SPLPEItem const* /*lpeitem*/) override;
    void doOnVisibilityToggled(SPLPEItem const* lpeitem) override;
    void doAfterEffect (SPLPEItem const* lpeitem) override;
    void addInverse (SPItem * clip_data, SPCurve * clipcurve, Geom::Affine affine, bool root);
    void updateInverse (SPItem * clip_data);
    void removeInverse (SPItem * clip_data);
    void flattenClip(SPItem * clip_data, Geom::PathVector &path_in);
private:
    HiddenParam is_inverse;
    HiddenParam uri;
    BoolParam inverse;
    BoolParam flatten;
    BoolParam hide_clip;
    MessageParam message;
    Geom::Path clip_box;
    Geom::Affine base;
    Geom::Affine lastapplied;
};

void sp_remove_powerclip(Inkscape::Selection *sel);
void sp_inverse_powerclip(Inkscape::Selection *sel);

} //namespace LivePathEffect
} //namespace Inkscape
#endif

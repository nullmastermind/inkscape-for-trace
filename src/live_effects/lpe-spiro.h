// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LPE_SPIRO_H
#define INKSCAPE_LPE_SPIRO_H

/*
 * Inkscape::LPESpiro
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/effect.h"


namespace Inkscape {
namespace LivePathEffect {

class LPESpiro : public Effect {
public:
    LPESpiro(LivePathEffectObject *lpeobject);
    ~LPESpiro() override;
    LPESpiro(const LPESpiro&) = delete;
    LPESpiro& operator=(const LPESpiro&) = delete;

    LPEPathFlashType pathFlashType() const override { return SUPPRESS_FLASH; }

    void doEffect(SPCurve * curve) override;
};

void sp_spiro_do_effect(SPCurve *curve);

}; //namespace LivePathEffect
}; //namespace Inkscape

#endif

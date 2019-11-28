// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LPE_BSPLINE_H
#define INKSCAPE_LPE_BSPLINE_H

/*
 * Inkscape::LPEBSpline
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/effect.h"
#include <vector>

namespace Inkscape {
namespace LivePathEffect {

class LPEBSpline : public Effect {
public:
    LPEBSpline(LivePathEffectObject *lpeobject);
    ~LPEBSpline() override;
    LPEBSpline(const LPEBSpline &) = delete;
    LPEBSpline &operator=(const LPEBSpline &) = delete;

    LPEPathFlashType pathFlashType() const override
    {
        return SUPPRESS_FLASH;
    }
    void doOnApply(SPLPEItem const* lpeitem) override;
    void doEffect(SPCurve *curve) override;
    void doBeforeEffect (SPLPEItem const* lpeitem) override;
    void doBSplineFromWidget(SPCurve *curve, double value);
    void addCanvasIndicators(SPLPEItem const */*lpeitem*/, std::vector<Geom::PathVector> &hp_vec) override;
    Gtk::Widget *newWidget() override;
    void changeWeight(double weightValue);
    void toDefaultWeight();
    void toMakeCusp();
    void toWeight();

    // TODO make this private
    ScalarParam steps;

private:
    ScalarParam helper_size;
    BoolParam apply_no_weight;
    BoolParam apply_with_weight;
    BoolParam only_selected;
    ScalarParam weight;
    Geom::PathVector hp;
};
void sp_bspline_do_effect(SPCurve *curve, double helper_size, Geom::PathVector &hp);

} //namespace LivePathEffect
} //namespace Inkscape
#endif

// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LPE_DASHED_STROKE_H
#define INKSCAPE_LPE_DASHED_STROKE_H

/*
 * Inkscape::LPEDashedStroke
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/effect.h"
#include "live_effects/parameter/message.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEDashedStroke : public Effect {
  public:
    LPEDashedStroke(LivePathEffectObject *lpeobject);
    ~LPEDashedStroke() override;
    void doBeforeEffect (SPLPEItem const* lpeitem) override;
    Geom::PathVector doEffect_path (Geom::PathVector const & path_in) override;
    double timeAtLength(double const A, Geom::Path const &segment);
    double timeAtLength(double const A, Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2);
private:
    ScalarParam numberdashes;
    ScalarParam holefactor;
    BoolParam splitsegments;
    BoolParam halfextreme;
    BoolParam unifysegment;
    MessageParam message;
};

} //namespace LivePathEffect
} //namespace Inkscape
#endif

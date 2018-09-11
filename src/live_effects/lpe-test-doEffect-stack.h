// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LPE_DOEFFECT_STACK_H
#define INKSCAPE_LPE_DOEFFECT_STACK_H

/*
 * Inkscape::LPEdoEffectStackTest
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 *   This effect is to test whether running up and down the doEffect stack does not change the original-d too much.
 *   i.e. for this effect, the output should match more or less exactly with the input.
 *
 */

#include "live_effects/effect.h"
#include "live_effects/parameter/parameter.h"
#include "live_effects/parameter/point.h"
#include "live_effects/parameter/path.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEdoEffectStackTest : public Effect {
public:
    LPEdoEffectStackTest(LivePathEffectObject *lpeobject);
    ~LPEdoEffectStackTest() override;

    void                                     doEffect (SPCurve * curve) override;
    Geom::PathVector                  doEffect_path (Geom::PathVector const & path_in) override;
    Geom::Piecewise<Geom::D2<Geom::SBasis> > doEffect_pwd2 (Geom::Piecewise<Geom::D2<Geom::SBasis> > const & pwd2_in) override;

private:
    ScalarParam step;
    PointParam point;
    PathParam path;

    LPEdoEffectStackTest(const LPEdoEffectStackTest&) = delete;
    LPEdoEffectStackTest& operator=(const LPEdoEffectStackTest&) = delete;
};

}; //namespace LivePathEffect
}; //namespace Inkscape

#endif

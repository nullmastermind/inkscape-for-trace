// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LPE_LATTICE_H
#define INKSCAPE_LPE_LATTICE_H

/** \file
 * LPE <lattice> implementation, see lpe-lattice.cpp.
 */

/*
 * Authors:
 *   Johan Engelen
 *   Steren Giannini
 *   Noé Falzon
 *   Victor Navez
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/parameter/enum.h"
#include "live_effects/effect.h"
#include "live_effects/parameter/point.h"
#include "live_effects/lpegroupbbox.h"

namespace Inkscape {
namespace LivePathEffect {

class LPELattice : public Effect, GroupBBoxEffect {
public:

    LPELattice(LivePathEffectObject *lpeobject);
    ~LPELattice() override;

    void doBeforeEffect (SPLPEItem const* lpeitem) override;

    Geom::Piecewise<Geom::D2<Geom::SBasis> > doEffect_pwd2 (Geom::Piecewise<Geom::D2<Geom::SBasis> > const & pwd2_in) override;
    
    void resetDefaults(SPItem const* item) override;

protected:
    //virtual void addHelperPathsImpl(SPLPEItem *lpeitem, SPDesktop *desktop);


private:
    PointParam grid_point0;
    PointParam grid_point1;
    PointParam grid_point2;
    PointParam grid_point3;
    PointParam grid_point4;
    PointParam grid_point5;
    PointParam grid_point6;
    PointParam grid_point7;
    PointParam grid_point8;
    PointParam grid_point9;
    PointParam grid_point10;
    PointParam grid_point11;
    PointParam grid_point12;
    PointParam grid_point13;
    PointParam grid_point14;
    PointParam grid_point15;
    LPELattice(const LPELattice&) = delete;
    LPELattice& operator=(const LPELattice&) = delete;
};

} //namespace LivePathEffect
} //namespace Inkscape

#endif

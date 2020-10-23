// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LPE_BENDPATH_H
#define INKSCAPE_LPE_BENDPATH_H

/*
 * Inkscape::LPEPathAlongPath
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 * Copyright (C) Steren Giannini 2008 <steren.giannini@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/parameter/enum.h"
#include "live_effects/effect.h"
#include "live_effects/parameter/path.h"
#include "live_effects/parameter/bool.h"

#include <2geom/sbasis.h>
#include <2geom/sbasis-geometric.h>
#include <2geom/bezier-to-sbasis.h>
#include <2geom/sbasis-to-bezier.h>
#include <2geom/d2.h>
#include <2geom/piecewise.h>

#include "live_effects/lpegroupbbox.h"

namespace Inkscape {
namespace UI {
namespace Toolbar {
class PencilToolbar;
}
} // namespace UI
namespace LivePathEffect {

namespace BeP {
class KnotHolderEntityWidthBendPath;
}

//for Bend path on group : we need information concerning the group Bounding box
class LPEBendPath : public Effect, GroupBBoxEffect {
public:
    LPEBendPath(LivePathEffectObject *lpeobject);
    ~LPEBendPath() override;

    void doBeforeEffect (SPLPEItem const* lpeitem) override;

    Geom::Piecewise<Geom::D2<Geom::SBasis> > doEffect_pwd2 (Geom::Piecewise<Geom::D2<Geom::SBasis> > const & pwd2_in) override;

    void resetDefaults(SPItem const* item) override;

    void transform_multiply(Geom::Affine const &postmul, bool set) override;

    void addCanvasIndicators(SPLPEItem const */*lpeitem*/, std::vector<Geom::PathVector> &hp_vec) override;
      
    void addKnotHolderEntities(KnotHolder * knotholder, SPItem * item) override;

    PathParam bend_path;

    friend class BeP::KnotHolderEntityWidthBendPath;
    friend class Inkscape::UI::Toolbar::PencilToolbar;

protected:
    double original_height;
    ScalarParam prop_scale;
private:
    BoolParam scale_y_rel;
    BoolParam vertical_pattern;
    BoolParam hide_knot;
    KnotHolderEntity * _knot_entity;
    Geom::PathVector helper_path;
    Geom::Piecewise<Geom::D2<Geom::SBasis> > uskeleton;
    Geom::Piecewise<Geom::D2<Geom::SBasis> > n;

    void on_pattern_pasted();

    LPEBendPath(const LPEBendPath&);
    LPEBendPath& operator=(const LPEBendPath&);
};

}; //namespace LivePathEffect
}; //namespace Inkscape

#endif

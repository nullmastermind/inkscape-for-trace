// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LPE_INTERPOLATE_H
#define INKSCAPE_LPE_INTERPOLATE_H

/** \file
 * LPE interpolate implementation, see lpe-interpolate.cpp.
 */

/*
 * Authors:
 *   Johan Engelen
 *
 * Copyright (C) Johan Engelen 2007-2008 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/effect.h"
#include "live_effects/parameter/bool.h"
#include "live_effects/parameter/parameter.h"
#include "live_effects/parameter/path.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEInterpolate : public Effect {
  public:
    LPEInterpolate(LivePathEffectObject *lpeobject);
    ~LPEInterpolate() override;

    Geom::PathVector doEffect_path(Geom::PathVector const &path_in) override;
    void transform_multiply(Geom::Affine const &postmul, bool set) override;

    void resetDefaults(SPItem const *item) override;

  private:
    PathParam trajectory_path;
    ScalarParam number_of_steps;
    BoolParam equidistant_spacing;

    Geom::Piecewise<Geom::D2<Geom::SBasis> > calculate_trajectory(Geom::OptRect bounds_A, Geom::OptRect bounds_B);

    LPEInterpolate(const LPEInterpolate &) = delete;
    LPEInterpolate &operator=(const LPEInterpolate &) = delete;
};

} // namespace LivePathEffect
} // namespace Inkscape

#endif // INKSCAPE_LPE_INTERPOLATE_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

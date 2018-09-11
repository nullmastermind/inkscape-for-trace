// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LPE_INTERPOLATEPOINTS_H
#define INKSCAPE_LPE_INTERPOLATEPOINTS_H

/** \file
 * LPE interpolate_points implementation, see lpe-interpolate_points.cpp.
 */

/*
 * Authors:
 *   Johan Engelen
 *
 * Copyright (C) Johan Engelen 2014 <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/parameter/enum.h"
#include "live_effects/effect.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEInterpolatePoints : public Effect {
public:
    LPEInterpolatePoints(LivePathEffectObject *lpeobject);
    ~LPEInterpolatePoints() override;

    Geom::PathVector doEffect_path (Geom::PathVector const & path_in) override;

private:
    EnumParam<unsigned> interpolator_type;

    LPEInterpolatePoints(const LPEInterpolatePoints&) = delete;
    LPEInterpolatePoints& operator=(const LPEInterpolatePoints&) = delete;
};

} //namespace LivePathEffect
} //namespace Inkscape

#endif  // INKSCAPE_LPE_INTERPOLATEPOINTS_H

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

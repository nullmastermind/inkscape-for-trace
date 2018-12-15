// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LPE_CURVESTITCH_H
#define INKSCAPE_LPE_CURVESTITCH_H

/** \file
 * Implementation of the curve stitch effect, see lpe-curvestitch.cpp
 */

/*
 * Authors:
 *   Johan Engelen
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/effect.h"
#include "live_effects/parameter/path.h"
#include "live_effects/parameter/parameter.h"
#include "live_effects/parameter/bool.h"
#include "live_effects/parameter/random.h"

namespace Inkscape {
namespace LivePathEffect {

class LPECurveStitch : public Effect {
public:
    LPECurveStitch(LivePathEffectObject *lpeobject);
    ~LPECurveStitch() override;

    Geom::PathVector doEffect_path (Geom::PathVector const & path_in) override;

    void resetDefaults(SPItem const* item) override;

private:
    PathParam strokepath;
    ScalarParam nrofpaths;
    RandomParam startpoint_edge_variation;
    RandomParam startpoint_spacing_variation;
    RandomParam endpoint_edge_variation;
    RandomParam endpoint_spacing_variation;
    ScalarParam prop_scale;
    BoolParam scale_y_rel;
    bool transformed;

    LPECurveStitch(const LPECurveStitch&) = delete;
    LPECurveStitch& operator=(const LPECurveStitch&) = delete;
};

} //namespace LivePathEffect
} //namespace Inkscape

#endif

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief LPE effect that draws a circle based on two points and a radius
 */
/* Authors:
 *   Johan Engelen <j.b.c.engelen@utwente.nl>
 *
 * Copyright (C) 2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_LPE_CIRCLE_WITH_RADIUS_H
#define INKSCAPE_LPE_CIRCLE_WITH_RADIUS_H

#include "live_effects/effect.h"
#include "live_effects/parameter/parameter.h"
#include "live_effects/parameter/path.h"
#include "live_effects/parameter/point.h"

namespace Inkscape {
namespace LivePathEffect {

class LPECircleWithRadius : public Effect {
public:
    LPECircleWithRadius(LivePathEffectObject *lpeobject);
    ~LPECircleWithRadius() override;

//  Choose to implement one of the doEffect functions. You can delete or comment out the others.
    Geom::PathVector doEffect_path (Geom::PathVector const & path_in) override;

private:
    // add the parameters for your effect here:
    //ScalarParam radius;
    // there are all kinds of parameters. Check the /live_effects/parameter directory which types exist!

    LPECircleWithRadius(const LPECircleWithRadius&) = delete;
    LPECircleWithRadius& operator=(const LPECircleWithRadius&) = delete;
};

} //namespace LivePathEffect
} //namespace Inkscape

#endif

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

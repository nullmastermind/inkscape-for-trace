// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief LPE effect for extruding paths (making them "3D").
 */
/* Authors:
 *   Johan Engelen <j.b.c.engelen@utwente.nl>
 *
 * Copyright (C) 2009 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_LPE_EXTRUDE_H
#define INKSCAPE_LPE_EXTRUDE_H

#include "live_effects/effect.h"
#include "live_effects/parameter/parameter.h"
#include "live_effects/parameter/vector.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEExtrude : public Effect {
public:
    LPEExtrude(LivePathEffectObject *lpeobject);
    ~LPEExtrude() override;

    Geom::Piecewise<Geom::D2<Geom::SBasis> > doEffect_pwd2 (Geom::Piecewise<Geom::D2<Geom::SBasis> > const & pwd2_in) override;

    void resetDefaults(SPItem const* item) override;

private:
    VectorParam extrude_vector;

    LPEExtrude(const LPEExtrude&) = delete;
    LPEExtrude& operator=(const LPEExtrude&) = delete;
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

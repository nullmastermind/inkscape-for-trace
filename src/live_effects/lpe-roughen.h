// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Roughen LPE effect, see lpe-roughen.cpp.
 */
/* Authors:
 *   Jabier Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2014 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_LPE_ROUGHEN_H
#define INKSCAPE_LPE_ROUGHEN_H

#include "live_effects/effect.h"
#include "live_effects/parameter/enum.h"
#include "live_effects/parameter/parameter.h"
#include "live_effects/parameter/path.h"
#include "live_effects/parameter/bool.h"
#include "live_effects/parameter/random.h"

#include <memory>

namespace Inkscape {
namespace LivePathEffect {

enum DivisionMethod {
    DM_SEGMENTS,
    DM_SIZE,
    DM_END
};

enum HandlesMethod {
    HM_ALONG_NODES,
    HM_RAND,
    HM_RETRACT,
    HM_SMOOTH,
    HM_END
};


class LPERoughen : public Effect {

public:
    LPERoughen(LivePathEffectObject *lpeobject);
    ~LPERoughen() override;

    void doOnApply(SPLPEItem const *lpeitem) override;
    void doEffect(SPCurve *curve) override;
    virtual double sign(double randNumber);
    virtual Geom::Point randomize(double max_length, bool is_node = false);
    void doBeforeEffect(SPLPEItem const * lpeitem) override;
    virtual Geom::Point tPoint(Geom::Point A, Geom::Point B, double t = 0.5);
    Gtk::Widget *newWidget() override;

private:
    std::unique_ptr<SPCurve> addNodesAndJitter(Geom::Curve const *A, Geom::Point &prev, Geom::Point &last_move,
                                               double t, bool last);
    std::unique_ptr<SPCurve> jitter(Geom::Curve const *A, Geom::Point &prev, Geom::Point &last_move);

    EnumParam<DivisionMethod> method;
    ScalarParam max_segment_size;
    ScalarParam segments;
    RandomParam displace_x;
    RandomParam displace_y;
    RandomParam global_randomize;
    EnumParam<HandlesMethod> handles;
    BoolParam shift_nodes;
    BoolParam fixed_displacement;
    BoolParam spray_tool_friendly;
    long seed;
    LPERoughen(const LPERoughen &) = delete;
    LPERoughen &operator=(const LPERoughen &) = delete;

};

}; //namespace LivePathEffect
}; //namespace Inkscape
#endif

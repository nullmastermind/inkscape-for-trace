#ifndef INKSCAPE_LPE_FILL_BETWEEN_MANY_H
#define INKSCAPE_LPE_FILL_BETWEEN_MANY_H

/*
 * Inkscape::LPEFillBetweenStrokes
 *
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/effect.h"
#include "live_effects/parameter/enum.h"
#include "live_effects/parameter/originalpatharray.h"
#include "live_effects/parameter/hidden.h"

namespace Inkscape {
namespace LivePathEffect {

enum Filllpemethod {
    FLM_ORIGINALD,
    FLM_BSPLINESPIRO,
    FLM_D,
    FLM_END
};

class LPEFillBetweenMany : public Effect {
public:
    LPEFillBetweenMany(LivePathEffectObject *lpeobject);
    ~LPEFillBetweenMany() override;
    void doOnApply (SPLPEItem const* lpeitem) override;
    void doEffect (SPCurve * curve) override;

private:
    OriginalPathArrayParam linked_paths;
    EnumParam<Filllpemethod> method;
    BoolParam fuse;
    BoolParam allow_transforms;
    BoolParam join;
    BoolParam close;
    HiddenParam applied;
    Filllpemethod previous_method;
private:
    LPEFillBetweenMany(const LPEFillBetweenMany&) = delete;
    LPEFillBetweenMany& operator=(const LPEFillBetweenMany&) = delete;
};

}; //namespace LivePathEffect
}; //namespace Inkscape

#endif

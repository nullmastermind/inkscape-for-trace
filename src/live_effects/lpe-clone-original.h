#ifndef INKSCAPE_LPE_CLONE_ORIGINAL_H
#define INKSCAPE_LPE_CLONE_ORIGINAL_H

/*
 * Inkscape::LPECloneOriginal
 *
 * Copyright (C) Johan Engelen 2012 <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#include <gtkmm/expander.h>
#include "live_effects/effect.h"
#include "live_effects/parameter/enum.h"
#include "live_effects/parameter/originalitem.h"
#include "live_effects/parameter/text.h"
#include "live_effects/lpegroupbbox.h"

namespace Inkscape {
namespace LivePathEffect {

enum Clonelpemethod {
    CLM_NONE,
    CLM_ORIGINALD,
    CLM_BSPLINESPIRO,
    CLM_D,
    CLM_END
};

class LPECloneOriginal : public Effect, GroupBBoxEffect {
public:
    LPECloneOriginal(LivePathEffectObject *lpeobject);
    virtual ~LPECloneOriginal();
    virtual void doEffect (SPCurve * curve);
    virtual void doBeforeEffect (SPLPEItem const* lpeitem);
    virtual void transform_multiply(Geom::Affine const& postmul, bool set);
    void cloneAttrbutes(SPObject *origin, SPObject *dest, const char * attributes, const char * style_attributes);

private:
    OriginalItemParam linkeditem;
    EnumParam<Clonelpemethod> method;
    TextParam attributes;
    TextParam style_attributes;
    BoolParam allow_transforms;
    LPECloneOriginal(const LPECloneOriginal&);
    LPECloneOriginal& operator=(const LPECloneOriginal&);
};

}; //namespace LivePathEffect
}; //namespace Inkscape

#endif

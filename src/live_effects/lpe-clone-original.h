#ifndef INKSCAPE_LPE_CLONE_ORIGINAL_H
#define INKSCAPE_LPE_CLONE_ORIGINAL_H

/*
 * Inkscape::LPECloneOriginal
 *
 * Copyright (C) Johan Engelen 2012 <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/effect.h"
#include "live_effects/parameter/originalitem.h"
#include "live_effects/parameter/parameter.h"
#include "live_effects/parameter/point.h"
#include "live_effects/parameter/text.h"
#include "live_effects/lpegroupbbox.h"

namespace Inkscape {
namespace LivePathEffect {

class LPECloneOriginal : public Effect, GroupBBoxEffect {
public:
    LPECloneOriginal(LivePathEffectObject *lpeobject);
    virtual ~LPECloneOriginal();

    virtual void doEffect (SPCurve * curve);
    virtual void doBeforeEffect (SPLPEItem const* lpeitem);
    virtual void doAfterEffect (SPLPEItem const* lpeitem);
    virtual void doOnApply(SPLPEItem const* lpeitem);
    void cloneAttrbutes(SPObject *origin, SPObject *dest, bool live, const char * first_attribute, ...);
    void cloneStyleAttrbutes(SPObject *origin, SPObject *dest, const char * first_attribute, ...);

private:
    OriginalItemParam  linked_item;
    BoolParam preserve_position;
    TextParam attributes;
    TextParam style_attributes;
    LPECloneOriginal(const LPECloneOriginal&);
    LPECloneOriginal& operator=(const LPECloneOriginal&);
};

}; //namespace LivePathEffect
}; //namespace Inkscape

#endif

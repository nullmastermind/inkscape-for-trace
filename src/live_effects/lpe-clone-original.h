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
#include "live_effects/parameter/originalpath.h"
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
    virtual void doOnApply(SPLPEItem const* lpeitem);
    virtual void doEffect (SPCurve * curve);
    virtual void doBeforeEffect (SPLPEItem const* lpeitem);
    virtual void transform_multiply(Geom::Affine const& postmul, bool set);
    virtual Gtk::Widget * newWidget();
    void cloneAttrbutes(SPObject *origin, SPObject *dest, bool live, const char * attributes, const char * style_attributes, bool root);

private:
    OriginalPathParam  linked_path;
    OriginalItemParam  linked_item;
    ScalarParam scale;
    BoolParam preserve_position;
    BoolParam inverse;
    BoolParam use_center;
    TextParam attributes;
    TextParam style_attributes;
    bool preserve_position_changed;
    Geom::Affine preserve_affine;
    LPECloneOriginal(const LPECloneOriginal&);
    LPECloneOriginal& operator=(const LPECloneOriginal&);
};

}; //namespace LivePathEffect
}; //namespace Inkscape

#endif

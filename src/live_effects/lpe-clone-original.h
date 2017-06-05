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
    virtual void doEffect (SPCurve * curve);
    virtual void doBeforeEffect (SPLPEItem const* lpeitem);
    virtual void transform_multiply(Geom::Affine const& postmul, bool set);
    virtual Gtk::Widget * newWidget();
    void onExpanderChanged();
    void cloneAttrbutes(SPObject *origin, SPObject *dest, bool live, const char * attributes, const char * style_attributes, bool root);

private:
    OriginalItemParam  linkeditem;
    ScalarParam scale;
    BoolParam preserve_position;
    BoolParam inverse;
    BoolParam d;
    BoolParam transform;
    BoolParam fill;
    BoolParam stroke;
    BoolParam paintorder;
    BoolParam opacity;
    BoolParam filter;
    TextParam attributes;
    TextParam style_attributes;
    Geom::Point origin;
    bool preserve_position_changed;
    bool expanded;
    Gtk::Expander * expander;
    Geom::Affine preserve_affine;
    LPECloneOriginal(const LPECloneOriginal&);
    LPECloneOriginal& operator=(const LPECloneOriginal&);
};

}; //namespace LivePathEffect
}; //namespace Inkscape

#endif

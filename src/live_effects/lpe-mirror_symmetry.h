// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LPE_MIRROR_SYMMETRY_H
#define INKSCAPE_LPE_MIRROR_SYMMETRY_H

/** \file
 * LPE <mirror_symmetry> implementation: mirrors a path with respect to a given line.
 */
/*
 * Authors:
 *   Maximilian Albert
 *   Johan Engelen
 *   Jabiertxof
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 * Copyright (C) Maximilin Albert 2008 <maximilian.albert@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/effect.h"
#include "live_effects/lpeobject.h"
#include "live_effects/lpeobject-reference.h"
#include "live_effects/parameter/parameter.h"
#include "live_effects/parameter/text.h"
#include "live_effects/parameter/point.h"
#include "live_effects/parameter/enum.h"
#include "live_effects/lpegroupbbox.h"

namespace Inkscape {
namespace LivePathEffect {

enum ModeType {
    MT_V,
    MT_H,
    MT_FREE,
    MT_X,
    MT_Y,
    MT_END
};

class LPEMirrorSymmetry : public Effect, GroupBBoxEffect {
public:
    LPEMirrorSymmetry(LivePathEffectObject *lpeobject);
    ~LPEMirrorSymmetry() override;
    void doOnApply (SPLPEItem const* lpeitem) override;
    void doBeforeEffect (SPLPEItem const* lpeitem) override;
    void doAfterEffect (SPLPEItem const* lpeitem) override;
    Geom::PathVector doEffect_path (Geom::PathVector const & path_in) override;
    void doOnRemove (SPLPEItem const* /*lpeitem*/) override;
    void doOnVisibilityToggled(SPLPEItem const* /*lpeitem*/) override;
    Gtk::Widget * newWidget() override;
    void cloneStyle(SPObject *orig, SPObject *dest);
    void toMirror(Geom::Affine transform, bool reset);
    void cloneD(SPObject *orig, SPObject *dest, bool reset);
    Inkscape::XML::Node * createPathBase(SPObject *elemref);
    void resetStyles();
    void centerVert();
    void centerHoriz();

protected:
    void addCanvasIndicators(SPLPEItem const *lpeitem, std::vector<Geom::PathVector> &hp_vec) override;

private:
    EnumParam<ModeType> mode;
    BoolParam discard_orig_path;
    BoolParam fuse_paths;
    BoolParam oposite_fuse;
    BoolParam split_items;
    PointParam start_point;
    PointParam end_point;
    PointParam center_point;
    Geom::Point previous_center;
    SPObject * container;
    bool reset;
    bool center_vert;
    bool center_horiz;
    LPEMirrorSymmetry(const LPEMirrorSymmetry&) = delete;
    LPEMirrorSymmetry& operator=(const LPEMirrorSymmetry&) = delete;
};

} //namespace LivePathEffect
} //namespace Inkscape

#endif

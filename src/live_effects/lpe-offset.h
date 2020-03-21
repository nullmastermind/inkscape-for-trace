// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LPE_OFFSET_H
#define INKSCAPE_LPE_OFFSET_H

/** \file
 * LPE <offset> implementation, see lpe-offset.cpp.
 */

/*
 * Authors:
 *   Maximilian Albert
 *   Jabiertxo Arraiza
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 * Copyright (C) Maximilian Albert 2008 <maximilian.albert@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/effect.h"
#include "live_effects/lpegroupbbox.h"
#include "live_effects/parameter/enum.h"
#include "live_effects/parameter/parameter.h"
#include "live_effects/parameter/unit.h"

namespace Inkscape {
namespace LivePathEffect {

namespace OfS {
// we need a separate namespace to avoid clashes with other LPEs
class KnotHolderEntityOffsetPoint;
}

class LPEOffset : public Effect, GroupBBoxEffect {
public:
    LPEOffset(LivePathEffectObject *lpeobject);
    ~LPEOffset() override;
    void doBeforeEffect (SPLPEItem const* lpeitem) override;
    Geom::PathVector doEffect_path (Geom::PathVector const & path_in) override;
    void transform_multiply(Geom::Affine const &postmul, bool set) override;
    void addKnotHolderEntities(KnotHolder * knotholder, SPItem * item) override;
    void addCanvasIndicators(SPLPEItem const *lpeitem, std::vector<Geom::PathVector> &hp_vec) override;
    void calculateOffset (Geom::PathVector const & path_in);
    Geom::Point get_default_point(Geom::PathVector pathv) const;
    Geom::Point get_nearest_point(Geom::PathVector pathv, Geom::Point point)  const;
    double sp_get_offset(Geom::Point origin);
    friend class OfS::KnotHolderEntityOffsetPoint;

private:
    UnitParam unit;
    ScalarParam offset;
    EnumParam<unsigned> linejoin_type;
    ScalarParam miter_limit;
    BoolParam attempt_force_join;
    BoolParam update_on_knot_move;
    Geom::Point offset_pt;
    Glib::ustring prev_unit;
    double scale = 1; //take document scale and additional parent transformations into account
    KnotHolderEntity * _knot_entity;
    Geom::PathVector filled_rule_pathv;
    Geom::PathVector helper_path;
    Inkscape::UI::Widget::Scalar *offset_widget;

    LPEOffset(const LPEOffset&);
    LPEOffset& operator=(const LPEOffset&);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

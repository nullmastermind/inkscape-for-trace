// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Boolean operation live path effect
 *
 * Copyright (C) 2016 Michael Soegtrop
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_LPE_BOOL_H
#define INKSCAPE_LPE_BOOL_H

#include "livarot/LivarotDefs.h"
#include "live_effects/effect.h"
#include "live_effects/parameter/bool.h"
#include "live_effects/parameter/enum.h"
#include "live_effects/parameter/hidden.h"
#include "live_effects/parameter/originalitem.h"
#include "live_effects/parameter/parameter.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEBool : public Effect {
public:
    LPEBool(LivePathEffectObject *lpeobject);
    ~LPEBool() override;

    void doEffect(SPCurve *curve) override;
    void addCanvasIndicators(SPLPEItem const *lpeitem, std::vector<Geom::PathVector> &hp_vec) override;
    void doBeforeEffect(SPLPEItem const *lpeitem) override;
    void transform_multiply(Geom::Affine const &postmul, bool set) override;
    void doOnVisibilityToggled(SPLPEItem const * /*lpeitem*/) override;
    void doOnRemove(SPLPEItem const * /*lpeitem*/) override;
    void add_filter();
    Geom::PathVector get_union(SPObject *object);
    void remove_filter();
    enum bool_op_ex
    {
        bool_op_ex_union = bool_op_union,
        bool_op_ex_inters = bool_op_inters,
        bool_op_ex_diff = bool_op_diff,
        bool_op_ex_symdiff = bool_op_symdiff,
        bool_op_ex_cut = bool_op_cut,
        // bool_op_ex_slice = bool_op_slice,
        // bool_op_ex_slice_inside,  // like bool_op_slice, but leaves only the contour pieces inside of the cut path
        // bool_op_ex_slice_outside, // like bool_op_slice, but leaves only the contour pieces outside of the cut path
        bool_op_ex_count
    };

    inline friend bool_op to_bool_op(bool_op_ex val)
    {
        //assert(val <= bool_op_ex_slice);
        assert(val <= bool_op_ex_cut);
        return (bool_op) val;
    }

private:
    LPEBool(const LPEBool &) = delete;
    LPEBool &operator=(const LPEBool &) = delete;

    OriginalItemParam operand_path;
    EnumParam<bool_op_ex> bool_operation;
    EnumParam<fill_typ> fill_type_this;
    EnumParam<fill_typ> fill_type_operand;
    BoolParam swap_operands;
    BoolParam rmv_inner;
    SPItem *operand = nullptr;
    SPObject *parentlpe = nullptr;
    Glib::ustring operand_id = "";
    HiddenParam filter;
    Geom::PathVector _hp;
    Geom::Affine prev_affine;
};

}; //namespace LivePathEffect
}; //namespace Inkscape

#endif

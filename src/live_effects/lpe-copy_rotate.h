#ifndef INKSCAPE_LPE_COPY_ROTATE_H
#define INKSCAPE_LPE_COPY_ROTATE_H

/** \file
 * LPE <copy_rotate> implementation, see lpe-copy_rotate.cpp.
 */

/*
 * Authors:
 *   Johan Engelen
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/effect.h"
#include "live_effects/parameter/parameter.h"
#include "live_effects/parameter/text.h"
#include "live_effects/parameter/point.h"
#include "live_effects/lpegroupbbox.h"

namespace Inkscape {
namespace LivePathEffect {

class LPECopyRotate : public Effect, GroupBBoxEffect {
public:
    LPECopyRotate(LivePathEffectObject *lpeobject);
    virtual ~LPECopyRotate();
    virtual void doOnApply (SPLPEItem const* lpeitem);
    virtual Geom::PathVector doEffect_path (Geom::PathVector const & path_in);
    virtual Geom::Piecewise<Geom::D2<Geom::SBasis> > doEffect_pwd2 (Geom::Piecewise<Geom::D2<Geom::SBasis> > const & pwd2_in);
    virtual void doBeforeEffect (SPLPEItem const* lpeitem);
    virtual void doAfterEffect (SPLPEItem const* lpeitem);
    virtual void setFusion(Geom::PathVector &path_in, Geom::Path divider, double sizeDivider);
    virtual void split(Geom::PathVector &path_in, Geom::Path const &divider);
    virtual void resetDefaults(SPItem const* item);
    virtual void transform_multiply(Geom::Affine const& postmul, bool set);
    virtual void doOnRemove (SPLPEItem const* /*lpeitem*/);
    virtual void doOnVisibilityToggled(SPLPEItem const* /*lpeitem*/);
    virtual Gtk::Widget * newWidget();
    void processObjects(LpeAction lpe_action);
    void toItem(Geom::Affine transform, size_t i, bool reset);
    void cloneD(SPObject *origin, SPObject *dest, bool root, bool reset);
    void resetStyles();
protected:
    virtual void addCanvasIndicators(SPLPEItem const *lpeitem, std::vector<Geom::PathVector> &hp_vec);

private:
    PointParam origin;
    PointParam starting_point;
    ScalarParam starting_angle;
    ScalarParam rotation_angle;
    ScalarParam num_copies;
    ScalarParam split_gap;
    BoolParam copies_to_360;
    BoolParam fuse_paths;
    BoolParam join_paths;
    BoolParam split_items;
    TextParam id_origin;
    Geom::Point A;
    Geom::Point B;
    Geom::Point dir;
    Geom::Point start_pos;
    Geom::Point rot_pos;
    Geom::Point previous_start_point;
    double dist_angle_handle;
    double previous_num_copies;
    std::vector<const char *> items;
    bool reset;
    SPObject * container;
    LPECopyRotate(const LPECopyRotate&);
    LPECopyRotate& operator=(const LPECopyRotate&);
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

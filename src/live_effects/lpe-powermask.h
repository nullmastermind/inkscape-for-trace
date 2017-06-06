#ifndef INKSCAPE_LPE_POWERMASK_H
#define INKSCAPE_LPE_POWERMASK_H

/*
 * Inkscape::LPEPowerMask
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/effect.h"
#include "live_effects/parameter/hidden.h"
#include "live_effects/parameter/path.h"
#include "live_effects/lpegroupbbox.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEPowerMask : public Effect, GroupBBoxEffect {
public:
    LPEPowerMask(LivePathEffectObject *lpeobject);
    virtual ~LPEPowerMask();
    virtual void doBeforeEffect (SPLPEItem const* lpeitem);
    virtual Geom::PathVector doEffect_path (Geom::PathVector const & path_in);
    //virtual void doOnVisibilityToggled(SPLPEItem const* lpeitem);
    virtual void doOnRemove (SPLPEItem const* /*lpeitem*/);
    virtual Gtk::Widget * newWidget();
    void toggleMask();
    void addInverse (SPItem * mask_data);
    void removeInverse (SPItem * mask_data);
    void flattenMask(SPItem * mask_data, Geom::PathVector &path_in);
    void convertShapes();
private:
    BoolParam inverse;
    BoolParam flatten;
    HiddenParam is_inverse;
    Geom::Path mask_box;
    bool is_mask;
    bool convert_shapes;
    bool hide_mask;
    bool previous_hide_mask;
};

} //namespace LivePathEffect
} //namespace Inkscape
#endif

#ifndef INKSCAPE_LPE_POWERCLIP_H
#define INKSCAPE_LPE_POWERCLIP_H

/*
 * Inkscape::LPEPowerClip
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/effect.h"
#include "live_effects/parameter/message.h"
#include "live_effects/parameter/hidden.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEPowerClip : public Effect {
public:
    LPEPowerClip(LivePathEffectObject *lpeobject);
    ~LPEPowerClip() override;
    void doBeforeEffect (SPLPEItem const* lpeitem) override;
    Geom::PathVector doEffect_path (Geom::PathVector const & path_in) override;
    void doOnRemove (SPLPEItem const* /*lpeitem*/) override;
    Gtk::Widget * newWidget() override;
    //virtual void transform_multiply(Geom::Affine const& postmul, bool set);
    void doOnVisibilityToggled(SPLPEItem const* lpeitem) override;
    void doAfterEffect (SPLPEItem const* lpeitem) override;
    void addInverse (SPItem * clip_data);
    void removeInverse (SPItem * clip_data);
    void flattenClip(SPItem * clip_data, Geom::PathVector &path_in);
    void convertShapes();
private:
    HiddenParam is_inverse;
    HiddenParam uri;
    BoolParam inverse;
    BoolParam flatten;
    BoolParam hide_clip;
    MessageParam message;
    Geom::Path clip_box;
    Geom::Affine base;
    bool convert_shapes;
};

void sp_inverse_powerclip(Inkscape::Selection *sel);

} //namespace LivePathEffect
} //namespace Inkscape
#endif

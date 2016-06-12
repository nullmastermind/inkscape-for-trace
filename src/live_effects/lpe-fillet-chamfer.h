#ifndef INKSCAPE_LPE_FILLET_CHAMFER_H
#define INKSCAPE_LPE_FILLET_CHAMFER_H

/*
 * Author(s):
 *     Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2014 Author(s)
 *
 * Jabiertxof:Thanks to all people help me
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/parameter/enum.h"
#include "live_effects/parameter/satellitesarray.h"
#include "live_effects/effect.h"
#include "helper/geom-pathvectorsatellites.h"

namespace Inkscape {
namespace LivePathEffect {

enum Fillet_method {
    FM_AUTO,
    FM_ARC,
    FM_BEZIER,
    FM_END
};

class LPEFilletChamfer : public Effect {
public:
    LPEFilletChamfer(LivePathEffectObject *lpeobject);
    virtual void doBeforeEffect(SPLPEItem const *lpeItem);
    virtual Geom::PathVector
    doEffect_path(Geom::PathVector const &path_in);
    virtual void doOnApply(SPLPEItem const *lpeItem);
    virtual Gtk::Widget *newWidget();
    void addCanvasIndicators(SPLPEItem const */*lpeitem*/, std::vector<Geom::PathVector> &hp_vec);
    void updateSatelliteType(SatelliteType satellitetype);
    void updateChamferSteps();
    void updateAmount();
    void refreshKnots();

    SatellitesArrayParam _satellites_param;

private:
    EnumParam<Fillet_method> _method;
    ScalarParam _radius;
    ScalarParam _chamfer_steps;
    BoolParam _flexible;
    BoolParam _mirror_knots;
    BoolParam _only_selected;
    BoolParam _use_knot_distance;
    BoolParam _hide_knots;
    BoolParam _apply_no_radius;
    BoolParam _apply_with_radius;
    ScalarParam _helper_size;

    bool _degenerate_hide;
    PathVectorSatellites *_pathvector_satellites;
    Geom::PathVector _hp;

    LPEFilletChamfer(const LPEFilletChamfer &);
    LPEFilletChamfer &operator=(const LPEFilletChamfer &);

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

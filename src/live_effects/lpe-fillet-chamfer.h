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

enum FilletMethod {
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
    void chamfer();
    void inverseChamfer();
    void fillet();
    void inverseFillet();

    SatellitesArrayParam satellites_param;

private:
    EnumParam<FilletMethod> method;
    ScalarParam radius;
    ScalarParam chamfer_steps;
    BoolParam flexible;
    BoolParam mirror_knots;
    BoolParam only_selected;
    BoolParam use_knot_distance;
    BoolParam hide_knots;
    BoolParam apply_no_radius;
    BoolParam apply_with_radius;
    ScalarParam helper_size;

    PathVectorSatellites pathVectorSatellites;
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

#ifndef INKSCAPE_LPE_FILLET_CHAMFER_H
#define INKSCAPE_LPE_FILLET_CHAMFER_H

/*
 * Author(s):
 *     Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2014 Author(s)
 *
 * Special thanks to Johan Engelen for the base of the effect -powerstroke-
 * Also to ScislaC for point me to the idea
 * Also su_v for his construvtive feedback and time
 * and finaly to Liam P. White for his big help on coding, that save me a lot of
 * hours
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/parameter/enum.h"
#include "live_effects/parameter/unit.h"
#include "live_effects/parameter/satellitearray.h"
#include "live_effects/effect.h"
#include "helper/geom-pointwise.h"

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
    virtual ~LPEFilletChamfer();
    virtual void doBeforeEffect(SPLPEItem const *lpeItem);
    virtual std::vector<Geom::Path>
    doEffect_path(std::vector<Geom::Path> const &path_in);
    virtual void doOnApply(SPLPEItem const *lpeItem);
    virtual void adjustForNewPath(std::vector<Geom::Path> const &path_in);
    virtual Gtk::Widget *newWidget();
    void addCanvasIndicators(SPLPEItem const */*lpeitem*/, std::vector<Geom::PathVector> &hp_vec);
    void updateSatelliteType(Geom::SatelliteType satellitetype);
    void updateChamferSteps();
    void updateAmount();
    void refreshKnots();
    void chamfer();
    void inverseChamfer();
    void fillet();
    void inverseFillet();

    SatelliteArrayParam satellites_param;

private:
    UnitParam unit;
    EnumParam<FilletMethod> method;
    ScalarParam radius;
    ScalarParam chamfer_steps;
    BoolParam flexible;
    BoolParam mirror_knots;
    BoolParam only_selected;
    BoolParam use_knot_distance;
    BoolParam hide_knots;
    BoolParam ignore_radius_0;
    ScalarParam helper_size;

    Geom::Pointwise *pointwise;
    double segment_size;
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

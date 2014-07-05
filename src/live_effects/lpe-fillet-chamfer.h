#ifndef INKSCAPE_LPE_FILLET_CHAMFER_H
#define INKSCAPE_LPE_FILLET_CHAMFER_H

/*
 * Inkscape::LPEFilletChamfer
 * Copyright (C) Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 * Special thanks to Johan Engelen for the base of the effect -powerstroke-
 * Also to ScislaC for point me to the idea
 * Also su_v for his construvtive feedback and time
 * and finaly to Liam P. White for his big help on coding, that save me a lot of
 * hours
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#include "live_effects/parameter/enum.h"
#include "live_effects/parameter/unit.h"
#include "live_effects/effect.h"
#include "live_effects/parameter/bool.h"
#include "live_effects/parameter/filletchamferpointarray.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEFilletChamfer : public Effect {

public:
    LPEFilletChamfer(LivePathEffectObject *lpeobject);
    virtual ~LPEFilletChamfer();

    std::vector<Geom::Path> doEffect_path(std::vector<Geom::Path> const &path_in);

    virtual void doOnApply(SPLPEItem const *lpeItem);
    virtual void doBeforeEffect(SPLPEItem const *lpeItem);
    virtual int getKnotsNumber(SPCurve const *c);
    virtual void adjustForNewPath(std::vector<Geom::Path> const &path_in);
    virtual void toggleHide();
    virtual void toggleFlexFixed();
    virtual void chamfer();
    virtual void fillet();
    virtual void doubleChamfer();
    virtual void inverse();
    virtual void updateFillet();
    virtual void doUpdateFillet(std::vector<Geom::Path> original_pathv,
                                double power);
    virtual void doChangeType(std::vector<Geom::Path> original_pathv, int type);
    virtual bool nodeIsSelected(Geom::Point nodePoint,
                                std::vector<Geom::Point> points);
    virtual void refreshKnots();
    virtual Gtk::Widget *newWidget();
    FilletChamferPointArrayParam fillet_chamfer_values;

private:

    BoolParam hide_knots;
    BoolParam ignore_radius_0;
    BoolParam only_selected;
    BoolParam flexible;
    UnitParam unit;
    ScalarParam radius;
    ScalarParam helper_size;

    LPEFilletChamfer(const LPEFilletChamfer &);
    LPEFilletChamfer &operator=(const LPEFilletChamfer &);

};

}; //namespace LivePathEffect
}; //namespace Inkscape
#endif

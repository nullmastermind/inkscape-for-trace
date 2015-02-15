/*
 * Author(s):
 *   Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2014 Author(s)
 *
 * Special thanks to Johan Engelen for the base of the effect -powerstroke-
 * Also to ScislaC for point me to the idea
 * Also su_v for his construvtive feedback and time
 * Also to Mc- (IRC nick) for his important contribution to find real time
 * values based on
 * and finaly to Liam P. White for his big help on coding, that save me a lot of hours
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */


#include "live_effects/lpe-fillet-chamfer.h"
#include <sp-shape.h>
#include <2geom/pointwise.h>
#include <2geom/satellite.h>
#include <2geom/satellite-enum.h>
#include "helper/geom-nodetype.h"
#include "helper/geom.h"
#include "display/curve.h"
#include <vector>
// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>


namespace Inkscape {
namespace LivePathEffect {

LPEFilletChamfer::LPEFilletChamfer(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    satellitepairarrayparam_values(_("Fillet point"), _("Fillet point"), "satellitepairarrayparam_values", &wr, this)
{
    registerParameter(&satellitepairarrayparam_values);
}

LPEFilletChamfer::~LPEFilletChamfer() {}


void LPEFilletChamfer::doOnApply(SPLPEItem const *lpeItem)
{
    SPLPEItem * splpeitem = const_cast<SPLPEItem *>(lpeItem);
    SPShape * shape = dynamic_cast<SPShape *>(splpeitem);
    if (shape) {
        Geom::PathVector const &original_pathv = pathv_to_linear_and_cubic_beziers(shape->getCurve()->get_pathvector());
        std::vector<std::pair<int,Geom::Satellite> > satellites;
        Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2_in = paths_to_pw(original_pathv);
        pwd2_in = remove_short_cuts(pwd2_in, .01);
        Geom::Piecewise<Geom::D2<Geom::SBasis> > der = derivative(pwd2_in);
        Geom::Piecewise<Geom::D2<Geom::SBasis> > n = rot90(unitVector(der));
        satellitepairarrayparam_values.set_pwd2(pwd2_in, n);
        for (Geom::PathVector::const_iterator path_it = original_pathv.begin(); path_it != original_pathv.end(); ++path_it) {
            if (path_it->empty()){
                continue;
            }
            Geom::Path::const_iterator curve_it1 = path_it->begin();
            Geom::Path::const_iterator curve_endit = path_it->end_default();
            if (path_it->closed()) {
              const Geom::Curve &closingline = path_it->back_closed(); 
              // the closing line segment is always of type 
              // Geom::LineSegment.
              if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
                // closingline.isDegenerate() did not work, because it only checks for
                // *exact* zero length, which goes wrong for relative coordinates and
                // rounding errors...
                // the closing line segment has zero-length. So stop before that one!
                curve_endit = path_it->end_open();
              }
            }
            Geom::Path::const_iterator curve_end = curve_endit;
            --curve_end;
            int counter = 0;
            while (curve_it1 != curve_endit) {
                Geom::Satellite satellite(Geom::FILLET, true, true, false, false, 0.0, 0.2);
                Geom::NodeType nodetype;
                if (counter==0) {
                    if (path_it->closed()) {
                        nodetype = Geom::get_nodetype(*curve_end, *curve_it1);
                    } else {
                        nodetype = Geom::NODE_NONE;
                    }
                } else {
                    nodetype = get_nodetype((*path_it)[counter - 1], *curve_it1);
                }
                if (nodetype == Geom::NODE_CUSP) {
                    satellites.push_back(std::make_pair(counter, satellite));
                }
                ++curve_it1;
                counter++;
            }
        }
        satellitepairarrayparam_values.param_set_and_write_new_value(satellites);
    } else {
        g_warning("LPE Fillet/Chamfer can only be applied to shapes (not groups).");
        SPLPEItem * item = const_cast<SPLPEItem*>(lpeItem);
        item->removeCurrentPathEffect(false);
    }
}

void
LPEFilletChamfer::adjustForNewPath(std::vector<Geom::Path> const &path_in)
{
    if (!path_in.empty()) {
        //fillet_chamfer_values.recalculate_controlpoints_for_new_pwd2(pathv_to_linear_and_cubic_beziers(path_in)[0].toPwSb());
    }
}


}; //namespace LivePathEffect
}; /* namespace Inkscape */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offset:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

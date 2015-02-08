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

#include "live_effects/parameter/pointwise.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

using namespace Geom;
namespace Inkscape {
namespace LivePathEffect {

LPEFilletChamfer::LPEFilletChamfer(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    pointwise_values(_("Fillet point"), _("Fillet point"), "pointwise_values", &wr, this)
{
    registerParameter(&pointwise_values);
}

LPEFilletChamfer::~LPEFilletChamfer() {}


void LPEFilletChamfer::doOnApply(SPLPEItem const *lpeItem)
{
    SPShape * shape = dynamic_cast<SPshape *>(lpeItem);
    if (shape) {
        std::vector<std::pair<int,GeomSatellite> pointwise;
        PathVector const &original_pathv = pathv_to_linear_and_cubic_beziers(shape->gerCurve->get_pathvector());
        //Piecewise<D2<SBasis> > pwd2_in = paths_to_pw(original_pathv);
        for (PathVector::const_iterator path_it = original_pathv.begin(); path_it != original_pathv.end(); ++path_it) {
            if (path_it->empty())
                continue;

            Geom::Path::const_iterator curve_it1 = path_it->begin();
            Geom::Path::const_iterator curve_it2 = ++(path_it->begin());
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
            int counter = 0;
            while (curve_it1 != curve_endit) {
                Geom::Saltellite sat(FILLET, true, true, false, false, false, 0, 0.2);
                std::pair<std::size_t, std::size_t> positions = pointwise_values.get_positions(counter, original_pathv);
                Geom::NodeType nodetype;
                if (positions.second == 0) {
                    nodetype = NODE_NONE;
                } else {
                    nodetype = get_nodetype((*path_it)[counter - 1], *curve_it1);
                }
                if (nodetype == NODE_CUSP) {
                    pointwise.push_back(std::pair<counter,sat>);
                }
                ++curve_it1;
                if (curve_it2 != curve_endit) {
                    ++curve_it2;
                }
                counter++;
            }
        }
        pointwise_values.param_set_and_write_new_value(pointwise);
    } else {
        g_warning("LPE Fillet/Chamfer can only be applied to shapes (not groups).");
        SPLPEItem * item = const_cast<SPLPEItem*>(lpeItem);
        item->removeCurrentPathEffect(false);
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

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
#include "live_effects/lpeobject.h"
#include <sp-shape.h>
#include <sp-path.h>
#include <2geom/pointwise.h>
#include <2geom/satellite.h>
#include <2geom/satellite-enum.h>
#include <2geom/svg-elliptical-arc.h>
#include "helper/geom-nodetype.h"
#include "helper/geom-curves.h"
#include "helper/geom.h"
#include "display/curve.h"
#include <vector>
// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

using namespace Geom;
namespace Inkscape {
namespace LivePathEffect {

static const Util::EnumData<FilletMethod> FilletMethodData[FM_END] = {
    { FM_AUTO, N_("Auto"), "auto" },
    { FM_ARC, N_("Force arc"), "arc" },
    { FM_BEZIER, N_("Force bezier"), "bezier" }
};
static const Util::EnumDataConverter<FilletMethod>
FMConverter(FilletMethodData, FM_END);

LPEFilletChamfer::LPEFilletChamfer(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    satellitepairarrayparam_values(_("Fillet point"), _("Fillet point"), "satellitepairarrayparam_values", &wr, this),
    method(_("Method:"), _("Fillets methods"), "method", FMConverter, &wr, this, FM_AUTO),
    flexible(_("Flexible radius size (%)"), _("Flexible radius size (%)"), "flexible", &wr, this, false),
    pointwise()
{
    registerParameter(&satellitepairarrayparam_values);
    registerParameter(&method);
    registerParameter(&flexible);
}

LPEFilletChamfer::~LPEFilletChamfer() {}


void LPEFilletChamfer::doOnApply(SPLPEItem const *lpeItem)
{
    SPLPEItem * splpeitem = const_cast<SPLPEItem *>(lpeItem);
    SPShape * shape = dynamic_cast<SPShape *>(splpeitem);
    if (shape) {
        PathVector const &original_pathv = pathv_to_linear_and_cubic_beziers(shape->getCurve()->get_pathvector());
        Piecewise<D2<SBasis> > pwd2_in = paths_to_pw(original_pathv);
        pwd2_in = remove_short_cuts(pwd2_in, .01);
        satellitepairarrayparam_values.set_pwd2(pwd2_in);
        int counterTotal = 0;
        std::vector<std::pair<int,Geom::Satellite> >  satellites;
        for (PathVector::const_iterator path_it = original_pathv.begin(); path_it != original_pathv.end(); ++path_it) {
            if (path_it->empty()){
                continue;
            }
            Geom::Path::const_iterator curve_it1 = path_it->begin();
            Geom::Path::const_iterator curve_endit = path_it->end_default();
            if (path_it->closed()) {
              const Curve &closingline = path_it->back_closed(); 
              // the closing line segment is always of type 
              // LineSegment.
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
                Satellite satellite(F, flexible, true, false, false, 0.0, 0.0);
                Geom::NodeType nodetype;
                if (counter==0) {
                    if (path_it->closed()) {
                        nodetype = get_nodetype(*curve_end, *curve_it1);
                    } else {
                        nodetype = NODE_NONE;
                    }
                } else {
                    nodetype = get_nodetype((*path_it)[counter - 1], *curve_it1);
                }
                if (nodetype == NODE_CUSP) {
                    satellites.push_back(std::make_pair(counterTotal, satellite));
                }
                ++curve_it1;
                counter++;
                counterTotal++;
            }
        }
        satellitepairarrayparam_values.param_set_and_write_new_value(satellites);
    } else {
        g_warning("LPE Fillet/Chamfer can only be applied to shapes (not groups).");
        SPLPEItem * item = const_cast<SPLPEItem*>(lpeItem);
        item->removeCurrentPathEffect(false);
    }
}

void LPEFilletChamfer::doBeforeEffect(SPLPEItem const *lpeItem)
{
    SPLPEItem * splpeitem = const_cast<SPLPEItem *>(lpeItem);
    SPShape * shape = dynamic_cast<SPShape *>(splpeitem);
    if (shape) {
        SPCurve *c = shape->getCurve();
        SPPath * path = dynamic_cast<SPPath *>(shape);
        if(path){
            c = path->get_original_curve();
        }
        PathVector const &original_pathv = pathv_to_linear_and_cubic_beziers(c->get_pathvector());
        Piecewise<D2<SBasis> > pwd2_in = paths_to_pw(pathv_to_linear_and_cubic_beziers(original_pathv));
        pwd2_in = remove_short_cuts(pwd2_in, .01);
        //wating to recalculate
        //recalculate_controlpoints_for_new_pwd2(pwd2_in);
        satellitepairarrayparam_values.set_pwd2(pwd2_in);
        std::vector<std::pair<int,Geom::Satellite> >  satellites = satellitepairarrayparam_values.data();
        pointwise = new Pointwise( pwd2_in,satellites);
        bool changed = false;
        for (std::vector<std::pair<int,Geom::Satellite> >::iterator it = satellites.begin(); it != satellites.end(); ++it) {
            if(it->second.getIsTime() != flexible){
                it->second.setIsTime(flexible);
                changed = true;
            }
        }
        if(changed){
            satellitepairarrayparam_values.param_set_and_write_new_value(satellites);
        }
    } else {
        g_warning("LPE Fillet can only be applied to shapes (not groups).");
    }
}


std::vector<Geom::Path>
LPEFilletChamfer::doEffect_path(std::vector<Geom::Path> const &path_in)
{
    const double gapHelper = 0.00001;
    std::vector<Geom::Path> pathvector_out;
    unsigned int counter = 0;
    const double K = (4.0 / 3.0) * (sqrt(2.0) - 1.0);
    std::vector<Geom::Path> path_in_processed = pathv_to_linear_and_cubic_beziers(path_in);
    for (PathVector::const_iterator path_it = path_in_processed.begin();
            path_it != path_in_processed.end(); ++path_it) {
        if (path_it->empty()){
            continue;
        }
        Geom::Path path_out;
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
        unsigned int counterCurves = 0;
        unsigned int first = counter;
        double time0 = 0;
        while (curve_it1 != curve_endit) {
            if((*curve_it1).isDegenerate() || (*curve_it1).isDegenerate()){
                g_warning("LPE Fillet not handle degenerate curves.");
                return path_in;
            }
            std::vector<Satellite> satVector;
            Satellite sat;
            Curve *curve_it2Fixed = path_it->begin()->duplicate();
            if(!path_it->closed() || curve_it2 != curve_endit){
                curve_it2Fixed = (*curve_it2).duplicate();
                satVector = pointwise->findSatellites(counter+1,1);
                if(satVector.size()>0){
                    sat = satVector[0];
                }
            } else {
                satVector = pointwise->findSatellites(first,1);
                if(satVector.size()>0){
                    sat = satVector[0];
                }
            }
            if(first == counter){
                satVector = pointwise->findSatellites(first,1);
                if(satVector.size()>0){
                    time0 = satVector[0].getTime();
                }
            }

            bool last = curve_it2 == curve_endit;
            double time1 = sat.getOpositeTime((*curve_it1).toSBasis());
            double time2 = sat.getTime();
            if(time1 <= time0){
                time1 = time0;
            }
            std::vector<double> times;
            times.push_back(time0);
            times.push_back(time1);
            times.push_back(time2);
            std::cout << ":::" << counter    << ":::::::::::::::\n";
            std::cout << time0 << "::Time 0\n";
            std::cout << time1 << "::Time 1\n";
            std::cout << time2 << "::Time 2\n";
            Curve *knotCurve1 = curve_it1->portion(times[0], times[1]);
            if (counterCurves > 0) {
                knotCurve1->setInitial(path_out.finalPoint());
            } else {
                path_out.start((*curve_it1).pointAt(times[0]));
            }
            
            Point startArcPoint = knotCurve1->finalPoint();
            Point endArcPoint = curve_it2Fixed->pointAt(times[2]);
            if(times[2] == 1){
                endArcPoint = curve_it2Fixed->pointAt(times[2]-gapHelper);
            }
            if(times[1] == times[0]){
                startArcPoint = knotCurve1->pointAt(1-gapHelper);
            }
            double k1 = distance(startArcPoint, curve_it1->finalPoint()) * K;
            double k2 = distance(endArcPoint, curve_it2Fixed->initialPoint()) * K;
            Geom::CubicBezier const *cubic1 = dynamic_cast<Geom::CubicBezier const *>(&*knotCurve1);
            Ray ray1(startArcPoint, curve_it1->finalPoint());
            if (cubic1) {
                ray1.setPoints((*cubic1)[2], startArcPoint);
            }
            Point handle1 =  Point::polar(ray1.angle(),k1) + startArcPoint;
            if(time0 == 1){
                handle1 =  startArcPoint;
            }
            Curve *knotCurve2 = curve_it2Fixed->portion(times[2], 1);
            Geom::CubicBezier const *cubic2 =  dynamic_cast<Geom::CubicBezier const *>(&*knotCurve2);
            Ray ray2(curve_it2Fixed->initialPoint(), endArcPoint);
            if (cubic2) {
                ray2.setPoints(endArcPoint, (*cubic2)[1]);
            }
            Point handle2 = endArcPoint - Point::polar(ray2.angle(),k2);
            
            bool ccwToggle = cross(curve_it1->finalPoint() - startArcPoint, endArcPoint - startArcPoint) < 0;
            double angle = angle_between(ray1, ray2, ccwToggle);
            double handleAngle = ray1.angle() - angle;
            if (ccwToggle) {
                handleAngle = ray1.angle() + angle;
            }
            Point inverseHandle1 = Point::polar(handleAngle,k1) + startArcPoint;
            if(time0 == 1){
                inverseHandle1 =  startArcPoint;
            }
            handleAngle = ray2.angle() + angle;
            if (ccwToggle) {
                handleAngle = ray2.angle() - angle;
            }
            Point inverseHandle2 = endArcPoint - Point::polar(handleAngle,k2);
            if(times[2] == 1){
                endArcPoint = curve_it2Fixed->pointAt(times[2]);
            }
            if(times[1] == times[0]){
                startArcPoint = knotCurve1->pointAt(1);
            }
            Line const x_line(Geom::Point(0,0),Geom::Point(1,0));
            Line const angled_line(startArcPoint,endArcPoint);
            double angleArc = Geom::angle_between( x_line,angled_line);
            double radius = Geom::distance(startArcPoint,middle_point(startArcPoint,endArcPoint))/sin(angle/2.0);
            Coord rx = radius;
            Coord ry = rx;
            if (times[1] != 1) {
                if (times[1] != times[0] || times[1] == times[0] == 1) {
                    if(!knotCurve1->isDegenerate()){
                        path_out.append(*knotCurve1);
                    }
                }
                SatelliteType type = F;
                type = sat.getSatelliteType();
                if(are_near(middle_point(startArcPoint,endArcPoint),curve_it1->finalPoint(), 0.0001)){
                   // path_out.appendNew<Geom::LineSegment>(endArcPoint);
                } else if (type == C) {
                /*
                    unsigned int chamferSubs = type-3000;
                    Geom::Path path_chamfer;
                    path_chamfer.start(path_out.finalPoint());
                    if((is_straight_curve(*curve_it1) && is_straight_curve(*curve_it2Fixed) && method != FM_BEZIER )|| method == FM_ARC){ 
                        path_chamfer.appendNew<SVGEllipticalArc>(rx, ry, angleArc, 0, ccwToggle, endArcPoint);
                    } else {
                        path_chamfer.appendNew<Geom::CubicBezier>(handle1, handle2, endArcPoint);
                    }
                    double chamfer_stepsTime = 1.0/chamferSubs;
                    for(unsigned int i = 1; i < chamferSubs; i++){
                        Geom::Point chamferStep = path_chamfer.pointAt(chamfer_stepsTime * i);
                        path_out.appendNew<Geom::LineSegment>(chamferStep);
                    }
                    path_out.appendNew<Geom::LineSegment>(endArcPoint);
                /*/
                } else if (type == IC) {
                   /*
                    unsigned int chamferSubs = type-4000;
                    Geom::Path path_chamfer;
                    path_chamfer.start(path_out.finalPoint());
                    if((is_straight_curve(*curve_it1) && is_straight_curve(*curve_it2Fixed) && method != FM_BEZIER )|| method == FM_ARC){ 
                        ccwToggle = ccwToggle?0:1;
                        path_chamfer.appendNew<SVGEllipticalArc>(rx, ry, angleArc, 0, ccwToggle, endArcPoint);
                    }else{
                        path_chamfer.appendNew<Geom::CubicBezier>(inverseHandle1, inverseHandle2, endArcPoint);
                    }
                    double chamfer_stepsTime = 1.0/chamferSubs;
                    for(unsigned int i = 1; i < chamferSubs; i++){
                        Geom::Point chamferStep = path_chamfer.pointAt(chamfer_stepsTime * i);
                        path_out.appendNew<Geom::LineSegment>(chamferStep);
                    }
                    path_out.appendNew<Geom::LineSegment>(endArcPoint);
                */
                } else if (type == IF) {
                    if((is_straight_curve(*curve_it1) && is_straight_curve(*curve_it2Fixed) && method != FM_BEZIER )|| method == FM_ARC){ 
                        ccwToggle = ccwToggle?0:1;
                        path_out.appendNew<SVGEllipticalArc>(rx, ry, angleArc, 0, ccwToggle, endArcPoint);
                    }else{
                        path_out.appendNew<Geom::CubicBezier>(inverseHandle1, inverseHandle2, endArcPoint);
                    }
                } else if (type == F){
                    if((is_straight_curve(*curve_it1) && is_straight_curve(*curve_it2Fixed) && method != FM_BEZIER )|| method == FM_ARC){ 
                        path_out.appendNew<SVGEllipticalArc>(rx, ry, angleArc, 0, ccwToggle, endArcPoint);
                    } else {
                        path_out.appendNew<Geom::CubicBezier>(handle1, handle2, endArcPoint);
                    }
                }
            } else {
                if(!knotCurve1->isDegenerate()){
                    path_out.append(*knotCurve1);
                }
            }
            if (path_it->closed() && last) {
                path_out.close();
            }
            /*
            if(!path_it->closed() || curve_it2 != curve_endit){
                satellites[counter + 1] = std::make_pair(counter + 1, sat);
            } else {
                satellites[first] = std::make_pair(first, sat);
            }
            */
            ++curve_it1;
            if (curve_it2 != curve_endit) {
                ++curve_it2;
            }
            counter++;
            counterCurves++;
            time0 = times[2];
        }
        pathvector_out.push_back(path_out);
    }
    return pathvector_out;
}

void
LPEFilletChamfer::adjustForNewPath(std::vector<Geom::Path> const &path_in)
{   
    if (!path_in.empty()) {
        //satellitepairarrayparam_values.recalculate_controlpoints_for_new_pwd2(pathv_to_linear_and_cubic_beziers(path_in)[0].toPwSb());
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

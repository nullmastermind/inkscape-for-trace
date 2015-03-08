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
#include "ui/tools-switch.h"
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
    satellitepairarrayparam_values(_("pair_array_param"), _("pair_array_param"), "satellitepairarrayparam_values", &wr, this),
    unit(_("Unit:"), _("Unit"), "unit", &wr, this),
    method(_("Method:"), _("Fillets methods"), "method", FMConverter, &wr, this, FM_AUTO),
    radius(_("Radius (unit or %):"), _("Radius, in unit or %"), "radius", &wr, this, 0.),
    chamfer_steps(_("Chamfer steps:"), _("Chamfer steps"), "chamfer_steps", &wr, this, 3),
    flexible(_("Flexible radius size (%)"), _("Flexible radius size (%)"), "flexible", &wr, this, false),
    mirror_knots(_("Mirror Knots"), _("Mirror Knots"), "mirror_knots", &wr, this, true),
    only_selected(_("Change only selected nodes"), _("Change only selected nodes"), "only_selected", &wr, this, false),
    use_knot_distance(_("Use knots distance instead radius"), _("Use knots distance instead radius"), "use_knot_distance", &wr, this, false),
    hide_knots(_("Hide knots"), _("Hide knots"), "hide_knots", &wr, this, false),
    ignore_radius_0(_("Ignore 0 radius knots"), _("Ignore 0 radius knots"), "ignore_radius_0", &wr, this, false),
    helper(_("Show helper lines"), _("Show helper lines"), "helper", &wr, this, false),
    pointwise()
{
    registerParameter(&satellitepairarrayparam_values);
    registerParameter(&unit);
    registerParameter(&method);
    registerParameter(&radius);
    registerParameter(&chamfer_steps);
    registerParameter(&flexible);
    registerParameter(&use_knot_distance);
    registerParameter(&mirror_knots);
    registerParameter(&ignore_radius_0);
    registerParameter(&only_selected);
    registerParameter(&hide_knots);
    registerParameter(&helper);

    radius.param_set_range(0., infinity());
    radius.param_set_increments(1, 1);
    radius.param_set_digits(4);
    chamfer_steps.param_set_range(1, 999);
    chamfer_steps.param_set_increments(1, 1);
    chamfer_steps.param_set_digits(0);
}

LPEFilletChamfer::~LPEFilletChamfer() {}


void LPEFilletChamfer::doOnApply(SPLPEItem const *lpeItem)
{
    SPLPEItem * splpeitem = const_cast<SPLPEItem *>(lpeItem);
    SPShape * shape = dynamic_cast<SPShape *>(splpeitem);
    if (shape) {
        PathVector const &original_pathv = pathv_to_linear_and_cubic_beziers(shape->getCurve()->get_pathvector());
        Piecewise<D2<SBasis> > pwd2_in = paths_to_pw(original_pathv);
        pwd2_in = remove_short_cuts(pwd2_in, 0.01);
        int counterTotal = 0;
        std::vector<std::pair<unsigned int,Geom::Satellite> >  satellites;
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
            unsigned int steps = 0;
            while (curve_it1 != curve_endit) {
                bool isStart = false;
                if(counter == 0){
                    isStart = true;
                }
                bool isClosing = false;
                if(path_it->closed() && curve_it1 == curve_end){
                    isClosing = true;
                }
                bool active = true;
                bool hidden = false;
                if (counter==0) {
                    if (!path_it->closed()) {
                        active = false;
                    }
                }
                Satellite satellite(F, flexible, isClosing, isStart, active, mirror_knots, hidden, 0.0, 0.0, steps);
                satellites.push_back(std::make_pair(counterTotal, satellite));
                ++curve_it1;
                counter++;
                counterTotal++;
            }
            if (!path_it->closed()){
                bool active = false;
                bool isClosing = false;
                bool isStart = false;
                bool hidden = true;
                Satellite satellite(F, flexible, isClosing, isStart, active, mirror_knots, hidden, 0.0, 0.0, steps);
                satellites.push_back(std::make_pair(counterTotal, satellite));
            }
        }
        pointwise = new Pointwise( pwd2_in,satellites);
        satellitepairarrayparam_values.param_set_and_write_new_value(satellites);
    } else {
        g_warning("LPE Fillet/Chamfer can only be applied to shapes (not groups).");
        SPLPEItem * item = const_cast<SPLPEItem*>(lpeItem);
        item->removeCurrentPathEffect(false);
    }
}

Gtk::Widget *LPEFilletChamfer::newWidget()
{
    // use manage here, because after deletion of Effect object, others might
    // still be pointing to this widget.
    Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox(Effect::newWidget()));

    vbox->set_border_width(5);
    vbox->set_homogeneous(false);
    vbox->set_spacing(2);
    std::vector<Parameter *>::iterator it = param_vector.begin();
    while (it != param_vector.end()) {
        if ((*it)->widget_is_visible) {
            Parameter *param = *it;
            Gtk::Widget *widg = param->param_newWidget();
            if (param->param_key == "radius") {
                Inkscape::UI::Widget::Scalar *widgRegistered = Gtk::manage(dynamic_cast<Inkscape::UI::Widget::Scalar *>(widg));
                widgRegistered->signal_value_changed().connect(sigc::mem_fun(*this, &LPEFilletChamfer::updateAmount));
                widg = widgRegistered;
                if (widg) {
                    Gtk::HBox *scalarParameter = dynamic_cast<Gtk::HBox *>(widg);
                    std::vector<Gtk::Widget *> childList = scalarParameter->get_children();
                    Gtk::Entry *entryWidg = dynamic_cast<Gtk::Entry *>(childList[1]);
                    entryWidg->set_width_chars(6);
                }
            } else if (param->param_key == "chamfer_steps") {
                Inkscape::UI::Widget::Scalar *widgRegistered = Gtk::manage(dynamic_cast<Inkscape::UI::Widget::Scalar *>(widg));
                widgRegistered->signal_value_changed().connect(sigc::mem_fun(*this, &LPEFilletChamfer::updateChamferSteps));
                widg = widgRegistered;
                if (widg) {
                    Gtk::HBox *scalarParameter = dynamic_cast<Gtk::HBox *>(widg);
                    std::vector<Gtk::Widget *> childList = scalarParameter->get_children();
                    Gtk::Entry *entryWidg = dynamic_cast<Gtk::Entry *>(childList[1]);
                    entryWidg->set_width_chars(3);
                }
            } else if (param->param_key == "only_selected") {
                Gtk::manage(widg);
            }
            Glib::ustring *tip = param->param_getTooltip();
            if (widg) {
                vbox->pack_start(*widg, true, true, 2);
                if (tip) {
                    widg->set_tooltip_text(*tip);
                } else {
                    widg->set_tooltip_text("");
                    widg->set_has_tooltip(false);
                }
            }
        }
        ++it;
    }
    
    Gtk::HBox *filletContainer = Gtk::manage(new Gtk::HBox(true, 0));
    Gtk::Button *fillet = Gtk::manage(new Gtk::Button(Glib::ustring(_("Fillet"))));
    fillet->signal_clicked().connect(sigc::mem_fun(*this, &LPEFilletChamfer::fillet));

    filletContainer->pack_start(*fillet, true, true, 2);
    Gtk::Button *inverseFillet = Gtk::manage(new Gtk::Button(Glib::ustring(_("Inverse fillet"))));
    inverseFillet->signal_clicked().connect(sigc::mem_fun(*this, &LPEFilletChamfer::inverseFillet));
    filletContainer->pack_start(*inverseFillet, true, true, 2);
    
    Gtk::HBox *chamferContainer = Gtk::manage(new Gtk::HBox(true, 0));
    Gtk::Button *chamfer = Gtk::manage(new Gtk::Button(Glib::ustring(_("Chamfer"))));
    chamfer->signal_clicked().connect(sigc::mem_fun(*this, &LPEFilletChamfer::chamfer));

    chamferContainer->pack_start(*chamfer, true, true, 2);
    Gtk::Button *inverseChamfer = Gtk::manage(new Gtk::Button(Glib::ustring(_("Inverse chamfer"))));
    inverseChamfer->signal_clicked().connect(sigc::mem_fun(*this, &LPEFilletChamfer::inverseChamfer));
    chamferContainer->pack_start(*inverseChamfer, true, true, 2);

    vbox->pack_start(*filletContainer, true, true, 2);
    vbox->pack_start(*chamferContainer, true, true, 2);

    return vbox;
}


void LPEFilletChamfer::fillet()
{
    updateSatelliteType(F);
}

void LPEFilletChamfer::inverseFillet()
{
    updateSatelliteType(IF);
}

void LPEFilletChamfer::chamfer()
{
    updateSatelliteType(C);
}

void LPEFilletChamfer::inverseChamfer()
{
    updateSatelliteType(IC);
}

void LPEFilletChamfer::refreshKnots()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (tools_isactive(desktop, TOOLS_NODES)) {
        tools_switch(desktop, TOOLS_SELECT);
        tools_switch(desktop, TOOLS_NODES);
    }
}

void LPEFilletChamfer::updateAmount()
{
    double power = 0;
    if (!flexible) {
        power = Inkscape::Util::Quantity::convert(radius, unit.get_abbreviation(), *defaultUnit);
    } else {
        power = radius/100;
    }
    std::vector<std::pair<unsigned int,Geom::Satellite> > satellites = pointwise->getSatellites();
    Piecewise<D2<SBasis> > pwd2 = pointwise->getPwd2();
    for (std::vector<std::pair<unsigned int,Geom::Satellite> >::iterator it = satellites.begin(); it != satellites.end(); ++it) {
        if(pointwise->findClosingSatellites(it->first).size() == 0 && it->second.getIsStart()){
            it->second.setAmount(0);
            continue;
        }
        if(ignore_radius_0 && it->second.getAmount() == 0){
            continue;
        }
        if(only_selected){
            Geom::Point satPoint = pwd2.valueAt(it->first);
            if(isNodePointSelected(satPoint)){
                if(!use_knot_distance && !flexible){
                    it->second.setAmount(pointwise->rad_to_len(power,*it));
                } else {
                    it->second.setAmount(power);
                }
            }
        } else {
            if(!use_knot_distance && !flexible){
                it->second.setAmount(pointwise->rad_to_len(power,*it));
            } else {
                it->second.setAmount(power);
            }
        }
    }
    pointwise->setSatellites(satellites);
    satellitepairarrayparam_values.param_set_and_write_new_value(satellites);
}

void LPEFilletChamfer::updateChamferSteps()
{
    std::vector<std::pair<unsigned int,Geom::Satellite> > satellites = pointwise->getSatellites();
    Piecewise<D2<SBasis> > pwd2 = pointwise->getPwd2();
    for (std::vector<std::pair<unsigned int,Geom::Satellite> >::iterator it = satellites.begin(); it != satellites.end(); ++it) {
        if(ignore_radius_0 && it->second.getAmount() == 0){
            continue;
        }
        if(only_selected){
            Geom::Point satPoint = pwd2.valueAt(it->first);
            if(isNodePointSelected(satPoint)){
               it->second.setSteps(chamfer_steps);
            }
        } else {
            it->second.setSteps(chamfer_steps);
        }
    }
    pointwise->setSatellites(satellites);
    satellitepairarrayparam_values.param_set_and_write_new_value(satellites);
}

void LPEFilletChamfer::updateSatelliteType(Geom::SatelliteType satellitetype)
{
    std::vector<std::pair<unsigned int,Geom::Satellite> > satellites = pointwise->getSatellites();
    Piecewise<D2<SBasis> > pwd2 = pointwise->getPwd2();
    for (std::vector<std::pair<unsigned int,Geom::Satellite> >::iterator it = satellites.begin(); it != satellites.end(); ++it) {
        if(ignore_radius_0 && it->second.getAmount() == 0){
            continue;
        }
        if(only_selected){
            Geom::Point satPoint = pwd2.valueAt(it->first);
            if(isNodePointSelected(satPoint)){
                it->second.setSatelliteType(satellitetype);
            }
        } else {
            it->second.setSatelliteType(satellitetype);
        }
    }
    pointwise->setSatellites(satellites);
    satellitepairarrayparam_values.param_set_and_write_new_value(satellites);
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
        //fillet chamfer specific calls
        satellitepairarrayparam_values.set_document_unit(defaultUnit);
        satellitepairarrayparam_values.set_use_distance(use_knot_distance);
        satellitepairarrayparam_values.set_unit(unit.get_abbreviation());
        //mandatory call
        satellitepairarrayparam_values.set_effect_type(this->effectType());
        
        PathVector const &original_pathv = pathv_to_linear_and_cubic_beziers(c->get_pathvector());
        Piecewise<D2<SBasis> > pwd2_in = paths_to_pw(original_pathv);
        pwd2_in = remove_short_cuts(pwd2_in, 0.01);
        std::vector<std::pair<unsigned int,Geom::Satellite> >  satellites = satellitepairarrayparam_values.data();
        pointwise = new Pointwise(pwd2_in,satellites);
        
        //mandatory call
        satellitepairarrayparam_values.set_pointwise(pointwise);
        //optional call
        if(hide_knots || !helper){
            satellitepairarrayparam_values.set_helper_size(0);
        } else {
            double radiusHelperNodes = 6.0;
            if(current_zoom != 0){
                radiusHelperNodes *= 1/current_zoom;
                radiusHelperNodes = Inkscape::Util::Quantity::convert(radiusHelperNodes, "px", *defaultUnit);
            }
            satellitepairarrayparam_values.set_helper_size(radiusHelperNodes);
        }
        bool changed = false;
        bool refresh = false;
        for (std::vector<std::pair<unsigned int,Geom::Satellite> >::iterator it = satellites.begin(); it != satellites.end(); ++it) {
            if(it->second.getIsTime() != flexible){
                it->second.setIsTime(flexible);
                double amount = it->second.getAmount();
                D2<SBasis> d2_in = pwd2_in[it->first];
                if(it->second.getIsTime()){
                    double time = it->second.toTime(amount,d2_in);
                    it->second.setAmount(time);
                } else {
                    double size = it->second.toSize(amount,d2_in);
                    it->second.setAmount(size);
                }
                changed = true;
            }
            if(it->second.getHasMirror() != mirror_knots){
                it->second.setHasMirror(mirror_knots);
                changed = true;
                refresh = true;
            }
            bool hide = !hide_knots;
            if(it->second.getHidden() != hide){
                it->second.setHidden(hide);
                changed = true;
                refresh = true;
            }
        }
        if(changed){
            pointwise->setSatellites(satellites);
            satellitepairarrayparam_values.param_set_and_write_new_value(satellites);
        }
        if(refresh){
            refreshKnots();
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
            Satellite satellite;
            Curve *curve_it2Fixed = path_it->begin()->duplicate();
            if(!path_it->closed() || curve_it2 != curve_endit){
                curve_it2Fixed = (*curve_it2).duplicate();
                satVector = pointwise->findSatellites(counter+1,1);
                if(satVector.size()>0){
                    satellite = satVector[0];
                }
            } else {
                satVector = pointwise->findSatellites(first,1);
                if(satVector.size()>0){
                    satellite = satVector[0];
                }
            }
            if(first == counter){
                satVector = pointwise->findSatellites(first,1);
                if(satVector.size()>0){
                    time0 = satVector[0].getTime(path_it->begin()->duplicate()->toSBasis());
                }
            }

            bool last = curve_it2 == curve_endit;
            double s = satellite.getSize(curve_it2Fixed->toSBasis());
            double time1 = satellite.getOpositeTime(s,(*curve_it1).toSBasis());
            double time2 = satellite.getTime(curve_it2Fixed->toSBasis());
            if(!satellite.getActive()){
                time1 = 1;
                time0 = 0;
            }
            if(time1 <= time0){
                time1 = time0;
            }
            std::cout << counter << ":::::::::::::::::::::::::::::\n";
            std::cout << time0 << "time0\n";
            std::cout << time1 << "time1\n";
            std::cout << time2 << "time2\n";
            std::vector<double> times;
            times.push_back(time0);
            times.push_back(time1);
            times.push_back(time2);
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
                startArcPoint = curve_it1->pointAt(times[0]+gapHelper);
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
                startArcPoint = curve_it1->pointAt(times[0]);
            }
            Line const x_line(Geom::Point(0,0),Geom::Point(1,0));
            Line const angled_line(startArcPoint,endArcPoint);
            double angleArc = Geom::angle_between( x_line,angled_line);
            double radius = Geom::distance(startArcPoint,middle_point(startArcPoint,endArcPoint))/sin(angle/2.0);
            Coord rx = radius;
            Coord ry = rx;
            if (times[1] != 1) {
                if (times[1] != times[0] || times[1] == times[0] == 1 ) {
                    if(!knotCurve1->isDegenerate()){
                        path_out.append(*knotCurve1);
                    }
                }
                SatelliteType type = satellite.getSatelliteType();
                unsigned int steps = satellite.getSteps();
                if(steps < 1){
                    steps = 1;
                }
                if (type == C) {
                    Geom::Path path_chamfer;
                    path_chamfer.start(path_out.finalPoint());
                    if((is_straight_curve(*curve_it1) && is_straight_curve(*curve_it2Fixed) && method != FM_BEZIER )|| method == FM_ARC){ 
                        path_chamfer.appendNew<SVGEllipticalArc>(rx, ry, angleArc, 0, ccwToggle, endArcPoint);
                    } else {
                        path_chamfer.appendNew<Geom::CubicBezier>(handle1, handle2, endArcPoint);
                    }
                    double chamfer_stepsTime = 1.0/steps;
                    for(unsigned int i = 1; i < steps; i++){
                        Geom::Point chamferStep = path_chamfer.pointAt(chamfer_stepsTime * i);
                        path_out.appendNew<Geom::LineSegment>(chamferStep);
                    }
                    path_out.appendNew<Geom::LineSegment>(endArcPoint);
                } else if (type == IC) {
                    Geom::Path path_chamfer;
                    path_chamfer.start(path_out.finalPoint());
                    if((is_straight_curve(*curve_it1) && is_straight_curve(*curve_it2Fixed) && method != FM_BEZIER )|| method == FM_ARC){ 
                        ccwToggle = ccwToggle?0:1;
                        path_chamfer.appendNew<SVGEllipticalArc>(rx, ry, angleArc, 0, ccwToggle, endArcPoint);
                    }else{
                        path_chamfer.appendNew<Geom::CubicBezier>(inverseHandle1, inverseHandle2, endArcPoint);
                    }
                    double chamfer_stepsTime = 1.0/steps;
                    for(unsigned int i = 1; i < steps; i++){
                        Geom::Point chamferStep = path_chamfer.pointAt(chamfer_stepsTime * i);
                        path_out.appendNew<Geom::LineSegment>(chamferStep);
                    }
                    path_out.appendNew<Geom::LineSegment>(endArcPoint);
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

/*
 * Author(s):
 *   Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2014 Author(s)
 *
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/lpe-fillet-chamfer.h"
#include "helper/geom.h"
#include "display/curve.h"
#include "helper/geom-curves.h"
#include "helper/geom-satellite.h"
#include <2geom/elliptical-arc.h>
#include "knotholder.h"
#include <boost/optional.hpp>
#include "ui/tools-switch.h"
// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>
namespace Inkscape {
namespace LivePathEffect {

static const Util::EnumData<FilletMethod> FilletMethodData[FM_END] = {
    { FM_AUTO, N_("Auto"), "auto" }, { FM_ARC, N_("Force arc"), "arc" },
    { FM_BEZIER, N_("Force bezier"), "bezier" }
};
static const Util::EnumDataConverter<FilletMethod> FMConverter(FilletMethodData,
        FM_END);

LPEFilletChamfer::LPEFilletChamfer(LivePathEffectObject *lpeobject)
    : Effect(lpeobject),
      satellites_param(_("pair_array_param"), _("pair_array_param"),
                       "satellites_param", &wr, this),
      method(_("Method:"), _("Methods to calculate the fillet or chamfer"),
             "method", FMConverter, &wr, this, FM_AUTO),
      radius(_("Radius (unit or %):"), _("Radius, in unit or %"), "radius", &wr,
             this, 0.0),
      chamfer_steps(_("Chamfer steps:"), _("Chamfer steps"), "chamfer_steps",
                    &wr, this, 1),
      flexible(_("Flexible radius size (%)"), _("Flexible radius size (%)"),
               "flexible", &wr, this, false),
      mirror_knots(_("Mirror Knots"), _("Mirror Knots"), "mirror_knots", &wr,
                   this, true),
      only_selected(_("Change only selected nodes"),
                    _("Change only selected nodes"), "only_selected", &wr, this,
                    false),
      use_knot_distance(_("Use knots distance instead radius"),
                        _("Use knots distance instead radius"),
                        "use_knot_distance", &wr, this, false),
      hide_knots(_("Hide knots"), _("Hide knots"), "hide_knots", &wr, this,
                 false),
      apply_no_radius(_("Apply changes if radius = 0"), _("Apply changes if radius = 0"), "apply_no_radius", &wr, this, true),
      apply_with_radius(_("Apply changes if radius > 0"), _("Apply changes if radius > 0"), "apply_with_radius", &wr, this, true),
      helper_size(_("Helper size with direction:"),
                  _("Helper size with direction"), "helper_size", &wr, this, 0)
{
    registerParameter(&satellites_param);
    registerParameter(&method);
    registerParameter(&radius);
    registerParameter(&chamfer_steps);
    registerParameter(&helper_size);
    registerParameter(&flexible);
    registerParameter(&use_knot_distance);
    registerParameter(&mirror_knots);
    registerParameter(&apply_no_radius);
    registerParameter(&apply_with_radius);
    registerParameter(&only_selected);
    registerParameter(&hide_knots);

    radius.param_set_range(0.0, Geom::infinity());
    radius.param_set_increments(1, 1);
    radius.param_set_digits(4);
    radius.param_overwrite_widget(true);
    chamfer_steps.param_set_range(1, 999);
    chamfer_steps.param_set_increments(1, 1);
    chamfer_steps.param_set_digits(0);
    //chamfer_steps.param_overwrite_widget(true);
    helper_size.param_set_range(0, 999);
    helper_size.param_set_increments(5, 5);
    helper_size.param_set_digits(0);
    //helper_size.param_overwrite_widget(true);
}

void LPEFilletChamfer::doOnApply(SPLPEItem const *lpeItem)
{
    SPLPEItem *splpeitem = const_cast<SPLPEItem *>(lpeItem);
    SPShape *shape = dynamic_cast<SPShape *>(splpeitem);
    if (shape) {
        Geom::PathVector const pathv = pathv_to_linear_and_cubic_beziers(shape->getCurve()->get_pathvector());
        Satellites satellites;
        for (Geom::PathVector::const_iterator path_it = pathv.begin(); path_it !=  pathv.end(); ++path_it) {
            if (path_it->empty()) {
                continue;
            }
            std::vector<Satellite> subpath_satellites;
            for (Geom::Path::const_iterator curve_it = path_it->begin(); curve_it !=  path_it->end(); ++curve_it) {
                //Maybe we want this satellites...
                //if (curve_it->isDegenerate()) {
                //  continue 
                //}
                Satellite satellite(FILLET);
                satellite.setSteps(chamfer_steps);
                subpath_satellites.push_back(satellite);
            }
            //we add the last satellite on open path because pointwise is related to nodes, not curves
            //so maybe in the future we can need this last satellite in other effects
            //dont remove for this effect because pointwise class has methods when the path is modiffied
            //and we want one method for all uses
            if (!path_it->closed()) {
                Satellite satellite(FILLET);
                satellite.setSteps(chamfer_steps);
                subpath_satellites.push_back(satellite);
            }
            satellites.push_back(subpath_satellites);
        }
        pointwise.setPathVector(pathv);
        pointwise.setSatellites(satellites);
        satellites_param.setPointwise(pointwise);
    } else {
        g_warning("LPE Fillet/Chamfer can only be applied to shapes (not groups).");
        SPLPEItem *item = const_cast<SPLPEItem *>(lpeItem);
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
                Inkscape::UI::Widget::Scalar *widg_registered =
                    Gtk::manage(dynamic_cast<Inkscape::UI::Widget::Scalar *>(widg));
                widg_registered->signal_value_changed().connect(
                    sigc::mem_fun(*this, &LPEFilletChamfer::updateAmount));
                widg = widg_registered;
                if (widg) {
                    Gtk::HBox *scalar_parameter = dynamic_cast<Gtk::HBox *>(widg);
                    std::vector<Gtk::Widget *> childList = scalar_parameter->get_children();
                    Gtk::Entry *entry_widget = dynamic_cast<Gtk::Entry *>(childList[1]);
                    entry_widget->set_width_chars(6);
                }
            } else if (param->param_key == "chamfer_steps") {
                Inkscape::UI::Widget::Scalar *widg_registered =
                    Gtk::manage(dynamic_cast<Inkscape::UI::Widget::Scalar *>(widg));
                widg_registered->signal_value_changed().connect(
                    sigc::mem_fun(*this, &LPEFilletChamfer::updateChamferSteps));
                widg = widg_registered;
                if (widg) {
                    Gtk::HBox *scalar_parameter = dynamic_cast<Gtk::HBox *>(widg);
                    std::vector<Gtk::Widget *> childList = scalar_parameter->get_children();
                    Gtk::Entry *entry_widget = dynamic_cast<Gtk::Entry *>(childList[1]);
                    entry_widget->set_width_chars(3);
                }
            } else if (param->param_key == "helper_size") {
                Inkscape::UI::Widget::Scalar *widg_registered =
                    Gtk::manage(dynamic_cast<Inkscape::UI::Widget::Scalar *>(widg));
                widg_registered->signal_value_changed().connect(
                    sigc::mem_fun(*this, &LPEFilletChamfer::refreshKnots));
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

    Gtk::HBox *fillet_container = Gtk::manage(new Gtk::HBox(true, 0));
    Gtk::Button *fillet =
        Gtk::manage(new Gtk::Button(Glib::ustring(_("Fillet"))));
    fillet->signal_clicked()
    .connect(sigc::mem_fun(*this, &LPEFilletChamfer::fillet));

    fillet_container->pack_start(*fillet, true, true, 2);
    Gtk::Button *inverse_fillet =
        Gtk::manage(new Gtk::Button(Glib::ustring(_("Inverse fillet"))));
    inverse_fillet->signal_clicked()
    .connect(sigc::mem_fun(*this, &LPEFilletChamfer::inverseFillet));
    fillet_container->pack_start(*inverse_fillet, true, true, 2);

    Gtk::HBox *chamfer_container = Gtk::manage(new Gtk::HBox(true, 0));
    Gtk::Button *chamfer =
        Gtk::manage(new Gtk::Button(Glib::ustring(_("Chamfer"))));
    chamfer->signal_clicked()
    .connect(sigc::mem_fun(*this, &LPEFilletChamfer::chamfer));

    chamfer_container->pack_start(*chamfer, true, true, 2);
    Gtk::Button *inverse_chamfer =
        Gtk::manage(new Gtk::Button(Glib::ustring(_("Inverse chamfer"))));
    inverse_chamfer->signal_clicked()
    .connect(sigc::mem_fun(*this, &LPEFilletChamfer::inverseChamfer));
    chamfer_container->pack_start(*inverse_chamfer, true, true, 2);

    vbox->pack_start(*fillet_container, true, true, 2);
    vbox->pack_start(*chamfer_container, true, true, 2);

    return vbox;
}

void LPEFilletChamfer::fillet()
{
    updateSatelliteType(FILLET);
}

void LPEFilletChamfer::inverseFillet()
{
    updateSatelliteType(INVERSE_FILLET);
}

void LPEFilletChamfer::chamfer()
{
    updateSatelliteType(CHAMFER);
}

void LPEFilletChamfer::inverseChamfer()
{
    updateSatelliteType(INVERSE_CHAMFER);
}

void LPEFilletChamfer::refreshKnots()
{
    //Find another way to update knots satellites_param.knoth->update_knots(); do the thing
    //but not updat the knot index on node delete
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
        power = radius;
    } else {
        power = radius / 100;
    }
    Satellites satellites = pointwise.getSatellites();
    Geom::PathVector pathv = pointwise.getPathVector();
    for (size_t i = 0; i < satellites.size(); ++i) {
        for (size_t j = 0; j < satellites[i].size(); ++j) {
            boost::optional<size_t> previous_index = boost::none;
            if(j == 0 && pathv[i].closed()){
                previous_index = pathv[i].size() - 1;
            } else if(!pathv[i].closed() || j != 0) {
                previous_index = j - 1;
            }
            if (!pathv[i].closed() && j == 0) {
                satellites[i][j].amount = 0;
                continue;
            }
            if (pathv[i].size() == j) {
                continue;
            }
            if ((!apply_no_radius && satellites[i][j].amount == 0) ||
                (!apply_with_radius && satellites[i][j].amount != 0)) 
            {
                continue;
            }
            
            Geom::Point satellite_point = pathv[i].pointAt(j);
            if (isNodePointSelected(satellite_point) || !only_selected) {
                if (!use_knot_distance && !flexible) {
                    if(previous_index) {
                        satellites[i][j].amount = satellites[i][j].radToLen(power, pathv[i][*previous_index], pathv[i][j]);
                    } else {
                        satellites[i][j].amount = 0.0;
                    }
                } else {
                    satellites[i][j].amount = power;
                }
            }
        }
    }
    pointwise.setSatellites(satellites);
    satellites_param.setPointwise(pointwise);
}

void LPEFilletChamfer::updateChamferSteps()
{
    Satellites satellites = pointwise.getSatellites();
    Geom::PathVector pathv = pointwise.getPathVector();
    for (size_t i = 0; i < satellites.size(); ++i) {
        for (size_t j = 0; j < satellites[i].size(); ++j) {
            if ((!apply_no_radius && satellites[i][j].amount == 0) ||
                (!apply_with_radius && satellites[i][j].amount != 0)) 
            {
                continue;
            }
            if (only_selected) {
                Geom::Point satellite_point = pathv[i].pointAt(j);
                if (isNodePointSelected(satellite_point)) {
                    satellites[i][j].steps = chamfer_steps;
                }
            } else {
                satellites[i][j].steps = chamfer_steps;
            }
        }
    }
    pointwise.setSatellites(satellites);
    satellites_param.setPointwise(pointwise);
}

void LPEFilletChamfer::updateSatelliteType(SatelliteType satellitetype)
{
    Satellites satellites = pointwise.getSatellites();
    Geom::PathVector pathv = pointwise.getPathVector();
    for (size_t i = 0; i < satellites.size(); ++i) {
        for (size_t j = 0; j < satellites[i].size(); ++j) {
            if ((!apply_no_radius && satellites[i][j].amount == 0) ||
                (!apply_with_radius && satellites[i][j].amount != 0)) 
            {
                continue;
            }
            if (pathv[i].size() == j) {
                if (!only_selected) {
                    satellites[i][j].satellite_type = satellitetype;
                }
                continue;
            }
            if (only_selected) {
                Geom::Point satellite_point = pathv[i].pointAt(j);
                if (isNodePointSelected(satellite_point)) {
                    satellites[i][j].satellite_type = satellitetype;
                }
            } else {
                satellites[i][j].satellite_type = satellitetype;
            }
        }
    }
    pointwise.setSatellites(satellites);
    satellites_param.setPointwise(pointwise);
}

void LPEFilletChamfer::doBeforeEffect(SPLPEItem const *lpeItem)
{
    SPLPEItem *splpeitem = const_cast<SPLPEItem *>(lpeItem);
    SPShape *shape = dynamic_cast<SPShape *>(splpeitem);
    if (shape) {
        SPCurve *c = shape->getCurve();
        SPPath *path = dynamic_cast<SPPath *>(shape);
        if (path) {
            c = path->get_original_curve();
        }
        //fillet chamfer specific calls
        satellites_param.setUseDistance(use_knot_distance);
        //mandatory call
        satellites_param.setEffectType(effectType());
        satellites_param.setPathUpdate(false);

        Geom::PathVector const pathv = pathv_to_linear_and_cubic_beziers(c->get_pathvector());
        Satellites satellites = satellites_param.data();
        if(satellites.empty()) {
            doOnApply(lpeItem);
            satellites = satellites_param.data();
        }
        if (hide_knots) {
            satellites_param.setHelperSize(0);
        } else {
            satellites_param.setHelperSize(helper_size);
        }
        size_t number_nodes = pathv.nodes().size();
        for (size_t i = 0; i < satellites.size(); ++i) {
            for (size_t j = 0; j < satellites[i].size(); ++j) {
                if (satellites[i][j].is_time != flexible) {
                    satellites[i][j].is_time = flexible;
                    double amount = satellites[i][j].amount;
                    if (pathv[i].size() == j){
                        continue;
                    }
                    Geom::Curve const &curve_in = pathv[i][j];
                    if (satellites[i][j].is_time) {
                        double time = timeAtArcLength(amount, curve_in);
                        satellites[i][j].amount = time;
                    } else {
                        double size = arcLengthAt(amount, curve_in);
                        satellites[i][j].amount = size;
                    }
                }
                if (satellites[i][j].has_mirror != mirror_knots) {
                    satellites[i][j].has_mirror = mirror_knots;
                }
                satellites[i][j].hidden = hide_knots;
            }
        }
        //if are diferent sizes call to poinwise recalculate
        //TODO: Update the satellite data in paths modified, Goal 0.93
        size_t satellites_counter = pointwise.getTotalSatellites();
        if (satellites_counter != 0 ){
            std::cout << number_nodes << "aaaaa" << pointwise.getTotalSatellites() << "\n";
        }
        if (satellites_counter != 0 && number_nodes != satellites_counter) {
            satellites_param.setPathUpdate(true);
            Satellite satellite(satellites[0][0].satellite_type);
            satellite.setIsTime(satellites[0][0].is_time);
            satellite.setHasMirror(mirror_knots);
            satellite.setHidden(hide_knots);
            pointwise.recalculateForNewPathVector(pathv, satellite);
        } else {
            pointwise.setPathVector(pathv);
            pointwise.setSatellites(satellites);
        }
        refreshKnots();
        satellites_param.setPointwise(pointwise);
    } else {
        g_warning("LPE Fillet can only be applied to shapes (not groups).");
    }
}

void
LPEFilletChamfer::addCanvasIndicators(SPLPEItem const */*lpeitem*/, std::vector<Geom::PathVector> &hp_vec)
{
    hp_vec.push_back(_hp);
}

Geom::PathVector
LPEFilletChamfer::doEffect_path(Geom::PathVector const &path_in)
{
    const double GAP_HELPER = 0.00001;
    Geom::PathVector path_out;
    size_t path = 0;
    const double K = (4.0 / 3.0) * (sqrt(2.0) - 1.0);
    Geom::PathVector pathv = pathv_to_linear_and_cubic_beziers(path_in);
    for (Geom::PathVector::const_iterator path_it = pathv.begin(); path_it != pathv.end(); ++path_it) {
        if (path_it->empty()) {
            continue;
        }
        _hp.push_back(*path_it);
        Geom::Path tmp_path;
        if (path_it->size() == 1) {
            path++;
            tmp_path.start(path_it[0].pointAt(0));
            tmp_path.append(path_it[0]);
            path_out.push_back(tmp_path);
            continue;
        }
        double time0 = 0;
        size_t curve = 0;
        Satellites satellites = pointwise.getSatellites();
        for (Geom::Path::const_iterator curve_it1 = path_it->begin(); curve_it1 !=  path_it->end(); ++curve_it1) {
            size_t next_index = curve + 1;
            if (curve == pathv[path].size() - 1 && pathv[path].closed()) {
                next_index = 0;
            }
            if (curve == pathv[path].size() -1 && !pathv[path].closed()) { //the path is open and we are at end of path
                if (time0 != 1) { //Previous satellite not at 100% amount
                    Geom::Curve *last_curve = curve_it1->portion(time0, 1);
                    last_curve->setInitial(tmp_path.finalPoint());
                    tmp_path.append(*last_curve);
                }
                continue;
            }
            Satellite satellite = satellites[path][next_index];
            if (!curve) { //curve == 0 
                if (!path_it->closed()) {
                    time0 = 0;
                } else {
                    time0 = satellites[path][0].time(*curve_it1);
                }
            }
            bool last = pathv[path].size() - 1 == curve;
            Geom::Curve const &curve_it2 = pathv[path][next_index];
            double s = satellite.arcDistance(curve_it2);
            double time1 = satellite.time(s, true, (*curve_it1));
            double time2 = satellite.time(curve_it2);
            if (time1 <= time0) {
                time1 = time0;
            }
            if (time2 > 1) {
                time2 = 1;
            }
            std::vector<double> times;
            times.push_back(time0);
            times.push_back(time1);
            times.push_back(time2);
            Geom::Curve *knot_curve_1 = curve_it1->portion(times[0], times[1]);
            if (curve > 0) {
                knot_curve_1->setInitial(tmp_path.finalPoint());
            } else {
                tmp_path.start((*curve_it1).pointAt(times[0]));
            }

            Geom::Point start_arc_point = knot_curve_1->finalPoint();
            Geom::Point end_arc_point = curve_it2.pointAt(times[2]);
            if (times[2] == 1) {
                end_arc_point = curve_it2.pointAt(times[2] - GAP_HELPER);
            }
            if (times[1] == times[0]) {
                start_arc_point = curve_it1->pointAt(times[0] + GAP_HELPER);
            }
            double k1 = distance(start_arc_point, curve_it1->finalPoint()) * K;
            double k2 = distance(end_arc_point, curve_it2.initialPoint()) * K;
            Geom::CubicBezier const *cubic_1 =
                dynamic_cast<Geom::CubicBezier const *>(&*knot_curve_1);
            Geom::Ray ray_1(start_arc_point, curve_it1->finalPoint());
            if (cubic_1) {
                ray_1.setPoints((*cubic_1)[2], start_arc_point);
            }
            Geom::Point handle_1 = Geom::Point::polar(ray_1.angle(), k1) + start_arc_point;
            if (time0 == 1) {
                handle_1 = start_arc_point;
            }
            Geom::Curve *knot_curve_2 = curve_it2.portion(times[2], 1);
            Geom::CubicBezier const *cubic_2 =
                dynamic_cast<Geom::CubicBezier const *>(&*knot_curve_2);
            Geom::Ray ray_2(curve_it2.initialPoint(), end_arc_point);
            if (cubic_2) {
                ray_2.setPoints(end_arc_point, (*cubic_2)[1]);
            }
            Geom::Point handle_2 = end_arc_point - Geom::Point::polar(ray_2.angle(), k2);

            bool ccw_toggle = cross(curve_it1->finalPoint() - start_arc_point,
                                    end_arc_point - start_arc_point) < 0;
            double angle = angle_between(ray_1, ray_2, ccw_toggle);
            double handleAngle = ray_1.angle() - angle;
            if (ccw_toggle) {
                handleAngle = ray_1.angle() + angle;
            }
            Geom::Point inverse_handle_1 = Geom::Point::polar(handleAngle, k1) + start_arc_point;
            if (time0 == 1) {
                inverse_handle_1 = start_arc_point;
            }
            handleAngle = ray_2.angle() + angle;
            if (ccw_toggle) {
                handleAngle = ray_2.angle() - angle;
            }
            Geom::Point inverse_handle_2 = end_arc_point - Geom::Point::polar(handleAngle, k2);
            if (times[2] == 1) {
                end_arc_point = curve_it2.pointAt(times[2]);
            }
            if (times[1] == times[0]) {
                start_arc_point = curve_it1->pointAt(times[0]);
            }
            Geom::Line const x_line(Geom::Point(0, 0), Geom::Point(1, 0));
            Geom::Line const angled_line(start_arc_point, end_arc_point);
            double arc_angle = Geom::angle_between(x_line, angled_line);
            double radius = Geom::distance(start_arc_point,
                                           middle_point(start_arc_point, end_arc_point)) /
                            sin(angle / 2.0);
            Geom::Coord rx = radius;
            Geom::Coord ry = rx;
            if (times[1] != 1) {
                if (times[1] != times[0] || (times[1] == 1 && times[0] == 1)) {
                    if (!knot_curve_1->isDegenerate()) {
                        tmp_path.append(*knot_curve_1);
                    }
                }
                SatelliteType type = satellite.satellite_type;
                size_t steps = satellite.steps;
                if (steps < 1) {
                    steps = 1;
                }
                if (type == CHAMFER) {
                    Geom::Path path_chamfer;
                    path_chamfer.start(tmp_path.finalPoint());
                    if ((is_straight_curve(*curve_it1) &&
                            is_straight_curve(curve_it2) && method != FM_BEZIER) ||
                            method == FM_ARC) {
                        ccw_toggle = ccw_toggle ? 0 : 1;
                        path_chamfer.appendNew<Geom::EllipticalArc>(rx, ry, arc_angle, 0,
                                ccw_toggle, end_arc_point);
                    } else {
                        path_chamfer.appendNew<Geom::CubicBezier>(handle_1, handle_2,
                                end_arc_point);
                    }
                    double chamfer_stepsTime = 1.0 / steps;
                    for (size_t i = 1; i < steps; i++) {
                        Geom::Point chamfer_step = path_chamfer.pointAt(chamfer_stepsTime * i);
                        tmp_path.appendNew<Geom::LineSegment>(chamfer_step);
                    }
                    tmp_path.appendNew<Geom::LineSegment>(end_arc_point);
                } else if (type == INVERSE_CHAMFER) {
                    Geom::Path path_chamfer;
                    path_chamfer.start(tmp_path.finalPoint());
                    if ((is_straight_curve(*curve_it1) &&
                            is_straight_curve(curve_it2) && method != FM_BEZIER) ||
                            method == FM_ARC) {
                        path_chamfer.appendNew<Geom::EllipticalArc>(rx, ry, arc_angle, 0,
                                ccw_toggle, end_arc_point);
                    } else {
                        path_chamfer.appendNew<Geom::CubicBezier>(
                            inverse_handle_1, inverse_handle_2, end_arc_point);
                    }
                    double chamfer_stepsTime = 1.0 / steps;
                    for (size_t i = 1; i < steps; i++) {
                        Geom::Point chamfer_step =
                            path_chamfer.pointAt(chamfer_stepsTime * i);
                        tmp_path.appendNew<Geom::LineSegment>(chamfer_step);
                    }
                    tmp_path.appendNew<Geom::LineSegment>(end_arc_point);
                } else if (type == INVERSE_FILLET) {
                    if ((is_straight_curve(*curve_it1) &&
                            is_straight_curve(curve_it2) && method != FM_BEZIER) ||
                            method == FM_ARC) {
                        tmp_path.appendNew<Geom::EllipticalArc>(rx, ry, arc_angle, 0, ccw_toggle,
                                                                end_arc_point);
                    } else {
                        tmp_path.appendNew<Geom::CubicBezier>(inverse_handle_1,
                                                              inverse_handle_2, end_arc_point);
                    }
                } else if (type == FILLET) {
                    if ((is_straight_curve(*curve_it1) &&
                            is_straight_curve(curve_it2) && method != FM_BEZIER) ||
                            method == FM_ARC) {
                        ccw_toggle = ccw_toggle ? 0 : 1;
                        tmp_path.appendNew<Geom::EllipticalArc>(rx, ry, arc_angle, 0, ccw_toggle,
                                                                end_arc_point);
                    } else {
                        tmp_path.appendNew<Geom::CubicBezier>(handle_1, handle_2,
                                                              end_arc_point);
                    }
                }
            } else {
                if (!knot_curve_1->isDegenerate()) {
                    tmp_path.append(*knot_curve_1);
                }
            }
            if (path_it->closed() && last) {
                tmp_path.close();
            }
            curve++;
            time0 = times[2];
        }
        path++;
        path_out.push_back(tmp_path);
    }
    return path_out;
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

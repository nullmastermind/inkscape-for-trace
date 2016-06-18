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
// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

static const Util::EnumData<Fillet_method> Fillet_methodData[] = {
    { FM_AUTO, N_("Auto"), "auto" }, 
    { FM_ARC, N_("Force arc"), "arc" },
    { FM_BEZIER, N_("Force bezier"), "bezier" }
};
static const Util::EnumDataConverter<Fillet_method> FMConverter(Fillet_methodData, FM_END);

LPEFilletChamfer::LPEFilletChamfer(LivePathEffectObject *lpeobject)
    : Effect(lpeobject),
      _satellites_param("satellites_param", "satellites_param",
                       "_satellites_param", &wr, this),
      _method(_("_method:"), _("_methods to calculate the fillet or chamfer"),
             "_method", FMConverter, &wr, this, FM_AUTO),
      _radius(_("_radius (unit or %):"), _("_radius, in unit or %"), "_radius", &wr,
             this, 0.0),
      _chamfer_steps(_("Chamfer steps:"), _("Chamfer steps"), "_chamfer_steps",
                    &wr, this, 1),
      _flexible(_("_flexible _radius size (%)"), _("_flexible _radius size (%)"),
               "_flexible", &wr, this, false),
      _mirror_knots(_("Mirror Knots"), _("Mirror Knots"), "_mirror_knots", &wr,
                   this, true),
      _only_selected(_("Change only selected nodes"),
                    _("Change only selected nodes"), "_only_selected", &wr, this,
                    false),
      _use_knot_distance(_("Use knots distance instead _radius"),
                        _("Use knots distance instead _radius"),
                        "_use_knot_distance", &wr, this, false),
      _hide_knots(_("Hide knots"), _("Hide knots"), "_hide_knots", &wr, this,
                 false),
      _apply_no_radius(_("Apply changes if _radius = 0"), _("Apply changes if _radius = 0"), "_apply_no_radius", &wr, this, true),
      _apply_with_radius(_("Apply changes if _radius > 0"), _("Apply changes if _radius > 0"), "_apply_with_radius", &wr, this, true),
      _helper_size(_("Helper path size with direction to node:"),
                  _("Helper path size with direction to node"), "_helper_size", &wr, this, 0),
      _pathvector_satellites(NULL),
      _degenerate_hide(false)
{
    registerParameter(&_satellites_param);
    registerParameter(&_method);
    registerParameter(&_radius);
    registerParameter(&_chamfer_steps);
    registerParameter(&_helper_size);
    registerParameter(&_flexible);
    registerParameter(&_use_knot_distance);
    registerParameter(&_mirror_knots);
    registerParameter(&_apply_no_radius);
    registerParameter(&_apply_with_radius);
    registerParameter(&_only_selected);
    registerParameter(&_hide_knots);

    _radius.param_set_range(0.0, Geom::infinity());
    _radius.param_set_increments(1, 1);
    _radius.param_set_digits(4);
    _radius.param_overwrite_widget(true);
    _chamfer_steps.param_set_range(1, 999);
    _chamfer_steps.param_set_increments(1, 1);
    _chamfer_steps.param_set_digits(0);
    _helper_size.param_set_range(0, 999);
    _helper_size.param_set_increments(5, 5);
    _helper_size.param_set_digits(0);
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
                satellite.setSteps(_chamfer_steps);
                subpath_satellites.push_back(satellite);
            }
            //we add the last satellite on open path because _pathvector_satellites is related to nodes, not curves
            //so maybe in the future we can need this last satellite in other effects
            //dont remove for this effect because _pathvector_satellites class has _methods when the path is modiffied
            //and we want one _method for all uses
            if (!path_it->closed()) {
                Satellite satellite(FILLET);
                satellite.setSteps(_chamfer_steps);
                subpath_satellites.push_back(satellite);
            }
            satellites.push_back(subpath_satellites);
        }
        _pathvector_satellites = new PathVectorSatellites();
        _pathvector_satellites->setPathVector(pathv);
        _pathvector_satellites->setSatellites(satellites);
        _satellites_param.setPathVectorSatellites(_pathvector_satellites);
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
            if (param->param_key == "_radius") {
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
            } else if (param->param_key == "_chamfer_steps") {
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
            } else if (param->param_key == "_helper_size") {
                Inkscape::UI::Widget::Scalar *widg_registered =
                    Gtk::manage(dynamic_cast<Inkscape::UI::Widget::Scalar *>(widg));
                widg_registered->signal_value_changed().connect(
                    sigc::mem_fun(*this, &LPEFilletChamfer::refreshKnots));
            } else if (param->param_key == "_only_selected") {
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
    Gtk::Button *fillet =  Gtk::manage(new Gtk::Button(Glib::ustring(_("Fillet"))));
    fillet->signal_clicked()
    .connect(sigc::bind<SatelliteType>(sigc::mem_fun(*this, &LPEFilletChamfer::updateSatelliteType),FILLET));

    fillet_container->pack_start(*fillet, true, true, 2);
    Gtk::Button *inverse_fillet = Gtk::manage(new Gtk::Button(Glib::ustring(_("Inverse fillet"))));
    inverse_fillet->signal_clicked()
    .connect(sigc::bind<SatelliteType>(sigc::mem_fun(*this, &LPEFilletChamfer::updateSatelliteType),INVERSE_FILLET));
    fillet_container->pack_start(*inverse_fillet, true, true, 2);

    Gtk::HBox *chamfer_container = Gtk::manage(new Gtk::HBox(true, 0));
    Gtk::Button *chamfer = Gtk::manage(new Gtk::Button(Glib::ustring(_("Chamfer"))));
    chamfer->signal_clicked()
    .connect(sigc::bind<SatelliteType>(sigc::mem_fun(*this, &LPEFilletChamfer::updateSatelliteType),CHAMFER));

    chamfer_container->pack_start(*chamfer, true, true, 2);
    Gtk::Button *inverse_chamfer = Gtk::manage(new Gtk::Button(Glib::ustring(_("Inverse chamfer"))));
    inverse_chamfer->signal_clicked()
    .connect(sigc::bind<SatelliteType>(sigc::mem_fun(*this, &LPEFilletChamfer::updateSatelliteType),INVERSE_CHAMFER));
    chamfer_container->pack_start(*inverse_chamfer, true, true, 2);

    vbox->pack_start(*fillet_container, true, true, 2);
    vbox->pack_start(*chamfer_container, true, true, 2);

    return vbox;
}

void LPEFilletChamfer::refreshKnots()
{
    if (_satellites_param._knoth) {
        _satellites_param._knoth->update_knots();
    }
}

void LPEFilletChamfer::updateAmount()
{
    _pathvector_satellites->updateAmount(_radius, _apply_no_radius, _apply_with_radius, _only_selected, 
                                         _use_knot_distance, _flexible);
    _satellites_param.setPathVectorSatellites(_pathvector_satellites);
}

void LPEFilletChamfer::updateChamferSteps()
{
    _pathvector_satellites->updateSteps(_chamfer_steps, _apply_no_radius, _apply_with_radius, _only_selected);
    _satellites_param.setPathVectorSatellites(_pathvector_satellites);
}

void LPEFilletChamfer::updateSatelliteType(SatelliteType satellitetype)
{
    _pathvector_satellites->updateSatelliteType(satellitetype, _apply_no_radius, _apply_with_radius, _only_selected);
    _satellites_param.setPathVectorSatellites(_pathvector_satellites);
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
        _satellites_param.setUseDistance(_use_knot_distance);
        _satellites_param.setCurrentZoom(getCurrentZoom());
        //mandatory call
        _satellites_param.setEffectType(effectType());
        Geom::PathVector const pathv = pathv_to_linear_and_cubic_beziers(c->get_pathvector());
        //if are diferent sizes call to poinwise recalculate
        //TODO: Update the satellite data in paths modified, Goal 0.93
        if (_pathvector_satellites) {
            size_t number_nodes = pathv.nodes().size();
            size_t previous_number_nodes = _pathvector_satellites->getPathVector().nodes().size();
            if (number_nodes != previous_number_nodes) {
                Satellite satellite(FILLET);
                satellite.setIsTime(_flexible);
                satellite.setHasMirror(_mirror_knots);
                satellite.setHidden(_hide_knots);
                _pathvector_satellites->recalculateForNewPathVector(pathv, satellite);
                _pathvector_satellites->setSelected(getSelectedNodes());
                _satellites_param.setPathVectorSatellites(_pathvector_satellites);
                refreshKnots();
                return;
            }
        }
        Satellites satellites = _satellites_param.data();
        if(satellites.empty()) {
            doOnApply(lpeItem);
            satellites = _satellites_param.data();
        }
        if (_degenerate_hide) {
            _satellites_param.setGlobalKnotHide(true);
        } else {
            _satellites_param.setGlobalKnotHide(false);
        }
        if (_hide_knots) {
            _satellites_param.setHelperSize(0);
        } else {
            _satellites_param.setHelperSize(_helper_size);
        }
        for (size_t i = 0; i < satellites.size(); ++i) {
            for (size_t j = 0; j < satellites[i].size(); ++j) {
                if (satellites[i][j].is_time != _flexible) {
                    satellites[i][j].is_time = _flexible;
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
                if (satellites[i][j].has_mirror != _mirror_knots) {
                    satellites[i][j].has_mirror = _mirror_knots;
                }
                satellites[i][j].hidden = _hide_knots;
            }
        }
        _pathvector_satellites = new PathVectorSatellites();
        _pathvector_satellites->setPathVector(pathv);
        _pathvector_satellites->setSatellites(satellites);
        _pathvector_satellites->setSelected(getSelectedNodes());
        _satellites_param.setPathVectorSatellites(_pathvector_satellites);
        refreshKnots();
    } else {
        g_warning("LPE Fillet can only be applied to shapes (not groups).");
    }
}

void
LPEFilletChamfer::addCanvasIndicators(SPLPEItem const */*lpeitem*/, std::vector<Geom::PathVector> &hp_vec)
{
    hp_vec.push_back(_hp);
}

void
LPEFilletChamfer::addChamferSteps(Geom::Path &tmp_path, Geom::Path path_chamfer, Geom::Point end_arc_point, size_t steps)
{
    double path_subdivision = 1.0 / steps;
    for (size_t i = 1; i < steps; i++) {
        Geom::Point chamfer_step = path_chamfer.pointAt(path_subdivision * i);
        tmp_path.appendNew<Geom::LineSegment>(chamfer_step);
    }
    tmp_path.appendNew<Geom::LineSegment>(end_arc_point);
}

Geom::PathVector
LPEFilletChamfer::doEffect_path(Geom::PathVector const &path_in)
{
    const double GAP_HELPER = 0.00001;
    Geom::PathVector path_out;
    size_t path = 0;
    const double K = (4.0 / 3.0) * (sqrt(2.0) - 1.0);
    _degenerate_hide = false;
    Geom::PathVector const pathv = pathv_to_linear_and_cubic_beziers(path_in);
    Satellites satellites = _pathvector_satellites->getSatellites();
    for (Geom::PathVector::const_iterator path_it = pathv.begin(); path_it != pathv.end(); ++path_it) {
        if (path_it->empty()) {
            continue;
        }
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
            Geom::Curve const &curve_it2 = pathv[path][next_index];
            Satellite satellite = satellites[path][next_index];
            if (Geom::are_near((*curve_it1).initialPoint(), (*curve_it1).finalPoint()) ||
                Geom::are_near(curve_it2.initialPoint(), curve_it2.finalPoint())) {
                _degenerate_hide = true;
                g_warning("Knots hidded if consecutive nodes has the same position.");
                return path_in;
            }
            if (!curve) { //curve == 0 
                if (!path_it->closed()) {
                    time0 = 0;
                } else {
                    time0 = satellites[path][0].time(*curve_it1);
                }
            }
            bool last = pathv[path].size() - 1 == curve;
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
            Geom::Curve *knot_curve_2 = curve_it2.portion(times[2], 1);
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
            if (times[1] != 1) {
                if (times[1] != times[0] || (times[1] == 1 && times[0] == 1)) {
                    if (!knot_curve_1->isDegenerate()) {
                        tmp_path.append(*knot_curve_1);
                    }
                }
                SatelliteType type = satellite.satellite_type;
                size_t steps = satellite.steps;
                if (!steps) steps = 1;
                Geom::Line const x_line(Geom::Point(0, 0), Geom::Point(1, 0));
                Geom::Line const angled_line(start_arc_point, end_arc_point);
                double arc_angle = Geom::angle_between(x_line, angled_line);
                double radius = Geom::distance(start_arc_point, middle_point(start_arc_point, end_arc_point)) /
                                                sin(angle / 2.0);
                Geom::Coord rx = radius;
                Geom::Coord ry = rx;
                bool eliptical = (is_straight_curve(*curve_it1) &&
                                  is_straight_curve(curve_it2) && _method != FM_BEZIER) ||
                                  _method == FM_ARC;
                switch (type) {
                case CHAMFER:
                    {
                        Geom::Path path_chamfer;
                        path_chamfer.start(tmp_path.finalPoint());
                        if (eliptical) {
                            ccw_toggle = ccw_toggle ? 0 : 1;
                            path_chamfer.appendNew<Geom::EllipticalArc>(rx, ry, arc_angle, 0, ccw_toggle, end_arc_point);
                        } else {
                            path_chamfer.appendNew<Geom::CubicBezier>(handle_1, handle_2, end_arc_point);
                        }
                        addChamferSteps(tmp_path, path_chamfer, end_arc_point, steps);
                    }
                    break;
                case INVERSE_CHAMFER:
                    {
                        Geom::Path path_chamfer;
                        path_chamfer.start(tmp_path.finalPoint());
                        if (eliptical) {
                            path_chamfer.appendNew<Geom::EllipticalArc>(rx, ry, arc_angle, 0, ccw_toggle, end_arc_point);
                        } else {
                            path_chamfer.appendNew<Geom::CubicBezier>(inverse_handle_1, inverse_handle_2, end_arc_point);
                        }
                        addChamferSteps(tmp_path, path_chamfer, end_arc_point, steps);
                    }
                    break;
                case INVERSE_FILLET:
                    {
                        if (eliptical) {
                            tmp_path.appendNew<Geom::EllipticalArc>(rx, ry, arc_angle, 0, ccw_toggle, end_arc_point);
                        } else {
                            tmp_path.appendNew<Geom::CubicBezier>(inverse_handle_1, inverse_handle_2, end_arc_point);
                        }
                    }
                    break;
                default: //fillet
                    {
                        if (eliptical) {
                            ccw_toggle = ccw_toggle ? 0 : 1;
                            tmp_path.appendNew<Geom::EllipticalArc>(rx, ry, arc_angle, 0, ccw_toggle, end_arc_point);
                        } else {
                            tmp_path.appendNew<Geom::CubicBezier>(handle_1, handle_2, end_arc_point);
                        }
                    }
                    break;
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

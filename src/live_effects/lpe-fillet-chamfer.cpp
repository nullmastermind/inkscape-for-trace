/*
 * Copyright (C) Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 * Special thanks to Johan Engelen for the base of the effect -powerstroke-
 * Also to ScislaC for point me to the idea
 * Also su_v for his construvtive feedback and time
 * Also to Mc- (IRC nick) for his important contribution to find real time
 * values based on
 * and finaly to Liam P. White for his big help on coding, that save me a lot of
 * hours
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/lpe-fillet-chamfer.h"
#include <glibmm/i18n.h>
#include "style.h"
#include "live_effects/parameter/filletchamferpointarray.h"
//for update knot
#include <2geom/svg-elliptical-arc.h>
#include <2geom/line.h>
#include "selection.h"
#include "ui/tool/control-point-selection.h"
#include "ui/tool/selectable-control-point.h"
#include <2geom/sbasis-to-bezier.h>
#include "ui/tool/node.h"
// FIXME: The following are only needed to convert the path's SPCurve* to pwd2.
//        There must be a more convenient way to achieve this.
#include "display/curve.h"
#include "helper/geom-nodetype.h"
#include "desktop.h"
#include "ui/tools/node-tool.h"
#include <iostream>
#include <tools-switch.h>
#include <cmath>

using namespace Geom;
namespace Inkscape {
namespace LivePathEffect {

const double tolerance = 0.001;
const double gapHelper = 0.00001;

LPEFilletChamfer::LPEFilletChamfer(LivePathEffectObject *lpeobject)
    : Effect(lpeobject),
      fillet_chamfer_values(_("Fillet point"), _("Fillet point"),
                            "fillet_chamfer_values", &wr, this),
      hide_knots(_("Hide knots"), _("Hide knots"), "hide_knots", &wr, this,
                 false),
      ignore_radius_0(_("Ignore 0 radius knots"), _("Ignore 0 radius knots"),
                      "ignore_radius_0", &wr, this, false),
      only_selected(_("Change only selected nodes"),
                    _("Change only selected nodes"), "only_selected", &wr, this,
                    false),
      flexible(_("Flexible radius size (%)"), _("Flexible radius size (%)"),
               "flexible", &wr, this, false),
      unit(_("Unit"), _("Unit"), "unit", &wr, this),
      radius(_("Radius (unit or %)"), _("Radius, in unit or %"), "radius", &wr,
             this, 0.),
      helper_size(_("Helper size with direction"),
                  _("Helper size with direction"), "helper_size", &wr, this,
                  0)
{
    registerParameter(dynamic_cast<Parameter *>(&fillet_chamfer_values));
    registerParameter(dynamic_cast<Parameter *>(&unit));
    registerParameter(dynamic_cast<Parameter *>(&radius));
    registerParameter(dynamic_cast<Parameter *>(&helper_size));
    registerParameter(dynamic_cast<Parameter *>(&flexible));
    registerParameter(dynamic_cast<Parameter *>(&ignore_radius_0));
    registerParameter(dynamic_cast<Parameter *>(&only_selected));
    registerParameter(dynamic_cast<Parameter *>(&hide_knots));
    radius.param_set_range(0., infinity());
    radius.param_set_increments(1, 1);
    radius.param_set_digits(4);
    helper_size.param_set_range(0, infinity());
    helper_size.param_set_increments(5, 5);
    helper_size.param_set_digits(0);
}

LPEFilletChamfer::~LPEFilletChamfer() {}

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
            Gtk::Widget *widg = dynamic_cast<Gtk::Widget *>(param->param_newWidget());
            if (param->param_key == "radius") {
                Inkscape::UI::Widget::Scalar *widgRegistered =
                    Gtk::manage(dynamic_cast<Inkscape::UI::Widget::Scalar *>(widg));
                widgRegistered->signal_value_changed()
                    .connect(sigc::mem_fun(*this, &LPEFilletChamfer::updateFillet));
                widg = dynamic_cast<Gtk::Widget *>(widgRegistered);
                if (widg) {
                    Gtk::HBox *scalarParameter = dynamic_cast<Gtk::HBox *>(widg);
                    std::vector<Gtk::Widget *> childList =
                        scalarParameter->get_children();
                    Gtk::Entry *entryWidg = dynamic_cast<Gtk::Entry *>(childList[1]);
                    entryWidg->set_width_chars(6);
                }
            }
            if (param->param_key == "flexible") {
                Gtk::CheckButton *widgRegistered =
                    Gtk::manage(dynamic_cast<Gtk::CheckButton *>(widg));
                widgRegistered->signal_clicked()
                    .connect(sigc::mem_fun(*this, &LPEFilletChamfer::toggleFlexFixed));
                widg = dynamic_cast<Gtk::Widget *>(widgRegistered);
            }
            if (param->param_key == "helper_size") {
                Inkscape::UI::Widget::Scalar *widgRegistered =
                    Gtk::manage(dynamic_cast<Inkscape::UI::Widget::Scalar *>(widg));
                widgRegistered->signal_value_changed()
                    .connect(sigc::mem_fun(*this, &LPEFilletChamfer::refreshKnots));
                widg = dynamic_cast<Gtk::Widget *>(widgRegistered);

            }
            if (param->param_key == "hide_knots") {
                Gtk::CheckButton *widgRegistered =
                    Gtk::manage(dynamic_cast<Gtk::CheckButton *>(widg));
                widgRegistered->signal_clicked()
                    .connect(sigc::mem_fun(*this, &LPEFilletChamfer::toggleHide));
                widg = dynamic_cast<Gtk::Widget *>(widgRegistered);
            }
            if (param->param_key == "only_selected") {
                Gtk::CheckButton *widgRegistered =
                    Gtk::manage(dynamic_cast<Gtk::CheckButton *>(widg));
                widg = dynamic_cast<Gtk::Widget *>(widgRegistered);
            }
            if (param->param_key == "ignore_radius_0") {
                Gtk::CheckButton *widgRegistered =
                    Gtk::manage(dynamic_cast<Gtk::CheckButton *>(widg));
                widg = dynamic_cast<Gtk::Widget *>(widgRegistered);
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

    Gtk::HBox *filletButtonsContainer = Gtk::manage(new Gtk::HBox(true, 0));
    Gtk::Button *fillet =
        Gtk::manage(new Gtk::Button(Glib::ustring(_("Fillet"))));
    fillet->signal_clicked()
        .connect(sigc::mem_fun(*this, &LPEFilletChamfer::fillet));
    filletButtonsContainer->pack_start(*fillet, true, true, 2);
    Gtk::Button *inverse =
        Gtk::manage(new Gtk::Button(Glib::ustring(_("Inverse fillet"))));
    inverse->signal_clicked()
        .connect(sigc::mem_fun(*this, &LPEFilletChamfer::inverse));
    filletButtonsContainer->pack_start(*inverse, true, true, 2);

    Gtk::HBox *chamferButtonsContainer = Gtk::manage(new Gtk::HBox(true, 0));
    Gtk::Button *chamfer =
        Gtk::manage(new Gtk::Button(Glib::ustring(_("Chamfer"))));
    chamfer->signal_clicked()
        .connect(sigc::mem_fun(*this, &LPEFilletChamfer::chamfer));
    chamferButtonsContainer->pack_start(*chamfer, true, true, 2);
    Gtk::Button *doubleChamfer =
        Gtk::manage(new Gtk::Button(Glib::ustring(_("Double chamfer"))));
    doubleChamfer->signal_clicked()
        .connect(sigc::mem_fun(*this, &LPEFilletChamfer::doubleChamfer));
    chamferButtonsContainer->pack_start(*doubleChamfer, true, true, 2);
    vbox->pack_start(*filletButtonsContainer, true, true, 2);
    vbox->pack_start(*chamferButtonsContainer, true, true, 2);

    return dynamic_cast<Gtk::Widget *>(vbox);
}

void LPEFilletChamfer::toggleHide()
{
    std::vector<Point> filletChamferData = fillet_chamfer_values.data();
    std::vector<Geom::Point> result;
    for (std::vector<Point>::const_iterator point_it = filletChamferData.begin();
            point_it != filletChamferData.end(); ++point_it) {
        if (hide_knots) {
            result.push_back(Point((*point_it)[X], abs((*point_it)[Y]) * -1));
        } else {
            result.push_back(Point((*point_it)[X], abs((*point_it)[Y])));
        }
    }
    fillet_chamfer_values.param_set_and_write_new_value(result);
    refreshKnots();
}

void LPEFilletChamfer::toggleFlexFixed()
{
    std::vector<Point> filletChamferData = fillet_chamfer_values.data();
    std::vector<Geom::Point> result;
    unsigned int i = 0;
    for (std::vector<Point>::const_iterator point_it = filletChamferData.begin();
            point_it != filletChamferData.end(); ++point_it) {
        if (flexible) {
            result.push_back(Point(fillet_chamfer_values.to_time(i, (*point_it)[X]),
                                   (*point_it)[Y]));
        } else {
            result.push_back(Point(fillet_chamfer_values.to_len(i, (*point_it)[X]),
                                   (*point_it)[Y]));
        }
        i++;
    }
    if (flexible) {
        radius.param_set_range(0., 100);
        radius.param_set_value(0);
    } else {
        radius.param_set_range(0., infinity());
        radius.param_set_value(0);
    }
    fillet_chamfer_values.param_set_and_write_new_value(result);
}

void LPEFilletChamfer::updateFillet()
{
    double power = 0;
    if (!flexible) {
        power = Inkscape::Util::Quantity::convert(radius, unit.get_abbreviation(),
                "px") * -1;
    } else {
        power = radius;
    }
    Piecewise<D2<SBasis> > const &pwd2 = fillet_chamfer_values.get_pwd2();
    doUpdateFillet(path_from_piecewise(pwd2, tolerance), power);
}

void LPEFilletChamfer::fillet()
{
    Piecewise<D2<SBasis> > const &pwd2 = fillet_chamfer_values.get_pwd2();
    doChangeType(path_from_piecewise(pwd2, tolerance), 1);
}

void LPEFilletChamfer::inverse()
{
    Piecewise<D2<SBasis> > const &pwd2 = fillet_chamfer_values.get_pwd2();
    doChangeType(path_from_piecewise(pwd2, tolerance), 2);
}

void LPEFilletChamfer::chamfer()
{
    Piecewise<D2<SBasis> > const &pwd2 = fillet_chamfer_values.get_pwd2();
    doChangeType(path_from_piecewise(pwd2, tolerance), 3);
}

void LPEFilletChamfer::doubleChamfer()
{
    Piecewise<D2<SBasis> > const &pwd2 = fillet_chamfer_values.get_pwd2();
    doChangeType(path_from_piecewise(pwd2, tolerance), 4);
}

void LPEFilletChamfer::refreshKnots()
{
    Piecewise<D2<SBasis> > const &pwd2 = fillet_chamfer_values.get_pwd2();
    fillet_chamfer_values.recalculate_knots(pwd2);
    SPDesktop *desktop = inkscape_active_desktop();
    if (tools_isactive(desktop, TOOLS_NODES)) {
        tools_switch(desktop, TOOLS_SELECT);
        tools_switch(desktop, TOOLS_NODES);
    }
}

bool LPEFilletChamfer::nodeIsSelected(Geom::Point nodePoint,
                                      std::vector<Geom::Point> point)
{
    if (point.size() > 0) {
        for (std::vector<Geom::Point>::iterator i = point.begin(); i != point.end();
                ++i) {
            Geom::Point p = *i;
            if (Geom::are_near(p, nodePoint, 2)) {
                return true;
            }
        }
    }
    return false;
}
void LPEFilletChamfer::doUpdateFillet(std::vector<Geom::Path> original_pathv,
                                      double power)
{
    std::vector<Geom::Point> point;
    SPDesktop *desktop = inkscape_active_desktop();
    if (INK_IS_NODE_TOOL(desktop->event_context)) {
        Inkscape::UI::Tools::NodeTool *nodeTool =
            INK_NODE_TOOL(desktop->event_context);
        Inkscape::UI::ControlPointSelection::Set &selection =
            nodeTool->_selected_nodes->allPoints();
        std::vector<Geom::Point>::iterator pBegin;
        for (Inkscape::UI::ControlPointSelection::Set::iterator i =
                    selection.begin();
                i != selection.end(); ++i) {
            if ((*i)->selected()) {
                Inkscape::UI::Node *n = dynamic_cast<Inkscape::UI::Node *>(*i);
                pBegin = point.begin();
                point.insert(pBegin, desktop->doc2dt(n->position()));
            }
        }
    }
    std::vector<Point> filletChamferData = fillet_chamfer_values.data();
    std::vector<Geom::Point> result;
    int counter = 0;
    for (PathVector::const_iterator path_it = original_pathv.begin();
            path_it != original_pathv.end(); ++path_it) {
        int pathCounter = 0;
        if (path_it->empty())
            continue;

        Geom::Path::const_iterator curve_it1 = path_it->begin();
        Geom::Path::const_iterator curve_it2 = ++(path_it->begin());
        Geom::Path::const_iterator curve_endit = path_it->end_default();
        if (path_it->closed() && path_it->back_closed().isDegenerate()) {
            const Curve &closingline = path_it->back_closed();
            if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
                curve_endit = path_it->end_open();
            }
        }
        double powerend = 0;
        while (curve_it1 != curve_endit) {
            powerend = power;
            if (power > 0) {
                powerend = counter + (power / 100);
            }
            if (ignore_radius_0 && (filletChamferData[counter][X] == 0 ||
                                    filletChamferData[counter][X] == counter)) {
                powerend = filletChamferData[counter][X];
            }
            if (filletChamferData[counter][Y] == 0) {
                powerend = filletChamferData[counter][X];
            }
            if (only_selected && !nodeIsSelected(curve_it1->initialPoint(), point)) {
                powerend = filletChamferData[counter][X];
            }
            result.push_back(Point(powerend, filletChamferData[counter][Y]));
            ++curve_it1;
            ++curve_it2;
            counter++;
            pathCounter++;
        }
    }
    fillet_chamfer_values.param_set_and_write_new_value(result);
}

void LPEFilletChamfer::doChangeType(std::vector<Geom::Path> original_pathv,
                                    int type)
{
    std::vector<Geom::Point> point;
    SPDesktop *desktop = inkscape_active_desktop();
    if (INK_IS_NODE_TOOL(desktop->event_context)) {
        Inkscape::UI::Tools::NodeTool *nodeTool =
            INK_NODE_TOOL(desktop->event_context);
        Inkscape::UI::ControlPointSelection::Set &selection =
            nodeTool->_selected_nodes->allPoints();
        std::vector<Geom::Point>::iterator pBegin;
        for (Inkscape::UI::ControlPointSelection::Set::iterator i =
                    selection.begin();
                i != selection.end(); ++i) {
            if ((*i)->selected()) {
                Inkscape::UI::Node *n = dynamic_cast<Inkscape::UI::Node *>(*i);
                pBegin = point.begin();
                point.insert(pBegin, desktop->doc2dt(n->position()));
            }
        }
    }
    std::vector<Point> filletChamferData = fillet_chamfer_values.data();
    std::vector<Geom::Point> result;
    int counter = 0;
    for (PathVector::const_iterator path_it = original_pathv.begin();
            path_it != original_pathv.end(); ++path_it) {
        int pathCounter = 0;
        if (path_it->empty())
            continue;

        Geom::Path::const_iterator curve_it1 = path_it->begin();
        Geom::Path::const_iterator curve_it2 = ++(path_it->begin());
        Geom::Path::const_iterator curve_endit = path_it->end_default();
        if (path_it->closed() && path_it->back_closed().isDegenerate()) {
            const Curve &closingline = path_it->back_closed();
            if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
                curve_endit = path_it->end_open();
            }
        }
        while (curve_it1 != curve_endit) {
            bool toggle = true;
            if (filletChamferData[counter][Y] == 0 ||
                    (ignore_radius_0 && (filletChamferData[counter][X] == 0 ||
                                         filletChamferData[counter][X] == counter)) ||
                    (only_selected &&
                     !nodeIsSelected(curve_it1->initialPoint(), point))) {
                toggle = false;
            }
            if (toggle) {
                result.push_back(Point(filletChamferData[counter][X], type));
            } else {
                result.push_back(filletChamferData[counter]);
            }
            ++curve_it1;
            if (curve_it2 != curve_endit) {
                ++curve_it2;
            }
            counter++;
            pathCounter++;
        }
    }
    fillet_chamfer_values.param_set_and_write_new_value(result);
}

void LPEFilletChamfer::doOnApply(SPLPEItem const *lpeItem)
{
    if (SP_IS_SHAPE(lpeItem)) {
        std::vector<Point> point;
        PathVector const &original_pathv =
            SP_SHAPE(lpeItem)->_curve->get_pathvector();
        Piecewise<D2<SBasis> > pwd2_in = paths_to_pw(original_pathv);
        for (PathVector::const_iterator path_it = original_pathv.begin();
                path_it != original_pathv.end(); ++path_it) {
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
                Geom::NodeType nodetype;
                if (counter == 0) {
                    if (path_it->closed()) {
                        nodetype = get_nodetype(path_it->back_default(), *curve_it1);
                    } else {
                        nodetype = NODE_NONE;
                    }
                } else {
                    nodetype = get_nodetype((*path_it)[counter - 1], *curve_it1);
                }
                if (nodetype == NODE_CUSP) {
                    point.push_back(Point(0, 1));
                } else {
                    point.push_back(Point(0, 0));
                }
                ++curve_it1;
                if (curve_it2 != curve_endit) {
                    ++curve_it2;
                }
                counter++;
            }
        }
        fillet_chamfer_values.param_set_and_write_new_value(point);
    } else {
        g_warning("LPE Fillet can only be applied to shapes (not groups).");
    }
}

void LPEFilletChamfer::doBeforeEffect(SPLPEItem const *lpeItem)
{
    if (SP_IS_SHAPE(lpeItem)) {
        fillet_chamfer_values.set_helper_size(helper_size);
        fillet_chamfer_values.set_unit(unit.get_abbreviation());
        SPCurve *c = SP_IS_PATH(lpeItem) ? static_cast<SPPath const *>(lpeItem)
                     ->get_original_curve()
                     : SP_SHAPE(lpeItem)->getCurve();
        std::vector<Point> filletChamferData = fillet_chamfer_values.data();
        if (!filletChamferData.empty() && getKnotsNumber(c) != (int)
                filletChamferData.size()) {
            PathVector const original_pathv = c->get_pathvector();
            Piecewise<D2<SBasis> > pwd2_in = paths_to_pw(original_pathv);
            fillet_chamfer_values.recalculate_controlpoints_for_new_pwd2(pwd2_in);
        }
    } else {
        g_warning("LPE Fillet can only be applied to shapes (not groups).");
    }
}

int LPEFilletChamfer::getKnotsNumber(SPCurve const *c)
{
    int nKnots = c->nodes_in_path();
    PathVector const pv = c->get_pathvector();
    for (std::vector<Geom::Path>::const_iterator path_it = pv.begin();
            path_it != pv.end(); ++path_it) {
        if (!(*path_it).closed()) {
            nKnots--;
        }
    }
    return nKnots;
}

void
LPEFilletChamfer::adjustForNewPath(std::vector<Geom::Path> const &path_in)
{
    if (!path_in.empty()) {
        fillet_chamfer_values.recalculate_controlpoints_for_new_pwd2(path_in[0]
                .toPwSb());
    }
}

std::vector<Geom::Path>
LPEFilletChamfer::doEffect_path(std::vector<Geom::Path> const &path_in)
{
    std::vector<Geom::Path> pathvector_out;
    Piecewise<D2<SBasis> > pwd2_in = paths_to_pw(path_in);
    pwd2_in = remove_short_cuts(pwd2_in, .01);
    Piecewise<D2<SBasis> > der = derivative(pwd2_in);
    Piecewise<D2<SBasis> > n = rot90(unitVector(der));
    fillet_chamfer_values.set_pwd2(pwd2_in, n);
    std::vector<Point> filletChamferData = fillet_chamfer_values.data();
    int counter = 0;
    //from http://launchpadlibrarian.net/12692602/rcp.svg
    const double K = (4.0 / 3.0) * (sqrt(2.0) - 1.0);
    for (PathVector::const_iterator path_it = path_in.begin();
            path_it != path_in.end(); ++path_it) {
        if (path_it->empty())
            continue;
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
        int counterCurves = 0;
        while (curve_it1 != curve_endit) {
            Coord it1_length = (*curve_it1).length(tolerance);
            double time_it1, time_it2, time_it1_B, intpart;
            time_it1 = modf(
                           fillet_chamfer_values.to_time(counter, filletChamferData[counter][X]),
                           &intpart);
            if (filletChamferData[counter][Y] == 0) {
                time_it1 = 0;
            }
            time_it2 = modf(fillet_chamfer_values.to_time(
                                counter + 1, filletChamferData[counter + 1][X]),
                            &intpart);
            if (curve_it2 == curve_endit) {
                time_it2 = modf(fillet_chamfer_values.to_time(
                                    counter - counterCurves,
                                    filletChamferData[counter - counterCurves][X]),
                                &intpart);
            }
            double resultLenght =
                it1_length + fillet_chamfer_values.to_len(
                    counter + 1, filletChamferData[counter + 1][X]);
            time_it1_B = 1;
            if (path_it->closed() && curve_it2 == curve_endit) {
                resultLenght =
                    it1_length + fillet_chamfer_values.to_len(
                        counter - counterCurves,
                        filletChamferData[counter - counterCurves][X]);
            }
            if (resultLenght > 0 && time_it2 != 0) {
                time_it1_B = modf(fillet_chamfer_values.to_time(counter, -resultLenght),
                                  &intpart);
            } else {
                if (time_it2 == 0) {
                    time_it1_B = 1;
                } else {
                    time_it1_B = gapHelper;
                }
            }
            if (filletChamferData[counter + 1][Y] == 0) {
                time_it1_B = 1;
                time_it2 = 0;
            }
            if (time_it1_B < time_it1) {
                time_it1_B = time_it1 + gapHelper;
            }
            Curve *knotCurve1 = curve_it1->portion(time_it1, time_it1_B);
            if (counterCurves > 0) {
                knotCurve1->setInitial(path_out.finalPoint());
            } else {
                path_out.start((*curve_it1).pointAt(time_it1));
            }
            Curve *knotCurve2 = (*path_it).front().portion(time_it2, 1);
            if (curve_it2 != curve_endit) {
                knotCurve2 = (*curve_it2).portion(time_it2, 1);
            }
            Point startArcPoint = knotCurve1->finalPoint();
            Point endArcPoint = (*path_it).front().pointAt(time_it2);
            if (curve_it2 != curve_endit) {
                endArcPoint = (*curve_it2).pointAt(time_it2);
            }
            double k1 = distance(startArcPoint, curve_it1->finalPoint()) * K;
            double k2 = distance(endArcPoint, curve_it1->finalPoint()) * K;
            Geom::CubicBezier const *cubic1 =
                dynamic_cast<Geom::CubicBezier const *>(&*knotCurve1);
            Ray ray1(startArcPoint, curve_it1->finalPoint());
            if (cubic1) {
                ray1.setPoints((*cubic1)[2], startArcPoint);
            }
            Point handle1 =  Point::polar(ray1.angle(),k1) + startArcPoint;
            Geom::CubicBezier const *cubic2 =
                dynamic_cast<Geom::CubicBezier const *>(&*knotCurve2);
            Ray ray2(curve_it1->finalPoint(), endArcPoint);
            if (cubic2) {
                ray2.setPoints(endArcPoint, (*cubic2)[1]);
            }
            Point handle2 = endArcPoint - Point::polar(ray2.angle(),k2);
            bool ccwToggle = cross(curve_it1->finalPoint() - startArcPoint,
                                   endArcPoint - startArcPoint) < 0;
            double angle = angle_between(ray1, ray2, ccwToggle);
            double handleAngle = ray1.angle() - angle;
            if (ccwToggle) {
                handleAngle = ray1.angle() + angle;
            }
            Point inverseHandle1 = Point::polar(handleAngle,k1) + startArcPoint;
            handleAngle = ray2.angle() + angle;
            if (ccwToggle) {
                handleAngle = ray2.angle() - angle;
            }
            Point inverseHandle2 = endArcPoint - Point::polar(handleAngle,k2);
            if (time_it1_B != 1) {
                if (time_it1_B != gapHelper && time_it1_B != time_it1 + gapHelper) {
                    path_out.append(*knotCurve1);
                }
                int type = abs(filletChamferData[counter + 1][Y]);
                if (type == 3 || type == 4) {
                    if (type == 4) {
                        Geom::Point central = middle_point(startArcPoint, endArcPoint);
                        LineSegment chamferCenter(central, curve_it1->finalPoint());
                        path_out.appendNew<Geom::LineSegment>(chamferCenter.pointAt(0.5));
                    }
                    path_out.appendNew<Geom::LineSegment>(endArcPoint);
                } else if (type == 2) {
                    path_out.appendNew<Geom::CubicBezier>(inverseHandle1, inverseHandle2,
                                                          endArcPoint);
                } else {
                    path_out.appendNew<Geom::CubicBezier>(handle1, handle2, endArcPoint);
                }
            } else {
                path_out.append(*knotCurve1);
            }
            if (path_it->closed() && curve_it2 == curve_endit) {
                path_out.close();
            }
            ++curve_it1;
            if (curve_it2 != curve_endit) {
                ++curve_it2;
            }
            counter++;
            counterCurves++;
        }
        pathvector_out.push_back(path_out);
    }
    return pathvector_out;
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

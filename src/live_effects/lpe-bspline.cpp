/*
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtkmm.h>

#if WITH_GLIBMM_2_32
# include <glibmm/threads.h>
#endif

#include <glib.h>
#include <glibmm/i18n.h>


#include "display/curve.h"
#include <2geom/bezier-curve.h>
#include <2geom/point.h>
#include "helper/geom-curves.h"
#include "live_effects/lpe-bspline.h"
#include "live_effects/lpeobject.h"
#include "live_effects/parameter/parameter.h"
#include "ui/widget/scalar.h"
#include "ui/tool/node.h"
#include "ui/tools/node-tool.h"
#include "ui/tool/control-point-selection.h"
#include "ui/tool/selectable-control-point.h"
#include "selection.h"
#include "xml/repr.h"
#include "svg/svg.h"
#include "sp-path.h"
#include "style.h"
#include "document-private.h"
#include "document.h"
#include "document-undo.h"
#include "desktop-handles.h"
#include "verbs.h"
#include "sp-lpe-item.h"
#include "display/sp-canvas.h"
#include <typeinfo>
#include <vector>
// For handling un-continuous paths:
#include "message-stack.h"
#include "inkscape.h"
#include "desktop.h"

using Inkscape::DocumentUndo;

namespace Inkscape {
namespace LivePathEffect {

LPEBSpline::LPEBSpline(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    steps(_("Steps whith CTRL:"), _("Change number of steps whith CTRL pressed"), "steps", &wr, this, 2),
    ignoreCusp(_("Ignore cusp nodes"), _("Change ignoring cusp nodes"), "ignoreCusp", &wr, this, true),
    onlySelected(_("Change only selected nodes"), _("Change only selected nodes"), "onlySelected", &wr, this, false),
    weight(_("Change weight:"), _("Change weight of the effect"), "weight", &wr, this, 0.3334)
{
    registerParameter(&weight);
    registerParameter(&steps);
    registerParameter(&ignoreCusp);
    registerParameter(&onlySelected);

    weight.param_set_range(0.0000, 1);
    weight.param_set_increments(0.1, 0.1);
    weight.param_set_digits(4);

    steps.param_set_range(1, 10);
    steps.param_set_increments(1, 1);
    steps.param_set_digits(0);
}

LPEBSpline::~LPEBSpline() {}

void LPEBSpline::createAndApply(const char *name, SPDocument *doc, SPItem *item)
{
    if (!SP_IS_SHAPE(item)) {
        g_warning("LPE BSpline can only be applied to shapes (not groups).");
    } else {
        // Path effect definition
        Inkscape::XML::Document *xml_doc = doc->getReprDoc();
        Inkscape::XML::Node *repr = xml_doc->createElement("inkscape:path-effect");
        repr->setAttribute("effect", name);

        doc->getDefs()->getRepr()->addChild(repr, NULL); // adds to <defs> and assigns the 'id' attribute
        const gchar *repr_id = repr->attribute("id");
        Inkscape::GC::release(repr);

        gchar *href = g_strdup_printf("#%s", repr_id);
        SP_LPE_ITEM(item)->addPathEffect(href, true);
        g_free(href);
    }
}

void LPEBSpline::doEffect(SPCurve *curve)
{
    if (curve->get_segment_count() < 1)
        return;
    // Make copy of old path as it is changed during processing
    Geom::PathVector const original_pathv = curve->get_pathvector();
    curve->reset();

    //Recorremos todos los paths a los que queremos aplicar el efecto, hasta el
    //penúltimo
    for (Geom::PathVector::const_iterator path_it = original_pathv.begin();
            path_it != original_pathv.end(); ++path_it) {
        //Si está vacío...
        if (path_it->empty())
            continue;
        //Itreadores

        Geom::Path::const_iterator curve_it1 = path_it->begin(); // incoming curve
        Geom::Path::const_iterator curve_it2 =
            ++(path_it->begin());                                // outgoing curve
        Geom::Path::const_iterator curve_endit =
            path_it->end_default(); // this determines when the loop has to stop
        //Creamos las lineas rectas que unen todos los puntos del trazado y donde se
        //calcularán
        //los puntos clave para los manejadores.
        //Esto hace que la curva BSpline no pierda su condición aunque se trasladen
        //dichos manejadores
        SPCurve *nCurve = new SPCurve();
        Geom::Point previousNode(0, 0);
        Geom::Point node(0, 0);
        Geom::Point pointAt1(0, 0);
        Geom::Point pointAt2(0, 0);
        Geom::Point nextPointAt1(0, 0);
        Geom::Point nextPointAt2(0, 0);
        Geom::Point nextPointAt3(0, 0);
        Geom::D2<Geom::SBasis> SBasisIn;
        Geom::D2<Geom::SBasis> SBasisOut;
        Geom::D2<Geom::SBasis> SBasisHelper;
        Geom::CubicBezier const *cubic = NULL;
        if (path_it->closed()) {
            // if the path is closed, maybe we have to stop a bit earlier because the
            // closing line segment has zerolength.
            const Geom::Curve &closingline =
                path_it->back_closed(); // the closing line segment is always of type
            // Geom::LineSegment.
            if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
                // closingline.isDegenerate() did not work, because it only checks for
                // *exact* zero length, which goes wrong for relative coordinates and
                // rounding errors...
                // the closing line segment has zero-length. So stop before that one!
                curve_endit = path_it->end_open();
            }
        }
        //Si la curva está cerrada calculamos el punto donde
        //deveria estar el nodo BSpline de cierre/inicio de la curva
        //en posible caso de que se cierre con una linea recta creando un nodo
        //BSPline
        nCurve->moveto(curve_it1->initialPoint());
        //Recorremos todos los segmentos menos el último
        while (curve_it2 != curve_endit) {
            //previousPointAt3 = pointAt3;
            //Calculamos los puntos que dividirían en tres segmentos iguales el path
            //recto de entrada y de salida
            SPCurve *in = new SPCurve();
            in->moveto(curve_it1->initialPoint());
            in->lineto(curve_it1->finalPoint());
            cubic = dynamic_cast<Geom::CubicBezier const *>(&*curve_it1);
            if (cubic) {
                SBasisIn = in->first_segment()->toSBasis();
                pointAt1 = SBasisIn.valueAt(
                               Geom::nearest_point((*cubic)[1], *in->first_segment()));
                pointAt2 = SBasisIn.valueAt(
                               Geom::nearest_point((*cubic)[2], *in->first_segment()));
            } else {
                pointAt1 = in->first_segment()->initialPoint();
                pointAt2 = in->first_segment()->finalPoint();
            }
            in->reset();
            delete in;
            //Y hacemos lo propio con el path de salida
            //nextPointAt0 = curveOut.valueAt(0);
            SPCurve *out = new SPCurve();
            out->moveto(curve_it2->initialPoint());
            out->lineto(curve_it2->finalPoint());
            cubic = dynamic_cast<Geom::CubicBezier const *>(&*curve_it2);
            if (cubic) {
                SBasisOut = out->first_segment()->toSBasis();
                nextPointAt1 = SBasisOut.valueAt(
                                   Geom::nearest_point((*cubic)[1], *out->first_segment()));
                nextPointAt2 = SBasisOut.valueAt(
                                   Geom::nearest_point((*cubic)[2], *out->first_segment()));
                ;
                nextPointAt3 = out->first_segment()->finalPoint();
            } else {
                nextPointAt1 = out->first_segment()->initialPoint();
                nextPointAt2 = out->first_segment()->finalPoint();
                nextPointAt3 = out->first_segment()->finalPoint();
            }
            out->reset();
            delete out;
            //La curva BSpline se forma calculando el centro del segmanto de unión
            //de el punto situado en las 2/3 partes de el segmento de entrada
            //con el punto situado en la posición 1/3 del segmento de salida
            //Estos dos puntos ademas estan posicionados en el lugas correspondiente
            //de
            //los manejadores de la curva
            SPCurve *lineHelper = new SPCurve();
            lineHelper->moveto(pointAt2);
            lineHelper->lineto(nextPointAt1);
            SBasisHelper = lineHelper->first_segment()->toSBasis();
            lineHelper->reset();
            delete lineHelper;
            //almacenamos el punto del anterior bucle -o el de cierre- que nos hara de
            //principio de curva
            previousNode = node;
            //Y este hará de final de curva
            node = SBasisHelper.valueAt(0.5);
            nCurve->curveto(pointAt1, pointAt2, node);
            //aumentamos los valores para el siguiente paso en el bucle
            ++curve_it1;
            ++curve_it2;
        }
        SPCurve *out = new SPCurve();
        out->moveto(curve_it1->initialPoint());
        out->lineto(curve_it1->finalPoint());
        cubic = dynamic_cast<Geom::CubicBezier const *>(&*curve_it1);
        if (cubic) {
            SBasisOut = out->first_segment()->toSBasis();
            nextPointAt1 = SBasisOut.valueAt(Geom::nearest_point((*cubic)[1], *out->first_segment()));
            nextPointAt2 = SBasisOut.valueAt(Geom::nearest_point((*cubic)[2], *out->first_segment()));
            nextPointAt3 = out->first_segment()->finalPoint();
        } else {
            nextPointAt1 = out->first_segment()->initialPoint();
            nextPointAt2 = out->first_segment()->finalPoint();
            nextPointAt3 = out->first_segment()->finalPoint();
        }
        out->reset();
        delete out;
        //Si está cerrada la curva, la cerramos sobre  el valor guardado
        //previamente
        //Si no finalizamos en el punto final
        Geom::Point startNode = path_it->begin()->initialPoint();
        if (path_it->closed()) {
            SPCurve *start = new SPCurve();
            start->moveto(path_it->begin()->initialPoint());
            start->lineto(path_it->begin()->finalPoint());
            Geom::D2<Geom::SBasis> SBasisStart = start->first_segment()->toSBasis();
            SPCurve *lineHelper = new SPCurve();
            cubic = dynamic_cast<Geom::CubicBezier const *>(&*path_it->begin());
            if (cubic) {
                lineHelper->moveto(SBasisStart.valueAt(
                                       Geom::nearest_point((*cubic)[1], *start->first_segment())));
            } else {
                lineHelper->moveto(start->first_segment()->initialPoint());
            }
            start->reset();
            delete start;

            SPCurve *end = new SPCurve();
            end->moveto(curve_it1->initialPoint());
            end->lineto(curve_it1->finalPoint());
            Geom::D2<Geom::SBasis> SBasisEnd = end->first_segment()->toSBasis();

            cubic = dynamic_cast<Geom::CubicBezier const *>(&*curve_it1);
            if (cubic) {
                lineHelper->lineto(SBasisEnd.valueAt(
                                       Geom::nearest_point((*cubic)[2], *end->first_segment())));
            } else {
                lineHelper->lineto(end->first_segment()->finalPoint());
            }
            end->reset();
            delete end;
            SBasisHelper = lineHelper->first_segment()->toSBasis();
            lineHelper->reset();
            delete lineHelper;
            startNode = SBasisHelper.valueAt(0.5);
            nCurve->curveto(nextPointAt1, nextPointAt2, startNode);
            nCurve->move_endpoints(startNode, startNode);
        } else {
            SPCurve *start = new SPCurve();
            start->moveto(path_it->begin()->initialPoint());
            start->lineto(path_it->begin()->finalPoint());
            startNode = start->first_segment()->initialPoint();
            start->reset();
            delete start;
            nCurve->curveto(nextPointAt1, nextPointAt2, nextPointAt3);
            nCurve->move_endpoints(startNode, nextPointAt3);
        }
        //y cerramos la curva
        if (path_it->closed()) {
            nCurve->closepath_current();
        }
        curve->append(nCurve, false);
        nCurve->reset();
        delete nCurve;
    }
}

Gtk::Widget *LPEBSpline::newWidget()
{
    // use manage here, because after deletion of Effect object, others might
    // still be pointing to this widget.
    Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox(Effect::newWidget()));

    vbox->set_border_width(5);
    std::vector<Parameter *>::iterator it = param_vector.begin();
    while (it != param_vector.end()) {
        if ((*it)->widget_is_visible) {
            Parameter *param = *it;
            Gtk::Widget *widg = Gtk::manage(param->param_newWidget());
            if (param->param_key == "weight") {
                Gtk::HBox * buttons = Gtk::manage(new Gtk::HBox(true, 0));

                Gtk::Button *defaultWeight = Gtk::manage(new Gtk::Button(Glib::ustring(_("Default weight"))));
                defaultWeight->signal_clicked().connect(sigc::bind<Gtk::Widget *>(sigc::mem_fun(*this, &LPEBSpline::toDefaultWeight), widg));
                buttons->pack_start(*defaultWeight, true, true, 2);

                Gtk::Button *makeCusp = Gtk::manage(new Gtk::Button(Glib::ustring(_("Make cusp"))));
                makeCusp->signal_clicked().connect(sigc::bind<Gtk::Widget *>(sigc::mem_fun(*this, &LPEBSpline::toMakeCusp), widg));
                buttons->pack_start(*makeCusp, true, true, 2);

                vbox->pack_start(*buttons, true, true, 2);
            }
            if (param->param_key == "weight" || param->param_key == "steps") {
                Inkscape::UI::Widget::Scalar *widgRegistered = Gtk::manage(dynamic_cast<Inkscape::UI::Widget::Scalar *>(widg));
                widgRegistered->signal_value_changed().connect(sigc::mem_fun(*this, &LPEBSpline::toWeight));

                if (widg) {
                    Gtk::HBox * scalarParameter = dynamic_cast<Gtk::HBox *>(widg);
                    std::vector<Gtk::Widget *> childList = scalarParameter->get_children();
                    Gtk::Entry* entryWidg = dynamic_cast<Gtk::Entry *>(childList[1]);
                    entryWidg->set_width_chars(6);
                }
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
    return vbox;
}

void LPEBSpline::toDefaultWeight(Gtk::Widget *widgWeight)
{
    weight.param_set_value(0.3334);
    changeWeight(0.3334);
    Gtk::HBox * scalarParameter = dynamic_cast<Gtk::HBox *>(widgWeight);
    std::vector< Gtk::Widget* > childList = scalarParameter->get_children();
    Gtk::Entry* entryWidg = dynamic_cast<Gtk::Entry *>(childList[1]);
    entryWidg->set_text("0.3334");
}

void LPEBSpline::toMakeCusp(Gtk::Widget *widgWeight)
{
    weight.param_set_value(0.0000);
    changeWeight(0.0000);
    Gtk::HBox * scalarParameter = dynamic_cast<Gtk::HBox *>(widgWeight);
    std::vector< Gtk::Widget* > childList = scalarParameter->get_children();
    Gtk::Entry* entryWidg = dynamic_cast<Gtk::Entry *>(childList[1]);
    entryWidg->set_text("0.0000");
}

void LPEBSpline::toWeight()
{
    changeWeight(weight);
}

void LPEBSpline::changeWeight(double weightValue)
{
    SPDesktop *desktop = inkscape_active_desktop();
    Inkscape::Selection *selection = sp_desktop_selection(desktop);
    GSList *items = (GSList *)selection->itemList();
    SPItem *item = (SPItem *)g_slist_nth(items, 0)->data;
    SPPath *path = SP_PATH(item);
    SPCurve *curve = path->get_curve_for_edit();
    LPEBSpline::doBSplineFromWidget(curve, weightValue);
    gchar *str = sp_svg_write_path(curve->get_pathvector());
    path->getRepr()->setAttribute("inkscape:original-d", str);
    if (INK_IS_NODE_TOOL(desktop->event_context)) {
        Inkscape::UI::Tools::NodeTool *nt = INK_NODE_TOOL(desktop->event_context);
        nt->desktop->updateNow();
    }
    g_free(str);
    curve->unref();
    desktop->clearWaitingCursor();
    DocumentUndo::done(sp_desktop_document(desktop), SP_VERB_CONTEXT_LPE, _("Modified the weight of a BSpline"));
}

bool LPEBSpline::nodeIsSelected(Geom::Point nodePoint)
{
    using Geom::X;
    using Geom::Y;

    if (points.size() > 0) {
        for (std::vector<Geom::Point>::iterator i = points.begin();
                i != points.end(); ++i) {
            Geom::Point p = *i;
            if (Geom::are_near(p, nodePoint, 0.0001)) {
                return true;
            } else {
            }
        }
    }
    return false;
}

void LPEBSpline::doBSplineFromWidget(SPCurve *curve, double weightValue)
{
    using Geom::X;
    using Geom::Y;
    SPDesktop *desktop = inkscape_active_desktop();
    if (INK_IS_NODE_TOOL(desktop->event_context)) {
        Inkscape::UI::Tools::NodeTool *nt = INK_NODE_TOOL(desktop->event_context);
        Inkscape::UI::ControlPointSelection::Set &selection =
            nt->_selected_nodes->allPoints();
        points.clear();
        std::vector<Geom::Point>::iterator pbegin;
        for (Inkscape::UI::ControlPointSelection::Set::iterator i =
                    selection.begin();
                i != selection.end(); ++i) {
            if ((*i)->selected()) {
                Inkscape::UI::Node *n = dynamic_cast<Inkscape::UI::Node *>(*i);
                pbegin = points.begin();
                points.insert(pbegin, desktop->doc2dt(n->position()));
            }
        }
    }
    //bool hasNodesSelected = LPEBspline::hasNodesSelected();
    if (curve->get_segment_count() < 1)
        return;
    // Make copy of old path as it is changed during processing
    Geom::PathVector const original_pathv = curve->get_pathvector();
    curve->reset();

    //Recorremos todos los paths a los que queremos aplicar el efecto, hasta el
    //penúltimo
    for (Geom::PathVector::const_iterator path_it = original_pathv.begin();
            path_it != original_pathv.end(); ++path_it) {
        //Si está vacío...
        if (path_it->empty())
            continue;
        //Itreadores

        Geom::Path::const_iterator curve_it1 = path_it->begin(); // incoming curve
        Geom::Path::const_iterator curve_it2 =
            ++(path_it->begin());                                // outgoing curve
        Geom::Path::const_iterator curve_endit =
            path_it->end_default(); // this determines when the loop has to stop
        //Creamos las lineas rectas que unen todos los puntos del trazado y donde se
        //calcularán
        //los puntos clave para los manejadores.
        //Esto hace que la curva BSpline no pierda su condición aunque se trasladen
        //dichos manejadores
        SPCurve *nCurve = new SPCurve();
        Geom::Point pointAt0(0, 0);
        Geom::Point pointAt1(0, 0);
        Geom::Point pointAt2(0, 0);
        Geom::Point pointAt3(0, 0);
        Geom::Point nextPointAt0(0, 0);
        Geom::Point nextPointAt1(0, 0);
        Geom::Point nextPointAt2(0, 0);
        Geom::Point nextPointAt3(0, 0);
        Geom::D2<Geom::SBasis> SBasisIn;
        Geom::D2<Geom::SBasis> SBasisOut;
        Geom::CubicBezier const *cubic = NULL;
        if (path_it->closed()) {
            // if the path is closed, maybe we have to stop a bit earlier because the
            // closing line segment has zerolength.
            const Geom::Curve &closingline =
                path_it->back_closed(); // the closing line segment is always of type
            // Geom::LineSegment.
            if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
                // closingline.isDegenerate() did not work, because it only checks for
                // *exact* zero length, which goes wrong for relative coordinates and
                // rounding errors...
                // the closing line segment has zero-length. So stop before that one!
                curve_endit = path_it->end_open();
            }
        }
        //Si la curva está cerrada calculamos el punto donde
        //deveria estar el nodo BSpline de cierre/inicio de la curva
        //en posible caso de que se cierre con una linea recta creando un nodo
        //BSPline
        nCurve->moveto(curve_it1->initialPoint());
        //Recorremos todos los segmentos menos el último
        while (curve_it2 != curve_endit) {
            //previousPointAt3 = pointAt3;
            //Calculamos los puntos que dividirían en tres segmentos iguales el path
            //recto de entrada y de salida
            SPCurve *in = new SPCurve();
            in->moveto(curve_it1->initialPoint());
            in->lineto(curve_it1->finalPoint());
            cubic = dynamic_cast<Geom::CubicBezier const *>(&*curve_it1);
            pointAt0 = in->first_segment()->initialPoint();
            pointAt3 = in->first_segment()->finalPoint();
            SBasisIn = in->first_segment()->toSBasis();
            if (!onlySelected) {
                if (cubic) {
                    if (!ignoreCusp || !Geom::are_near((*cubic)[1], pointAt0)) {
                        pointAt1 = SBasisIn.valueAt(weightValue);
                        if (weightValue != 0.0000) {
                            pointAt1 =
                                Geom::Point(pointAt1[X] + 0.0001, pointAt1[Y] + 0.0001);
                        }
                    } else {
                        pointAt1 = in->first_segment()->initialPoint();
                    }
                    if (!ignoreCusp || !Geom::are_near((*cubic)[2], pointAt3)) {
                        pointAt2 = SBasisIn.valueAt(1 - weightValue);
                        if (weightValue != 0.0000) {
                            pointAt2 =
                                Geom::Point(pointAt2[X] + 0.0001, pointAt2[Y] + 0.0001);
                        }
                    } else {
                        pointAt2 = in->first_segment()->finalPoint();
                    }
                } else {
                    if (!ignoreCusp && weightValue != 0.0000) {
                        pointAt1 = SBasisIn.valueAt(weightValue);
                        if (weightValue != 0.0000) {
                            pointAt1 =
                                Geom::Point(pointAt1[X] + 0.0001, pointAt1[Y] + 0.0001);
                        }
                        pointAt2 = SBasisIn.valueAt(1 - weightValue);
                        if (weightValue != 0.0000) {
                            pointAt2 =
                                Geom::Point(pointAt2[X] + 0.0001, pointAt2[Y] + 0.0001);
                        }
                    } else {
                        pointAt1 = in->first_segment()->initialPoint();
                        pointAt2 = in->first_segment()->finalPoint();
                    }
                }
            } else {
                if (cubic) {
                    if (!ignoreCusp || !Geom::are_near((*cubic)[1], pointAt0)) {
                        if (nodeIsSelected(pointAt0)) {
                            pointAt1 = SBasisIn.valueAt(weightValue);
                            if (weightValue != 0.0000) {
                                pointAt1 =
                                    Geom::Point(pointAt1[X] + 0.0001, pointAt1[Y] + 0.0001);
                            }
                        } else {
                            pointAt1 = (*cubic)[1];
                        }
                    } else {
                        pointAt1 = in->first_segment()->initialPoint();
                    }
                    if (!ignoreCusp || !Geom::are_near((*cubic)[2], pointAt3)) {
                        if (nodeIsSelected(pointAt3)) {
                            pointAt2 = SBasisIn.valueAt(1 - weightValue);
                            if (weightValue != 0.0000) {
                                pointAt2 =
                                    Geom::Point(pointAt2[X] + 0.0001, pointAt2[Y] + 0.0001);
                            }
                        } else {
                            pointAt2 = (*cubic)[2];
                        }
                    } else {
                        pointAt2 = in->first_segment()->finalPoint();
                    }
                } else {
                    if (!ignoreCusp && weightValue != 0.000) {
                        if (nodeIsSelected(pointAt0)) {
                            pointAt1 = SBasisIn.valueAt(weightValue);
                            pointAt1 =
                                Geom::Point(pointAt1[X] + 0.0001, pointAt1[Y] + 0.0001);
                        } else {
                            pointAt1 = in->first_segment()->initialPoint();
                        }
                        if (nodeIsSelected(pointAt3)) {
                            pointAt2 = SBasisIn.valueAt(weightValue);
                            pointAt2 =
                                Geom::Point(pointAt2[X] + 0.0001, pointAt2[Y] + 0.0001);
                        } else {
                            pointAt2 = in->first_segment()->finalPoint();
                        }
                    } else {
                        pointAt1 = in->first_segment()->initialPoint();
                        pointAt2 = in->first_segment()->finalPoint();
                    }
                }
            }
            in->reset();
            delete in;
            //La curva BSpline se forma calculando el centro del segmanto de unión
            //de el punto situado en las 2/3 partes de el segmento de entrada
            //con el punto situado en la posición 1/3 del segmento de salida
            //Estos dos puntos ademas estan posicionados en el lugas correspondiente
            //de
            //los manejadores de la curva
            nCurve->curveto(pointAt1, pointAt2, pointAt3);
            //aumentamos los valores para el siguiente paso en el bucle
            ++curve_it1;
            ++curve_it2;
        }
        SPCurve *out = new SPCurve();
        out->moveto(curve_it1->initialPoint());
        out->lineto(curve_it1->finalPoint());
        SBasisOut = out->first_segment()->toSBasis();
        nextPointAt0 = out->first_segment()->initialPoint();
        nextPointAt3 = out->first_segment()->finalPoint();
        cubic = dynamic_cast<Geom::CubicBezier const *>(&*curve_it1);
        if (!onlySelected) {
            if (cubic) {
                if (!ignoreCusp || !Geom::are_near((*cubic)[1], nextPointAt0)) {
                    nextPointAt1 = SBasisOut.valueAt(weightValue);
                    if (weightValue != 0.0000) {
                        nextPointAt1 =
                            Geom::Point(nextPointAt1[X] + 0.0001, nextPointAt1[Y] + 0.0001);
                    }
                } else {
                    nextPointAt1 = out->first_segment()->initialPoint();
                }
                if (!ignoreCusp || !Geom::are_near((*cubic)[2], nextPointAt3)) {
                    nextPointAt2 = SBasisOut.valueAt(1 - weightValue);
                    if (weightValue != 0.0000) {
                        nextPointAt2 =
                            Geom::Point(nextPointAt2[X] + 0.0001, nextPointAt2[Y] + 0.0001);
                    }
                } else {
                    nextPointAt2 = out->first_segment()->finalPoint();
                }
            } else {
                if (!ignoreCusp && weightValue != 0.0000) {
                    nextPointAt1 = SBasisOut.valueAt(weightValue);
                    nextPointAt1 =
                        Geom::Point(nextPointAt1[X] + 0.0001, nextPointAt1[Y] + 0.0001);
                    nextPointAt2 = SBasisOut.valueAt(1 - weightValue);
                    nextPointAt2 =
                        Geom::Point(nextPointAt2[X] + 0.0001, nextPointAt2[Y] + 0.0001);
                } else {
                    nextPointAt1 = out->first_segment()->initialPoint();
                    nextPointAt2 = out->first_segment()->finalPoint();
                }
            }
        } else {
            if (cubic) {
                if (!ignoreCusp || !Geom::are_near((*cubic)[1], nextPointAt0)) {
                    if (nodeIsSelected(nextPointAt0)) {
                        nextPointAt1 = SBasisOut.valueAt(weightValue);
                        if (weightValue != 0.0000) {
                            nextPointAt1 = Geom::Point(nextPointAt1[X] + 0.0001,
                                                       nextPointAt1[Y] + 0.0001);
                        }
                    } else {
                        nextPointAt1 = (*cubic)[1];
                    }
                } else {
                    nextPointAt1 = out->first_segment()->initialPoint();
                }
                if (!ignoreCusp || !Geom::are_near((*cubic)[2], nextPointAt3)) {
                    if (nodeIsSelected(nextPointAt3)) {
                        nextPointAt2 = SBasisOut.valueAt(1 - weightValue);
                        if (weightValue != 0.0000) {
                            nextPointAt2 = Geom::Point(nextPointAt2[X] + 0.0001,
                                                       nextPointAt2[Y] + 0.0001);
                        }
                    } else {
                        nextPointAt2 = (*cubic)[2];
                    }
                } else {
                    nextPointAt2 = out->first_segment()->finalPoint();
                }
            } else {
                if (!ignoreCusp && weightValue != 0.0000) {
                    if (nodeIsSelected(nextPointAt0)) {
                        nextPointAt1 = SBasisOut.valueAt(weightValue);
                        nextPointAt1 =
                            Geom::Point(nextPointAt1[X] + 0.0001, nextPointAt1[Y] + 0.0001);
                    } else {
                        nextPointAt1 = out->first_segment()->initialPoint();
                    }
                    if (nodeIsSelected(nextPointAt3)) {
                        nextPointAt2 = SBasisOut.valueAt(weightValue);
                        nextPointAt2 =
                            Geom::Point(nextPointAt2[X] + 0.0001, nextPointAt2[Y] + 0.0001);
                    } else {
                        nextPointAt2 = out->first_segment()->finalPoint();
                    }
                } else {
                    nextPointAt1 = out->first_segment()->initialPoint();
                    nextPointAt2 = out->first_segment()->finalPoint();
                }
            }
        }
        out->reset();
        delete out;
        //Aberiguamos la ultima parte de la curva correspondiente al último
        //segmento
        //Y hacemos lo propio con el path de salida
        //nextPointAt0 = curveOut.valueAt(0);
        if (path_it->closed()) {
            nCurve->curveto(nextPointAt1, nextPointAt2,
                            path_it->begin()->initialPoint());
            nCurve->move_endpoints(path_it->begin()->initialPoint(),
                                   path_it->begin()->initialPoint());
        } else {
            nCurve->curveto(nextPointAt1, nextPointAt2, nextPointAt3);
            nCurve->move_endpoints(path_it->begin()->initialPoint(), nextPointAt3);
        }
        //y cerramos la curva
        if (path_it->closed()) {
            nCurve->closepath_current();
        }
        curve->append(nCurve, false);
        nCurve->reset();
        delete nCurve;
    }
}

}; //namespace LivePathEffect
}; /* namespace Inkscape */

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

#define INKSCAPE_LPE_BSPLINE_C
/*
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/lpe-bspline.h"

#include "display/curve.h"
#include <typeinfo>
#include <2geom/pathvector.h>
#include <2geom/affine.h>
#include <2geom/bezier-curve.h>
#include "helper/geom-curves.h"
#include <glibmm/i18n.h>
// For handling un-continuous paths:
#include "message-stack.h"
#include "inkscape.h"
#include "desktop.h"

namespace Inkscape {
namespace LivePathEffect {

LPEBSpline::LPEBSpline(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),unify_weights(_("Unify weights:"),
                 _("Percent of the with for all poinrs"), "unify_weights", &wr, this, 33.)
{
    registerParameter( dynamic_cast<Parameter *>(&unify_weights) );
    unify_weights.param_set_range(0, 100.);
}

LPEBSpline::~LPEBSpline()
{
}

void
LPEBSpline::doEffect(SPCurve * curve)
{
    if(curve->get_segment_count() < 2)
        return;
    // Make copy of old path as it is changed during processing
    Geom::PathVector const original_pathv = curve->get_pathvector();
    curve->reset();

    //Recorremos todos los paths a los que queremos aplicar el efecto, hasta el penúltimo
    for(Geom::PathVector::const_iterator path_it = original_pathv.begin(); path_it != original_pathv.end(); ++path_it) {
        //Si está vacío... 
        if (path_it->empty())
            continue;
        //Itreadores
        
        Geom::Path::const_iterator curve_it1 = path_it->begin();      // incoming curve
        Geom::Path::const_iterator curve_it2 = ++(path_it->begin());         // outgoing curve
        Geom::Path::const_iterator curve_endit = path_it->end_default(); // this determines when the loop has to stop
        //Creamos las lineas rectas que unen todos los puntos del trazado y donde se calcularán
        //los puntos clave para los manejadores. 
        //Esto hace que la curva BSpline no pierda su condición aunque se trasladen
        //dichos manejadores
        SPCurve *nCurve = new SPCurve();
        Geom::Point previousNode(0,0);
        Geom::Point node(0,0);
        Geom::Point pointAt1(0,0);
        Geom::Point pointAt2(0,0);
        Geom::Point nextPointAt1(0,0);
        Geom::Point nextPointAt2(0,0);
        Geom::Point nextPointAt3(0,0);
        Geom::D2< Geom::SBasis > SBasisIn;
        Geom::D2< Geom::SBasis > SBasisOut;
        Geom::D2< Geom::SBasis > SBasisHelper;
        Geom::CubicBezier const *cubic = NULL;
        if (path_it->closed()) {
            // if the path is closed, maybe we have to stop a bit earlier because the closing line segment has zerolength.
            const Geom::Curve &closingline = path_it->back_closed(); // the closing line segment is always of type Geom::LineSegment.
            if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
                // closingline.isDegenerate() did not work, because it only checks for *exact* zero length, which goes wrong for relative coordinates and rounding errors...
                // the closing line segment has zero-length. So stop before that one!
                curve_endit = path_it->end_open();
            }
        }
        //Si la curva está cerrada calculamos el punto donde
        //deveria estar el nodo BSpline de cierre/inicio de la curva
        //en posible caso de que se cierre con una linea recta creando un nodo BSPline

        //Recorremos todos los segmentos menos el último
        while ( curve_it2 != curve_endit )
        {
            //previousPointAt3 = pointAt3;
            //Calculamos los puntos que dividirían en tres segmentos iguales el path recto de entrada y de salida
            SPCurve * in = new SPCurve();
            in->moveto(curve_it1->initialPoint());
            in->lineto(curve_it1->finalPoint());
            cubic = dynamic_cast<Geom::CubicBezier const*>(&*curve_it1);
            if(cubic){
                SBasisIn = in->first_segment()->toSBasis();
                pointAt1 = SBasisIn.valueAt(Geom::nearest_point((*cubic)[1],*in->first_segment()));
                pointAt2 = SBasisIn.valueAt(Geom::nearest_point((*cubic)[2],*in->first_segment()));
            }else{
                pointAt1 = in->first_segment()->initialPoint();
                pointAt2 = in->first_segment()->finalPoint();
            }
            in->reset();
            delete in;
            //Y hacemos lo propio con el path de salida
            //nextPointAt0 = curveOut.valueAt(0);
            SPCurve * out = new SPCurve();
            out->moveto(curve_it2->initialPoint());
            out->lineto(curve_it2->finalPoint());
            cubic = dynamic_cast<Geom::CubicBezier const*>(&*curve_it2);
            if(cubic){
                SBasisOut = out->first_segment()->toSBasis();
                nextPointAt1 = SBasisOut.valueAt(Geom::nearest_point((*cubic)[1],*out->first_segment()));
                nextPointAt2 = SBasisOut.valueAt(Geom::nearest_point((*cubic)[2],*out->first_segment()));;
                nextPointAt3 = (*cubic)[3];
            }else{
                nextPointAt1 = out->first_segment()->initialPoint();
                nextPointAt2 = out->first_segment()->finalPoint();
                nextPointAt3 = out->first_segment()->finalPoint();
            }
            out->reset();
            delete out;
            //La curva BSpline se forma calculando el centro del segmanto de unión
            //de el punto situado en las 2/3 partes de el segmento de entrada
            //con el punto situado en la posición 1/3 del segmento de salida
            //Estos dos puntos ademas estan posicionados en el lugas correspondiente de
            //los manejadores de la curva
            SPCurve *lineHelper = new SPCurve();
            lineHelper->moveto(pointAt2);
            lineHelper->lineto(nextPointAt1);
            SBasisHelper  = lineHelper->first_segment()->toSBasis();
            lineHelper->reset();
            delete lineHelper;
            //almacenamos el punto del anterior bucle -o el de cierre- que nos hara de principio de curva
            previousNode = node;
            //Y este hará de final de curva
            node = SBasisHelper.valueAt(0.5);
            SPCurve *curveHelper = new SPCurve();
            curveHelper->moveto(previousNode);
            curveHelper->curveto(pointAt1, pointAt2, node);
            //añadimos la curva generada a la curva pricipal
            nCurve->append_continuous(curveHelper, 0.0625);
            curveHelper->reset();
            delete curveHelper;
            //aumentamos los valores para el siguiente paso en el bucle
            ++curve_it1;
            ++curve_it2;
        }
        //Aberiguamos la ultima parte de la curva correspondiente al último segmento
        SPCurve *curveHelper = new SPCurve();
        curveHelper->moveto(node);
        //Si está cerrada la curva, la cerramos sobre  el valor guardado previamente
        //Si no finalizamos en el punto final
        Geom::Point startNode(0,0);
        if (path_it->closed()) {
            SPCurve * start = new SPCurve();
            start->moveto(path_it->begin()->initialPoint());
            start->lineto(path_it->begin()->finalPoint());
            Geom::D2< Geom::SBasis > SBasisStart = start->first_segment()->toSBasis();
            SPCurve *lineHelper = new SPCurve();
            cubic = dynamic_cast<Geom::CubicBezier const*>(&*path_it->begin());
            if(cubic){
                lineHelper->moveto(SBasisStart.valueAt(Geom::nearest_point((*cubic)[1],*start->first_segment())));
            }else{
                lineHelper->moveto(start->first_segment()->initialPoint());
            }
            start->reset();
            delete start;

            SPCurve * end = new SPCurve();
            end->moveto(curve_it1->initialPoint());
            end->lineto(curve_it1->finalPoint());
            Geom::D2< Geom::SBasis > SBasisEnd = end->first_segment()->toSBasis();
            //Geom::BezierCurve const *bezier = dynamic_cast<Geom::BezierCurve const*>(&*curve_endit);
            cubic = dynamic_cast<Geom::CubicBezier const*>(&*curve_it1);
            if(cubic){
                lineHelper->lineto(SBasisEnd.valueAt(Geom::nearest_point((*cubic)[2],*end->first_segment())));
            }else{
                lineHelper->lineto(end->first_segment()->finalPoint());
            }
            end->reset();
            delete end;
            SBasisHelper = lineHelper->first_segment()->toSBasis();
            lineHelper->reset();
            delete lineHelper;
            //Guardamos el principio de la curva
            startNode = SBasisHelper.valueAt(0.5);
            curveHelper->curveto(nextPointAt1, nextPointAt2, startNode);
            nCurve->append_continuous(curveHelper, 0.0625);
            nCurve->move_endpoints(startNode,startNode);
        }else{
            SPCurve * start = new SPCurve();
            start->moveto(path_it->begin()->initialPoint());
            start->lineto(path_it->begin()->finalPoint());
            startNode = start->first_segment()->initialPoint();
            start->reset();
            delete start;
            curveHelper->curveto(nextPointAt1, nextPointAt2, nextPointAt3);
            nCurve->append_continuous(curveHelper, 0.0625);
            nCurve->move_endpoints(startNode,nextPointAt3);
        }
        curveHelper->reset();
        delete curveHelper;
        //y cerramos la curva
        if (path_it->closed()) {
            nCurve->closepath_current();
        }
        curve->append(nCurve,false);
        nCurve->reset();
        delete nCurve;
    }
}

std::vector<Geom::Path>
LPEBSpline::doEffect_path (std::vector<Geom::Path> const &path_in)
{
    Geom::PathVector const original_pathv = path_in;
    SPCurve *curve = new SPCurve();
    //Recorremos todos los paths a los que queremos aplicar el efecto, hasta el penúltimo
    for(Geom::PathVector::const_iterator path_it = original_pathv.begin(); path_it != original_pathv.end(); ++path_it) {
        //Si está vacío... 
        if (path_it->empty())
            continue;
        //Itreadores
        
        Geom::Path::const_iterator curve_it1 = path_it->begin();      // incoming curve
        Geom::Path::const_iterator curve_it2 = ++(path_it->begin());         // outgoing curve
        Geom::Path::const_iterator curve_endit = path_it->end_default(); // this determines when the loop has to stop
        //Creamos las lineas rectas que unen todos los puntos del trazado y donde se calcularán
        //los puntos clave para los manejadores. 
        //Esto hace que la curva BSpline no pierda su condición aunque se trasladen
        //dichos manejadores
        SPCurve *nCurve = new SPCurve();
        Geom::Point previousNode(0,0);
        Geom::Point node(0,0);
        Geom::Point pointAt0(0,0);
        Geom::Point pointAt1(0,0);
        Geom::Point pointAt2(0,0);
        Geom::Point pointAt3(0,0);
        double pos1 = 0;
        double pos2 = 0;
        Geom::Point nextPointAt1(0,0);
        Geom::Point nextPointAt2(0,0);
        Geom::Point nextPointAt3(0,0);
        Geom::D2< Geom::SBasis > SBasisIn;
        Geom::D2< Geom::SBasis > SBasisOut;
        Geom::D2< Geom::SBasis > SBasisHelper;
        Geom::CubicBezier const *cubic = NULL;
        if (path_it->closed()) {
            // if the path is closed, maybe we have to stop a bit earlier because the closing line segment has zerolength.
            const Geom::Curve &closingline = path_it->back_closed(); // the closing line segment is always of type Geom::LineSegment.
            if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
                // closingline.isDegenerate() did not work, because it only checks for *exact* zero length, which goes wrong for relative coordinates and rounding errors...
                // the closing line segment has zero-length. So stop before that one!
                curve_endit = path_it->end_open();
            }
        }
        //Si la curva está cerrada calculamos el punto donde
        //deveria estar el nodo BSpline de cierre/inicio de la curva
        //en posible caso de que se cierre con una linea recta creando un nodo BSPline

        //Recorremos todos los segmentos menos el último
        while ( curve_it2 != curve_endit )
        {
            //previousPointAt3 = pointAt3;
            //Calculamos los puntos que dividirían en tres segmentos iguales el path recto de entrada y de salida
            SPCurve * in = new SPCurve();
            in->moveto(curve_it1->initialPoint());
            in->lineto(curve_it1->finalPoint());
            cubic = dynamic_cast<Geom::CubicBezier const*>(&*curve_it1);
            if(cubic){
                SBasisIn = in->first_segment()->toSBasis();
                pointAt0 = in->first_segment()->initialPoint();
                pos1 = unify_weights/100;
                pos2 = 1-unify_weights/100;
                pointAt1 = SBasisIn.valueAt(pos1);
                pointAt2 = SBasisIn.valueAt(pos2);
                pointAt3 = in->first_segment()->finalPoint();
            }else{
                pointAt0 = in->first_segment()->initialPoint();
                pointAt1 = in->first_segment()->initialPoint();
                pointAt2 = in->first_segment()->finalPoint();
                pointAt3 = in->first_segment()->finalPoint();
            }
            in->reset();
            delete in;
            //Y hacemos lo propio con el path de salida
            //nextPointAt0 = curveOut.valueAt(0);
            SPCurve * out = new SPCurve();
            out->moveto(curve_it2->initialPoint());
            out->lineto(curve_it2->finalPoint());
            cubic = dynamic_cast<Geom::CubicBezier const*>(&*curve_it2);
            if(cubic){
                SBasisOut = out->first_segment()->toSBasis();
                pos1 = unify_weights/100;
                pos2 = 1-unify_weights/100;
                nextPointAt1 = SBasisOut.valueAt(pos1);
                nextPointAt2 = SBasisOut.valueAt(pos2);
                nextPointAt3 = (*cubic)[3];
            }else{
                nextPointAt1 = out->first_segment()->initialPoint();
                nextPointAt2 = out->first_segment()->finalPoint();
                nextPointAt3 = out->first_segment()->finalPoint();
            }
            out->reset();
            delete out;
            //Y este hará de final de curva
            SPCurve *curveHelper = new SPCurve();
            curveHelper->moveto(pointAt0);
            curveHelper->curveto(pointAt1, pointAt2, pointAt3);
            //añadimos la curva generada a la curva pricipal
            nCurve->append_continuous(curveHelper, 0.0625);
            curveHelper->reset();
            delete curveHelper;
            //aumentamos los valores para el siguiente paso en el bucle
            ++curve_it1;
            ++curve_it2;
        }
        //Aberiguamos la ultima parte de la curva correspondiente al último segmento
        SPCurve *curveHelper = new SPCurve();
        curveHelper->moveto(node);
        //Si está cerrada la curva, la cerramos sobre  el valor guardado previamente
        //Si no finalizamos en el punto final
        Geom::Point startNode(0,0);
        startNode = path_it->begin()->initialPoint();
        curveHelper->curveto(nextPointAt1, nextPointAt2, nextPointAt3);
        nCurve->append_continuous(curveHelper, 0.0625);
        nCurve->move_endpoints(startNode,nextPointAt3);
        curveHelper->reset();
        delete curveHelper;
        //y cerramos la curva
        if (path_it->closed()) {
            nCurve->closepath_current();
        }
        curve->append(nCurve,false);
        nCurve->reset();
        delete nCurve;
    }
    return curve->get_pathvector();
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

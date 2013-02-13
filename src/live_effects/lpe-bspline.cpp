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


// For handling un-continuous paths:
#include "message-stack.h"
#include "inkscape.h"
#include "desktop.h"

namespace Inkscape {
namespace LivePathEffect {

LPEBSpline::LPEBSpline(LivePathEffectObject *lpeobject) :
    Effect(lpeobject)
{
}

LPEBSpline::~LPEBSpline()
{
}


void
LPEBSpline::doEffect(SPCurve * curve)
{
    using Geom::X;
    using Geom::Y;

    if(curve->get_segment_count() < 2)
    return;
    // Make copy of old path as it is changed during processing
    Geom::PathVector const original_pathv = curve->get_pathvector();
    curve->reset();
    //Sbasis
    Geom::D2< Geom::SBasis > SBasisIn;
    Geom::D2< Geom::SBasis > SBasisOut;
    Geom::D2< Geom::SBasis > SBasisEnd;
    Geom::D2< Geom::SBasis > SBasisHelper;
    //curves
    SPCurve * in = new SPCurve();
    SPCurve * out = new SPCurve();
    SPCurve * end = new SPCurve();
    //Curvas temporales
    SPCurve *lineHelper = new SPCurve();
    SPCurve *curveHelper = new SPCurve();
    SPCurve *nCurve = new SPCurve();
    //Puntos a usar. Ponemos todos los posibles para hacer más inteligible el código
    Geom::Point startNode(0,0);
    Geom::Point previousNode(0,0);
    Geom::Point node(0,0);
    //Geom::Point previousPointAt3(0,0);
    Geom::Point pointAt0(0,0);
    Geom::Point pointAt1(0,0);
    Geom::Point pointAt2(0,0);
    Geom::Point pointAt3(0,0);
    //Geom::Point nextPointAt0(0,0);
    Geom::Point nextPointAt1(0,0);
    Geom::Point nextPointAt2(0,0);
    Geom::Point nextPointAt3(0,0);
    Geom::CubicBezier const *cubic;
    //Recorremos todos los paths a los que queremos aplicar el efecto, hasta el penúltimo
    for(Geom::PathVector::const_iterator path_it = original_pathv.begin(); path_it != original_pathv.end(); ++path_it) {
        //Si está vacío... 
        if (path_it->empty())
            continue;
        //Itreadores
        
        Geom::Path::const_iterator curve_it1 = path_it->begin();      // incoming curve
        Geom::Path::const_iterator curve_it2 = ++(path_it->begin());         // outgoing curve
        Geom::Path::const_iterator curve_end = path_it->end(); // end curve 
        Geom::Path::const_iterator curve_endit = path_it->end_default(); // this determines when the loop has to stop
        //Las lineas rectas forman nodos BSpline
        //Las curvas forman nodos CUSP
        bool isBSpline = true;
        //Creamos las lineas rectas que unen todos los puntos del trazado y donde se calcularán
        //los puntos clave para los manejadores. 
        //Esto hace que la curva BSpline no pierda su condición aunque se trasladen
        //dichos manejadores
        in->moveto(curve_it1->initialPoint());
        in->lineto(curve_it1->finalPoint());
        out->moveto(curve_it2->initialPoint());
        out->lineto(curve_it2->finalPoint());
        //este no cambia.
        end->moveto(curve_end->initialPoint());
        end->lineto(curve_end->finalPoint());
        //Si la curva está cerrada calculamos el punto donde
        //deveria estar el nodo BSpline de cierre/inicio de la curva
        //en posible caso de que se cierre con una linea recta creando un nodo BSPline
        cubic = dynamic_cast<Geom::CubicBezier const*>(&*curve_end);
        if(cubic && (*cubic)[2] != (*cubic)[3])
            isBSpline = false;
        if (path_it->closed() && isBSpline) {
            //Calculamos el nodo de inicio BSpline
            SBasisIn = in->first_segment()->toSBasis();
            SBasisEnd = end->first_segment()->toSBasis();
            lineHelper->moveto(SBasisIn.valueAt(0.3334));
            lineHelper->lineto(SBasisEnd.valueAt(0.6667));
            SBasisHelper = lineHelper->first_segment()->toSBasis();
            lineHelper->reset();
            //Guardamos el principio de la curva
            startNode = SBasisHelper.valueAt(0.5);
            //Definimos el punto de inicio original de la curva resultante
            node = startNode;
        }else{
            isBSpline = true;
            //Guardamos el principio de la curva
            startNode = in->first_segment()->initialPoint();
            //Definimos el punto de inicio original de la curva resultante
            node = startNode;
        }
        //Recorremos todos los segmentos menos el último
        while ( curve_it2 != curve_endit )
        {
            //Damos valor a el objeto SBasis para el path de entrada y el de salida
            SBasisIn = in->first_segment()->toSBasis();
            SBasisOut = out->first_segment()->toSBasis();
            //previousPointAt3 = pointAt3;
            //Calculamos los puntos que dividirían en tres segmentos iguales el path recto de entrada y de salida
            pointAt0 = SBasisIn.valueAt(0);
            pointAt1 = SBasisIn.valueAt(0.3334);
            pointAt2 = SBasisIn.valueAt(0.6667);
            pointAt3 = SBasisIn.valueAt(1);
            //Y hacemos lo propio con el path de salida
            //nextPointAt0 = curveOut.valueAt(0);
            nextPointAt1 = SBasisOut.valueAt(0.3334);
            nextPointAt2 = SBasisOut.valueAt(0.6667);;
            nextPointAt3 = SBasisOut.valueAt(1);
            //La curva BSpline se forma calculando el centro del segmanto de unión
            //de el punto situado en las 2/3 partes de el segmento de entrada
            //con el punto situado en la posición 1/3 del segmento de salida
            //Estos dos puntos ademas estan posicionados en el lugas correspondiente de
            //los manejadores de la curva
            lineHelper->moveto(pointAt2);
            lineHelper->lineto(nextPointAt1);
            SBasisHelper = lineHelper->first_segment()->toSBasis();
            lineHelper->reset();
            //almacenamos el punto del anterior bucle -o el de cierre- que nos hara de principio de curva
            previousNode = node;
            //Y este hará de final de curva
            node = SBasisHelper.valueAt(0.5);
            //Vemos si el nodo es BSpline o CUSP
            //Averiguamos si el punto de union tiene manejadores
            cubic = dynamic_cast<Geom::CubicBezier const*>(&*curve_it1);
            if(cubic && (*cubic)[2] != (*cubic)[3])
                isBSpline = false;
            cubic = dynamic_cast<Geom::CubicBezier const*>(&*curve_it2);
            if(cubic && (*cubic)[0] != (*cubic)[1])
                isBSpline = false;
            //Si no tiene manejador, tenemos que generar la curva con nodo final CUSP
            if(!isBSpline ){
                //Definimos como nodo el final del segmento de entrada
                isBSpline = true;
                node = pointAt3;
            }
            curveHelper->moveto(previousNode);
            curveHelper->curveto(pointAt1, pointAt2, node);
            //añadimos la curva generada a la curva pricipal
            nCurve->append_continuous(curveHelper, 0.0625);
            curveHelper->reset();
            //aumentamos los valores para el siguiente paso en el bucle
            ++curve_it1;
            ++curve_it2;
            in->reset();
            in->moveto(curve_it1->initialPoint());
            in->lineto(curve_it1->finalPoint());
            out->reset();
            if(curve_it1 != curve_end){
                out->moveto(curve_it2->initialPoint());
                out->lineto(curve_it2->finalPoint());
            }
        }
        //Aberiguamos la ultima parte de la curva correspondiente al último segmento
        curveHelper->moveto(node);
        //Si está cerrada la curva, la cerramos sobre  el valor guardado previamente
        //Si no finalizamos en el punto final
        if (path_it->closed()) {
            curveHelper->curveto(nextPointAt1, nextPointAt2, startNode);
        }else{
            curveHelper->curveto(nextPointAt1, nextPointAt2, nextPointAt3);
            isBSpline = true;
        }
        //añadimos este último segmento
        nCurve->append_continuous(curveHelper, 0.0625);
        //y cerramos la curva
        if (path_it->closed()) {
            nCurve->closepath_current();
        }
        curve->append(nCurve,false);
        nCurve->reset();
        //Limpiamos
        in->reset();
        out->reset();
        end->reset();
        lineHelper->reset();
        curveHelper->reset();
    }
    delete in;
    delete out;
    delete end;
    delete lineHelper;
    delete curveHelper;
    //Todo: remove?
    //delete SBasisIn;
    //delete SBasisOut;
    //delete SBasisEnd;
    //delete SBasisHelper;
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

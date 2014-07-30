#define INKSCAPE_LPE_ROUGHEN_C
/*
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/lpe-roughen.h"
#include "display/curve.h"
#include "live_effects/parameter/parameter.h"
#include "helper/geom.h"
#include <glibmm/i18n.h>
#include <gtkmm/separator.h>
#include <cmath>

namespace Inkscape {
namespace LivePathEffect {

static const Util::EnumData<DivisionMethod> DivisionMethodData[DM_END] = {
  { DM_SEGMENTS, N_("By number of segments"), "segments" },
  { DM_SIZE, N_("By max. segment size"), "size" }
};
static const Util::EnumDataConverter<DivisionMethod>
    DMConverter(DivisionMethodData, DM_END);

LPERoughen::LPERoughen(LivePathEffectObject *lpeobject)
    : Effect(lpeobject),
      // initialise your parameters here:
      unit(_("Unit:"), _("Unit"), "unit", &wr, this),
      method(_("Method:"), _("Division method"), "method", DMConverter, &wr,
             this, DM_SEGMENTS),
      maxSegmentSize(_("Max. segment size:"), _("Max. segment size"),
                     "maxSegmentSize", &wr, this, 10.),
      segments(_("Number of segments:"), _("Number of segments"), "segments",
               &wr, this, 2),
      displaceX(_("Max. displacement in X:"), _("Max. displacement in X"),
                "displaceX", &wr, this, 10.),
      displaceY(_("Max. displacement in Y:"), _("Max. displacement in Y"),
                "displaceY", &wr, this, 10.),
      shiftNodes(_("Shift nodes"), _("Shift nodes"), "shiftNodes", &wr, this,
                 true),
      shiftNodeHandles(_("Shift node handles"), _("Shift node handles"),
                       "shiftNodeHandles", &wr, this, true) {
  registerParameter(dynamic_cast<Parameter *>(&unit));
  registerParameter(dynamic_cast<Parameter *>(&method));
  registerParameter(dynamic_cast<Parameter *>(&maxSegmentSize));
  registerParameter(dynamic_cast<Parameter *>(&segments));
  registerParameter(dynamic_cast<Parameter *>(&displaceX));
  registerParameter(dynamic_cast<Parameter *>(&displaceY));
  registerParameter(dynamic_cast<Parameter *>(&shiftNodes));
  registerParameter(dynamic_cast<Parameter *>(&shiftNodeHandles));
  displaceX.param_set_range(0., Geom::infinity());
  displaceY.param_set_range(0., Geom::infinity());
  maxSegmentSize.param_set_range(0., Geom::infinity());
  maxSegmentSize.param_set_increments(1, 1);
  maxSegmentSize.param_set_digits(1);
  segments.param_set_range(1, Geom::infinity());
  segments.param_set_increments(1, 1);
  segments.param_set_digits(0);
}

LPERoughen::~LPERoughen() {}

void LPERoughen::doBeforeEffect(SPLPEItem const */*lpeitem*/) {
  displaceX.resetRandomizer();
  displaceY.resetRandomizer();
  srand(1);
}

Gtk::Widget *LPERoughen::newWidget() {
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
      if (param->param_key == "unit") {
        Gtk::Label *unitLabel = Gtk::manage(new Gtk::Label(
            Glib::ustring(_("<b>Roughen unit</b>")), Gtk::ALIGN_START));
        unitLabel->set_use_markup(true);
        vbox->pack_start(*unitLabel, false, false, 2);
        vbox->pack_start(*Gtk::manage(new Gtk::HSeparator()),
                         Gtk::PACK_EXPAND_WIDGET);
      }
      if (param->param_key == "method") {
        Gtk::Label *methodLabel = Gtk::manage(new Gtk::Label(
            Glib::ustring(_("<b>Add nodes</b> Subdivide each segment")),
            Gtk::ALIGN_START));
        methodLabel->set_use_markup(true);
        vbox->pack_start(*methodLabel, false, false, 2);
        vbox->pack_start(*Gtk::manage(new Gtk::HSeparator()),
                         Gtk::PACK_EXPAND_WIDGET);
      }
      if (param->param_key == "displaceX") {
        Gtk::Label *displaceXLabel = Gtk::manage(new Gtk::Label(
            Glib::ustring(_("<b>Jitter nodes</b> Move nodes/handles")),
            Gtk::ALIGN_START));
        displaceXLabel->set_use_markup(true);
        vbox->pack_start(*displaceXLabel, false, false, 2);
        vbox->pack_start(*Gtk::manage(new Gtk::HSeparator()),
                         Gtk::PACK_EXPAND_WIDGET);
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
  return dynamic_cast<Gtk::Widget *>(vbox);
}

double LPERoughen::sign(double randNumber) {
  if (rand() % 100 < 49) {
    randNumber *= -1.;
  }
  return randNumber;
}

Geom::Point LPERoughen::randomize() {
  double displaceXParsed = Inkscape::Util::Quantity::convert(
      displaceX, unit.get_abbreviation(), "px");
  double displaceYParsed = Inkscape::Util::Quantity::convert(
      displaceY, unit.get_abbreviation(), "px");
  //maybe is better divide this point by 2...
  Geom::Point output =
      Geom::Point(sign(displaceXParsed), sign(displaceYParsed));
  return output;
}

void LPERoughen::doEffect(SPCurve *curve) {
  Geom::PathVector const original_pathv =
      pathv_to_linear_and_cubic_beziers(curve->get_pathvector());
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
    Geom::Point initialMove(0, 0);
    if (shiftNodes) {
      initialMove = randomize();
    }
    Geom::Point initialPoint = curve_it1->initialPoint() + initialMove;
    nCurve->moveto(initialPoint);
    Geom::Point A0(0, 0);
    Geom::Point A1(0, 0);
    Geom::Point A2(0, 0);
    Geom::Point A3(0, 0);
    while (curve_it2 != curve_endit) {
      //aumentamos los valores para el siguiente paso en el bucle
      //Recorremos todos los segmentos menos el último
      Geom::CubicBezier const *cubic = NULL;
      A0 = curve_it1->initialPoint();
      A1 = curve_it1->initialPoint();
      A2 = curve_it1->finalPoint();
      A3 = curve_it1->finalPoint();
      cubic = dynamic_cast<Geom::CubicBezier const *>(&*curve_it1);
      if (cubic) {
        A1 = (*cubic)[1];
        if (shiftNodes) {
          A1 = (*cubic)[1] + initialMove;
        }
        A2 = (*cubic)[2];
        nCurve->curveto(A1, A2, A3);
      } else {
        nCurve->lineto(A3);
      }
      double length = Inkscape::Util::Quantity::convert(
          curve_it1->length(0.001), "px", unit.get_abbreviation());
      unsigned int splits = 0;
      if (method == DM_SEGMENTS) {
        splits = segments;
      } else {
        splits = ceil(length / maxSegmentSize);
      }
      for (unsigned int t = splits; t >= 1; t--) {
        if (t == 1 && splits != 1) {
          continue;
        }
        const SPCurve *tmp;
        if (splits == 1) {
          tmp = jitter(nCurve->last_segment());
        } else {
          tmp = addNodesAndJitter(nCurve->last_segment(), 1. / t);
        }
        if (nCurve->get_segment_count() > 1) {
          nCurve->backspace();
          nCurve->append_continuous(tmp, 0.001);
        } else {
          nCurve = tmp->copy();
        }
        delete tmp;
      }
      ++curve_it1;
      ++curve_it2;
    }
    Geom::CubicBezier const *cubic = NULL;
    A0 = curve_it1->initialPoint();
    A1 = curve_it1->initialPoint();
    A2 = curve_it1->finalPoint();
    A3 = curve_it1->finalPoint();
    cubic = dynamic_cast<Geom::CubicBezier const *>(&*curve_it1);
    if (cubic) {
      A1 = (*cubic)[1];
      A2 = (*cubic)[2];
      if (path_it->closed()) {
        A2 = A2 + initialMove;
        A3 = initialPoint;
      }
      nCurve->curveto(A1, A2, A3);
    } else {
      if (path_it->closed()) {
        A3 = initialPoint;
      }
      nCurve->lineto(A3);
    }
    double length = Inkscape::Util::Quantity::convert(
        curve_it1->length(0.001), "px", unit.get_abbreviation());
    unsigned int splits = 0;
    if (method == DM_SEGMENTS) {
      splits = segments;
    } else {
      splits = ceil(length / maxSegmentSize);
    }
    for (unsigned int t = splits; t >= 1; t--) {
      if (t == 1 && splits != 1) {
        continue;
      }
      const SPCurve *tmp;
      if (splits == 1) {
        tmp = jitter(nCurve->last_segment());
      } else {
        tmp = addNodesAndJitter(nCurve->last_segment(), 1. / t);
      }
      if (nCurve->get_segment_count() > 1) {
        nCurve->backspace();
        nCurve->append_continuous(tmp, 0.001);
      } else {
        nCurve = tmp->copy();
      }
      delete tmp;
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

SPCurve *LPERoughen::addNodesAndJitter(const Geom::Curve *A, double t) {
  SPCurve *out = new SPCurve();
  Geom::CubicBezier const *cubic = dynamic_cast<Geom::CubicBezier const *>(&*A);
  Geom::Point A1(0, 0);
  Geom::Point A2(0, 0);
  Geom::Point A3(0, 0);
  Geom::Point B1(0, 0);
  Geom::Point B2(0, 0);
  Geom::Point B3(0, 0);
  if (shiftNodes) {
    A3 = randomize();
    B3 = randomize();
  }
  if (shiftNodeHandles) {
    A1 = randomize();
    A2 = randomize();
    B1 = randomize();
    B2 = randomize();
  } else {
    A2 = A3;
    B1 = A3;
    B2 = B3;
  }
  if (cubic) {
    std::pair<Geom::CubicBezier, Geom::CubicBezier> div = cubic->subdivide(t);
    std::vector<Geom::Point> seg1 = div.first.points(),
                             seg2 = div.second.points();
    out->moveto(seg1[0]);
    out->curveto(seg1[1] + A1, seg1[2] + A2, seg1[3] + A3);
    out->curveto(seg2[1] + B1, seg2[2], seg2[3]);
  } else if (shiftNodeHandles) {
    out->moveto(A->initialPoint());
    out->curveto(A->pointAt(t / 3) + A1, A->pointAt((t / 3) * 2) + A2,
                 A->pointAt(t) + A3);
    out->curveto(A->pointAt(t + (t / 3)) + B1, A->pointAt(t + ((t / 3) * 2)),
                 A->finalPoint());
  } else {
    out->moveto(A->initialPoint());
    out->lineto(A->pointAt(t) + A3);
    out->lineto(A->finalPoint());
  }
  return out;
}

SPCurve *LPERoughen::jitter(const Geom::Curve *A) {
  SPCurve *out = new SPCurve();
  Geom::CubicBezier const *cubic = dynamic_cast<Geom::CubicBezier const *>(&*A);
  Geom::Point A1(0, 0);
  Geom::Point A2(0, 0);
  Geom::Point A3(0, 0);
  if (shiftNodes) {
    A3 = randomize();
  }
  if (shiftNodeHandles) {
    A1 = randomize();
    A2 = randomize();
  } else {
    A2 = A3;
  }
  if (cubic) {
    out->moveto((*cubic)[0]);
    out->curveto((*cubic)[1] + A1, (*cubic)[2] + A2, (*cubic)[3] + A3);
  } else if (shiftNodeHandles) {
    out->moveto(A->initialPoint());
    out->curveto(A->pointAt(0.3333) + A1, A->pointAt(0.6666) + A2,
                 A->finalPoint() + A3);
  } else {
    out->moveto(A->initialPoint());
    out->lineto(A->finalPoint() + A3);
  }
  return out;
}

Geom::Point LPERoughen::tpoint(Geom::Point A, Geom::Point B, double t) {
  using Geom::X;
  using Geom::Y;
  return Geom::Point(A[X] + t * (B[X] - A[X]), A[Y] + t * (B[Y] - A[Y]));
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

/** \file
 * LPE "Transform through 2 points" implementation
 */

/*
 * Authors:
 *   Jabier Arraiza Cenoz<jabier.arraiza@marker.es>
 *
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gtkmm.h>

#include "live_effects/lpe-transform_2pts.h"
#include "display/curve.h"
#include <2geom/transforms.h>
#include <2geom/path.h>
#include "sp-path.h"
#include "ui/icon-names.h"

#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

LPETransform2Pts::LPETransform2Pts(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    from_original_width(_("From original width"), _("From original width"), "from_original_width", &wr, this, false,"", INKSCAPE_ICON("on"), INKSCAPE_ICON("off")),
    start(_("Start"), _("Start point"), "start", &wr, this, "Start point"),
    end(_("End"), _("End point"), "end", &wr, this, "End point"),
    first_knot(_("First Knot"), _("First Knot"), "first_knot", &wr, this, 1),
    last_knot(_("Last Knot"), _("Last Knot"), "last_knot", &wr, this, 1),
    from_original_width_toogler(false),
    point_a(Geom::Point(0,0)),
    point_b(Geom::Point(0,0)),
    curve_c(NULL),
    append_path(false)
{
    registerParameter(&start);
    registerParameter(&end);
    registerParameter(&first_knot);
    registerParameter(&last_knot);
    registerParameter(&from_original_width);

    first_knot.param_make_integer(true);
    last_knot.param_make_integer(true);
}

LPETransform2Pts::~LPETransform2Pts()
{
}

void
LPETransform2Pts::doOnApply(SPLPEItem const* lpeitem)
{
    using namespace Geom;
    original_bbox(lpeitem);

    point_a = Point(boundingbox_X.min(), boundingbox_Y.middle());
    point_b = Point(boundingbox_X.max(), boundingbox_Y.middle());
    SPLPEItem * splpeitem = const_cast<SPLPEItem *>(lpeitem);
    SPPath *path = dynamic_cast<SPPath *>(splpeitem);
    if (path) {
        curve_c = path->get_original_curve();
    }
    if(curve_c && !curve_c->is_closed() && curve_c->first_path() == curve_c->last_path()) {
        point_a = *(curve_c->first_point());
        point_b = *(curve_c->last_point());
        int nnodes = (int)curve_c->nodes_in_path();
        last_knot.param_set_value(nnodes);
    }
    start.param_update_default(point_a);
    start.param_set_default();
    end.param_update_default(point_b);
    end.param_set_default();
}

void
LPETransform2Pts::doBeforeEffect (SPLPEItem const* lpeitem)
{
    using namespace Geom;
    original_bbox(lpeitem);
    point_a = Point(boundingbox_X.min(), boundingbox_Y.middle());
    point_b = Point(boundingbox_X.max(), boundingbox_Y.middle());

    SPLPEItem * splpeitem = const_cast<SPLPEItem *>(lpeitem);
    SPPath *path = dynamic_cast<SPPath *>(splpeitem);
    if (path) {
        curve_c = path->get_original_curve();
    }
    if(from_original_width_toogler != from_original_width) {
        from_original_width_toogler = from_original_width;
        reset();
    }
    if(curve_c && !from_original_width) {
        if(!curve_c->is_closed() && curve_c->first_path() == curve_c->last_path()) {
            append_path = false;
            Geom::PathVector const originalPV = curve_c->get_pathvector();
            point_a = originalPV[0][0].initialPoint();
            if((int)first_knot > 1) {
                point_a = originalPV[0][(int)first_knot-2].finalPoint();
            }
            point_b = originalPV[0][0].initialPoint();
            if((int)last_knot > 1) {
                point_b = originalPV[0][(int)last_knot-2].finalPoint();
            }
            int nnodes = (int)curve_c->nodes_in_path();
            first_knot.param_set_range(1, last_knot-1);
            last_knot.param_set_range(first_knot+1, nnodes);
            from_original_width.param_setValue(false);
        } else {
            first_knot.param_set_value(1);
            last_knot.param_set_value(2);
            first_knot.param_set_range(1,1);
            last_knot.param_set_range(2,2);
            if(append_path == false) {
                append_path = true;
            } else {
                from_original_width.param_setValue(true);
            }
        }
    } else {
        first_knot.param_set_value(1);
        last_knot.param_set_value(2);
        first_knot.param_set_range(1,1);
        last_knot.param_set_range(2,2);
        from_original_width.param_setValue(true);
        append_path = false;
    }
    splpeitem->apply_to_clippath(splpeitem);
    splpeitem->apply_to_mask(splpeitem);
}

void
LPETransform2Pts::updateIndex()
{
    SPCurve * curve2 = NULL;
    SPShape *shape = SP_SHAPE(sp_lpe_item);
    if (shape) {
        curve2 = shape->getCurve();
    }
    if(curve2 && !from_original_width && !curve_c->is_closed() && curve_c->first_path() == curve_c->last_path()) {
        Geom::PathVector const originalPV = curve2->get_pathvector();
        Geom::Point point_c = originalPV[0][0].initialPoint();
        Geom::Point point_d = originalPV[0][0].initialPoint();
        if((int)first_knot > 1) {
            point_c = originalPV[0][(int)first_knot-2].finalPoint();
        }
        if((int)last_knot > 1) {
            point_d = originalPV[0][(int)last_knot-2].finalPoint();
        }
        start.param_update_default(point_c);
        start.param_set_default();
        end.param_update_default(point_d);
        end.param_set_default();
        start.param_update_default(point_a);
        end.param_update_default(point_b);
        start.param_set_default();
        end.param_set_default();
    }
}

void
LPETransform2Pts::reset()
{
    point_a = Geom::Point(boundingbox_X.min(), boundingbox_Y.middle());
    point_b = Geom::Point(boundingbox_X.max(), boundingbox_Y.middle());
    if(curve_c && !curve_c->is_closed() && curve_c->first_path() == curve_c->last_path() && !from_original_width) {
        int nnodes = (int)curve_c->nodes_in_path();
        first_knot.param_set_range(1, last_knot-1);
        last_knot.param_set_range(first_knot+1, nnodes);
        first_knot.param_set_value(1);
        last_knot.param_set_value(nnodes);
        point_a = *(curve_c->first_point());
        point_b = *(curve_c->last_point());
    } else {
        first_knot.param_set_value(1);
        last_knot.param_set_value(2);
    }
    start.param_update_default(point_a);
    end.param_update_default(point_b);
    start.param_set_default();
    end.param_set_default();
}

Gtk::Widget *LPETransform2Pts::newWidget()
{
    // use manage here, because after deletion of Effect object, others might
    // still be pointing to this widget.
    Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox(Effect::newWidget()));

    vbox->set_border_width(5);
    vbox->set_homogeneous(false);
    vbox->set_spacing(6);

    std::vector<Parameter *>::iterator it = param_vector.begin();
    Gtk::HBox * button = Gtk::manage(new Gtk::HBox(true,0));
    while (it != param_vector.end()) {
        if ((*it)->widget_is_visible) {
            Parameter *param = *it;
            Gtk::Widget *widg = dynamic_cast<Gtk::Widget *>(param->param_newWidget());
            Glib::ustring *tip = param->param_getTooltip();
            if (param->param_key == "first_knot" || param->param_key == "last_knot") {
                Inkscape::UI::Widget::Scalar *registered_widget = Gtk::manage(dynamic_cast<Inkscape::UI::Widget::Scalar *>(widg));
                registered_widget->signal_value_changed().connect(sigc::mem_fun(*this, &LPETransform2Pts::updateIndex));
                widg = registered_widget;
                if (widg) {
                    Gtk::HBox *hbox_scalar = dynamic_cast<Gtk::HBox *>(widg);
                    std::vector<Gtk::Widget *> child_list = hbox_scalar->get_children();
                    Gtk::Entry *entry_widget = dynamic_cast<Gtk::Entry *>(child_list[1]);
                    entry_widget->set_width_chars(3);
                    vbox->pack_start(*widg, true, true, 2);
                    if (tip) {
                        widg->set_tooltip_text(*tip);
                    } else {
                        widg->set_tooltip_text("");
                        widg->set_has_tooltip(false);
                    }
                }
            } else if (param->param_key == "from_original_width") {
                Glib::ustring * tip = param->param_getTooltip();
                if (widg) {
                    button->pack_start(*widg, true, true, 2);
                    if (tip) {
                        widg->set_tooltip_text(*tip);
                    } else {
                        widg->set_tooltip_text("");
                        widg->set_has_tooltip(false);
                    }
                }
            } else if (widg) {
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
    Gtk::Button *reset = Gtk::manage(new Gtk::Button(Glib::ustring(_("Reset"))));
    reset->signal_clicked().connect(sigc::mem_fun(*this, &LPETransform2Pts::reset));
    button->pack_start(*reset, true, true, 2);
    vbox->pack_start(*button, true, true, 2);
    return dynamic_cast<Gtk::Widget *>(vbox);
}

Geom::Piecewise<Geom::D2<Geom::SBasis> >
LPETransform2Pts::doEffect_pwd2 (Geom::Piecewise<Geom::D2<Geom::SBasis> > const & pwd2_in)
{
    Geom::Piecewise<Geom::D2<Geom::SBasis> > output;
    double sca = Geom::distance((Geom::Point)start,(Geom::Point)end)/Geom::distance(point_a,point_b);
    Geom::Ray original(point_a,point_b);
    Geom::Ray transformed((Geom::Point)start,(Geom::Point)end);
    double rot = transformed.angle() - original.angle();
    Geom::Path helper;
    helper.start(point_a);
    helper.appendNew<Geom::LineSegment>(point_b);
    Geom::Affine m;
    m *= Geom::Scale(sca);
    m *= Geom::Rotate(rot);
    helper *= m;
    m *= Geom::Translate((Geom::Point)start - helper.initialPoint());
    output.concat(pwd2_in * m);

    return output;
}

void
LPETransform2Pts::addCanvasIndicators(SPLPEItem const */*lpeitem*/, std::vector<Geom::PathVector> &hp_vec)
{
    using namespace Geom;
    hp_vec.clear();
    Geom::Path hp;
    hp.start((Geom::Point)start);
    hp.appendNew<Geom::LineSegment>((Geom::Point)end);
    Geom::PathVector pathv;
    pathv.push_back(hp);
    hp_vec.push_back(pathv);
}


/* ######################## */

} //namespace LivePathEffect
} /* namespace Inkscape */

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

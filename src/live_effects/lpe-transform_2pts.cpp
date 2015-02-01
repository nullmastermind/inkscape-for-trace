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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtkmm.h>

#include "live_effects/lpe-transform_2pts.h"
#include <glibmm/i18n.h>
#include "display/curve.h"
#include <2geom/transforms.h>
#include <2geom/path.h>
#include "sp-path.h"
#include "ui/tools-switch.h"

#include "inkscape.h"

namespace Inkscape {
namespace LivePathEffect {

LPETransform2Pts::LPETransform2Pts(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    fromOriginalWidth(_("Use bounding box"), _("Use bounding box"), "fromOriginalWidth", &wr, this, false),
    start(_("Start"), _("Start point"), "start", &wr, this, "Start point"),
    end(_("End"), _("End point"), "end", &wr, this, "End point"),
    firstKnot(_("First Knot"), _("First Knot"), "firstKnot", &wr, this, 1),
    lastKnot(_("Last Knot"), _("Last Knot"), "lastKnot", &wr, this, 1)
{
    registerParameter(&start);
    registerParameter(&end);
    registerParameter(&firstKnot);
    registerParameter(&lastKnot);
    registerParameter(&fromOriginalWidth);
    
    firstKnot.param_make_integer(true);
    lastKnot.param_make_integer(true);
}

LPETransform2Pts::~LPETransform2Pts()
{
}

void
LPETransform2Pts::doOnApply(SPLPEItem const* lpeitem)
{
    using namespace Geom;
    original_bbox(lpeitem);

    A = Point(boundingbox_X.min(), boundingbox_Y.middle());
    B = Point(boundingbox_X.max(), boundingbox_Y.middle());
    SPLPEItem* item = const_cast<SPLPEItem*>(lpeitem);
    SPPath *path = dynamic_cast<SPPath *>(item);
    if (path) {
        SPCurve * c = NULL;
        c = path->get_original_curve();
        if(!c->is_closed() && c->first_path() == c->last_path()){
            A = *(c->first_point());
            B = *(c->last_point());
            int nnodes = (int)c->nodes_in_path();
            lastKnot.param_set_value((int)c->nodes_in_path());
        }
    }
    start.param_setValue(A);
    end.param_setValue(B);
}

void
LPETransform2Pts::doBeforeEffect (SPLPEItem const* lpeitem)
{
    using namespace Geom;
    original_bbox(lpeitem);
    SPLPEItem* item = const_cast<SPLPEItem*>(lpeitem);
    SPPath *path = dynamic_cast<SPPath *>(item);
    A = Point(boundingbox_X.min(), boundingbox_Y.middle());
    B = Point(boundingbox_X.max(), boundingbox_Y.middle());
    if(path && !fromOriginalWidth){
        SPCurve * c = NULL;
        c = path->get_original_curve();
        if(!c->is_closed() && c->first_path() == c->last_path()){
            Geom::PathVector const originalPV = c->get_pathvector();
            A = originalPV[0][0].initialPoint();
            if((int)firstKnot > 1){
                A = originalPV[0][(int)firstKnot-2].finalPoint();
            }
            B = originalPV[0][0].initialPoint();
            if((int)lastKnot > 1){
                B = originalPV[0][(int)lastKnot-2].finalPoint();
            }
            int nnodes = (int)c->nodes_in_path();
            firstKnot.param_set_range(1, lastKnot-1);
            lastKnot.param_set_range(firstKnot+1, nnodes);
        } else {
            firstKnot.param_set_value(1);
            lastKnot.param_set_value(2);
            firstKnot.param_set_range(1,1);
            lastKnot.param_set_range(2,2);
        }
    } else {
        firstKnot.param_set_value(1);
        lastKnot.param_set_value(2);
        firstKnot.param_set_range(1,1);
        lastKnot.param_set_range(2,2);
    }
    item->apply_to_clippath(item);
    item->apply_to_mask(item);
}

void
LPETransform2Pts::updateIndex()
{
    SPPath *path = dynamic_cast<SPPath *>(sp_lpe_item);
    if(path && !fromOriginalWidth){
        SPCurve * c = NULL;
        c = path->get_original_curve();
        int nnodes = (int)c->nodes_in_path();
        if(!c->is_closed() && c->first_path() == c->last_path()){
            c->reset();
            c = path->getCurve();
            Geom::PathVector const originalPV = c->get_pathvector();
            Geom::Point C = originalPV[0][0].initialPoint();
            Geom::Point D = originalPV[0][0].initialPoint();
            if((int)firstKnot > 1){
                C = originalPV[0][(int)firstKnot-2].finalPoint();
            }
            if((int)lastKnot > 1){
                D = originalPV[0][(int)lastKnot-2].finalPoint();
            }
            start.param_update_default(C);
            start.param_set_and_write_default();
            end.param_update_default(D);
            end.param_set_and_write_default();
            start.param_update_default(A);
            end.param_update_default(B);
            start.param_set_and_write_default();
            end.param_set_and_write_default();
            SPDesktop * desktop = SP_ACTIVE_DESKTOP;
            tools_switch(desktop, TOOLS_SELECT);
            tools_switch(desktop, TOOLS_NODES);
        }
    }
}

void
LPETransform2Pts::reset()
{
    SPPath *path = dynamic_cast<SPPath *>(sp_lpe_item);
    A = Geom::Point(boundingbox_X.min(), boundingbox_Y.middle());
    B = Geom::Point(boundingbox_X.max(), boundingbox_Y.middle());
    if(path && !fromOriginalWidth){
        SPCurve * c = NULL;
        c = path->get_original_curve();
        int nnodes = (int)c->nodes_in_path();
        firstKnot.param_set_value(1);
        lastKnot.param_set_value(nnodes);
        A = *(c->first_point());
        B = *(c->last_point());
    } else {
        firstKnot.param_set_value(1);
        lastKnot.param_set_value(2);
    }
    start.param_update_default(A);
    end.param_update_default(B);
    start.param_set_and_write_default();
    end.param_set_and_write_default();
    SPDesktop * desktop = SP_ACTIVE_DESKTOP;
    tools_switch(desktop, TOOLS_SELECT);
    tools_switch(desktop, TOOLS_NODES);
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

    while (it != param_vector.end()) {
        if ((*it)->widget_is_visible) {
            Parameter *param = *it;
            Gtk::Widget *widg = dynamic_cast<Gtk::Widget *>(param->param_newWidget());
            Glib::ustring *tip = param->param_getTooltip();
            if (param->param_key == "firstKnot" || param->param_key == "lastKnot") {
                Inkscape::UI::Widget::Scalar *widgRegistered = Gtk::manage(dynamic_cast<Inkscape::UI::Widget::Scalar *>(widg));
                widgRegistered->signal_value_changed().connect(sigc::mem_fun(*this, &LPETransform2Pts::updateIndex));
                widg = widgRegistered;
                if (widg) {
                    Gtk::HBox *scalarParameter = dynamic_cast<Gtk::HBox *>(widg);
                    std::vector<Gtk::Widget *> childList = scalarParameter->get_children();
                    Gtk::Entry *entryWidg = dynamic_cast<Gtk::Entry *>(childList[1]);
                    entryWidg->set_width_chars(3);
                }
            }
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
    Gtk::HBox * button = Gtk::manage(new Gtk::HBox(true,0));
    Gtk::Button *reset = Gtk::manage(new Gtk::Button(Glib::ustring(_("Reset"))));
    reset->signal_clicked().connect(sigc::mem_fun(*this, &LPETransform2Pts::reset));
    reset->set_size_request(140,45);
    button->pack_start(*reset, false, false, 2);
    vbox->pack_start(*button, true, true, 2);
    return dynamic_cast<Gtk::Widget *>(vbox);
}

Geom::Piecewise<Geom::D2<Geom::SBasis> >
LPETransform2Pts::doEffect_pwd2 (Geom::Piecewise<Geom::D2<Geom::SBasis> > const & pwd2_in)
{
    Geom::Piecewise<Geom::D2<Geom::SBasis> > output;
    double sca = Geom::distance((Geom::Point)start,(Geom::Point)end)/Geom::distance(A,B);
    Geom::Ray original(A,B);
    Geom::Ray transformed((Geom::Point)start,(Geom::Point)end);
    double rot = transformed.angle() - original.angle();
    Geom::Path helper;
    helper.start(A);
    helper.appendNew<Geom::LineSegment>(B);
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

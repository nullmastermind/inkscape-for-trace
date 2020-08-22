// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Maximilian Albert 2008 <maximilian.albert@gmail.com>
 *
 * Authors:
 *   Maximilian Albert
 *   Johan Engelen
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>
#include <gtkmm/alignment.h>

#include <2geom/sbasis-geometric.h>

#include "inkscape.h"
#include "verbs.h"

#include "display/control/canvas-item-text.h"

#include "live_effects/effect.h"
#include "live_effects/parameter/text.h"

#include "svg/stringstream.h"
#include "svg/svg.h"

#include "ui/widget/registered-widget.h"


namespace Inkscape {

namespace LivePathEffect {

TextParam::TextParam( const Glib::ustring& label, const Glib::ustring& tip,
                      const Glib::ustring& key, Inkscape::UI::Widget::Registry* wr,
                      Effect* effect, const Glib::ustring default_value )
    : Parameter(label, tip, key, wr, effect)
    , value(default_value)
    , defvalue(default_value)
{
    if (SPDesktop *desktop = SP_ACTIVE_DESKTOP) { // FIXME: we shouldn't use this!
        canvas_text = new Inkscape::CanvasItemText(desktop->getCanvasTemp(), Geom::Point(0, 0), default_value);
    }
}

TextParam::~TextParam()
{
    if (canvas_text) {
        delete canvas_text;
    }
}

void
TextParam::param_set_default()
{
    param_setValue(defvalue);
}

void
TextParam::param_update_default(const gchar * default_value)
{
    defvalue = (Glib::ustring)default_value;
}

// This is a bit silly, we should have an option in the constructor to not create the canvas_text object.
void
TextParam::param_hide_canvas_text()
{
    if (canvas_text) {
        delete canvas_text;
        canvas_text = nullptr;
    }
}

void
TextParam::setPos(Geom::Point pos)
{
    if (canvas_text) {
        canvas_text->set_coord(pos);
    }
}

void
TextParam::setPosAndAnchor(const Geom::Piecewise<Geom::D2<Geom::SBasis> > &pwd2,
                           const double t, const double length, bool /*use_curvature*/)
{
    using namespace Geom;

    Piecewise<D2<SBasis> > pwd2_reparam = arc_length_parametrization(pwd2, 2 , 0.1);
    double t_reparam = pwd2_reparam.cuts.back() * t;
    Point pos = pwd2_reparam.valueAt(t_reparam);
    Point dir = unit_vector(derivative(pwd2_reparam).valueAt(t_reparam));
    Point n = -rot90(dir);
    double angle = Geom::angle_between(dir, Point(1,0));
    if (canvas_text) {
        canvas_text->set_coord(pos + n * length);
        canvas_text->set_anchor(Geom::Point(std::sin(angle), -std::cos(angle)));
    }
}

void
TextParam::setAnchor(double x_value, double y_value)
{
    anchor_x = x_value;
    anchor_y = y_value;
    if (canvas_text) {
        canvas_text->set_anchor(Geom::Point(anchor_x, anchor_y));
    }
}

bool
TextParam::param_readSVGValue(const gchar * strvalue)
{
    param_setValue(strvalue);
    return true;
}

Glib::ustring
TextParam::param_getSVGValue() const
{
    return value;
}

Glib::ustring
TextParam::param_getDefaultSVGValue() const
{
    return defvalue;
}

void 
TextParam::setTextParam(Inkscape::UI::Widget::RegisteredText *rsu) 
{
    Glib::ustring str(rsu->getText());
    param_setValue(str);
    write_to_SVG();
}

Gtk::Widget *
TextParam::param_newWidget()
{
    Inkscape::UI::Widget::RegisteredText *rsu = Gtk::manage(new Inkscape::UI::Widget::RegisteredText(
        param_label, param_tooltip, param_key, *param_wr, param_effect->getRepr(), param_effect->getSPDoc()));
    rsu->setText(value);
    rsu->setProgrammatically = false;
    rsu->set_undo_parameters(SP_VERB_DIALOG_LIVE_PATH_EFFECT, _("Change text parameter"));
    Gtk::Box *text_container = Gtk::manage(new Gtk::Box());
    Gtk::Button *set =  Gtk::manage(new Gtk::Button(Glib::ustring("✔")));
    set->signal_clicked()
    .connect(sigc::bind<Inkscape::UI::Widget::RegisteredText *>(sigc::mem_fun(*this, &TextParam::setTextParam),rsu));
    text_container->pack_start(*rsu, false, false, 2);
    text_container->pack_start(*set, false, false, 2);
    Gtk::Widget *return_widg = dynamic_cast<Gtk::Widget *> (text_container);
    return_widg->set_halign(Gtk::ALIGN_END);
    return return_widg;
}

void
TextParam::param_setValue(const Glib::ustring newvalue)
{
    if (value != newvalue) {
        param_effect->refresh_widgets = true;
    }
    value = newvalue;
    if (canvas_text) {
        canvas_text->set_text(newvalue);
    }
}

} /* namespace LivePathEffect */

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

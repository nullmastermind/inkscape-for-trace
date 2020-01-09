// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include "live_effects/parameter/parameter.h"
#include "live_effects/effect.h"
#include "svg/svg.h"
#include "xml/repr.h"

#include "svg/stringstream.h"

#include "verbs.h"

#include <glibmm/i18n.h>

#include <utility>

#define noLPEREALPARAM_DEBUG

namespace Inkscape {

namespace LivePathEffect {


Parameter::Parameter(Glib::ustring label, Glib::ustring tip, Glib::ustring key, Inkscape::UI::Widget::Registry *wr,
                     Effect *effect)
    : param_key(std::move(key))
    , param_wr(wr)
    , param_label(std::move(label))
    , oncanvas_editable(false)
    , widget_is_visible(true)
    , widget_is_enabled(true)
    , param_tooltip(std::move(tip))
    , param_effect(effect)
{
}

void Parameter::param_write_to_repr(const char *svgd)
{
    param_effect->getRepr()->setAttribute(param_key, svgd);
}

void Parameter::write_to_SVG()
{
    param_write_to_repr(param_getSVGValue().c_str());
}

/*###########################################
 *   REAL PARAM
 */
ScalarParam::ScalarParam(const Glib::ustring &label, const Glib::ustring &tip, const Glib::ustring &key,
                         Inkscape::UI::Widget::Registry *wr, Effect *effect, gdouble default_value)
    : Parameter(label, tip, key, wr, effect)
    , value(default_value)
    , min(-SCALARPARAM_G_MAXDOUBLE)
    , max(SCALARPARAM_G_MAXDOUBLE)
    , integer(false)
    , defvalue(default_value)
    , digits(2)
    , inc_step(0.1)
    , inc_page(1)
    , add_slider(false)
    , _set_undo(true)
{
}

ScalarParam::~ScalarParam() = default;

bool ScalarParam::param_readSVGValue(const gchar *strvalue)
{
    double newval;
    unsigned int success = sp_svg_number_read_d(strvalue, &newval);
    if (success == 1) {
        param_set_value(newval);
        return true;
    }
    return false;
}

Glib::ustring ScalarParam::param_getSVGValue() const
{
    Inkscape::SVGOStringStream os;
    os << value;
    return os.str();
}

Glib::ustring ScalarParam::param_getDefaultSVGValue() const
{
    Inkscape::SVGOStringStream os;
    os << defvalue;
    return os.str();
}

void ScalarParam::param_set_default() { param_set_value(defvalue); }

void ScalarParam::param_update_default(gdouble default_value) { defvalue = default_value; }

void ScalarParam::param_update_default(const gchar *default_value)
{
    double newval;
    unsigned int success = sp_svg_number_read_d(default_value, &newval);
    if (success == 1) {
        param_update_default(newval);
    }
}

void ScalarParam::param_transform_multiply(Geom::Affine const &postmul, bool set)
{
    // Check if proportional stroke-width scaling is on
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool transform_stroke = prefs ? prefs->getBool("/options/transform/stroke", true) : true;
    if (transform_stroke || set) {
        param_set_value(value * postmul.descrim());
        write_to_SVG();
    }
}

void ScalarParam::param_set_value(gdouble val)
{
    value = val;
    if (integer)
        value = round(value);
    if (value > max)
        value = max;
    if (value < min)
        value = min;
}

void ScalarParam::param_set_range(gdouble min, gdouble max)
{
    // if you look at client code, you'll see that many effects
    // has a tendency to set an upper range of Geom::infinity().
    // Once again, in gtk2, this is not a problem. But in gtk3,
    // widgets get allocated the amount of size they ask for,
    // leading to excessively long widgets.
    if (min >= -SCALARPARAM_G_MAXDOUBLE) {
        this->min = min;
    } else {
        this->min = -SCALARPARAM_G_MAXDOUBLE;
    }
    if (max <= SCALARPARAM_G_MAXDOUBLE) {
        this->max = max;
    } else {
        this->max = SCALARPARAM_G_MAXDOUBLE;
    }
    param_set_value(value); // reset value to see whether it is in ranges
}

void ScalarParam::param_make_integer(bool yes)
{
    integer = yes;
    digits = 0;
    inc_step = 1;
    inc_page = 10;
}

void ScalarParam::param_set_undo(bool set_undo) { _set_undo = set_undo; }

Gtk::Widget *ScalarParam::param_newWidget()
{
    if (widget_is_visible) {
        Inkscape::UI::Widget::RegisteredScalar *rsu = Gtk::manage(new Inkscape::UI::Widget::RegisteredScalar(
            param_label, param_tooltip, param_key, *param_wr, param_effect->getRepr(), param_effect->getSPDoc()));

        rsu->setValue(value);
        rsu->setDigits(digits);
        rsu->setIncrements(inc_step, inc_page);
        rsu->setRange(min, max);
        rsu->setProgrammatically = false;
        if (add_slider) {
            rsu->addSlider();
        }
        if (_set_undo) {
            rsu->set_undo_parameters(SP_VERB_DIALOG_LIVE_PATH_EFFECT, _("Change scalar parameter"));
        }
        return dynamic_cast<Gtk::Widget *>(rsu);
    } else {
        return nullptr;
    }
}

void ScalarParam::param_set_digits(unsigned digits) { this->digits = digits; }

void ScalarParam::param_set_increments(double step, double page)
{
    inc_step = step;
    inc_page = page;
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

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *
 * Copyright (C) 2012 Author
 *               2017 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "spin-scale.h"

#include <glibmm/i18n.h>
#include <glibmm/stringutils.h>

namespace Inkscape {
namespace UI {
namespace Widget {

SpinScale::SpinScale(const Glib::ustring label, double value,
                     double lower, double upper,
                     double step_increment, double page_increment, int digits,
                     const SPAttributeEnum a, const Glib::ustring tip_text)
    : AttrWidget(a, value)
    , _inkspinscale(value, lower, upper, step_increment, page_increment, 0)
{
    set_name("SpinScale");

    _inkspinscale.set_label (label);
    _inkspinscale.set_digits (digits);
    _inkspinscale.set_tooltip_text (tip_text);

    _adjustment = _inkspinscale.get_adjustment();

    signal_value_changed().connect(signal_attr_changed().make_slot());

    pack_start(_inkspinscale);

    show_all_children();
}

SpinScale::SpinScale(const Glib::ustring label,
                     Glib::RefPtr<Gtk::Adjustment> adjustment, int digits,
                     const SPAttributeEnum a, const Glib::ustring tip_text)
    : AttrWidget(a, 0.0)
    , _inkspinscale(adjustment)
{
    set_name("SpinScale");

    _inkspinscale.set_label (label);
    _inkspinscale.set_digits (digits);
    _inkspinscale.set_tooltip_text (tip_text);

    _adjustment = _inkspinscale.get_adjustment();

    signal_value_changed().connect(signal_attr_changed().make_slot());

    pack_start(_inkspinscale);

    show_all_children();
}

Glib::ustring SpinScale::get_as_attribute() const
{
    const double val = _adjustment->get_value();

    if( _inkspinscale.get_digits() == 0)
        return Glib::Ascii::dtostr((int)val);
    else
        return Glib::Ascii::dtostr(val);
}

void SpinScale::set_from_attribute(SPObject* o)
{
    const gchar* val = attribute_value(o);
    if (val)
        _adjustment->set_value(Glib::Ascii::strtod(val));
    else
        _adjustment->set_value(get_default()->as_double());
}

Glib::SignalProxy0<void> SpinScale::signal_value_changed()
{
    return _adjustment->signal_value_changed();
}

double SpinScale::get_value() const
{
    return _adjustment->get_value();
}

void SpinScale::set_value(const double val)
{
    _adjustment->set_value(val);
}

void SpinScale::set_focuswidget(GtkWidget *widget)
{
    _inkspinscale.set_focus_widget(widget);
}

const decltype(SpinScale::_adjustment) SpinScale::get_adjustment() const
{
    return _adjustment;
}

decltype(SpinScale::_adjustment) SpinScale::get_adjustment()
{
    return _adjustment;
}


DualSpinScale::DualSpinScale(const Glib::ustring label1, const Glib::ustring label2,
                             double value, double lower, double upper,
                             double step_increment, double page_increment, int digits,
                             const SPAttributeEnum a,
                             const Glib::ustring tip_text1, const Glib::ustring tip_text2)
    : AttrWidget(a),
      _s1(label1, value, lower, upper, step_increment, page_increment, digits, SP_ATTR_INVALID, tip_text1),
      _s2(label2, value, lower, upper, step_increment, page_increment, digits, SP_ATTR_INVALID, tip_text2),
      //TRANSLATORS: "Link" means to _link_ two sliders together
      _link(C_("Sliders", "Link"))
{
    set_name("DualSpinScale");
    signal_value_changed().connect(signal_attr_changed().make_slot());

    _s1.get_adjustment()->signal_value_changed().connect(_signal_value_changed.make_slot());
    _s2.get_adjustment()->signal_value_changed().connect(_signal_value_changed.make_slot());
    _s1.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &DualSpinScale::update_linked));

    _link.signal_toggled().connect(sigc::mem_fun(*this, &DualSpinScale::link_toggled));

    Gtk::Box* vb = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    vb->add(_s1);
    vb->add(_s2);
    pack_start(*vb);
    pack_start(_link, false, false);
    _link.set_active(true);

    show_all();
}

Glib::ustring DualSpinScale::get_as_attribute() const
{
    if(_link.get_active())
        return _s1.get_as_attribute();
    else
        return _s1.get_as_attribute() + " " + _s2.get_as_attribute();
}

void DualSpinScale::set_from_attribute(SPObject* o)
{
    const gchar* val = attribute_value(o);
    if(val) {
        // Split val into parts
        gchar** toks = g_strsplit(val, " ", 2);

        if(toks) {
            double v1 = 0.0, v2 = 0.0;
            if(toks[0])
                v1 = v2 = Glib::Ascii::strtod(toks[0]);
            if(toks[1])
                v2 = Glib::Ascii::strtod(toks[1]);

            _link.set_active(toks[1] == nullptr);

            _s1.get_adjustment()->set_value(v1);
            _s2.get_adjustment()->set_value(v2);

            g_strfreev(toks);
        }
    }
}

sigc::signal<void>& DualSpinScale::signal_value_changed()
{
    return _signal_value_changed;
}

const SpinScale& DualSpinScale::get_SpinScale1() const
{
    return _s1;
}

SpinScale& DualSpinScale::get_SpinScale1()
{
    return _s1;
}

const SpinScale& DualSpinScale::get_SpinScale2() const
{
    return _s2;
}

SpinScale& DualSpinScale::get_SpinScale2()
{
    return _s2;
}

void DualSpinScale::link_toggled()
{
    _s2.set_sensitive(!_link.get_active());
    update_linked();
}

void DualSpinScale::update_linked()
{
    if(_link.get_active())
        _s2.set_value(_s1.get_value());
}


} // namespace Widget
} // namespace UI
} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

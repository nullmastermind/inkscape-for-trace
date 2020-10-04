// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "color-wheel-selector.h"

#include <glibmm/i18n.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/label.h>
#include <gtkmm/spinbutton.h>
#include "ui/dialog-events.h"
#include "ui/widget/color-scales.h"
#include "ui/widget/color-slider.h"
#include "ui/widget/ink-color-wheel.h"
#include "ui/widget/scrollprotected.h"

namespace Inkscape {
namespace UI {
namespace Widget {


#define XPAD 4
#define YPAD 1


const gchar *ColorWheelSelector::MODE_NAME = N_("Wheel");

ColorWheelSelector::ColorWheelSelector(SelectedColor &color)
    : Gtk::Grid()
    , _color(color)
    , _updating(false)
    , _wheel(nullptr)
    , _slider(nullptr)
{
    set_name("ColorWheelSelector");

    _initUI();
    _color_changed_connection = color.signal_changed.connect(sigc::mem_fun(this, &ColorWheelSelector::_colorChanged));
    _color_dragged_connection = color.signal_dragged.connect(sigc::mem_fun(this, &ColorWheelSelector::_colorChanged));
}

ColorWheelSelector::~ColorWheelSelector()
{
    _color_changed_connection.disconnect();
    _color_dragged_connection.disconnect();
}

void ColorWheelSelector::_initUI()
{
    /* Create components */
    gint row = 0;

    _wheel = Gtk::manage(new Inkscape::UI::Widget::ColorWheel());
    _wheel->set_halign(Gtk::ALIGN_FILL);
    _wheel->set_valign(Gtk::ALIGN_FILL);
    _wheel->set_hexpand(true);
    _wheel->set_vexpand(true);
    attach(*_wheel, 0, row, 3, 1);

    row++;

    /* Label */
    Gtk::Label *label = Gtk::manage(new Gtk::Label(_("_A:"), true));
    label->set_halign(Gtk::ALIGN_END);
    label->set_valign(Gtk::ALIGN_CENTER);

    label->set_margin_start(XPAD);
    label->set_margin_end(XPAD);
    label->set_margin_top(YPAD);
    label->set_margin_bottom(YPAD);
    label->set_halign(Gtk::ALIGN_FILL);
    label->set_valign(Gtk::ALIGN_FILL);
    attach(*label, 0, row, 1, 1);

    /* Adjustment */
    _alpha_adjustment = Gtk::Adjustment::create(0.0, 0.0, 100.0, 1.0, 10.0, 10.0);

    /* Slider */
    _slider = Gtk::manage(new Inkscape::UI::Widget::ColorSlider(_alpha_adjustment));
    _slider->set_tooltip_text(_("Alpha (opacity)"));

    _slider->set_margin_start(XPAD);
    _slider->set_margin_end(XPAD);
    _slider->set_margin_top(YPAD);
    _slider->set_margin_bottom(YPAD);
    _slider->set_hexpand(true);
    _slider->set_halign(Gtk::ALIGN_FILL);
    _slider->set_valign(Gtk::ALIGN_FILL);
    attach(*_slider, 1, row, 1, 1);

    _slider->setColors(SP_RGBA32_F_COMPOSE(1.0, 1.0, 1.0, 0.0), SP_RGBA32_F_COMPOSE(1.0, 1.0, 1.0, 0.5),
                       SP_RGBA32_F_COMPOSE(1.0, 1.0, 1.0, 1.0));

    /* Spinbutton */
    auto spin_button = Gtk::manage(new ScrollProtected<Gtk::SpinButton>(_alpha_adjustment, 1.0, 0));
    spin_button->set_tooltip_text(_("Alpha (opacity)"));
    sp_dialog_defocus_on_enter(GTK_WIDGET(spin_button->gobj()));
    label->set_mnemonic_widget(*spin_button);

    spin_button->set_margin_start(XPAD);
    spin_button->set_margin_end(XPAD);
    spin_button->set_margin_top(YPAD);
    spin_button->set_margin_bottom(YPAD);
    spin_button->set_halign(Gtk::ALIGN_CENTER);
    spin_button->set_valign(Gtk::ALIGN_CENTER);
    attach(*spin_button, 2, row, 1, 1);

    /* Signals */
    _alpha_adjustment->signal_value_changed().connect(sigc::mem_fun(this, &ColorWheelSelector::_adjustmentChanged));
    _slider->signal_grabbed.connect(sigc::mem_fun(*this, &ColorWheelSelector::_sliderGrabbed));
    _slider->signal_released.connect(sigc::mem_fun(*this, &ColorWheelSelector::_sliderReleased));
    _slider->signal_value_changed.connect(sigc::mem_fun(*this, &ColorWheelSelector::_sliderChanged));
    _wheel->signal_color_changed().connect(sigc::mem_fun(*this, &ColorWheelSelector::_wheelChanged));

    show_all();
}

void ColorWheelSelector::on_show()
{
    Gtk::Grid::on_show();
    _updateDisplay();
}

void ColorWheelSelector::_colorChanged()
{
    _updateDisplay();
}

void ColorWheelSelector::_adjustmentChanged()
{
    if (_updating) {
        return;
    }

    _color.preserveICC();
    _color.setAlpha(ColorScales::getScaled(_alpha_adjustment));
}

void ColorWheelSelector::_sliderGrabbed()
{
    _color.preserveICC();
    _color.setHeld(true);
}

void ColorWheelSelector::_sliderReleased()
{
    _color.preserveICC();
    _color.setHeld(false);
}

void ColorWheelSelector::_sliderChanged()
{
    if (_updating) {
        return;
    }

    _color.preserveICC();
    _color.setAlpha(ColorScales::getScaled(_alpha_adjustment));
}

void ColorWheelSelector::_wheelChanged()
{
    if (_updating) {
        return;
    }

    double rgb[3] = { 0, 0, 0 };
    _wheel->get_rgb(rgb[0], rgb[1], rgb[2]);

    SPColor color(rgb[0], rgb[1], rgb[2]);

    guint32 start = color.toRGBA32(0x00);
    guint32 mid = color.toRGBA32(0x7f);
    guint32 end = color.toRGBA32(0xff);

    _updating = true;
    _slider->setColors(start, mid, end);
    _color.preserveICC();

    _color.setHeld(_wheel->is_adjusting());
    _color.setColor(color);
    _updating = false;
}

void ColorWheelSelector::_updateDisplay()
{
    if(_updating) { return; }

#ifdef DUMP_CHANGE_INFO
    g_message("ColorWheelSelector::_colorChanged( this=%p, %f, %f, %f,   %f)", this, _color.color().v.c[0],
              _color.color().v.c[1], _color.color().v.c[2], alpha);
#endif

    _updating = true;
    {
        float hsv[3] = { 0, 0, 0 };
        SPColor::rgb_to_hsv_floatv(hsv, _color.color().v.c[0], _color.color().v.c[1], _color.color().v.c[2]);
        _wheel->set_rgb(_color.color().v.c[0], _color.color().v.c[1], _color.color().v.c[2]);
    }

    guint32 start = _color.color().toRGBA32(0x00);
    guint32 mid = _color.color().toRGBA32(0x7f);
    guint32 end = _color.color().toRGBA32(0xff);

    _slider->setColors(start, mid, end);

    ColorScales::setScaled(_alpha_adjustment, _color.alpha());

    _updating = false;
}


Gtk::Widget *ColorWheelSelectorFactory::createWidget(Inkscape::UI::SelectedColor &color) const
{
    Gtk::Widget *w = Gtk::manage(new ColorWheelSelector(color));
    return w;
}

Glib::ustring ColorWheelSelectorFactory::modeName() const { return gettext(ColorWheelSelector::MODE_NAME); }
}
}
}
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

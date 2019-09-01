// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2005-2007 Authors:
 *   Ted Gould <ted@gould.cx>
 *   Johan Engelen <johan@shouraizou.nl>
 *   Christopher Brown <audiere@gmail.com>
 *   Jon A. Cruz <jon@joncruz.org>
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "parameter-color.h"

#include <iostream>
#include <sstream>

#include <gtkmm/box.h>
#include <gtkmm/colorbutton.h>
#include <gtkmm/label.h>

#include "color.h"
#include "preferences.h"

#include "extension/extension.h"

#include "ui/widget/color-notebook.h"

#include "xml/node.h"


namespace Inkscape {
namespace Extension {

ParamColor::ParamColor(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext)
    : InxParameter(xml, ext)
{
    // get value
    unsigned int _value = 0x000000ff; // default to black
    if (xml->firstChild()) {
        const char *value = xml->firstChild()->content();
        if (value) {
            _value = strtoul(value, nullptr, 0);
        }
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _value = prefs->getUInt(pref_name(), _value);

    _color.setValue(_value);

    _color_changed = _color.signal_changed.connect(sigc::mem_fun(this, &ParamColor::_onColorChanged));
    // TODO: SelectedColor does not properly emit signal_changed after dragging, so we also need the following
    _color_released = _color.signal_released.connect(sigc::mem_fun(this, &ParamColor::_onColorChanged));

    // parse appearance
    if (_appearance) {
        if (!strcmp(_appearance, "colorbutton")) {
            _mode = COLOR_BUTTON;
        } else {
            g_warning("Invalid value ('%s') for appearance of parameter '%s' in extension '%s'",
                      _appearance, _name, _extension->get_id());
        }
    }
}

ParamColor::~ParamColor()
{
    _color_changed.disconnect();
    _color_released.disconnect();
}

unsigned int ParamColor::set(unsigned int in)
{
    _color.setValue(in);

    return in;
}

Gtk::Widget *ParamColor::get_widget(sigc::signal<void> *changeSignal)
{
    if (_hidden) {
        return nullptr;
    }

    if (changeSignal) {
        _changeSignal = new sigc::signal<void>(*changeSignal);
    }

    Gtk::HBox *hbox = Gtk::manage(new Gtk::HBox(false, GUI_PARAM_WIDGETS_SPACING));
    if (_mode == COLOR_BUTTON) {
        Gtk::Label *label = Gtk::manage(new Gtk::Label(_text, Gtk::ALIGN_START));
        label->show();
        hbox->pack_start(*label, true, true);

        Gdk::RGBA rgba;
        rgba.set_red_u  (((_color.value() >> 24) & 255) << 8);
        rgba.set_green_u(((_color.value() >> 16) & 255) << 8);
        rgba.set_blue_u (((_color.value() >>  8) & 255) << 8);
        rgba.set_alpha_u(((_color.value() >>  0) & 255) << 8);

        // TODO: It would be nicer to have a custom Gtk::ColorButton() implementation here,
        //       that wraps an Inkscape::UI::Widget::ColorNotebook into a new dialog
        _color_button = Gtk::manage(new Gtk::ColorButton(rgba));
        _color_button->set_title(_text);
        _color_button->set_use_alpha();
        _color_button->show();
        hbox->pack_end(*_color_button, false, false);

        _color_button->signal_color_set().connect(sigc::mem_fun(this, &ParamColor::_onColorButtonChanged));
    } else {
        Gtk::Widget *selector = Gtk::manage(new Inkscape::UI::Widget::ColorNotebook(_color));
        hbox->pack_start(*selector, true, true, 0);
        selector->show();
    }
    hbox->show();
    return hbox;

}

void ParamColor::_onColorChanged()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setUInt(pref_name(), _color.value());

    if (_changeSignal)
        _changeSignal->emit();
}

void ParamColor::_onColorButtonChanged()
{
    Gdk::RGBA rgba = _color_button->get_rgba();
    unsigned int value = ((rgba.get_red_u()   >> 8) << 24) +
                         ((rgba.get_green_u() >> 8) << 16) +
                         ((rgba.get_blue_u()  >> 8) <<  8) +
                         ((rgba.get_alpha_u() >> 8) <<  0);
    set(value);
}

std::string ParamColor::value_to_string() const
{
    char value_string[16];
    snprintf(value_string, 16, "%u", _color.value());
    return value_string;
}

};  /* namespace Extension */
};  /* namespace Inkscape */

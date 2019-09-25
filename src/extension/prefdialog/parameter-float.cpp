// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2005-2007 Authors:
 *   Ted Gould <ted@gould.cx>
 *   Johan Engelen <johan@shouraizou.nl> *
 *   Jon A. Cruz <jon@joncruz.org>
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "parameter-float.h"

#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>

#include "preferences.h"

#include "extension/extension.h"

#include "ui/widget/spin-scale.h"
#include "ui/widget/spinbutton.h"

#include "xml/node.h"


namespace Inkscape {
namespace Extension {

ParamFloat::ParamFloat(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext)
    : InxParameter(xml, ext)
{
    // get value
    if (xml->firstChild()) {
        const char *value = xml->firstChild()->content();
        if (value) {
            _value = g_ascii_strtod(value, nullptr);
        }
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _value = prefs->getDouble(pref_name(), _value);

    // parse and apply limits
    const char *min = xml->attribute("min");
    if (min) {
        _min = g_ascii_strtod(min, nullptr);
    }

    const char *max = xml->attribute("max");
    if (max) {
        _max = g_ascii_strtod(max, nullptr);
    }

    if (_value < _min) {
        _value = _min;
    }

    if (_value > _max) {
        _value = _max;
    }

    // parse precision
    const char *precision = xml->attribute("precision");
    if (precision != nullptr) {
        _precision = strtol(precision, nullptr, 0);
    }


    // parse appearance
    if (_appearance) {
        if (!strcmp(_appearance, "full")) {
            _mode = FULL;
        } else {
            g_warning("Invalid value ('%s') for appearance of parameter '%s' in extension '%s'",
                      _appearance, _name, _extension->get_id());
        }
    }
}

/**
 * A function to set the \c _value.
 *
 * This function sets the internal value, but it also sets the value
 * in the preferences structure.  To put it in the right place \c pref_name() is used.
 *
 * @param  in   The value to set to.
 */
float ParamFloat::set(float in)
{
    _value = in;
    if (_value > _max) {
        _value = _max;
    }
    if (_value < _min) {
        _value = _min;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble(pref_name(), _value);

    return _value;
}

std::string ParamFloat::value_to_string() const
{
    char value_string[G_ASCII_DTOSTR_BUF_SIZE];
    // TODO: Some strange rounding is going on here, resulting in parameter values quite different
    //       from the original string value. Needs some investigation to make it less bad.
    //       See also https://gitlab.gnome.org/GNOME/glib/issues/964
    g_ascii_dtostr(value_string, G_ASCII_DTOSTR_BUF_SIZE, _value);
    return value_string;
}

/** A class to make an adjustment that uses Extension params. */
class ParamFloatAdjustment : public Gtk::Adjustment {
    /** The parameter to adjust. */
    ParamFloat *_pref;
    sigc::signal<void> *_changeSignal;
public:
    /** Make the adjustment using an extension and the string
                describing the parameter. */
    ParamFloatAdjustment(ParamFloat *param, sigc::signal<void> *changeSignal)
        : Gtk::Adjustment(0.0, param->min(), param->max(), 0.1, 1.0, 0)
        , _pref(param)
        , _changeSignal(changeSignal) {
        this->set_value(_pref->get());
        this->signal_value_changed().connect(sigc::mem_fun(this, &ParamFloatAdjustment::val_changed));
        return;
    };

    void val_changed ();
}; /* class ParamFloatAdjustment */

/**
 * A function to respond to the value_changed signal from the adjustment.
 *
 * This function just grabs the value from the adjustment and writes
 * it to the parameter.  Very simple, but yet beautiful.
 */
void ParamFloatAdjustment::val_changed()
{
    _pref->set(this->get_value());
    if (_changeSignal != nullptr) {
        _changeSignal->emit();
    }
    return;
}

/**
 * Creates a Float Adjustment for a float parameter.
 *
 * Builds a hbox with a label and a float adjustment in it.
 */
Gtk::Widget *ParamFloat::get_widget(sigc::signal<void> *changeSignal)
{
    if (_hidden) {
        return nullptr;
    }

    Gtk::HBox *hbox = Gtk::manage(new Gtk::HBox(false, GUI_PARAM_WIDGETS_SPACING));

    auto pfa = new ParamFloatAdjustment(this, changeSignal);
    Glib::RefPtr<Gtk::Adjustment> fadjust(pfa);

    if (_mode == FULL) {

        Glib::ustring text;
        if (_text != nullptr)
            text = _text;
        UI::Widget::SpinScale *scale = Gtk::manage(new UI::Widget::SpinScale(text, fadjust, _precision));
        scale->set_size_request(400, -1);
        scale->show();
        hbox->pack_start(*scale, true, true);

    }
    else if (_mode == DEFAULT) {

        Gtk::Label *label = Gtk::manage(new Gtk::Label(_text, Gtk::ALIGN_START));
        label->show();
        hbox->pack_start(*label, true, true);

	auto spin = Gtk::manage(new Inkscape::UI::Widget::SpinButton(fadjust, 0.1, _precision));
        spin->show();
        hbox->pack_start(*spin, false, false);
    }

    hbox->show();

    return dynamic_cast<Gtk::Widget *>(hbox);
}


}  /* namespace Extension */
}  /* namespace Inkscape */

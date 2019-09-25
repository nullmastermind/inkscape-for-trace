// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2005-2007 Authors:
 *   Ted Gould <ted@gould.cx>
 *   Johan Engelen <johan@shouraizou.nl> *
 *   Jon A. Cruz <jon@joncruz.org>
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "parameter-int.h"

#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>

#include "preferences.h"

#include "extension/extension.h"

#include "ui/widget/spinbutton.h"
#include "ui/widget/spin-scale.h"

#include "xml/node.h"


namespace Inkscape {
namespace Extension {


ParamInt::ParamInt(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext)
    : InxParameter(xml, ext)
{
    // get value
    if (xml->firstChild()) {
        const char *value = xml->firstChild()->content();
        if (value) {
            _value = strtol(value, nullptr, 0);
        }
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _value = prefs->getInt(pref_name(), _value);

    // parse and apply limits
    const char *min = xml->attribute("min");
    if (min) {
        _min = strtol(min, nullptr, 0);
    }

    const char *max = xml->attribute("max");
    if (max) {
        _max = strtol(max, nullptr, 0);
    }

    if (_value < _min) {
        _value = _min;
    }

    if (_value > _max) {
        _value = _max;
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
 * This function sets the internal value, but it also sets the value
 * in the preferences structure.  To put it in the right place \c pref_name() is used.
 *
 * @param  in   The value to set to.
 */
int ParamInt::set(int in)
{
    _value = in;
    if (_value > _max) {
        _value = _max;
    }
    if (_value < _min) {
        _value = _min;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt(pref_name(), _value);

    return _value;
}

/** A class to make an adjustment that uses Extension params. */
class ParamIntAdjustment : public Gtk::Adjustment {
    /** The parameter to adjust. */
    ParamInt *_pref;
    sigc::signal<void> *_changeSignal;
public:
    /** Make the adjustment using an extension and the string describing the parameter. */
    ParamIntAdjustment(ParamInt *param, sigc::signal<void> *changeSignal)
        : Gtk::Adjustment(0.0, param->min(), param->max(), 1.0, 10.0, 0)
        , _pref(param)
        , _changeSignal(changeSignal)
    {
        this->set_value(_pref->get());
        this->signal_value_changed().connect(sigc::mem_fun(this, &ParamIntAdjustment::val_changed));
    };

    void val_changed ();
}; /* class ParamIntAdjustment */

/**
 * A function to respond to the value_changed signal from the adjustment.
 *
 * This function just grabs the value from the adjustment and writes
 * it to the parameter.  Very simple, but yet beautiful.
 */
void ParamIntAdjustment::val_changed()
{
    _pref->set((int)this->get_value());
    if (_changeSignal != nullptr) {
        _changeSignal->emit();
    }
}

/**
 * Creates a Int Adjustment for a int parameter.
 *
 * Builds a hbox with a label and a int adjustment in it.
 */
Gtk::Widget *
ParamInt::get_widget(sigc::signal<void> *changeSignal)
{
    if (_hidden) {
        return nullptr;
    }

    Gtk::HBox *hbox = Gtk::manage(new Gtk::HBox(false, GUI_PARAM_WIDGETS_SPACING));

    auto pia = new ParamIntAdjustment(this, changeSignal);
    Glib::RefPtr<Gtk::Adjustment> fadjust(pia);

    if (_mode == FULL) {

        Glib::ustring text;
        if (_text != nullptr)
            text = _text;
        UI::Widget::SpinScale *scale = Gtk::manage(new UI::Widget::SpinScale(text, fadjust, 0));
        scale->set_size_request(400, -1);
        scale->show();
        hbox->pack_start(*scale, true, true);
    } else if (_mode == DEFAULT) {
        Gtk::Label *label = Gtk::manage(new Gtk::Label(_text, Gtk::ALIGN_START));
        label->show();
        hbox->pack_start(*label, true, true);

        auto spin = Gtk::manage(new Inkscape::UI::Widget::SpinButton(fadjust, 1.0, 0));
        spin->show();
        hbox->pack_start(*spin, false, false);
    }

    hbox->show();

    return dynamic_cast<Gtk::Widget *>(hbox);
}

std::string ParamInt::value_to_string() const
{
    char value_string[32];
    snprintf(value_string, 32, "%d", _value);
    return value_string;
}

}  // namespace Extension
}  // namespace Inkscape

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

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2005-2007 Authors:
 *   Ted Gould <ted@gould.cx>
 *   Johan Engelen <johan@shouraizou.nl> *
 *   Jon A. Cruz <jon@joncruz.org>
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "parameter-bool.h"

#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>

#include "xml/node.h"
#include "extension/extension.h"
#include "preferences.h"

namespace Inkscape {
namespace Extension {

ParamBool::ParamBool(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext)
    : InxParameter(xml, ext)
{
    // get value
    if (xml->firstChild()) {
        const char *value = xml->firstChild()->content();
        if (value) {
            if (!strcmp(value, "true")) {
                _value = true;
            } else if (!strcmp(value, "false")) {
                _value = false;
            } else {
                g_warning("Invalid default value ('%s') for parameter '%s' in extension '%s'",
                          value, _name, _extension->get_id());
            }
        }
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _value = prefs->getBool(pref_name(), _value);
}

bool ParamBool::set(bool in)
{
    _value = in;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool(pref_name(), _value);

    return _value;
}

bool ParamBool::get() const
{
    return _value;
}

/**
 * A check button which is Param aware.  It works with the
 * parameter to change it's value as the check button changes
 * value.
 */
class ParamBoolCheckButton : public Gtk::CheckButton {
public:
    /**
     * Initialize the check button.
     * This function sets the value of the checkbox to be that of the
     * parameter, and then sets up a callback to \c on_toggle.
     *
     * @param  param  Which parameter to adjust on changing the check button
     */
    ParamBoolCheckButton(ParamBool *param, char *label, sigc::signal<void> *changeSignal)
        : Gtk::CheckButton(label)
        , _pref(param)
        , _changeSignal(changeSignal) {
        this->set_active(_pref->get());
        this->signal_toggled().connect(sigc::mem_fun(this, &ParamBoolCheckButton::on_toggle));
        return;
    }

    /**
     * A function to respond to the check box changing.
     * Adjusts the value of the preference to match that in the check box.
     */
    void on_toggle ();

private:
    /** Param to change. */
    ParamBool *_pref;
    sigc::signal<void> *_changeSignal;
};

void ParamBoolCheckButton::on_toggle()
{
    _pref->set(this->get_active());
    if (_changeSignal != nullptr) {
        _changeSignal->emit();
    }
    return;
}

std::string ParamBool::value_to_string() const
{
    if (_value) {
        return "true";
    }
    return "false";
}

Gtk::Widget *ParamBool::get_widget(sigc::signal<void> *changeSignal)
{
    if (_hidden) {
        return nullptr;
    }

    auto hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, GUI_PARAM_WIDGETS_SPACING));
    hbox->set_homogeneous(false);

    ParamBoolCheckButton * checkbox = Gtk::manage(new ParamBoolCheckButton(this, _text, changeSignal));
    checkbox->show();
    hbox->pack_start(*checkbox, false, false);

    hbox->show();

    return dynamic_cast<Gtk::Widget *>(hbox);
}

}  /* namespace Extension */
}  /* namespace Inkscape */

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

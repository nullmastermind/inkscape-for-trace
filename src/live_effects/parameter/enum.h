// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_ENUM_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_ENUM_H

/*
 * Inkscape::LivePathEffectParameters
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/widget/registered-enums.h"
#include <glibmm/ustring.h>
#include "live_effects/effect.h"
#include "live_effects/parameter/parameter.h"
#include "verbs.h"

namespace Inkscape {

namespace LivePathEffect {

template<typename E> class EnumParam : public Parameter {
public:
    EnumParam(  const Glib::ustring& label,
                const Glib::ustring& tip,
                const Glib::ustring& key,
                const Util::EnumDataConverter<E>& c,
                Inkscape::UI::Widget::Registry* wr,
                Effect* effect,
                E default_value,
                bool sort = true)
        : Parameter(label, tip, key, wr, effect)
    {
        enumdataconv = &c;
        defvalue = default_value;
        value = defvalue;
        sorted = sort;
    };

    ~EnumParam() override = default;;
    EnumParam(const EnumParam&) = delete;
    EnumParam& operator=(const EnumParam&) = delete;

    Gtk::Widget * param_newWidget() override {
        Inkscape::UI::Widget::RegisteredEnum<E> *regenum = Gtk::manage ( 
            new Inkscape::UI::Widget::RegisteredEnum<E>( param_label, param_tooltip,
                       param_key, *enumdataconv, *param_wr, param_effect->getRepr(), param_effect->getSPDoc(), sorted ) );

        regenum->set_active_by_id(value);
        regenum->combobox()->setProgrammatically = false;
        regenum->combobox()->signal_changed().connect(sigc::mem_fun (*this, &EnumParam::_on_change_combo));
        regenum->set_undo_parameters(SP_VERB_DIALOG_LIVE_PATH_EFFECT, _("Change enumeration parameter"));
        
        return dynamic_cast<Gtk::Widget *> (regenum);
    };
    void _on_change_combo() { param_effect->refresh_widgets = true; }
    bool param_readSVGValue(const gchar * strvalue) override {
        if (!strvalue) {
            param_set_default();
            return true;
        }

        param_set_value( enumdataconv->get_id_from_key(Glib::ustring(strvalue)) );

        return true;
    };
    Glib::ustring param_getSVGValue() const override {
        return enumdataconv->get_key(value);
    };
    
    Glib::ustring param_getDefaultSVGValue() const override {
        return enumdataconv->get_key(defvalue).c_str();
    };
    
    E get_value() const {
        return value;
    }

    inline operator E() const {
        return value;
    };

    void param_set_default() override {
        param_set_value(defvalue);
    }
    
    void param_update_default(E default_value) {
        defvalue = default_value;
    }
    
    void param_update_default(const gchar * default_value) override {
        param_update_default(enumdataconv->get_id_from_key(Glib::ustring(default_value)));
    }
    
    void param_set_value(E val) {
        value = val;
    }

private:
    E value;
    E defvalue;
    bool sorted;

    const Util::EnumDataConverter<E> * enumdataconv;
};


}; //namespace LivePathEffect

}; //namespace Inkscape

#endif

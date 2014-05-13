#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_TOGGLEBUTTON_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_TOGGLEBUTTON_H

/*
 * Inkscape::LivePathEffectParameters
 *
* Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <glib.h>

#include "live_effects/parameter/parameter.h"

namespace Inkscape {

namespace LivePathEffect {


class ToggleButtonParam : public Parameter {
public:
    ToggleButtonParam( const Glib::ustring& label,
               const Glib::ustring& tip,
               const Glib::ustring& key,
               Inkscape::UI::Widget::Registry* wr,
               Effect* effect,
               bool default_value = false);
    virtual ~ToggleButtonParam();

    virtual Gtk::Widget * param_newWidget();

    virtual bool param_readSVGValue(const gchar * strvalue);
    virtual gchar * param_getSVGValue() const;

    void param_setValue(bool newvalue);
    virtual void param_set_default();

    bool get_value() const { return value; };

    inline operator bool() const { return value; };

private:
    ToggleButtonParam(const ToggleButtonParam&);
    ToggleButtonParam& operator=(const ToggleButtonParam&);

    bool value;
    bool defvalue;
};


} //namespace LivePathEffect

} //namespace Inkscape

#endif

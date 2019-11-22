// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_BOOL_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_BOOL_H

/*
 * Inkscape::LivePathEffectParameters
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glib.h>

#include "live_effects/parameter/parameter.h"

namespace Inkscape {

namespace LivePathEffect {


class BoolParam : public Parameter {
public:
    BoolParam( const Glib::ustring& label,
               const Glib::ustring& tip,
               const Glib::ustring& key,
               Inkscape::UI::Widget::Registry* wr,
               Effect* effect,
               bool default_value = false);
    ~BoolParam() override;
    BoolParam(const BoolParam&) = delete;
    BoolParam& operator=(const BoolParam&) = delete;

    Gtk::Widget * param_newWidget() override;

    bool param_readSVGValue(const gchar * strvalue) override;
    Glib::ustring param_getSVGValue() const override;
    Glib::ustring param_getDefaultSVGValue() const override;

    void param_setValue(bool newvalue);
    void param_set_default() override;
    void param_update_default(bool const default_value);
    void param_update_default(const gchar * default_value) override;
    bool get_value() const { return value; };
    inline operator bool() const { return value; };

private:
    bool value;
    bool defvalue;
};


} //namespace LivePathEffect

} //namespace Inkscape

#endif

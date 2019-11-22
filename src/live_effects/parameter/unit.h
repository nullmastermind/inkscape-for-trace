// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_UNIT_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_UNIT_H

/*
 * Inkscape::LivePathEffectParameters
 *
 * Copyright (C) Maximilian Albert 2008 <maximilian.albert@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/parameter/parameter.h"

namespace Inkscape {

namespace Util {
    class Unit;
}

namespace LivePathEffect {

class UnitParam : public Parameter {
public:
    UnitParam(const Glib::ustring& label,
		  const Glib::ustring& tip,
		  const Glib::ustring& key, 
		  Inkscape::UI::Widget::Registry* wr,
		  Effect* effect,
		  Glib::ustring default_unit = "px");
    ~UnitParam() override;

    bool param_readSVGValue(const gchar * strvalue) override;
    Glib::ustring param_getSVGValue() const override;
    Glib::ustring param_getDefaultSVGValue() const override;
    void param_set_default() override;
    void param_set_value(Inkscape::Util::Unit const &val);
    void param_update_default(const gchar * default_unit) override;
    const gchar *get_abbreviation() const;
    Gtk::Widget * param_newWidget() override;
    
    operator Inkscape::Util::Unit const *() const { return unit; }

private:
    Inkscape::Util::Unit const *unit;
    Inkscape::Util::Unit const *defunit;

    UnitParam(const UnitParam&) = delete;
    UnitParam& operator=(const UnitParam&) = delete;
};

} //namespace LivePathEffect

} //namespace Inkscape

#endif

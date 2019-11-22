// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_HIDDEN_H
#define INKSCAPE_LIVEPATHEFFECT_HIDDEN_H

/*
 * Inkscape::LivePathEffectParameters
 *
 * Authors:
 *   Jabiertxof
 *   Maximilian Albert
 *   Johan Engelen
 *
 * Copyright (C) jabiertxof 2017 <jabier.arraiza@marker.es>
 * Copyright (C) Maximilian Albert 2008 <maximilian.albert@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/parameter/parameter.h"


namespace Inkscape {

namespace LivePathEffect {

class HiddenParam : public Parameter {
public:
    HiddenParam( const Glib::ustring& label,
               const Glib::ustring& tip,
               const Glib::ustring& key,
               Inkscape::UI::Widget::Registry* wr,
               Effect* effect,
               const Glib::ustring default_value = "",
               bool widget_is_visible = false);
    ~HiddenParam() override = default;

    Gtk::Widget * param_newWidget() override;

    bool param_readSVGValue(const gchar * strvalue) override;
    Glib::ustring param_getSVGValue() const override;
    Glib::ustring param_getDefaultSVGValue() const override;

    void param_setValue(Glib::ustring newvalue, bool write = false);
    void param_set_default() override;
    void param_update_default(const gchar * default_value) override;

    const Glib::ustring get_value() const { return value; };

private:
    HiddenParam(const HiddenParam&) = delete;
    HiddenParam& operator=(const HiddenParam&) = delete;
    Glib::ustring value;
    Glib::ustring defvalue;
};

} //namespace LivePathEffect

} //namespace Inkscape

#endif

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

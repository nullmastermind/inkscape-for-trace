// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_TEXT_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_TEXT_H

/*
 * Inkscape::LivePathEffectParameters
 *
 * Authors:
 *   Maximilian Albert
 *   Johan Engelen
 *
 * Copyright (C) Maximilian Albert 2008 <maximilian.albert@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glib.h>

#include "live_effects/parameter/parameter.h"

namespace Inkscape {

class CanvasItemText;

namespace LivePathEffect {

class TextParam : public Parameter {
public:
    TextParam( const Glib::ustring& label,
               const Glib::ustring& tip,
               const Glib::ustring& key,
               Inkscape::UI::Widget::Registry* wr,
               Effect* effect,
               const Glib::ustring default_value = "");
    ~TextParam() override;

    Gtk::Widget * param_newWidget() override;

    bool param_readSVGValue(const gchar * strvalue) override;
    Glib::ustring param_getSVGValue() const override;
    Glib::ustring param_getDefaultSVGValue() const override;

    void param_setValue(Glib::ustring newvalue);
    void param_hide_canvas_text();
    void setTextParam(Inkscape::UI::Widget::RegisteredText *rsu);
    void param_set_default() override;
    void param_update_default(const gchar * default_value) override;
    void setPos(Geom::Point pos);
    void setPosAndAnchor(const Geom::Piecewise<Geom::D2<Geom::SBasis> > &pwd2,
			 const double t, const double length, bool use_curvature = false);
    void setAnchor(double x_value, double y_value);

    const Glib::ustring get_value() const { return value; };

private:
    TextParam(const TextParam&) = delete;
    TextParam& operator=(const TextParam&) = delete;
    double anchor_x;
    double anchor_y;
    Glib::ustring value;
    Glib::ustring defvalue;
    Inkscape::CanvasItemText *canvas_text = nullptr;
};

/*
 * This parameter does not display a widget in the LPE dialog; LPEs can use it to display on-canvas
 * text that should not be settable by the user. Note that since no widget is provided, the
 * parameter must be initialized differently than usual (only with a pointer to the parent effect;
 * no label, no tooltip, etc.).
 */
class TextParamInternal : public TextParam {
public:
    TextParamInternal(Effect* effect) :
        TextParam("", "", "", nullptr, effect) {}

    Gtk::Widget * param_newWidget() override { return nullptr; }
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

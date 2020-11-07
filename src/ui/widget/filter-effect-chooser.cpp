// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Filter effect selection selection widget
 *
 * Author:
 *   Nicholas Bishop <nicholasbishop@gmail.com>
 *   Tavmjong Bah
 *
 * Copyright (C) 2007, 2017 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "filter-effect-chooser.h"

#include "document.h"

namespace Inkscape {

const EnumData<SPBlendMode> SPBlendModeData[SP_CSS_BLEND_ENDMODE] = {
    { SP_CSS_BLEND_NORMAL, _("Normal"), "normal" },
    { SP_CSS_BLEND_MULTIPLY, _("Multiply"), "multiply" },
    { SP_CSS_BLEND_SCREEN, _("Screen"), "screen" },
    { SP_CSS_BLEND_DARKEN, _("Darken"), "darken" },
    { SP_CSS_BLEND_LIGHTEN, _("Lighten"), "lighten" },
    // New in Compositing and Blending Level 1
    { SP_CSS_BLEND_OVERLAY, _("Overlay"), "overlay" },
    { SP_CSS_BLEND_COLORDODGE, _("Color Dodge"), "color-dodge" },
    { SP_CSS_BLEND_COLORBURN, _("Color Burn"), "color-burn" },
    { SP_CSS_BLEND_HARDLIGHT, _("Hard Light"), "hard-light" },
    { SP_CSS_BLEND_SOFTLIGHT, _("Soft Light"), "soft-light" },
    { SP_CSS_BLEND_DIFFERENCE, _("Difference"), "difference" },
    { SP_CSS_BLEND_EXCLUSION, _("Exclusion"), "exclusion" },
    { SP_CSS_BLEND_HUE, _("Hue"), "hue" },
    { SP_CSS_BLEND_SATURATION, _("Saturation"), "saturation" },
    { SP_CSS_BLEND_COLOR, _("Color"), "color" },
    { SP_CSS_BLEND_LUMINOSITY, _("Luminosity"), "luminosity" }
};
const EnumDataConverter<SPBlendMode> SPBlendModeConverter(SPBlendModeData, SP_CSS_BLEND_ENDMODE);


namespace UI {
namespace Widget {

SimpleFilterModifier::SimpleFilterModifier(int flags)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL)
    , _flags(flags)
    , _lb_blend(_("Blend mode:"))
    , _lb_isolation("Isolate") // Translate for 1.1
    , _blend(SPBlendModeConverter, SPAttr::INVALID, false)
    , _blur(_("Blur (%)"), 0, 0, 100, 1, 0.1, 1)
    , _opacity(_("Opacity (%)"), 0, 0, 100, 1, 0.1, 1)
    , _notify(true)
    , _hb_blend(Gtk::ORIENTATION_HORIZONTAL)
{
    set_name("SimpleFilterModifier");

    _flags = flags;

    if (flags & BLEND) {
        add(_hb_blend);
        _lb_blend.set_use_underline();
        _hb_blend.set_halign(Gtk::ALIGN_END);
        _hb_blend.set_valign(Gtk::ALIGN_CENTER);
        _hb_blend.set_margin_top(3);
        _hb_blend.set_margin_end(5);
        _lb_blend.set_mnemonic_widget(_blend);
        _hb_blend.pack_start(_lb_blend, false, false, 5);
        _hb_blend.pack_start(_blend, false, false, 5);
        /*
        * For best fit inkscape-browsers with no GUI to isolation we need all groups, 
        * clones and symbols with isolation == isolate to not show to the user of
        * Inkscape a "strange" behabiour from the designer point of view. 
        * Is strange because only happends when object not has clip, mask, 
        * filter, blending or opacity .
        * Anyway the feature is a no-gui feature and render as spected.
        */
        /* if (flags & ISOLATION) {
            _isolation.property_active() = false;
            _hb_blend.pack_start(_isolation, false, false, 5);
            _hb_blend.pack_start(_lb_isolation, false, false, 5);
            _isolation.set_tooltip_text("Don't blend childrens with objects behind");
            _lb_isolation.set_tooltip_text("Don't blend childrens with objects behind");
        } */
        Gtk::Separator *separator = Gtk::manage(new Gtk::Separator());  
        separator->set_margin_top(8);
        separator->set_margin_bottom(8);
        add(*separator);
    }

    if (flags & BLUR) {
        add(_blur);
    }

    if (flags & OPACITY) {
        add(_opacity);
    }
    show_all_children();

    _blend.signal_changed().connect(signal_blend_changed());
    _blur.signal_value_changed().connect(signal_blur_changed());
    _opacity.signal_value_changed().connect(signal_opacity_changed());
    _isolation.signal_toggled().connect(signal_isolation_changed());
}

sigc::signal<void> &SimpleFilterModifier::signal_isolation_changed()
{
    if (_notify) {
        return _signal_isolation_changed;
    }
    _notify = true;
    return _signal_null;
}

sigc::signal<void>& SimpleFilterModifier::signal_blend_changed()
{
    if (_notify) {
        return _signal_blend_changed;
    }
    _notify = true;
    return _signal_null;
}

sigc::signal<void>& SimpleFilterModifier::signal_blur_changed()
{
    // we dont use notifi to block use aberaje for multiple
    return _signal_blur_changed;
}

sigc::signal<void>& SimpleFilterModifier::signal_opacity_changed()
{
    // we dont use notifi to block use averaje for multiple
    return _signal_opacity_changed;
}

SPIsolation SimpleFilterModifier::get_isolation_mode()
{
    return _isolation.get_active() ? SP_CSS_ISOLATION_ISOLATE : SP_CSS_ISOLATION_AUTO;
}

void SimpleFilterModifier::set_isolation_mode(const SPIsolation val, bool notify)
{
    _notify = notify;
    _isolation.set_active(val == SP_CSS_ISOLATION_ISOLATE);
}

SPBlendMode SimpleFilterModifier::get_blend_mode()
{
    const Util::EnumData<SPBlendMode> *d = _blend.get_active_data();
    if (d) {
        return _blend.get_active_data()->id;
    } else {
        return SP_CSS_BLEND_NORMAL;
    }
}

void SimpleFilterModifier::set_blend_mode(const SPBlendMode val, bool notify)
{
    _notify = notify;
    _blend.set_active(val);
}

double SimpleFilterModifier::get_blur_value() const
{
    return _blur.get_value();
}

void SimpleFilterModifier::set_blur_value(const double val)
{
    _blur.set_value(val);
}

double SimpleFilterModifier::get_opacity_value() const
{
    return _opacity.get_value();
}

void SimpleFilterModifier::set_opacity_value(const double val)
{
    _opacity.set_value(val);
}

}
}
}

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

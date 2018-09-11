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
namespace UI {
namespace Widget {

SimpleFilterModifier::SimpleFilterModifier(int flags)
    : _flags( flags )
    , _lb_blend(_("Blend mode:"))
    , _blend(BlendModeConverter, SP_ATTR_INVALID, false)
    , _blur(   _("Blur (%)"   ), 0, 0, 100, 1, 0.1, 1)
    , _opacity(_("Opacity (%)"), 0, 0, 100, 1, 0.1, 1)
{
    set_name("SimpleFilterModifier");

    _flags = flags;

    if (flags & BLEND) {
        add(_hb_blend);
        _lb_blend.set_use_underline();
        _lb_blend.set_mnemonic_widget(_blend);
        _hb_blend.pack_start(_lb_blend, false, false, 0);
        _hb_blend.pack_start(_blend);
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
}

sigc::signal<void>& SimpleFilterModifier::signal_blend_changed()
{
    return _signal_blend_changed;
}

sigc::signal<void>& SimpleFilterModifier::signal_blur_changed()
{
    return _signal_blur_changed;
}

sigc::signal<void>& SimpleFilterModifier::signal_opacity_changed()
{
    return _signal_opacity_changed;
}

const Glib::ustring SimpleFilterModifier::get_blend_mode()
{
    const Util::EnumData<Inkscape::Filters::FilterBlendMode> *d = _blend.get_active_data();
    if (d) {
        return _blend.get_active_data()->key;
    } else
        return "normal";
}

void SimpleFilterModifier::set_blend_mode(const int val)
{
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

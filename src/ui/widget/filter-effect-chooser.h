// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __FILTER_EFFECT_CHOOSER_H__
#define __FILTER_EFFECT_CHOOSER_H__

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

#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/combobox.h>
#include <gtkmm/separator.h>

#include "combo-enums.h"
#include "spin-scale.h"
#include "style-enums.h"

using Inkscape::Util::EnumData;
using Inkscape::Util::EnumDataConverter;

namespace Inkscape {
extern const Util::EnumDataConverter<SPBlendMode> SPBlendModeConverter;
namespace UI {
namespace Widget {

/* Allows basic control over feBlend and feGaussianBlur effects as well as opacity.
 *  Common for Object, Layers, and Fill and Stroke dialogs.
*/
class SimpleFilterModifier : public Gtk::VBox
{
public:
  enum Flags { NONE = 0, BLUR = 1, OPACITY = 2, BLEND = 4, ISOLATION = 16 };

  SimpleFilterModifier(int flags);

  sigc::signal<void> &signal_blend_changed();
  sigc::signal<void> &signal_blur_changed();
  sigc::signal<void> &signal_opacity_changed();
  sigc::signal<void> &signal_isolation_changed();

  SPIsolation get_isolation_mode();
  void set_isolation_mode(const SPIsolation, bool notify);

  SPBlendMode get_blend_mode();
  void set_blend_mode(const SPBlendMode, bool notify);

  double get_blur_value() const;
  void set_blur_value(const double);

  double get_opacity_value() const;
  void set_opacity_value(const double);

private:
    int _flags;
    bool _notify;

    Gtk::HBox _hb_blend;
    Gtk::Label _lb_blend;
    Gtk::Label _lb_isolation;
    ComboBoxEnum<SPBlendMode> _blend;
    SpinScale _blur;
    SpinScale _opacity;
    Gtk::CheckButton _isolation;

    sigc::signal<void> _signal_null;
    sigc::signal<void> _signal_blend_changed;
    sigc::signal<void> _signal_blur_changed;
    sigc::signal<void> _signal_opacity_changed;
    sigc::signal<void> _signal_isolation_changed;
};

}
}
}

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

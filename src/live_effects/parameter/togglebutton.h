// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_TOGGLEBUTTON_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_TOGGLEBUTTON_H

/*
 * Copyright (C) Jabiertxo Arraiza Cenoz 2014
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glib.h>
#include <sigc++/connection.h>
#include <sigc++/signal.h>

#include "live_effects/parameter/parameter.h"
#include "ui/widget/registered-widget.h"

namespace Inkscape {

namespace LivePathEffect {

/**
 * class ToggleButtonParam:
 *    represents a Gtk::ToggleButton as a Live Path Effect parameter
 */
class ToggleButtonParam : public Parameter {
public:
  ToggleButtonParam(const Glib::ustring &label, const Glib::ustring &tip, const Glib::ustring &key,
                    Inkscape::UI::Widget::Registry *wr, Effect *effect, bool default_value = false,
                    Glib::ustring inactive_label = "", char const *icon_active = nullptr,
                    char const *icon_inactive = nullptr, Gtk::BuiltinIconSize icon_size = Gtk::ICON_SIZE_SMALL_TOOLBAR);
  ~ToggleButtonParam() override;
  ToggleButtonParam(const ToggleButtonParam &) = delete;
  ToggleButtonParam &operator=(const ToggleButtonParam &) = delete;

  Gtk::Widget *param_newWidget() override;

  bool param_readSVGValue(const gchar *strvalue) override;
  Glib::ustring param_getSVGValue() const override;
  Glib::ustring param_getDefaultSVGValue() const override;

  void param_setValue(bool newvalue);
  void param_set_default() override;

  bool get_value() const { return value; };

  inline operator bool() const { return value; };

  sigc::signal<void> &signal_toggled() { return _signal_toggled; }
  virtual void toggled();
  void param_update_default(bool default_value);
  void param_update_default(const gchar *default_value) override;

private:
    void refresh_button();
    bool value;
    bool defvalue;
    const Glib::ustring inactive_label;
    const char * _icon_active;
    const char * _icon_inactive;
    Gtk::BuiltinIconSize _icon_size;
    Inkscape::UI::Widget::RegisteredToggleButton * checkwdg;

    sigc::signal<void> _signal_toggled;
    sigc::connection _toggled_connection;
};


} //namespace LivePathEffect

} //namespace Inkscape

#endif

// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INK_SPINSCALE_H
#define INK_SPINSCALE_H

/*
 * Authors:
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright  (C) 2017 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

/**
   A widget that allows entering a numerical value either by
   clicking/dragging on a custom Gtk::Scale or by using a
   Gtk::SpinButton. The custom Gtk::Scale differs from the stock
   Gtk::Scale in that it includes a label to save space and has a
   "slow-dragging" mode triggered by the Alt key.
*/

#include <glibmm/ustring.h>

#include <gtkmm/box.h>
#include <gtkmm/scale.h>

namespace Gtk {
  class SpinButton;
}

class InkScale : public Gtk::Scale
{
 public:
  InkScale(Glib::RefPtr<Gtk::Adjustment>, Gtk::SpinButton* spinbutton);
  ~InkScale() override = default;;

  void set_label(Glib::ustring label);

  bool on_draw(const::Cairo::RefPtr<::Cairo::Context>& cr) override;

 protected:

  bool on_button_press_event(GdkEventButton* button_event) override;
  bool on_button_release_event(GdkEventButton* button_event) override;
  bool on_motion_notify_event(GdkEventMotion* motion_event) override;

 private:

  double get_fraction();
  void set_adjustment_value(double x, bool constrained = false);

  Gtk::SpinButton * _spinbutton; // Needed to get placement/text color.
  Glib::ustring _label;

  bool   _dragging;
  double _drag_start;
  double _drag_offset;
};

class InkSpinScale : public Gtk::Box
{
 public:

  // Create an InkSpinScale with a new adjustment.
  InkSpinScale(double value,
               double lower,
               double upper,
               double step_increment = 1,
               double page_increment = 10,
               double page_size = 0);

  // Create an InkSpinScale with a preexisting adjustment.
  InkSpinScale(Glib::RefPtr<Gtk::Adjustment>);

  ~InkSpinScale() override = default;;

  void set_label(Glib::ustring label);
  void set_digits(int digits);
  int  get_digits() const;
  void set_focus_widget(GtkWidget *focus_widget);
  Glib::RefPtr<Gtk::Adjustment> get_adjustment() { return _adjustment; };

 protected:

  InkScale*                      _scale;
  Gtk::SpinButton*               _spinbutton;
  Glib::RefPtr<Gtk::Adjustment>  _adjustment;
  GtkWidget*                     _focus_widget = nullptr;

  bool on_key_release_event(GdkEventKey* key_event) override;

 private:

};

#endif // INK_SPINSCALE_H

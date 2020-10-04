// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright  (C) 2017 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

/** \file
   A widget that allows entering a numerical value either by
   clicking/dragging on a custom Gtk::Scale or by using a
   Gtk::SpinButton. The custom Gtk::Scale differs from the stock
   Gtk::Scale in that it includes a label to save space and has a
   "slow dragging" mode triggered by the Alt key.
*/

#include "ink-spinscale.h"
#include <gdkmm/general.h>
#include <gdkmm/cursor.h>
#include <gdkmm/event.h>

#include <gtkmm/spinbutton.h>

#include <gdk/gdk.h>

#include <iostream>
#include <utility>

InkScale::InkScale(Glib::RefPtr<Gtk::Adjustment> adjustment, Gtk::SpinButton* spinbutton)
  : Glib::ObjectBase("InkScale")
  , parent_type(adjustment)
  , _spinbutton(spinbutton)
  , _dragging(false)
  , _drag_start(0)
  , _drag_offset(0)
{
  set_name("InkScale");
  // std::cout << "GType name: " << G_OBJECT_TYPE_NAME(gobj()) << std::endl;
}

void
InkScale::set_label(Glib::ustring label) {
  _label = label;
}

bool
InkScale::on_draw(const::Cairo::RefPtr<::Cairo::Context>& cr) {

  Gtk::Range::on_draw(cr);

  // Get SpinButton style info...
  auto style_spin = _spinbutton->get_style_context();
  auto state_spin = style_spin->get_state();
  Gdk::RGBA text_color = style_spin->get_color( state_spin );

  // Create Pango layout.
  auto layout_label = create_pango_layout(_label);
  layout_label->set_ellipsize( Pango::ELLIPSIZE_END );
  layout_label->set_width(PANGO_SCALE * get_width());

  // Get y location of SpinButton text (to match vertical position of SpinButton text).
  int x, y;
  _spinbutton->get_layout_offsets(x, y);

  // Fill widget proportional to value.
  double fraction = get_fraction();

  // Get trough rectangle and clipping point for text.
  Gdk::Rectangle slider_area = get_range_rect();
  double clip_text_x = slider_area.get_x() + slider_area.get_width() * fraction;

  // Render text in normal text color.
  cr->save();
  cr->rectangle(clip_text_x, 0, get_width(), get_height());
  cr->clip();
  Gdk::Cairo::set_source_rgba(cr, text_color);
  //cr->set_source_rgba(0, 0, 0, 1);
  cr->move_to(5, y );
  layout_label->show_in_cairo_context(cr);
  cr->restore();

  // Render text, clipped, in white over bar (TODO: use same color as SpinButton progress bar).
  cr->save();
  cr->rectangle(0, 0, clip_text_x, get_height());
  cr->clip();
  cr->set_source_rgba(1, 1, 1, 1);
  cr->move_to(5, y);
  layout_label->show_in_cairo_context(cr);
  cr->restore();

  return true;
}

bool
InkScale::on_button_press_event(GdkEventButton* button_event) {

  if (! (button_event->state & GDK_MOD1_MASK) ) {
    bool constrained = button_event->state & GDK_CONTROL_MASK;
    set_adjustment_value(button_event->x, constrained);
  }

  // Dragging must be initialized after any adjustment due to button press.
  _dragging = true;
  _drag_start  = button_event->x;
  _drag_offset = get_width() * get_fraction(); 

  return true;
}

bool
InkScale::on_button_release_event(GdkEventButton* button_event) {

  _dragging = false;
  return true;
}

bool
InkScale::on_motion_notify_event(GdkEventMotion* motion_event) {

  double x = motion_event->x;

  if (_dragging) {

    if (! (motion_event->state & GDK_MOD1_MASK) ) {
      // Absolute change
      bool constrained = motion_event->state & GDK_CONTROL_MASK;
      set_adjustment_value(x, constrained);
    } else {
      // Relative change
      double xx = (_drag_offset + (x - _drag_start) * 0.1);
      set_adjustment_value(xx);
    }
    return true;
  }

  if (! (motion_event->state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK))) {

    auto display = get_display();
    auto cursor = Gdk::Cursor::create(display, Gdk::SB_UP_ARROW);
    // Get Gdk::window (not Gtk::window).. set cursor for entire window.
    // Would need to unset with leave event.
    // get_window()->set_cursor( cursor );

    // Can't see how to do this the C++ way since GdkEventMotion
    // is a structure with a C window member. There is a gdkmm
    // wrapping function for Gdk::EventMotion but only in unstable.

    // If the cursor theme doesn't have the `sb_up_arrow` cursor then the pointer will be NULL
    if (cursor)
      gdk_window_set_cursor( motion_event->window, cursor->gobj() );
  }

  return false;
}

double
InkScale::get_fraction() {

  Glib::RefPtr<Gtk::Adjustment> adjustment = get_adjustment();
  double upper = adjustment->get_upper();
  double lower = adjustment->get_lower();
  double value = adjustment->get_value();
  double fraction = (value - lower)/(upper - lower);

  return fraction;
}

void
InkScale::set_adjustment_value(double x, bool constrained) {

  Glib::RefPtr<Gtk::Adjustment> adjustment = get_adjustment();
  double upper = adjustment->get_upper();
  double lower = adjustment->get_lower();
  double range = upper-lower;

  Gdk::Rectangle slider_area = get_range_rect();
  double fraction = (x - slider_area.get_x()) / (double)slider_area.get_width();
  double value = fraction * range + lower;
  
  if (constrained) {
    // TODO: do we want preferences for (any of) these?
    if (fmod(range+1,16) == 0) {
        value = round(value/16) * 16;
    } else if (range >= 1000 && fmod(upper,100) == 0) {
        value = round(value/100) * 100;
    } else if (range >= 100 && fmod(upper,10) == 0) {
        value = round(value/10) * 10;
    } else if (range > 20 && fmod(upper,5) == 0) {
        value = round(value/5) * 5;
    } else if (range > 2) {
        value = round(value);
    } else if (range <= 2) {
        value = round(value*10) / 10;
    }
  }

  adjustment->set_value( value );
}

/*******************************************************************/

InkSpinScale::InkSpinScale(double value, double lower,
                           double upper, double step_increment,
                           double page_increment, double page_size)
{
  set_name("InkSpinScale");

  g_assert (upper - lower > 0);

  _adjustment = Gtk::Adjustment::create(value,
                                        lower,
                                        upper,
                                        step_increment,
                                        page_increment,
                                        page_size);

  _spinbutton = Gtk::manage(new Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton>(_adjustment));
  _spinbutton->set_numeric();
  _spinbutton->signal_key_release_event().connect(sigc::mem_fun(*this,&InkSpinScale::on_key_release_event),false);

  _scale       = Gtk::manage(new InkScale(_adjustment, _spinbutton));
  _scale->set_draw_value(false);

  pack_end( *_spinbutton,  Gtk::PACK_SHRINK );
  pack_end( *_scale,       Gtk::PACK_EXPAND_WIDGET );
}

InkSpinScale::InkSpinScale(Glib::RefPtr<Gtk::Adjustment> adjustment)
  : _adjustment(std::move(adjustment))
{
  set_name("InkSpinScale");

  g_assert (_adjustment->get_upper() - _adjustment->get_lower() > 0);

  _spinbutton = Gtk::manage(new Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton>(_adjustment));
  _spinbutton->set_numeric();

  _scale       = Gtk::manage(new InkScale(_adjustment, _spinbutton));
  _scale->set_draw_value(false);

  pack_end( *_spinbutton,  Gtk::PACK_SHRINK );
  pack_end( *_scale,       Gtk::PACK_EXPAND_WIDGET );
}

void
InkSpinScale::set_label(Glib::ustring label) {
  _scale->set_label(label);
}

void
InkSpinScale::set_digits(int digits) {
  _spinbutton->set_digits(digits);
}

int
InkSpinScale::get_digits() const {
  return _spinbutton->get_digits();
}

void
InkSpinScale::set_focus_widget(GtkWidget * focus_widget) {
  _focus_widget = focus_widget;
}

// Return focus to canvas.
bool
InkSpinScale::on_key_release_event(GdkEventKey* key_event) {

  switch (key_event->keyval) {
  case GDK_KEY_Escape:
  case GDK_KEY_Return:
  case GDK_KEY_KP_Enter:
    {
      if (_focus_widget) {
        gtk_widget_grab_focus( _focus_widget );
      }
    }
    break;
  }

  return false;
}

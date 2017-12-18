#ifndef INK_SPINSCALE_H
#define INK_SPINSCALE_H

#include <glibmm/ustring.h>

#include <gtkmm/box.h>
#include <gtkmm/range.h>

namespace Gtk {
  class SpinButton;
}

class InkRange : public Gtk::Range
{
 public:
  InkRange(Glib::RefPtr<Gtk::Adjustment>, Gtk::SpinButton* spinbutton);
  ~InkRange() {};

  void set_label(Glib::ustring label);

  bool on_draw(const::Cairo::RefPtr<::Cairo::Context>& cr) override;

 protected:

  bool on_button_press_event(GdkEventButton* button_event) override;
  bool on_button_release_event(GdkEventButton* button_event) override;
  bool on_motion_notify_event(GdkEventMotion* motion_event) override;

 private:

  double get_fraction();
  void set_adjustment_value(double x);

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

  virtual ~InkSpinScale() {};

  void set_label(Glib::ustring label);
  void set_digits(int digits);
  int  get_digits() const;
  void set_focus_widget(GtkWidget *focus_widget);
  Glib::RefPtr<Gtk::Adjustment> get_adjustment() { return _adjustment; };

 protected:

  InkRange*                      _range;
  Gtk::SpinButton*               _spinbutton;
  Glib::RefPtr<Gtk::Adjustment>  _adjustment;
  GtkWidget*                     _focus_widget;

  bool on_key_release_event(GdkEventKey* key_event) override;

 private:

};

#endif // INK_SPINSCALE_H

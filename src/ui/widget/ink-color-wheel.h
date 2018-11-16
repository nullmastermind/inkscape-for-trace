// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Color wheel widget. Outer ring for Hue. Inner triangle for Saturation and Value.
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#ifndef INK_COLORWHEEL_H
#define INK_COLORWHEEL_H

/* Rewrite of the C Gimp ColorWheel which came originally from GTK2. */

#include <gtkmm.h>
#include <iostream>

namespace Inkscape {
namespace UI {
namespace Widget {
  
class ColorWheel : public Gtk::DrawingArea
{
public:
    ColorWheel();
    void set_rgb(const double& r, const double& g, const double& b, bool override_hue = true);
    void get_rgb(double& r, double& g, double& b);
    guint32 get_rgb();
    bool is_adjusting() {return _mode != DRAG_NONE;}

protected:
    bool on_draw(const::Cairo::RefPtr<::Cairo::Context>& cr) override;

private:
    void triangle_corners(double& x0, double& y0,
                          double& x1, double& y1,
                          double& x2, double& y2);
    void set_from_xy(const double& x, const double& y);
    bool is_in_ring(    const double& x, const double& y);
    bool is_in_triangle(const double& x, const double& y);

    enum DragMode {
        DRAG_NONE,
        DRAG_H,
        DRAG_SV
    };

    double _hue;  // Range [0,1)
    double _saturation;
    double _value;
    double _ring_width;
    DragMode _mode;
    bool   _focus_on_ring;

    // Callbacks
    bool on_focus(Gtk::DirectionType direction) override;
    bool on_button_press_event(GdkEventButton* event) override;
    bool on_button_release_event(GdkEventButton* event) override;
    bool on_motion_notify_event(GdkEventMotion* event) override;
    bool on_key_press_event(GdkEventKey* key_event) override;

    // Signals
public:
    sigc::signal<void> signal_color_changed();

protected:
    sigc::signal<void> _signal_color_changed;

};

} // Namespace Inkscape
}
}
#endif // INK_COLOR_WHEEL_H

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

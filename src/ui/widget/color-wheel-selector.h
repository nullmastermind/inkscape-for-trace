// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Color selector widget containing GIMP color wheel and slider
 */
/* Authors:
 *   Tomasz Boczkowski <penginsbacon@gmail.com> (c++-sification)
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_COLOR_WHEEL_SELECTOR_H
#define SEEN_SP_COLOR_WHEEL_SELECTOR_H

#include <gtkmm/grid.h>

#include "ui/selected-color.h"

namespace Inkscape {
namespace UI {
namespace Widget {

class ColorSlider;
class ColorWheel;
class ColorWheelSelector
    : public Gtk::Grid
{
public:
    static const gchar *MODE_NAME;

    ColorWheelSelector(SelectedColor &color);
    ~ColorWheelSelector() override;

protected:
    void _initUI();

    void on_show() override;

    void _colorChanged();
    void _adjustmentChanged();
    void _sliderGrabbed();
    void _sliderReleased();
    void _sliderChanged();
    void _wheelChanged();

    void _updateDisplay();

    SelectedColor &_color;
    bool _updating;
    Glib::RefPtr<Gtk::Adjustment> _alpha_adjustment;
    Inkscape::UI::Widget::ColorWheel *_wheel;
    Inkscape::UI::Widget::ColorSlider *_slider;

private:
    // By default, disallow copy constructor and assignment operator
    ColorWheelSelector(const ColorWheelSelector &obj) = delete;
    ColorWheelSelector &operator=(const ColorWheelSelector &obj) = delete;

    sigc::connection _color_changed_connection;
    sigc::connection _color_dragged_connection;
};

class ColorWheelSelectorFactory : public ColorSelectorFactory {
public:
    Gtk::Widget *createWidget(SelectedColor &color) const override;
    Glib::ustring modeName() const override;
};
}
}
}

#endif // SEEN_SP_COLOR_WHEEL_SELECTOR_H

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

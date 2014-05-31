/**
 * @file
 * Color selector widget containing GIMP color wheel and slider
 */
/* Authors:
 *   Tomasz Boczkowski <penginsbacon@gmail.com> (c++-sification)
 *
 * Copyright (C) 2014 Authors
 *
 * This code is in public domain
 */
#ifndef SEEN_SP_COLOR_WHEEL_SELECTOR_H
#define SEEN_SP_COLOR_WHEEL_SELECTOR_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if GLIBMM_DISABLE_DEPRECATED && HAVE_GLIBMM_THREADS_H
#include <glibmm/threads.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>
#include <gtkmm/table.h>

#include "ui/selected-color.h"

typedef struct _GimpColorWheel GimpColorWheel;

namespace Inkscape {
namespace UI {
namespace Widget {

class ColorSlider;

class ColorWheelSelector
    : public Gtk::Table      //if GTK2
{
public:
	static const gchar* MODE_NAME;

    ColorWheelSelector(SelectedColor &color);
    virtual ~ColorWheelSelector();

protected:
    void _initUI();
    virtual void _colorChanged();

    static void _adjustmentChanged ( GtkAdjustment *adjustment, ColorWheelSelector *cs );

    void _sliderGrabbed();
    void _sliderReleased();
    void _sliderChanged();
    static void _wheelChanged( GimpColorWheel *wheel, ColorWheelSelector *cs );

    void _recalcColor( gboolean changing );

    SelectedColor &_color;
    bool _updating;

    GtkAdjustment* _adj; // Channel adjustment
    GtkWidget* _wheel;
    Inkscape::UI::Widget::ColorSlider* _slider;
    GtkWidget* _sbtn; // Spinbutton
    GtkWidget* _label; // Label

private:
    // By default, disallow copy constructor and assignment operator
    ColorWheelSelector( const ColorWheelSelector& obj );
    ColorWheelSelector& operator=( const ColorWheelSelector& obj );

    sigc::connection _color_changed_connection;
    sigc::connection _color_dragged_connection;
};

class ColorWheelSelectorFactory: public ColorSelectorFactory {
public:
    Gtk::Widget *createWidget(SelectedColor &color) const;
    Glib::ustring modeName() const;
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

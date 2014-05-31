#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "color-wheel-selector.h"

#include <math.h>
#include <gtk/gtk.h>
#include <glibmm/i18n.h>
#include <gtkmm/adjustment.h>
#include "dialogs/dialog-events.h"
#include "widgets/sp-color-scales.h"
#include "svg/svg-icc-color.h"
#include "ui/selected-color.h"
#include "ui/widget/color-slider.h"
#include "ui/widget/gimpcolorwheel.h"

namespace Inkscape {
namespace UI {
namespace Widget {


#define XPAD 4
#define YPAD 1


const gchar* ColorWheelSelector::MODE_NAME = N_("Wheel");

ColorWheelSelector::ColorWheelSelector(SelectedColor &color)
#if GTK_CHECK_VERSION(3,0,0)
    : Gtk::Grid()
#else
    : Gtk::Table(5, 3, false)
#endif
    , _color(color)
    , _updating(false),
      _adj(0),
      _wheel(0),
      _slider(0),
      _sbtn(0),
      _label(0)
{
    _initUI();
    _color_changed_connection = color.signal_changed.connect(sigc::mem_fun(this, &ColorWheelSelector::_colorChanged));
    _color_dragged_connection = color.signal_dragged.connect(sigc::mem_fun(this, &ColorWheelSelector::_colorChanged));

}

ColorWheelSelector::~ColorWheelSelector()
{
    _adj = 0;
    _wheel = 0;
    _sbtn = 0;
    _label = 0;

    _color_changed_connection.disconnect();
    _color_dragged_connection.disconnect();
}

void ColorWheelSelector::_initUI() {
    gint row = 0;

    GtkWidget *t = GTK_WIDGET(gobj());

    //gtk_widget_show (t);

    /* Create components */
    row = 0;

    _wheel = gimp_color_wheel_new();
    gtk_widget_show( _wheel );

#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(_wheel, GTK_ALIGN_FILL);
    gtk_widget_set_valign(_wheel, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(_wheel, TRUE);
    gtk_widget_set_vexpand(_wheel, TRUE);
    gtk_grid_attach(GTK_GRID(t), _wheel, 0, row, 3, 1);
#else
    gtk_table_attach(GTK_TABLE(t), _wheel, 0, 3, row, row + 1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 0, 0);
#endif

    row++;

    /* Label */
    _label = gtk_label_new_with_mnemonic (_("_A:"));
    gtk_misc_set_alignment (GTK_MISC (_label), 1.0, 0.5);
    gtk_widget_show (_label);

#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_margin_left(_label, XPAD);
    gtk_widget_set_margin_right(_label, XPAD);
    gtk_widget_set_margin_top(_label, YPAD);
    gtk_widget_set_margin_bottom(_label, YPAD);
    gtk_widget_set_halign(_label, GTK_ALIGN_FILL);
    gtk_widget_set_valign(_label, GTK_ALIGN_FILL);
    gtk_grid_attach(GTK_GRID(t), _label, 0, row, 1, 1);
#else
    gtk_table_attach (GTK_TABLE (t), _label, 0, 1, row, row + 1, GTK_FILL, GTK_FILL, XPAD, YPAD);
#endif

    /* Adjustment */
    _adj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 255.0, 1.0, 10.0, 10.0));

    /* Slider */
    _slider = Gtk::manage(new Inkscape::UI::Widget::ColorSlider(Glib::wrap(_adj, true)));
    _slider->set_tooltip_text(_("Alpha (opacity)"));
    _slider->show();

#if GTK_CHECK_VERSION(3,0,0)
    _slider->set_margin_left(XPAD);
    _slider->set_margin_right(XPAD);
    _slider->set_margin_top(YPAD);
    _slider->set_margin_bottom(YPAD);
    _slider->set_hexpand(true);
    _slider->set_halign(Gtk::ALIGN_FILL);
    _slider->set_valign(Gtk::ALIGN_FILL);
    gtk_grid_attach(GTK_GRID(t), _slider->gobj(), 1, row, 1, 1);
#else
    gtk_table_attach(GTK_TABLE (t), _slider->gobj(), 1, 2, row, row + 1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), GTK_FILL, XPAD, YPAD);
#endif

    _slider->setColors(SP_RGBA32_F_COMPOSE (1.0, 1.0, 1.0, 0.0),
                        SP_RGBA32_F_COMPOSE (1.0, 1.0, 1.0, 0.5),
                        SP_RGBA32_F_COMPOSE (1.0, 1.0, 1.0, 1.0));

    /* Spinbutton */
    _sbtn = gtk_spin_button_new (GTK_ADJUSTMENT (_adj), 1.0, 0);
    gtk_widget_set_tooltip_text (_sbtn, _("Alpha (opacity)"));
    sp_dialog_defocus_on_enter (_sbtn);
    gtk_label_set_mnemonic_widget (GTK_LABEL(_label), _sbtn);
    gtk_widget_show (_sbtn);

#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_margin_left(_sbtn, XPAD);
    gtk_widget_set_margin_right(_sbtn, XPAD);
    gtk_widget_set_margin_top(_sbtn, YPAD);
    gtk_widget_set_margin_bottom(_sbtn, YPAD);
    gtk_widget_set_halign(_sbtn, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(_sbtn, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(t), _sbtn, 2, row, 1, 1);
#else
    gtk_table_attach (GTK_TABLE (t), _sbtn, 2, 3, row, row + 1, (GtkAttachOptions)0, (GtkAttachOptions)0, XPAD, YPAD);
#endif

    /* Signals */
    g_signal_connect (G_OBJECT (_adj), "value_changed",
                        G_CALLBACK (_adjustmentChanged), this);

    _slider->signal_grabbed.connect(sigc::mem_fun(*this, &ColorWheelSelector::_sliderGrabbed));
    _slider->signal_released.connect(sigc::mem_fun(*this, &ColorWheelSelector::_sliderReleased));
    _slider->signal_value_changed.connect(sigc::mem_fun(*this, &ColorWheelSelector::_sliderChanged));

    g_signal_connect( G_OBJECT(_wheel), "changed",
                        G_CALLBACK (_wheelChanged), this );
}

void ColorWheelSelector::_colorChanged()
{
#ifdef DUMP_CHANGE_INFO
    g_message("ColorWheelSelector::_colorChanged( this=%p, %f, %f, %f,   %f)", this, color.v.c[0], color.v.c[1], color.v.c[2], alpha );
#endif
    if (_updating) {
        return;
    }

    _updating = true;
    {
        float hsv[3] = {0,0,0};
        sp_color_rgb_to_hsv_floatv(hsv, _color.color().v.c[0], _color.color().v.c[1], _color.color().v.c[2]);
        gimp_color_wheel_set_color( GIMP_COLOR_WHEEL(_wheel), hsv[0], hsv[1], hsv[2] );
    }

    guint32 start = _color.color().toRGBA32( 0x00 );
    guint32 mid = _color.color().toRGBA32( 0x7f );
    guint32 end = _color.color().toRGBA32( 0xff );

    _slider->setColors(start, mid, end);

    ColorScales::setScaled(_adj, _color.alpha());

    _updating = FALSE;
}

void ColorWheelSelector::_adjustmentChanged( GtkAdjustment *adjustment, ColorWheelSelector *cs )
{
    if (cs->_updating) {
        return;
    }

// TODO check this. It looks questionable:
    // if a value is entered between 0 and 1 exclusive, normalize it to (int) 0..255  or 0..100
    gdouble value = gtk_adjustment_get_value (adjustment);
    gdouble upper = gtk_adjustment_get_upper (adjustment);
    
    if (value > 0.0 && value < 1.0) {
        gtk_adjustment_set_value( adjustment, floor (value * upper + 0.5) );
    }

    cs->_updating = true;

    cs->_color.preserveICC();

    cs->_color.setAlpha(ColorScales::getScaled( cs->_adj ));

    cs->_updating = false;
}

void ColorWheelSelector::_sliderGrabbed()
{
    _color.preserveICC();
    _color.setHeld(true);
}

void ColorWheelSelector::_sliderReleased()
{
    _color.preserveICC();
    _color.setHeld(false);
}

void ColorWheelSelector::_sliderChanged()
{
    if (_updating) {
        return;
    }

    _updating = true;
    _color.preserveICC();
    _color.setAlpha(ColorScales::getScaled(_adj));
    _updating = false;
}

void ColorWheelSelector::_wheelChanged( GimpColorWheel *wheel, ColorWheelSelector *wheelSelector )
{
    if (wheelSelector->_updating) return;

    gdouble h = 0;
    gdouble s = 0;
    gdouble v = 0;
    gimp_color_wheel_get_color( wheel, &h, &s, &v );
    
    float rgb[3] = {0,0,0};
    sp_color_hsv_to_rgb_floatv (rgb, h, s, v); 

    SPColor color(rgb[0], rgb[1], rgb[2]);

    guint32 start = color.toRGBA32( 0x00 );
    guint32 mid = color.toRGBA32( 0x7f );
    guint32 end = color.toRGBA32( 0xff );

    wheelSelector->_slider->setColors(start, mid, end);

    wheelSelector->_updating = true;

    wheelSelector->_color.preserveICC();

    wheelSelector->_color.setHeld(gimp_color_wheel_is_adjusting(wheel));
    wheelSelector->_color.setColor(color);

    wheelSelector->_updating = false;
}


Gtk::Widget *ColorWheelSelectorFactory::createWidget(Inkscape::UI::SelectedColor &color) const {
    Gtk::Widget *w = Gtk::manage(new ColorWheelSelector(color));
    return w;
}

Glib::ustring ColorWheelSelectorFactory::modeName() const {
    return gettext(ColorWheelSelector::MODE_NAME);
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

#ifndef __SP_COLOR_SLIDER_H__
#define __SP_COLOR_SLIDER_H__

/* Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 *
 * This code is in public domain
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if GLIBMM_DISABLE_DEPRECATED && HAVE_GLIBMM_THREADS_H
#include <glibmm/threads.h>
#endif

#include <gtkmm/widget.h>

#include <sigc++/signal.h>


/*
 * A slider with colored background
 */
class ColorSlider: public Gtk::Widget {
public:
    //if GTK2
    ColorSlider(Gtk::Adjustment *adjustment);
    ~ColorSlider();

    void set_adjustment(Gtk::Adjustment *adjustment);

    void set_colors(guint32 start, guint32 mid, guint32 end);

    void set_map(const guchar* map);

    void set_background(guint dark, guint light, guint size);

    sigc::signal<void> signal_grabbed;
    sigc::signal<void> signal_dragged;
    sigc::signal<void> signal_released;
    sigc::signal<void> signal_value_changed;

protected:
    void on_size_allocate(Gtk::Allocation& allocation);
    void on_realize();
    void on_unrealize();
    bool on_button_press_event(GdkEventButton *event);
    bool on_button_release_event(GdkEventButton *event);
    bool on_motion_notify_event(GdkEventMotion *event);

    //if GTK2
    void on_size_request(Gtk::Requisition* requisition);
    bool on_expose_event(GdkEventExpose* event);
    //if GTK3
    //request mode, get preffered width/height vfunc
    //endif

    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);

    //TODO: on_adjustment_changed method
    //TODO: on_adjustment value changed method + connection

private:
    bool _dragging;

    Gtk::Adjustment *_adjustment;

    gfloat _value;
    gfloat _oldvalue;
    guchar _c0[4], _cm[4], _c1[4];
    guchar _b0, _b1;
    guchar _bmask;

    gint _mapsize;
    guchar *_map;

    Glib::RefPtr<Gdk::Window> _refGdkWindow;
};





#include <gtk/gtk.h>

#include <glib.h>



struct SPColorSlider;
struct SPColorSliderClass;

#define SP_TYPE_COLOR_SLIDER (sp_color_slider_get_type ())
#define SP_COLOR_SLIDER(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_COLOR_SLIDER, SPColorSlider))
#define SP_COLOR_SLIDER_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), SP_TYPE_COLOR_SLIDER, SPColorSliderClass))
#define SP_IS_COLOR_SLIDER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_COLOR_SLIDER))
#define SP_IS_COLOR_SLIDER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), SP_TYPE_COLOR_SLIDER))

struct SPColorSlider {
	GtkWidget widget;

	guint dragging : 1;

	GtkAdjustment *adjustment;

	gfloat value;
	gfloat oldvalue;
	guchar c0[4], cm[4], c1[4];
	guchar b0, b1;
	guchar bmask;

	gint mapsize;
	guchar *map;
};

struct SPColorSliderClass {
	GtkWidgetClass parent_class;

	void (* grabbed) (SPColorSlider *slider);
	void (* dragged) (SPColorSlider *slider);
	void (* released) (SPColorSlider *slider);
	void (* changed) (SPColorSlider *slider);
};

GType sp_color_slider_get_type (void);

GtkWidget *sp_color_slider_new (GtkAdjustment *adjustment);

void sp_color_slider_set_adjustment (SPColorSlider *slider, GtkAdjustment *adjustment);
void sp_color_slider_set_colors (SPColorSlider *slider, guint32 start, guint32 mid, guint32 end);
void sp_color_slider_set_map (SPColorSlider *slider, const guchar *map);
void sp_color_slider_set_background (SPColorSlider *slider, guint dark, guint light, guint size);



#endif

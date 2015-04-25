/** @file
 * Color selected in color selector widget.
 * This file was created during the refactoring of SPColorSelector
 *//*
 * Authors:
 *	 bulia byak <buliabyak@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Tomasz Boczkowski <penginsbacon@gmail.com>
 *
 * Copyright (C) 2014 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#ifndef SEEN_SELECTED_COLOR
#define SEEN_SELECTED_COLOR

#include <sigc++/signal.h>
#include <gtkmm/widget.h>
#include "color.h"

namespace Inkscape {
namespace UI {

class SelectedColor {
public:
    SelectedColor();
    virtual ~SelectedColor();

    void set_color( const SPColor& color );
    SPColor get_color() const;

    void set_alpha( gfloat alpha );
    gfloat get_alpha() const;

    void set_color_alpha( const SPColor& color, gfloat alpha, bool emit = false );
    void get_color_alpha( SPColor &color, gfloat &alpha ) const;

    sigc::signal<void> signal_changed;
private:
    // By default, disallow copy constructor and assignment operator
    SelectedColor( const SelectedColor& obj );
    SelectedColor& operator=( const SelectedColor& obj );

	SPColor _color;
	/**
	 * Color alpha value guaranteed to be in [0, 1].
	 */
	gfloat _alpha;

    /**
     * This flag is true if no color is set yet
     */
    bool _virgin;

    static double _epsilon;
};


class ColorSelectorFactory {
public:
    virtual ~ColorSelectorFactory() {}

    virtual Gtk::Widget* createWidget(SelectedColor& color) const = 0;
    virtual Glib::ustring modeName() const = 0;
};


}
}

#endif


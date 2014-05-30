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

#include "selected-color.h"

#include <cmath>
#include <glib.h>

namespace Inkscape {
namespace UI {

double SelectedColor::_epsilon = 1e-4;

SelectedColor::SelectedColor()
	: _color(0)
    , _alpha(1.0)
    , _virgin(true)
{

}

SelectedColor::~SelectedColor() {

}

void SelectedColor::set_color(const SPColor& color)
{
    set_color_alpha( color, _alpha );
}

SPColor SelectedColor::get_color() const
{
    return _color;
}

void SelectedColor::set_alpha(gfloat alpha)
{
    g_return_if_fail( ( 0.0 <= alpha ) && ( alpha <= 1.0 ) );
    set_color_alpha( _color, alpha );
}

gfloat SelectedColor::get_alpha() const
{
    return _alpha;
}

void SelectedColor::set_color_alpha(const SPColor& color, gfloat alpha, bool emit)
{
#ifdef DUMP_CHANGE_INFO
    g_message("SelectedColor::setColorAlpha( this=%p, %f, %f, %f, %s,   %f,   %s) in %s", this, color.v.c[0], color.v.c[1], color.v.c[2], (color.icc?color.icc->colorProfile.c_str():"<null>"), alpha, (emit?"YES":"no"), FOO_NAME(_csel));
#endif
    g_return_if_fail( ( 0.0 <= alpha ) && ( alpha <= 1.0 ) );

#ifdef DUMP_CHANGE_INFO
    g_message("---- SelectedColor::setColorAlpha    virgin:%s   !close:%s    alpha is:%s in %s",
              (_virgin?"YES":"no"),
              (!color.isClose( _color, _epsilon )?"YES":"no"),
              ((fabs((_alpha) - (alpha)) >= _epsilon )?"YES":"no"),
              FOO_NAME(_csel)
              );
#endif

    if ( _virgin || !color.isClose( _color, _epsilon ) ||
         (fabs((_alpha) - (alpha)) >= _epsilon )) {

        _virgin = false;

        _color = color;
        _alpha = alpha;

        if (emit) {
        	signal_changed.emit();
        }
#ifdef DUMP_CHANGE_INFO
    } else {
        g_message("++++ SelectedColor::setColorAlpha   color:%08x  ==>  _color:%08X   isClose:%s   in %s", color.toRGBA32(alpha), _color.toRGBA32(_alpha),
                  (color.isClose( _color, _epsilon )?"YES":"no"), FOO_NAME(_csel));
#endif
    }
}

void SelectedColor::get_color_alpha(SPColor &color, gfloat &alpha) const {
	color = _color;
	alpha = _alpha;
}

}
}


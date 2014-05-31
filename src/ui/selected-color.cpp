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

#include "svg/svg-icc-color.h"

namespace Inkscape {
namespace UI {

double const SelectedColor::_EPSILON = 1e-4;

SelectedColor::SelectedColor()
	: _color(0)
    , _alpha(1.0)
    , _held(false)
    , _virgin(true)
{

}

SelectedColor::~SelectedColor() {

}

void SelectedColor::setColor(SPColor const &color)
{
    setColorAlpha( color, _alpha, true);
}

SPColor SelectedColor::color() const
{
    return _color;
}

void SelectedColor::setAlpha(gfloat alpha)
{
    g_return_if_fail( ( 0.0 <= alpha ) && ( alpha <= 1.0 ) );
    setColorAlpha( _color, alpha, true);
}

gfloat SelectedColor::alpha() const
{
    return _alpha;
}

void SelectedColor::setColorAlpha(SPColor const &color, gfloat alpha, bool emit)
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

    if ( _virgin || !color.isClose( _color, _EPSILON ) ||
         (fabs((_alpha) - (alpha)) >= _EPSILON )) {

        _virgin = false;

        _color = color;
        _alpha = alpha;

        if (emit) {
            if (_held) {
                signal_dragged.emit();
            } else {
                signal_changed.emit();
            }
        }
#ifdef DUMP_CHANGE_INFO
    } else {
        g_message("++++ SelectedColor::setColorAlpha   color:%08x  ==>  _color:%08X   isClose:%s   in %s", color.toRGBA32(alpha), _color.toRGBA32(_alpha),
                  (color.isClose( _color, _epsilon )?"YES":"no"), FOO_NAME(_csel));
#endif
    }
}

void SelectedColor::colorAlpha(SPColor &color, gfloat &alpha) const {
	color = _color;
	alpha = _alpha;
}

void SelectedColor::setHeld(bool held) {
    bool grabbed = held && !_held;
    bool released = !held && _held;

    _held = held;

    if (grabbed) {
        signal_grabbed.emit();
    }

    if (released) {
        signal_released.emit();
        signal_changed.emit();
    }
}

void SelectedColor::preserveICC() {
    _color.icc = _color.icc ? new SVGICCColor(*_color.icc) : 0;
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

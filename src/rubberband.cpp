// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Rubberbanding selector.
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "desktop.h"

#include "rubberband.h"

#include "display/curve.h"
#include "display/control/canvas-item-bpath.h"
#include "display/control/canvas-item-rect.h"

#include "ui/widget/canvas.h" // Forced redraws

Inkscape::Rubberband *Inkscape::Rubberband::_instance = nullptr;

Inkscape::Rubberband::Rubberband(SPDesktop *dt)
    : _desktop(dt)
{
    _touchpath_curve = new SPCurve();
}

void Inkscape::Rubberband::delete_canvas_items()
{
    if (_rect) {
        delete _rect;
        _rect = nullptr;
    }

    if (_touchpath) {
        delete _touchpath;
        _touchpath = nullptr;
    }
}


void Inkscape::Rubberband::start(SPDesktop *d, Geom::Point const &p)
{
    _desktop = d;

    _start = p;
    _started = true;

    _touchpath_curve->reset();
    _touchpath_curve->moveto(p);

    _points.clear();
    _points.push_back(_desktop->d2w(p));

    delete_canvas_items();
    _desktop->getCanvas()->forced_redraws_start(5);
}

void Inkscape::Rubberband::stop()
{
    _started = false;
    defaultMode(); // restore the default

    _points.clear();
    _touchpath_curve->reset();

    delete_canvas_items();

    if (_desktop) {
        _desktop->getCanvas()->forced_redraws_stop();
    }
}

void Inkscape::Rubberband::move(Geom::Point const &p)
{
    if (!_started) 
        return;

    _end = p;
    _desktop->scroll_to_point(p);
    _touchpath_curve->lineto(p);

    Geom::Point next = _desktop->d2w(p);
    // we want the points to be at most 0.5 screen pixels apart,
    // so that we don't lose anything small;
    // if they are farther apart, we interpolate more points
    if (!_points.empty() && Geom::L2(next-_points.back()) > 0.5) {
        Geom::Point prev = _points.back();
        int subdiv = 2 * (int) round(Geom::L2(next-prev) + 0.5);
        for (int i = 1; i <= subdiv; i ++) {
            _points.push_back(prev + ((double)i/subdiv) * (next - prev));
        }
    } else {
        _points.push_back(next);
    }

    if (_touchpath) _touchpath->hide();
    if (_rect) _rect->hide();

    switch (_mode) {
        case RUBBERBAND_MODE_RECT:
            if (_rect == nullptr) {
                _rect = new Inkscape::CanvasItemRect(_desktop->getCanvasControls());
                _rect->set_stroke(0x808080ff);
                _rect->set_inverted(true);
            }
            _rect->set_rect(Geom::Rect(_start, _end));
            _rect->show();
            break;
        case RUBBERBAND_MODE_TOUCHRECT:
            if (_rect == nullptr) {
                _rect = new Inkscape::CanvasItemRect(_desktop->getCanvasControls());
                _rect->set_stroke(0xff0000ff);
                _rect->set_inverted(false);
            }
            _rect->set_rect(Geom::Rect(_start, _end));
            _rect->show();
            break;
        case RUBBERBAND_MODE_TOUCHPATH:
            if (_touchpath == nullptr) {
                _touchpath = new Inkscape::CanvasItemBpath(_desktop->getCanvasControls()); // Should be sketch?
                _touchpath->set_stroke(0xff0000ff);
                _touchpath->set_fill(0x0, SP_WIND_RULE_NONZERO);
            }
            _touchpath->set_bpath(_touchpath_curve);
            _touchpath->show();
            break;
        default:
            break;
    }
}

void Inkscape::Rubberband::setMode(int mode) 
{
    _mode = mode;
}
/**
 * Set the default mode (usually rect or touchrect)
 */
void Inkscape::Rubberband::defaultMode()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/tools/select/touch_box", false)) {
        _mode = RUBBERBAND_MODE_TOUCHRECT;
    } else {
        _mode = RUBBERBAND_MODE_RECT;
    }
}

/**
 * @return Rectangle in desktop coordinates
 */
Geom::OptRect Inkscape::Rubberband::getRectangle() const
{
    if (!_started) {
        return Geom::OptRect();
    }

    return Geom::Rect(_start, _end);
}

Inkscape::Rubberband *Inkscape::Rubberband::get(SPDesktop *desktop)
{
    if (_instance == nullptr) {
        _instance = new Inkscape::Rubberband(desktop);
    }

    return _instance;
}

bool Inkscape::Rubberband::is_started()
{
    return _started;
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

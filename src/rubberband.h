// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_RUBBERBAND_H
#define SEEN_RUBBERBAND_H
/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Carl Hetherington <inkscape@carlh.net>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/point.h>
#include <2geom/rect.h>
#include <optional>
#include <vector>

/* fixme: do multidocument safe */

class SPCurve;
class SPDesktop;

enum {
    RUBBERBAND_MODE_RECT,
    RUBBERBAND_MODE_TOUCHPATH,
    RUBBERBAND_MODE_TOUCHRECT
};

namespace Inkscape
{

class CanvasItemBpath;
class CanvasItemRect;

/**
 * Rubberbanding selector.
 */
class Rubberband
{
public:

    void start(SPDesktop *desktop, Geom::Point const &p);
    void move(Geom::Point const &p);
    Geom::OptRect getRectangle() const;
    void stop();
    bool is_started();

    inline int getMode() {return _mode;}
    inline std::vector<Geom::Point> getPoints() {return _points;}

    void setMode(int mode);
    void defaultMode();

    static Rubberband* get(SPDesktop *desktop);

private:

    Rubberband(SPDesktop *desktop);
    static Rubberband* _instance;
    
    SPDesktop *_desktop;
    Geom::Point _start;
    Geom::Point _end;

    std::vector<Geom::Point> _points;

    Inkscape::CanvasItemRect *_rect = nullptr;
    Inkscape::CanvasItemBpath *_touchpath = nullptr;
    SPCurve *_touchpath_curve = nullptr;

    void delete_canvas_items();

    bool _started = false;
    int _mode = RUBBERBAND_MODE_RECT;
};

}

#endif // SEEN_RUBBERBAND_H

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

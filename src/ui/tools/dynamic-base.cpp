// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Common drawing mode. Base class of Eraser and Calligraphic tools.
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/tools/dynamic-base.h"

#include "message-context.h"
#include "desktop.h"

#include "display/curve.h"
#include "display/control/canvas-item-bpath.h"

#include "util/units.h"

using Inkscape::Util::Unit;
using Inkscape::Util::Quantity;
using Inkscape::Util::unit_table;

#define MIN_PRESSURE      0.0
#define MAX_PRESSURE      1.0
#define DEFAULT_PRESSURE  1.0

#define DRAG_MIN 0.0
#define DRAG_DEFAULT 1.0
#define DRAG_MAX 1.0

namespace Inkscape {
namespace UI {
namespace Tools {

DynamicBase::DynamicBase(const std::string& cursor_filename)
    : ToolBase(cursor_filename)
    , accumulated(nullptr)
    , currentshape(nullptr)
    , currentcurve(nullptr)
    , cal1(nullptr)
    , cal2(nullptr)
    , point1()
    , point2()
    , npoints(0)
    , repr(nullptr)
    , cur(0, 0)
    , vel(0, 0)
    , vel_max(0)
    , acc(0, 0)
    , ang(0, 0)
    , last(0, 0)
    , del(0, 0)
    , pressure(DEFAULT_PRESSURE)
    , xtilt(0)
    , ytilt(0)
    , dragging(false)
    , usepressure(false)
    , usetilt(false)
    , mass(0.3)
    , drag(DRAG_DEFAULT)
    , angle(30.0)
    , width(0.2)
    , vel_thin(0.1)
    , flatness(0.9)
    , tremor(0)
    , cap_rounding(0)
    , is_drawing(false)
    , abs_width(false)
{
}

DynamicBase::~DynamicBase() {
    for (auto segment : segments) {
        delete segment;
    }
    segments.clear();

    if (this->currentshape) {
        delete currentshape;
    }
}

void DynamicBase::set(const Inkscape::Preferences::Entry& value) {
    Glib::ustring path = value.getEntryName();
    
    // ignore preset modifications
    static Glib::ustring const presets_path = this->pref_observer->observed_path + "/preset";
    Glib::ustring const &full_path = value.getPath();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Unit const *unit = unit_table.getUnit(prefs->getString("/tools/calligraphic/unit"));

    if (full_path.compare(0, presets_path.size(), presets_path) == 0) {
    	return;
    }

    if (path == "mass") {
        this->mass = 0.01 * CLAMP(value.getInt(10), 0, 100);
    } else if (path == "wiggle") {
        this->drag = CLAMP((1 - 0.01 * value.getInt()), DRAG_MIN, DRAG_MAX); // drag is inverse to wiggle
    } else if (path == "angle") {
        this->angle = CLAMP(value.getDouble(), -90, 90);
    } else if (path == "width") {
        this->width = 0.01 * CLAMP(value.getDouble(), Quantity::convert(0.001, unit, "px"), Quantity::convert(100, unit, "px"));
    } else if (path == "thinning") {
        this->vel_thin = 0.01 * CLAMP(value.getInt(10), -100, 100);
    } else if (path == "tremor") {
        this->tremor = 0.01 * CLAMP(value.getInt(), 0, 100);
    } else if (path == "flatness") {
        this->flatness = 0.01 * CLAMP(value.getInt(), 0, 100);
    } else if (path == "usepressure") {
        this->usepressure = value.getBool();
    } else if (path == "usetilt") {
        this->usetilt = value.getBool();
    } else if (path == "abs_width") {
        this->abs_width = value.getBool();
    } else if (path == "cap_rounding") {
        this->cap_rounding = value.getDouble();
    }
}

/* Get normalized point */
Geom::Point DynamicBase::getNormalizedPoint(Geom::Point v) const {
    auto drect = this->desktop->get_display_area();

    double const max = drect.maxExtent();

    return (v - drect.bounds().min()) / max;
}

/* Get view point */
Geom::Point DynamicBase::getViewPoint(Geom::Point n) const {
    auto drect = this->desktop->get_display_area();

    double const max = drect.maxExtent();

    return n * max + drect.bounds().min();
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

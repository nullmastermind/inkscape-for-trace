// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * PencilTool: a context for pencil tool events
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_PENCIL_CONTEXT_H
#define SEEN_PENCIL_CONTEXT_H



#include "ui/tools/freehand-base.h"

#include <2geom/piecewise.h>
#include <2geom/d2.h>
#include <2geom/sbasis.h>
#include <2geom/pathvector.h>

class SPShape;

#define DDC_MIN_PRESSURE      0.0
#define DDC_MAX_PRESSURE      1.0
#define DDC_DEFAULT_PRESSURE  1.0
#define SP_PENCIL_CONTEXT(obj) (dynamic_cast<Inkscape::UI::Tools::PencilTool*>((Inkscape::UI::Tools::ToolBase*)obj))
#define SP_IS_PENCIL_CONTEXT(obj) (dynamic_cast<const Inkscape::UI::Tools::PencilTool*>((const Inkscape::UI::Tools::ToolBase*)obj) != NULL)

namespace Inkscape {
namespace UI {
namespace Tools {

enum PencilState {
    SP_PENCIL_CONTEXT_IDLE,
    SP_PENCIL_CONTEXT_ADDLINE,
    SP_PENCIL_CONTEXT_FREEHAND,
    SP_PENCIL_CONTEXT_SKETCH
};

/**
 * PencilTool: a context for pencil tool events
 */
class PencilTool : public FreehandBase {
public:
    PencilTool();
    ~PencilTool() override;
    Geom::Point p[16];
    std::vector<Geom::Point> ps;
    std::vector<Geom::Point> points;
    void addPowerStrokePencil(SPCurve *& c);
    void addPowerStrokePencil();
    void removePowerStrokePreview();
    Geom::Piecewise<Geom::D2<Geom::SBasis> > sketch_interpolation; // the current proposal from the sketched paths
    unsigned sketch_n; // number of sketches done
    static const std::string prefsPath;
    const std::string& getPrefsPath() override;

protected:

    void setup() override;
    bool root_handler(GdkEvent* event) override;

private:
    bool _handleButtonPress(GdkEventButton const &bevent);
    bool _handleMotionNotify(GdkEventMotion const &mevent);
    bool _handleButtonRelease(GdkEventButton const &revent);
    bool _handleKeyPress(GdkEventKey const &event);
    bool _handleKeyRelease(GdkEventKey const &event);
    void _setStartpoint(Geom::Point const &p);
    void _powerStrokePreview(Geom::Path const path);
    void _setEndpoint(Geom::Point const &p);
    void _finishEndpoint();
    void _addFreehandPoint(Geom::Point const &p, guint state);
    void _fitAndSplit();
    void _interpolate();
    void _powerstrokeInterpolate(bool apply);
    void _sketchInterpolate();
    void _extinput(GdkEvent *event);
    void _cancel();
    void _endpointSnap(Geom::Point &p, guint const state);
    std::vector<double> _wps;
    std::vector<Geom::Point> _points_pos;
    std::vector<Geom::Point> _key_nodes;
    Geom::Point _last_point;
    double _previous_pressure;
    SPCurve * _curve;
    SPShape *_powerpreview;
    Geom::Point _req_tangent;
    bool _is_drawing;
    PencilState _state;
    gint _npoints;
};

}
}
}

#endif /* !SEEN_PENCIL_CONTEXT_H */

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

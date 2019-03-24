// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LPE_PTS_TO_ELLIPSE_H
#define INKSCAPE_LPE_PTS_TO_ELLIPSE_H

/** \file
 * LPE "Points to Ellipse" implementation
 */

/*
 * Authors:
 *   Markus Schwienbacher
 *
 * Copyright (C) Markus Schwienbacher 2013 <mschwienbacher@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/effect.h"
#include "live_effects/parameter/bool.h"
#include "live_effects/parameter/enum.h"

#include <gsl/gsl_linalg.h>


// struct gsl_vector;
// struct gsl_permutation;

namespace Inkscape {
namespace LivePathEffect {

enum EllipseMethod {
    EM_AUTO,
    EM_CIRCLE,
    EM_ISOMETRIC_CIRCLE,
    EM_PERSPECTIVE_CIRCLE,
    EM_STEINER_ELLIPSE,
    EM_STEINER_INELLIPSE,
    EM_END
};

class LPEPts2Ellipse : public Effect {
  public:
    LPEPts2Ellipse(LivePathEffectObject *lpeobject);
    ~LPEPts2Ellipse() override;

    Geom::PathVector doEffect_path(Geom::PathVector const &path_in) override;

  private:
    LPEPts2Ellipse(const LPEPts2Ellipse &) = delete;
    LPEPts2Ellipse &operator=(const LPEPts2Ellipse &) = delete;


    int genIsometricEllipse(std::vector<Geom::Point> const &points_in, Geom::PathVector &path_out);

    int genFitEllipse(std::vector<Geom::Point> const &points_in, Geom::PathVector &path_out);

    int genSteinerEllipse(std::vector<Geom::Point> const &points_in, bool gen_inellipse, Geom::PathVector &path_out);

    int genPerspectiveEllipse(std::vector<Geom::Point> const &points_in, Geom::PathVector &path_out);

    // utility functions
    static int unit_arc_path(Geom::Path &path_in, Geom::Affine &affine, double start = 0.0,
                             double end = 2.0 * M_PI, // angles
                             bool slice = false);
    static void gen_iso_frame_paths(Geom::PathVector &path_out, const Geom::Affine &affine);
    static void gen_perspective_frame_paths(Geom::PathVector &path_out, const double rot_angle,
                                            double projmatrix[3][3]);
    static void gen_axes_paths(Geom::PathVector &path_out, const Geom::Affine &affine);
    static void gen_perspective_axes_paths(Geom::PathVector &path_out, const double rot_angle, double projmatrix[3][3]);
    static bool is_ccw(const std::vector<Geom::Point> &pts);
    static Geom::Point projectPoint(Geom::Point p, double m[][3]);

    // GUI parameters
    EnumParam<EllipseMethod> method;
    BoolParam gen_isometric_frame;
    BoolParam gen_perspective_frame;
    BoolParam gen_arc;
    BoolParam other_arc;
    BoolParam slice_arc;
    BoolParam draw_axes;
    BoolParam draw_perspective_axes;
    ScalarParam rot_axes;
    BoolParam draw_ori_path;

    // collect the points from the input paths
    std::vector<Geom::Point> points;

    // used for solving perspective circle
    gsl_vector *gsl_x;
    gsl_permutation *gsl_p;
    std::vector<Geom::Point> five_pts;
};

} // namespace LivePathEffect
} // namespace Inkscape

#endif

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

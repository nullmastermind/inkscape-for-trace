// SPDX-License-Identifier: GPL-2.0-or-later
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

#include "lpe-pts2ellipse.h"


#include <object/sp-item-group.h>
#include <object/sp-item.h>
#include <object/sp-path.h>
#include <object/sp-shape.h>
#include <svg/svg.h>

#include <2geom/circle.h>
#include <2geom/ellipse.h>
#include <2geom/elliptical-arc.h>
#include <2geom/path.h>
#include <2geom/pathvector.h>

#include <glib/gi18n.h>

namespace Inkscape {
namespace LivePathEffect {

static const Util::EnumData<EllipseMethod> EllipseMethodData[] = {
    { EM_AUTO, N_("Auto ellipse"), "auto" },                       //!< (2..4 points: circle, from 5 points: ellipse)
    { EM_CIRCLE, N_("Force circle"), "circle" },                   //!< always fit a circle
    { EM_ISOMETRIC_CIRCLE, N_("Isometric circle"), "iso_circle" }, //!< use first two edges to generate a sheared
                                                                   //!< ellipse
    { EM_PERSPECTIVE_CIRCLE, N_("Perspective circle"), "perspective_circle" }, //!< use first three edges to generate an
                                                                               //!< ellipse representing a distorted
                                                                               //!< circle in perspective
    { EM_STEINER_ELLIPSE, N_("Steiner ellipse"), "steiner_ellipse" }, //!< generate a steiner ellipse from the first
                                                                      //!< three points
    { EM_STEINER_INELLIPSE, N_("Steiner inellipse"), "steiner_inellipse" } //!< generate a steiner inellipse from the
                                                                           //!< first three points
};
static const Util::EnumDataConverter<EllipseMethod> EMConverter(EllipseMethodData, EM_END);

LPEPts2Ellipse::LPEPts2Ellipse(LivePathEffectObject *lpeobject)
    : Effect(lpeobject)
    , method(
          _("Method:"),
          _("Methods to generate the ellipse\n- Auto ellipse: fits a circle (2..4 points) or an ellipse (at least 5 "
            "points)\n- Force circle: (at least 2 points) always fit to a circle\n- Isometric circle: (3 points) use "
            "first two edges\n- Perspective circle: (4 points) circle in a square in perspective view\n- Steiner "
            "ellipse: (3 points) ellipse on a triangle\n- Steiner inellipse: (3 points) ellipse inside a triangle"),
          "method", EMConverter, &wr, this, EM_AUTO)
    , gen_isometric_frame(_("_Frame (isometric rectangle)"), _("Draw parallelogram around the ellipse"),
                          "gen_isometric_frame", &wr, this, false)
    , gen_perspective_frame(
          _("_Perspective square"),
          _("Draw square surrounding the circle in perspective view\n(only in method \"Perspective circle\")"),
          "gen_perspective_frame", &wr, this, false)
    , gen_arc(_("_Arc"),
              _("Generate open arc (open ellipse) based on first and last point\n(only for methods \"Auto ellipse\" "
                "and \"Force circle\")"),
              "gen_arc", &wr, this, false)
    , other_arc(_("_Other arc side"), _("Switch sides of the arc"), "arc_other", &wr, this, false)
    , slice_arc(_("_Slice arc"), _("Slice the arc"), "slice_arc", &wr, this, false)
    , draw_axes(_("A_xes"), _("Draw both semi-major and semi-minor axes"), "draw_axes", &wr, this, false)
    , draw_perspective_axes(_("Perspective axes"),
                            _("Draw the axes in perspective view\n(only in method \"Perspective circle\")"),
                            "draw_perspective_axes", &wr, this, false)
    , rot_axes(_("Axes rotation"), _("Axes rotation angle [deg]"), "rot_axes", &wr, this, 0)
    , draw_ori_path(_("Source _path"), _("Show the original source path"), "draw_ori_path", &wr, this, false)
{
    registerParameter(&method);
    registerParameter(&gen_arc);
    registerParameter(&other_arc);
    registerParameter(&slice_arc);
    registerParameter(&gen_isometric_frame);
    registerParameter(&draw_axes);
    registerParameter(&gen_perspective_frame);
    registerParameter(&draw_perspective_axes);
    registerParameter(&rot_axes);
    registerParameter(&draw_ori_path);

    rot_axes.param_set_range(-360, 360);
    rot_axes.param_set_increments(1, 10);

    show_orig_path = true;

    gsl_x = gsl_vector_alloc(8);
    gsl_p = gsl_permutation_alloc(8);
}

LPEPts2Ellipse::~LPEPts2Ellipse()
{
    gsl_permutation_free(gsl_p);
    gsl_vector_free(gsl_x);
}

// helper function, transforms a given value into range [0, 2pi]
inline double range2pi(double a)
{
    a = fmod(a, 2 * M_PI);
    if (a < 0) {
        a += 2 * M_PI;
    }
    return a;
}

inline double deg2rad(double a) { return a * M_PI / 180.0; }

inline double rad2deg(double a) { return a * 180.0 / M_PI; }

// helper function, calculates the angle between a0 and a1 in ccw sense
// examples: 0..1->1, -1..1->2, pi/4..-pi/4->1.5pi
// full rotations: 0..2pi->2pi, -pi..pi->2pi, pi..-pi->0, 2pi..0->0
inline double calc_delta_angle(const double a0, const double a1)
{
    double da = range2pi(a1 - a0);
    if ((fabs(da) < 1e-9) && (a0 < a1)) {
        da = 2 * M_PI;
    }
    return da;
}

int LPEPts2Ellipse::unit_arc_path(Geom::Path &path_in, Geom::Affine &affine, double start, double end, bool slice)
{
    double arc_angle = calc_delta_angle(start, end);
    if (fabs(arc_angle) < 1e-9) {
        g_warning("angle was 0");
        return -1;
    }

    // the delta angle
    double da = M_PI_2;
    // number of segments with da length
    int nda = (int)ceil(arc_angle / M_PI_2);
    // recalculate da
    da = arc_angle / (double)nda;

    bool closed = false;
    if (fabs(arc_angle - 2 * M_PI) < 1e-8) {
        closed = true;
        da = M_PI_2;
        nda = 4;
    }

    double s = range2pi(start);
    end = s + arc_angle;

    double x0 = cos(s);
    double y0 = sin(s);
    // construct the path
    Geom::Path path(Geom::Point(x0, y0));
    path.setStitching(true);
    for (int i = 0; i < nda;) {
        double e = s + da;
        if (e > end) {
            e = end;
        }
        const double len = 4 * tan((e - s) / 4) / 3;
        const double x1 = x0 + len * cos(s + M_PI_2);
        const double y1 = y0 + len * sin(s + M_PI_2);
        const double x3 = cos(e);
        const double y3 = sin(e);
        const double x2 = x3 + len * cos(e - M_PI_2);
        const double y2 = y3 + len * sin(e - M_PI_2);
        path.appendNew<Geom::CubicBezier>(Geom::Point(x1, y1), Geom::Point(x2, y2), Geom::Point(x3, y3));
        s = (++i) * da + start;
        x0 = cos(s);
        y0 = sin(s);
    }

    if (slice && !closed) {
        path.appendNew<Geom::LineSegment>(Geom::Point(0.0, 0.0));
    }
    path *= affine;

    path_in.append(path);
    if ((slice && !closed) || closed) {
        path_in.close(true);
    }
    return 0;
}

void LPEPts2Ellipse::gen_iso_frame_paths(Geom::PathVector &path_out, const Geom::Affine &affine)
{
    Geom::Path rect(Geom::Point(-1, -1));
    rect.setStitching(true);
    rect.appendNew<Geom::LineSegment>(Geom::Point(+1, -1));
    rect.appendNew<Geom::LineSegment>(Geom::Point(+1, +1));
    rect.appendNew<Geom::LineSegment>(Geom::Point(-1, +1));
    rect *= affine;
    rect.close(true);
    path_out.push_back(rect);
}

void LPEPts2Ellipse::gen_perspective_frame_paths(Geom::PathVector &path_out, const double rot_angle,
                                                 double projmatrix[3][3])
{
    Geom::Point pts0[4] = { { -1.0, -1.0 }, { +1.0, -1.0 }, { +1.0, +1.0 }, { -1.0, +1.0 } };
    // five_pts.resize(4);
    int h = 0;
    Geom::Affine affine2;
    // const double rot_angle = deg2rad(rot_axes); // negative for ccw rotation
    affine2 *= Geom::Rotate(-rot_angle);
    for (auto &i : pts0) {
        Geom::Point point = i;
        point *= affine2;
        i = projectPoint(point, projmatrix);
    }

    Geom::Path rect(pts0[0]);
    rect.setStitching(true);
    for (int i = 1; i < 4; i++)
        rect.appendNew<Geom::LineSegment>(pts0[i]);
    rect.close(true);
    path_out.push_back(rect);
}

void LPEPts2Ellipse::gen_axes_paths(Geom::PathVector &path_out, const Geom::Affine &affine)
{
    Geom::LineSegment clx(Geom::Point(-1, 0), Geom::Point(1, 0));
    Geom::LineSegment cly(Geom::Point(0, -1), Geom::Point(0, 1));

    Geom::Path plx, ply;
    plx.append(clx);
    ply.append(cly);
    plx *= affine;
    ply *= affine;

    path_out.push_back(plx);
    path_out.push_back(ply);
}

void LPEPts2Ellipse::gen_perspective_axes_paths(Geom::PathVector &path_out, const double rot_angle,
                                                double projmatrix[3][3])
{
    Geom::Point pts[4];
    int h = 0;
    double dA = 2.0 * M_PI / 4.0; // delta Angle
    for (auto &i : pts) {
        const double angle = rot_angle + dA * h++;
        const Geom::Point circle_point(sin(angle), cos(angle));
        i = projectPoint(circle_point, projmatrix);
    }
    {
        Geom::LineSegment clx(pts[0], pts[2]);
        Geom::LineSegment cly(pts[1], pts[3]);

        Geom::Path plx, ply;
        plx.append(clx);
        ply.append(cly);

        path_out.push_back(plx);
        path_out.push_back(ply);
    }
}

bool LPEPts2Ellipse::is_ccw(const std::vector<Geom::Point> &pts)
{
    // method: sum up the angles between edges
    size_t n = pts.size();
    // edges about vertex 0
    Geom::Point e0(pts.front() - pts.back());
    Geom::Point e1(pts[1] - pts[0]);
    Geom::Coord sum = cross(e0, e1);
    // the rest
    for (size_t i = 1; i < n; i++) {
        e0 = e1;
        e1 = pts[i] - pts[i - 1];
        sum += cross(e0, e1);
    }
    // edges about last vertex (closing)
    e0 = e1;
    e1 = pts.front() - pts.back();
    sum += cross(e0, e1);

    // close the
    if (sum < 0) {
        return true;
    } else {
        return false;
    }
}

void endpoints2angles(const bool ccw_wind, const bool use_other_arc, const Geom::Point &p0, const Geom::Point &p1,
                      Geom::Coord &a0, Geom::Coord &a1)
{
    if (!p0.isZero() && !p1.isZero()) {
        a0 = atan2(p0);
        a1 = atan2(p1);
        if (!ccw_wind) {
            std::swap(a0, a1);
        }
        if (!use_other_arc) {
            std::swap(a0, a1);
        }
    }
}

/**
 * Generates an ellipse (or circle) from the vertices of a given path.  Thereby, using fitting
 * algorithms from 2geom. Depending on the settings made by the user regarding things like arc,
 * slice, circle etc. the final result will be different
 */
Geom::PathVector LPEPts2Ellipse::doEffect_path(Geom::PathVector const &path_in)
{
    Geom::PathVector path_out;

    // 1) draw original path?
    if (draw_ori_path.get_value()) {
        path_out.insert(path_out.end(), path_in.begin(), path_in.end());
    }


    // 2) get all points
    // (from: extension/internal/odf.cpp)
    points.resize(0);
    for (const auto &pit : path_in) {
        // extract first point of this path
        points.push_back(pit.initialPoint());
        // iterate over all curves
        for (const auto &cit : pit) {
            points.push_back(cit.finalPoint());
        }
    }
    // avoid identical start-point and end-point
    if (points.front() == points.back()) {
        points.pop_back();
    }

    // 3) modify GUI based on selected method
    // 3.1) arc options
    switch (method) {
        case EM_AUTO:
        case EM_CIRCLE:
            gen_arc.param_widget_is_enabled(true);
            if (gen_arc.get_value()) {
                slice_arc.param_widget_is_enabled(true);
                other_arc.param_widget_is_enabled(true);
            } else {
                other_arc.param_widget_is_enabled(false);
                slice_arc.param_widget_is_enabled(false);
            }
            break;
        default:
            gen_arc.param_widget_is_enabled(false);
            other_arc.param_widget_is_enabled(false);
            slice_arc.param_widget_is_enabled(false);
    }
    // 3.2) perspective options
    switch (method) {
        case EM_PERSPECTIVE_CIRCLE:
            gen_perspective_frame.param_widget_is_enabled(true);
            draw_perspective_axes.param_widget_is_enabled(true);
            break;
        default:
            gen_perspective_frame.param_widget_is_enabled(false);
            draw_perspective_axes.param_widget_is_enabled(false);
    }

    // 4) call method specific code
    switch (method) {
        case EM_ISOMETRIC_CIRCLE:
            // special mode: Use first two edges, interpret them as two sides of a parallelogram and
            // generate an ellipse residing inside the parallelogram. This effect is quite useful when
            // generating isometric views. Hence, the name.
            if (0 != genIsometricEllipse(points, path_out)) {
                return path_in;
            }
            break;
        case EM_PERSPECTIVE_CIRCLE:
            // special mode: Use first four points, interpret them as the perspective representation of a square and
            // draw the ellipse as it was a circle inside that square.
            if (0 != genPerspectiveEllipse(points, path_out)) {
                return path_in;
            }
            break;
        case EM_STEINER_ELLIPSE:
            if (0 != genSteinerEllipse(points, false, path_out)) {
                return path_in;
            }
            break;
        case EM_STEINER_INELLIPSE:
            if (0 != genSteinerEllipse(points, true, path_out)) {
                return path_in;
            }
            break;
        default:
            if (0 != genFitEllipse(points, path_out)) {
                return path_in;
            }
    }
    return path_out;
}

/**
 * Generates an ellipse (or circle) from the vertices of a given path. Thereby, using fitting
 * algorithms from 2geom. Depending on the settings made by the user regarding things like arc,
 * slice, circle etc. the final result will be different. We need at least 5 points to fit an
 * ellipse. With 5 points each point is on the ellipse. For less points we get a circle.
 */
int LPEPts2Ellipse::genFitEllipse(std::vector<Geom::Point> const &pts, Geom::PathVector &path_out)
{
    // rotation angle based on user provided rot_axes to position the vertices
    const double rot_angle = -deg2rad(rot_axes); // negative for ccw rotation
    Geom::Affine affine;
    affine *= Geom::Rotate(rot_angle);
    Geom::Coord a0 = 0;
    Geom::Coord a1 = 2 * M_PI;

    if (pts.size() < 2) {
        return -1;
    } else if (pts.size() == 2) {
        // simple line: circle in the middle of the line to the vertices
        Geom::Point line = pts.front() - pts.back();
        double radius = line.length() * 0.5;
        if (radius < 1e-9) {
            return -1;
        }
        Geom::Point center = middle_point(pts.front(), pts.back());
        Geom::Circle circle(center[0], center[1], radius);
        affine *= Geom::Scale(circle.radius());
        affine *= Geom::Translate(circle.center());
        Geom::Path path;
        unit_arc_path(path, affine);
        path_out.push_back(path);
    } else if (pts.size() >= 5 && EM_AUTO == method) {
        // do ellipse
        try {
            Geom::Ellipse ellipse;
            ellipse.fit(pts);
            affine *= Geom::Scale(ellipse.ray(Geom::X), ellipse.ray(Geom::Y));
            affine *= Geom::Rotate(ellipse.rotationAngle());
            affine *= Geom::Translate(ellipse.center());
            if (gen_arc.get_value()) {
                Geom::Affine inv_affine = affine.inverse();
                Geom::Point p0 = pts.front() * inv_affine;
                Geom::Point p1 = pts.back() * inv_affine;
                const bool ccw_wind = is_ccw(pts);
                endpoints2angles(ccw_wind, other_arc.get_value(), p0, p1, a0, a1);
            }
            Geom::Path path;
            unit_arc_path(path, affine, a0, a1, slice_arc.get_value());
            path_out.push_back(path);
        } catch (...) {
            return -1;
        }
    } else {
        // do a circle (3,4 points, or only_circle set)
        try {
            Geom::Circle circle;
            circle.fit(pts);
            affine *= Geom::Scale(circle.radius());
            affine *= Geom::Translate(circle.center());
            if (gen_arc.get_value()) {
                Geom::Point p0 = pts.front() - circle.center();
                Geom::Point p1 = pts.back() - circle.center();
                const bool ccw_wind = is_ccw(pts);
                endpoints2angles(ccw_wind, other_arc.get_value(), p0, p1, a0, a1);
            }
            Geom::Path path;
            unit_arc_path(path, affine, a0, a1, slice_arc.get_value());
            path_out.push_back(path);
        } catch (...) {
            return -1;
        }
    }

    // draw frame?
    if (gen_isometric_frame.get_value()) {
        gen_iso_frame_paths(path_out, affine);
    }

    // draw axes?
    if (draw_axes.get_value()) {
        gen_axes_paths(path_out, affine);
    }

    return 0;
}

int LPEPts2Ellipse::genIsometricEllipse(std::vector<Geom::Point> const &pts, Geom::PathVector &path_out)

{
    // take the first 3 vertices for the edges
    if (pts.size() < 3) {
        return -1;
    }
    // calc edges
    Geom::Point e0 = pts[0] - pts[1];
    Geom::Point e1 = pts[2] - pts[1];

    Geom::Coord ce = cross(e0, e1);
    // parallel or one is zero?
    if (fabs(ce) < 1e-9) {
        return -1;
    }
    // unit vectors along edges
    Geom::Point u0 = unit_vector(e0);
    Geom::Point u1 = unit_vector(e1);
    // calc angles
    Geom::Coord a0 = atan2(e0);
    // Coord a1=M_PI_2-atan2(e1)-a0;
    Geom::Coord a1 = acos(dot(u0, u1)) - M_PI_2;
    // if(fabs(a1)<1e-9) return -1;
    if (ce < 0) {
        a1 = -a1;
    }
    // lengths: l0= length of edge 0; l1= height of parallelogram
    Geom::Coord l0 = e0.length() * 0.5;
    Geom::Point e0n = e1 - dot(u0, e1) * u0;
    Geom::Coord l1 = e0n.length() * 0.5;

    // center of the ellipse
    Geom::Point pos = pts[1] + 0.5 * (e0 + e1);

    // rotation angle based on user provided rot_axes to position the vertices
    const double rot_angle = -deg2rad(rot_axes); // negative for ccw rotation

    // build up the affine transformation
    Geom::Affine affine;
    affine *= Geom::Rotate(rot_angle);
    affine *= Geom::Scale(l0, l1);
    affine *= Geom::HShear(-tan(a1));
    affine *= Geom::Rotate(a0);
    affine *= Geom::Translate(pos);

    Geom::Path path;
    unit_arc_path(path, affine);
    path_out.push_back(path);

    // draw frame?
    if (gen_isometric_frame.get_value()) {
        gen_iso_frame_paths(path_out, affine);
    }

    // draw axes?
    if (draw_axes.get_value()) {
        gen_axes_paths(path_out, affine);
    }

    return 0;
}

void evalSteinerEllipse(Geom::Point const &pCenter, Geom::Point const &pCenter_Pt2, Geom::Point const &pPt0_Pt1,
                        const double &angle, Geom::Point &pRes)
{
    // formula for the evaluation of points on the steiner ellipse using parameter angle
    pRes = pCenter + pCenter_Pt2 * cos(angle) + pPt0_Pt1 * sin(angle) / sqrt(3);
}

int LPEPts2Ellipse::genSteinerEllipse(std::vector<Geom::Point> const &pts, bool gen_inellipse,
                                      Geom::PathVector &path_out)
{
    // take the first 3 vertices for the edges
    if (pts.size() < 3) {
        return -1;
    }
    // calc center
    Geom::Point pCenter = (pts[0] + pts[1] + pts[2]) / 3;
    // calc main directions of affine triangle
    Geom::Point f1 = pts[2] - pCenter;
    Geom::Point f2 = (pts[1] - pts[0]) / sqrt(3);

    // calc zero angle t0
    const double denominator = dot(f1, f1) - dot(f2, f2);
    double t0 = 0;
    if (fabs(denominator) > 1e-12) {
        const double cot2t0 = 2.0 * dot(f1, f2) / denominator;
        t0 = atan(cot2t0) / 2.0;
    }

    // calc relative points of main axes (for axis directions)
    Geom::Point p0(0, 0), pRel0, pRel1;
    evalSteinerEllipse(p0, pts[2] - pCenter, pts[1] - pts[0], t0, pRel0);
    evalSteinerEllipse(p0, pts[2] - pCenter, pts[1] - pts[0], t0 + M_PI_2, pRel1);
    Geom::Coord l0 = pRel0.length();
    Geom::Coord l1 = pRel1.length();

    // basic rotation
    double a0 = atan2(pRel0);

    bool swapped = false;

    if (l1 > l0) {
        std::swap(l0, l1);
        a0 += M_PI_2;
        swapped = true;
    }

    // the Steiner inellipse is just scaled down by 2
    if (gen_inellipse) {
        l0 /= 2;
        l1 /= 2;
    }

    // rotation angle based on user provided rot_axes to position the vertices
    const double rot_angle = -deg2rad(rot_axes); // negative for ccw rotation

    // build up the affine transformation
    Geom::Affine affine;
    affine *= Geom::Rotate(rot_angle);
    affine *= Geom::Scale(l0, l1);
    affine *= Geom::Rotate(a0);
    affine *= Geom::Translate(pCenter);

    Geom::Path path;
    unit_arc_path(path, affine);
    path_out.push_back(path);

    // draw frame?
    if (gen_isometric_frame.get_value()) {
        gen_iso_frame_paths(path_out, affine);
    }

    // draw axes?
    if (draw_axes.get_value()) {
        gen_axes_paths(path_out, affine);
    }

    return 0;
}

// identical to lpe-perspective-envelope.cpp
Geom::Point LPEPts2Ellipse::projectPoint(Geom::Point p, double m[][3])
{
    Geom::Coord x = p[0];
    Geom::Coord y = p[1];
    return Geom::Point(Geom::Coord((x * m[0][0] + y * m[0][1] + m[0][2]) / (x * m[2][0] + y * m[2][1] + m[2][2])),
                       Geom::Coord((x * m[1][0] + y * m[1][1] + m[1][2]) / (x * m[2][0] + y * m[2][1] + m[2][2])));
}

int LPEPts2Ellipse::genPerspectiveEllipse(std::vector<Geom::Point> const &pts, Geom::PathVector &path_out)
{
    using Geom::X;
    using Geom::Y;
    // we need at least four points!
    if (pts.size() < 4)
        return -1;

    // 1) check if the first three edges are a valid perspective
    // calc edge
    Geom::Point e[] = { pts[0] - pts[1], pts[1] - pts[2], pts[2] - pts[3], pts[3] - pts[0] };
    // calc directions
    Geom::Coord c[] = { cross(e[0], e[1]), cross(e[1], e[2]), cross(e[2], e[3]), cross(e[3], e[0]) };
    // is this quad not convex?
    if (!((c[0] > 0 && c[1] > 0 && c[2] > 0 && c[3] > 0) || (c[0] < 0 && c[1] < 0 && c[2] < 0 && c[3] < 0)))
        return -1;

    // 2) solve the direct linear transformation (see e.g. lpe-perspective-envelope.cpp or
    // https://franklinta.com/2014/09/08/computing-css-matrix3d-transforms/)

    // the square points in the initial configuration (about the unit circle):
    Geom::Point pts0[4] = { { -1.0, -1.0 }, { +1.0, -1.0 }, { +1.0, +1.0 }, { -1.0, +1.0 } };

    // build equation in matrix form
    double eqnVec[8] = { 0 };
    double eqnMat[64] = { 0 };
    for (unsigned int i = 0; i < 4; ++i) {
        eqnMat[8 * (i + 0) + 0] = pts0[i][X];
        eqnMat[8 * (i + 0) + 1] = pts0[i][Y];
        eqnMat[8 * (i + 0) + 2] = 1;
        eqnMat[8 * (i + 0) + 6] = -pts[i][X] * pts0[i][X];
        eqnMat[8 * (i + 0) + 7] = -pts[i][X] * pts0[i][Y];
        eqnMat[8 * (i + 4) + 3] = pts0[i][X];
        eqnMat[8 * (i + 4) + 4] = pts0[i][Y];
        eqnMat[8 * (i + 4) + 5] = 1;
        eqnMat[8 * (i + 4) + 6] = -pts[i][Y] * pts0[i][X];
        eqnMat[8 * (i + 4) + 7] = -pts[i][Y] * pts0[i][Y];
        eqnVec[i] = pts[i][X];
        eqnVec[i + 4] = pts[i][Y];
    }
    // solve using gsl library
    gsl_matrix_view m = gsl_matrix_view_array(eqnMat, 8, 8);
    gsl_vector_view b = gsl_vector_view_array(eqnVec, 8);
    int s = 0;
    gsl_linalg_LU_decomp(&m.matrix, gsl_p, &s);
    gsl_linalg_LU_solve(&m.matrix, gsl_p, &b.vector, gsl_x);
    // transfer the solution to the projection matrix for further use
    size_t h = 0;
    double projmatrix[3][3];
    for (auto &matRow : projmatrix) {
        for (double &matElement : matRow) {
            if (h == 8) {
                projmatrix[2][2] = 1.0;
            } else {
                matElement = gsl_vector_get(gsl_x, h++);
            }
        }
    }

    // 3) generate five points on a unit circle and project them
    five_pts.resize(5); // reuse and avoid new/delete
    h = 0;
    double dA = 2.0 * M_PI / 5.0; // delta Angle
    for (auto &i : five_pts) {
        const double angle = dA * h++;
        const Geom::Point circle_point(sin(angle), cos(angle));
        i = projectPoint(circle_point, projmatrix);
    }

    // 4) fit the five points to an ellipse with the already known function inside genFitEllipse() function
    // build up the affine transformation
    const double rot_angle = -deg2rad(rot_axes); // negative for ccw rotation
    Geom::Affine affine;
    affine *= Geom::Rotate(rot_angle);

    try {
        Geom::Ellipse ellipse;
        ellipse.fit(five_pts);
        affine *= Geom::Scale(ellipse.ray(Geom::X), ellipse.ray(Geom::Y));
        affine *= Geom::Rotate(ellipse.rotationAngle());
        affine *= Geom::Translate(ellipse.center());
    } catch (...) {
        return -1;
    }

    Geom::Path path;
    unit_arc_path(path, affine);
    path_out.push_back(path);

    // 5) frames and axes
    bool ccw_wind = false;
    if (gen_perspective_frame.get_value() || draw_perspective_axes.get_value())
        ccw_wind = is_ccw(pts);
    const double ra = ccw_wind ? rot_angle : -rot_angle;

    // draw frame?
    if (gen_isometric_frame.get_value()) {
        gen_iso_frame_paths(path_out, affine);
    }

    // draw perspective frame?
    if (gen_perspective_frame.get_value()) {
        gen_perspective_frame_paths(path_out, ra, projmatrix);
    }

    // draw axes?
    if (draw_axes.get_value()) {
        gen_axes_paths(path_out, affine);
    }

    // draw perspective axes?
    if (draw_perspective_axes.get_value()) {
        gen_perspective_axes_paths(path_out, ra, projmatrix);
    }

    return 0;
}


/* ######################## */

} // namespace LivePathEffect
} /* namespace Inkscape */

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

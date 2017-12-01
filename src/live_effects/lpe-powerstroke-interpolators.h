/** @file
 * Interpolators for lists of points.
 */
/* Authors:
 *   Johan Engelen <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Copyright (C) 2010-2011 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef INKSCAPE_LPE_POWERSTROKE_INTERPOLATORS_H
#define INKSCAPE_LPE_POWERSTROKE_INTERPOLATORS_H

#include <2geom/path.h>
#include <2geom/bezier-utils.h>
#include <2geom/sbasis-to-bezier.h>
#include <math.h>
#include "live_effects/spiro.h"


/// @TODO  Move this to 2geom?
namespace Geom {
namespace Interpolate {

enum InterpolatorType {
  INTERP_LINEAR,
  INTERP_CUBICBEZIER,
  INTERP_CUBICBEZIER_JOHAN,
  INTERP_SPIRO,
  INTERP_CUBICBEZIER_SMOOTH,
  INTERP_CENTRIPETAL_CATMULLROM,
  INTERP_BSPLINE
};

class Interpolator {
public:
    Interpolator() {};
    virtual ~Interpolator() {};

    static Interpolator* create(InterpolatorType type);

    virtual Geom::Path interpolateToPath(std::vector<Point> const &points) const = 0;

private:
    Interpolator(const Interpolator&);
    Interpolator& operator=(const Interpolator&);
};

class Linear : public Interpolator {
public:
    Linear() {};
    virtual ~Linear() {};

    virtual Path interpolateToPath(std::vector<Point> const &points) const {
        Path path;
        path.start( points.at(0) );
        for (unsigned int i = 1 ; i < points.size(); ++i) {
            path.appendNew<Geom::LineSegment>(points.at(i));
        }
        return path;
    };

private:
    Linear(const Linear&);
    Linear& operator=(const Linear&);
};

class BSpline : public Interpolator {
public:
    BSpline() {};
    virtual ~BSpline() {};

    virtual Path interpolateToPath(std::vector<Point> const &points) const {
        Path path;
        path.start( points.at(0) );

        Geom::Point point_a = points.at(0);
        Geom::Point point_b = points.at(1);
        Geom::Point point_c = points.at(2);
        Geom::Point point_d = points.at(2);
        if (points.size() > 3) {
            point_d = points.at(3);
        }
        Geom::Point handle_1 = point_a;
        Geom::Point handle_2 = point_b;
        Geom::Point node_1 = point_a;
        Geom::Point node_2 = point_b; //if the line is straight with next
        Geom::Line line_ab(point_a, point_b);
        Geom::Line line_bc(point_b, point_c);
        Geom::Line line_cd(point_c, point_d);
        Geom::Point handle_next(line_bc.pointAt(0.33334)); //if the line is straight with next
        Geom::Point center_ab = line_ab.pointAt(0.5);
        Geom::Point center_bc = line_bc.pointAt(0.5);
        Geom::Point center_cd = line_cd.pointAt(0.5);
        Geom::Line cross_line_hlp(line_ab.pointAt(0.66667), center_cd);
        Geom::Point cross_center_hlp = cross_line_hlp.pointAt(0.5);
        double angle_ab = line_ab.angle();
        double angle_bc = line_bc.angle();
        double angle_cd = line_cd.angle();
        double cross_angle_hlp = cross_line_hlp.angle();
        Geom::Line perpend_ab(center_ab, center_ab + Geom::Point::polar(angle_ab + Geom::rad_from_deg(90),1));
        Geom::Line perpend_bc(center_bc, center_bc + Geom::Point::polar(angle_bc + Geom::rad_from_deg(90),1));
        Geom::Line cross_1   (cross_center_hlp, cross_center_hlp + Geom::Point::polar(cross_angle_hlp + Geom::rad_from_deg(90),1));
        std::vector<CurveIntersection> cross_hlp = perpend_ab.intersect(perpend_bc);
        if(cross_hlp.size() > 0) {
            cross_line_hlp.setPoints(cross_hlp[0], line_ab.pointAt(0.66667));
            cross_angle_hlp = cross_line_hlp.angle();
            Geom::Line cross_2(line_ab.pointAt(0.66667), line_ab.pointAt(0.66667) + Geom::Point::polar(cross_angle_hlp + Geom::rad_from_deg(90),1));
            std::vector<CurveIntersection> cross = cross_1.intersect(cross_2);
            if(cross.size() > 0) {
                Geom::Line handle_line(cross[0],point_b);
                handle_2 = handle_line.pointAt(2.0);
                Geom::Line rootline(points.at(0),handle_2);
                handle_1 =  rootline.pointAt(0.5);                
                node_2 = rootline.pointAt(1.5);
                handle_next = cross[0];
            }
        }
        path.appendNew<CubicBezier>(handle_1, handle_2, points.at(1));
        for (unsigned int i = 2 ; i < points.size(); ++i) {
            Geom::Line rootline(node_2, handle_next);
            handle_2 = rootline.pointAt(2.0);
            node_2 = rootline.pointAt(3.0);
            path.appendNew<CubicBezier>(handle_next, handle_2, points.at(i));
            Geom::Line handleline(handle_2, points.at(i));
            handle_next = handleline.pointAt(2.0);
        }
        Geom::Point last = points.at(points.size()-1);
        Geom::LineSegment last_segment(handle_next,last);
        path.appendNew<CubicBezier>(last_segment.initialPoint(), last_segment.pointAt(0.5), last);
        return path;
    };

private:
    BSpline(const BSpline&);
    BSpline& operator=(const BSpline&);
};

// this class is terrible
class CubicBezierFit : public Interpolator {
public:
    CubicBezierFit() {};
    virtual ~CubicBezierFit() {};

    virtual Path interpolateToPath(std::vector<Point> const &points) const {
        unsigned int n_points = points.size();
        // worst case gives us 2 segment per point
        int max_segs = 8*n_points;
        Geom::Point * b = g_new(Geom::Point, max_segs);
        Geom::Point * points_array = g_new(Geom::Point, 4*n_points);
        for (unsigned i = 0; i < n_points; ++i) {
            points_array[i] = points.at(i);
        }

        double tolerance_sq = 0; // this value is just a random guess

        int const n_segs = Geom::bezier_fit_cubic_r(b, points_array, n_points,
                                                 tolerance_sq, max_segs);

        Geom::Path fit;
        if ( n_segs > 0)
        {
            fit.start(b[0]);
            for (int c = 0; c < n_segs; c++) {
                fit.appendNew<Geom::CubicBezier>(b[4*c+1], b[4*c+2], b[4*c+3]);
            }
        }
        g_free(b);
        g_free(points_array);
        return fit;
    };

private:
    CubicBezierFit(const CubicBezierFit&);
    CubicBezierFit& operator=(const CubicBezierFit&);
};

/// @todo invent name for this class
class CubicBezierJohan : public Interpolator {
public:
    CubicBezierJohan(double beta = 0.2) {
        _beta = beta;
    };
    virtual ~CubicBezierJohan() {};

    virtual Path interpolateToPath(std::vector<Point> const &points) const {
        Path fit;
        fit.start(points.at(0));
        for (unsigned int i = 1; i < points.size(); ++i) {
            Point p0 = points.at(i-1);
            Point p1 = points.at(i);
            Point dx = Point(p1[X] - p0[X], 0);
            fit.appendNew<CubicBezier>(p0+_beta*dx, p1-_beta*dx, p1);
        }
        return fit;
    };

    void setBeta(double beta) {
        _beta = beta;
    }

    double _beta;

private:
    CubicBezierJohan(const CubicBezierJohan&);
    CubicBezierJohan& operator=(const CubicBezierJohan&);
};

/// @todo invent name for this class
class CubicBezierSmooth : public Interpolator {
public:
    CubicBezierSmooth(double beta = 0.2) {
        _beta = beta;
    };
    virtual ~CubicBezierSmooth() {};

    virtual Path interpolateToPath(std::vector<Point> const &points) const {
        Path fit;
        fit.start(points.at(0));
        unsigned int num_points = points.size();
        for (unsigned int i = 1; i < num_points; ++i) {
            Point p0 = points.at(i-1);
            Point p1 = points.at(i);
            Point dx = Point(p1[X] - p0[X], 0);
            if (i == 1) {
                fit.appendNew<CubicBezier>(p0, p1-0.75*dx, p1);
            } else if (i == points.size() - 1) {
                fit.appendNew<CubicBezier>(p0+0.75*dx, p1, p1);
            } else {
                fit.appendNew<CubicBezier>(p0+_beta*dx, p1-_beta*dx, p1);
            }
        }
        return fit;
    };

    void setBeta(double beta) {
        _beta = beta;
    }

    double _beta;

private:
    CubicBezierSmooth(const CubicBezierSmooth&);
    CubicBezierSmooth& operator=(const CubicBezierSmooth&);
};

class SpiroInterpolator : public Interpolator {
public:
    SpiroInterpolator() {};
    virtual ~SpiroInterpolator() {};

    virtual Path interpolateToPath(std::vector<Point> const &points) const {
        Path fit;

        Coord scale_y = 100.;

        guint len = points.size();
        Spiro::spiro_cp *controlpoints = g_new (Spiro::spiro_cp, len);
        for (unsigned int i = 0; i < len; ++i) {
            controlpoints[i].x = points[i][X];
            controlpoints[i].y = points[i][Y] / scale_y;
            controlpoints[i].ty = 'c';
        }
        controlpoints[0].ty = '{';
        controlpoints[1].ty = 'v';
        controlpoints[len-2].ty = 'v';
        controlpoints[len-1].ty = '}';

        Spiro::spiro_run(controlpoints, len, fit);

        fit *= Scale(1,scale_y);
        return fit;
    };

private:
    SpiroInterpolator(const SpiroInterpolator&);
    SpiroInterpolator& operator=(const SpiroInterpolator&);
};

// Quick mockup for testing the behavior for powerstroke controlpoint interpolation
class CentripetalCatmullRomInterpolator : public Interpolator {
public:
    CentripetalCatmullRomInterpolator() {};
    virtual ~CentripetalCatmullRomInterpolator() {};

    virtual Path interpolateToPath(std::vector<Point> const &points) const {
        unsigned int n_points = points.size();

        Geom::Path fit(points.front());

        if (n_points < 3) return fit; // TODO special cases for 0,1 and 2 input points

        // return n_points-1 cubic segments

        // duplicate first point
        fit.append(calc_bezier(points[0],points[0],points[1],points[2]));

        for (std::size_t i = 0; i < n_points-2; ++i) {
            Point p0 = points[i];
            Point p1 = points[i+1];
            Point p2 = points[i+2];
            Point p3 = (i < n_points-3) ? points[i+3] : points[i+2];

            fit.append(calc_bezier(p0, p1, p2, p3));
        }

        return fit;
    };

private:
    CubicBezier calc_bezier(Point p0, Point p1, Point p2, Point p3) const {
        // create interpolating bezier between p1 and p2

        // Part of the code comes from StackOverflow user eriatarka84
        // http://stackoverflow.com/a/23980479/2929337

        // calculate time coords (deltas) of points
        // the factor 0.25 can be generalized for other Catmull-Rom interpolation types 
        // see alpha in Yuksel et al. "On the Parameterization of Catmull-Rom Curves", 
        // --> http://www.cemyuksel.com/research/catmullrom_param/catmullrom.pdf
        double dt0 = powf(distanceSq(p0, p1), 0.25);
        double dt1 = powf(distanceSq(p1, p2), 0.25);
        double dt2 = powf(distanceSq(p2, p3), 0.25);


        // safety check for repeated points
        double eps = Geom::EPSILON;
        if (dt1 < eps)
            dt1 = 1.0;
        if (dt0 < eps)
            dt0 = dt1;
        if (dt2 < eps)
            dt2 = dt1;

        // compute tangents when parameterized in [t1,t2]
        Point tan1 = (p1 - p0) / dt0 - (p2 - p0) / (dt0 + dt1) + (p2 - p1) / dt1;
        Point tan2 = (p2 - p1) / dt1 - (p3 - p1) / (dt1 + dt2) + (p3 - p2) / dt2;
        // rescale tangents for parametrization in [0,1]
        tan1 *= dt1;
        tan2 *= dt1;

        // create bezier from tangents (this is already in 2geom somewhere, or should be moved to it)
        // the tangent of a bezier curve is: B'(t) = 3(1-t)^2 (b1 - b0) + 6(1-t)t(b2-b1) + 3t^2(b3-b2)
        // So we have to make sure that B'(0) = tan1  and  B'(1) = tan2, and we already know that b0=p1 and b3=p2
        // tan1 = B'(0) = 3 (b1 - p1)  --> p1 + (tan1)/3 = b1
        // tan2 = B'(1) = 3 (p2 - b2)  --> p2 - (tan2)/3 = b2

        Point b0 = p1;
        Point b1 = p1 + tan1 / 3;
        Point b2 = p2 - tan2 / 3;
        Point b3 = p2;

        return CubicBezier(b0, b1, b2, b3);
    }

    CentripetalCatmullRomInterpolator(const CentripetalCatmullRomInterpolator&);
    CentripetalCatmullRomInterpolator& operator=(const CentripetalCatmullRomInterpolator&);
};


inline Interpolator*
Interpolator::create(InterpolatorType type) {
    switch (type) {
      case INTERP_LINEAR:
        return new Geom::Interpolate::Linear();
      case INTERP_CUBICBEZIER:
        return new Geom::Interpolate::CubicBezierFit();
      case INTERP_CUBICBEZIER_JOHAN:
        return new Geom::Interpolate::CubicBezierJohan();
      case INTERP_SPIRO:
        return new Geom::Interpolate::SpiroInterpolator();
      case INTERP_CUBICBEZIER_SMOOTH:
        return new Geom::Interpolate::CubicBezierSmooth();
      case INTERP_CENTRIPETAL_CATMULLROM:
        return new Geom::Interpolate::CentripetalCatmullRomInterpolator();
      case INTERP_BSPLINE:
        return new Geom::Interpolate::BSpline();
      default:
        return new Geom::Interpolate::Linear();
    }
}

} //namespace Interpolate
} //namespace Geom

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

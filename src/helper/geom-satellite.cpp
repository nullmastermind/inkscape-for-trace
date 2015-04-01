/**
 * \file
 * \brief Satellite a per ?node/curve holder of data.
 */ /*
    * Authors:
    * 2015 Jabier Arraiza Cenoz<jabier.arraiza@marker.es>
    *
    * This code is in public domain
    */

#include <helper/geom-satellite.h>
#include <2geom/curve.h>
#include <2geom/nearest-point.h>
#include <2geom/sbasis-geometric.h>
#include <2geom/path-intersection.h>
#include <2geom/sbasis-to-bezier.h>
#include <2geom/ray.h>
#include <boost/optional.hpp>

namespace Geom {

/**
 * @brief Satellite a per ?node/curve holder of data.
 */
Satellite::Satellite() {}
;

Satellite::Satellite(SatelliteType satelliteType, bool isTime, bool active,
                     bool hasMirror, bool hidden, double amount, double angle,
                     size_t steps)
    : satelliteType(satelliteType), isTime(isTime), active(active),
      hasMirror(hasMirror), hidden(hidden), amount(amount), angle(angle),
      steps(steps) {}
;

Satellite::~Satellite() {}
;

/**
 * Calculate the time in d2_in with a size of A
 */
double Satellite::toTime(double A, Geom::D2<Geom::SBasis> d2_in) const
{
    if (!d2_in.isFinite() || d2_in.isZero() || A == 0) {
        return 0;
    }
    double t = 0;
    double lenghtPart = Geom::length(d2_in, Geom::EPSILON);
    if (A > lenghtPart || d2_in[0].degreesOfFreedom() == 2) {
        if (lenghtPart != 0) {
            t = A / lenghtPart;
        }
    } else if (d2_in[0].degreesOfFreedom() != 2) {
        Geom::Piecewise<Geom::D2<Geom::SBasis> > u;
        u.push_cut(0);
        u.push(d2_in, 1);
        std::vector<double> t_roots = roots(arcLengthSb(u) - A);
        if (t_roots.size() > 0) {
            t = t_roots[0];
        }
    }

    return t;
}

/**
 * Calculate the size in d2_in with a point at A
 */
double Satellite::toSize(double A, Geom::D2<Geom::SBasis> d2_in) const
{
    if (!d2_in.isFinite() || d2_in.isZero() || A == 0) {
        return 0;
    }
    double s = 0;
    double lenghtPart = Geom::length(d2_in, Geom::EPSILON);
    if (A > lenghtPart || d2_in[0].degreesOfFreedom() == 2) {
        s = (A * lenghtPart);
    } else if (d2_in[0].degreesOfFreedom() != 2) {
        Geom::Piecewise<Geom::D2<Geom::SBasis> > u;
        u.push_cut(0);
        u.push(d2_in, 1);
        u = Geom::portion(u, 0.0, A);
        s = Geom::length(u, 0.001);
    }
    return s;
}

/**
 * Calculate the lenght of a satellite from a radious A input.
 */
double Satellite::radToLen(
    double A, boost::optional<Geom::D2<Geom::SBasis> > d2_in,
    Geom::D2<Geom::SBasis> d2_out,
    boost::optional<Geom::Satellite> previousSatellite) const
{
    double len = 0;
    if (d2_in && previousSatellite) {
        Piecewise<D2<SBasis> > offset_curve0 =
            Piecewise<D2<SBasis> >(*d2_in) +
            rot90(unitVector(derivative(*d2_in))) * (A);
        Piecewise<D2<SBasis> > offset_curve1 =
            Piecewise<D2<SBasis> >(d2_out) +
            rot90(unitVector(derivative(d2_out))) * (A);
        Geom::Path p0 = path_from_piecewise(offset_curve0, 0.1)[0];
        Geom::Path p1 = path_from_piecewise(offset_curve1, 0.1)[0];
        Geom::Crossings cs = Geom::crossings(p0, p1);
        if (cs.size() > 0) {
            Point cp = p0(cs[0].ta);
            double p0pt = nearest_point(cp, d2_out);
            len = (*previousSatellite).toSize(p0pt, d2_out);
        } else {
            if (A > 0) {
                len = radToLen(A * -1, *d2_in, d2_out, previousSatellite);
            }
        }
    }
    return len;
}

/**
* Calculate the radious of a satellite from a lenght A input.
*/
double Satellite::lenToRad(
    double A, boost::optional<Geom::D2<Geom::SBasis> > d2_in,
    Geom::D2<Geom::SBasis> d2_out,
    boost::optional<Geom::Satellite> previousSatellite) const
{
    if (d2_in && previousSatellite) {
        double time_in = (*previousSatellite).time(A, true, *d2_in);
        double time_out = (*previousSatellite).toTime(A, d2_out);
        Geom::Point startArcPoint = (*d2_in).valueAt(time_in);
        Geom::Point endArcPoint = d2_out.valueAt(time_out);
        Piecewise<D2<SBasis> > u;
        u.push_cut(0);
        u.push(*d2_in, 1);
        Geom::Curve *C = path_from_piecewise(u, 0.1)[0][0].duplicate();
        Piecewise<D2<SBasis> > u2;
        u2.push_cut(0);
        u2.push(d2_out, 1);
        Geom::Curve *D = path_from_piecewise(u2, 0.1)[0][0].duplicate();
        Curve *knotCurve1 = C->portion(0, time_in);
        Curve *knotCurve2 = D->portion(time_out, 1);
        Geom::CubicBezier const *cubic1 =
            dynamic_cast<Geom::CubicBezier const *>(&*knotCurve1);
        Ray ray1(startArcPoint, (*d2_in).valueAt(1));
        if (cubic1) {
            ray1.setPoints((*cubic1)[2], startArcPoint);
        }
        Geom::CubicBezier const *cubic2 =
            dynamic_cast<Geom::CubicBezier const *>(&*knotCurve2);
        Ray ray2(d2_out.valueAt(0), endArcPoint);
        if (cubic2) {
            ray2.setPoints(endArcPoint, (*cubic2)[1]);
        }
        bool ccwToggle = cross((*d2_in).valueAt(1) - startArcPoint,
                               endArcPoint - startArcPoint) < 0;
        double distanceArc =
            Geom::distance(startArcPoint, middle_point(startArcPoint, endArcPoint));
        double angleBetween = angle_between(ray1, ray2, ccwToggle);
        double divisor = std::sin(angleBetween / 2.0);
        if (divisor > 0) {
            return distanceArc / divisor;
        }
    }
    return 0;
}

/**
 * Get the time position of the satellite in d2_in
 */
double Satellite::time(Geom::D2<Geom::SBasis> d2_in) const
{
    double t = amount;
    if (!isTime) {
        t = toTime(t, d2_in);
    }
    if (t > 1) {
        t = 1;
    }
    return t;
}

/**.
 * Get the time from a lenght A in other curve, a bolean I gived to reverse time
 */
double Satellite::time(double A, bool I,
                       Geom::D2<Geom::SBasis> d2_in) const
{
    if (A == 0 && I) {
        return 1;
    }
    if (A == 0 && !I) {
        return 0;
    }
    if (!I) {
        return toTime(A, d2_in);
    }
    double lenghtPart = Geom::length(d2_in, Geom::EPSILON);
    A = lenghtPart - A;
    return toTime(A, d2_in);
}

/**
 * Get the lenght of the satellite in d2_in
 */
double Satellite::size(Geom::D2<Geom::SBasis> d2_in) const
{
    double s = amount;
    if (isTime) {
        s = toSize(s, d2_in);
    }
    return s;
}

/**
 * Get the point position of the satellite
 */
Geom::Point Satellite::getPosition(Geom::D2<Geom::SBasis> d2_in) const
{
    double t = time(d2_in);
    return d2_in.valueAt(t);
}

/**
 * Set the position of the satellite from a gived point P
 */
void Satellite::setPosition(Geom::Point p, Geom::D2<Geom::SBasis> d2_in)
{
    double A = Geom::nearest_point(p, d2_in);
    if (!isTime) {
        A = toSize(A, d2_in);
    }
    amount = A;
}

/**
 * Map a satellite type with gchar
 */
void Satellite::setSatelliteType(gchar const *A)
{
    std::map<std::string, SatelliteType> GcharMapToSatelliteType =
        boost::assign::map_list_of("F", F)("IF", IF)("C", C)("IC", IC)("KO", KO);
    satelliteType = GcharMapToSatelliteType.find(std::string(A))->second;
}

/**
 * Map a gchar with satelliteType
 */
gchar const *Satellite::getSatelliteTypeGchar() const
{
    std::map<SatelliteType, gchar const *> SatelliteTypeToGcharMap =
        boost::assign::map_list_of(F, "F")(IF, "IF")(C, "C")(IC, "IC")(KO, "KO");
    return SatelliteTypeToGcharMap.at(satelliteType);
}

} //namespace Geom

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:
// filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99
// :

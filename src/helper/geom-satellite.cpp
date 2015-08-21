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
#include <2geom/nearest-time.h>
#include <2geom/path-intersection.h>
#include <2geom/sbasis-to-bezier.h>
#include <2geom/ray.h>
#include <boost/optional.hpp>

/**
 * @brief Satellite a per ?node/curve holder of data.
 */
Satellite::Satellite() {}


Satellite::Satellite(SatelliteType satellite_type)
    : satellite_type(satellite_type),
    is_time(false),
    active(false),
    has_mirror(false),
    hidden(true),
    amount(0.0),
    angle(0.0),
    steps(0)
{}

Satellite::~Satellite() {}

/**
 * Calculate the time in d2_in with a size of A
 * TODO: find a better place to it
 */
double timeAtArcLength(double A, Geom::D2<Geom::SBasis> const d2_in)
{
    if (!d2_in.isFinite() || d2_in.isZero() || A == 0) {
        return 0;
    }
    double t = 0;
    double length_part = Geom::length(d2_in, Geom::EPSILON);
    if (A >= length_part || d2_in[0].degreesOfFreedom() == 2) {
        if (length_part != 0) {
            t = A / length_part;
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
 * TODO: find a better place to it
 */
double arcLengthAt(double A, Geom::D2<Geom::SBasis> const d2_in)
{
    if (!d2_in.isFinite() || d2_in.isZero() || A == 0) {
        return 0;
    }
    double s = 0;
    double length_part = Geom::length(d2_in, Geom::EPSILON);
    if (A > length_part || d2_in[0].degreesOfFreedom() == 2) {
        s = (A * length_part);
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
 * Convert a arc radius of a fillet/chamfer to his satellite length -point position where fillet/chamfer knot be on original curve
 */
double Satellite::radToLen(
    double A, Geom::D2<Geom::SBasis> const d2_in,
    Geom::D2<Geom::SBasis> const d2_out) const
{
    double len = 0;
    Geom::Piecewise<Geom::D2<Geom::SBasis> > offset_curve0 =
        Geom::Piecewise<Geom::D2<Geom::SBasis> >(d2_in) +
        rot90(unitVector(derivative(d2_in))) * (A);
    Geom::Piecewise<Geom::D2<Geom::SBasis> > offset_curve1 =
        Geom::Piecewise<Geom::D2<Geom::SBasis> >(d2_out) +
        rot90(unitVector(derivative(d2_out))) * (A);
    Geom::Path p0 = path_from_piecewise(offset_curve0, 0.1)[0];
    Geom::Path p1 = path_from_piecewise(offset_curve1, 0.1)[0];
    Geom::Crossings cs = Geom::crossings(p0, p1);
    if (cs.size() > 0) {
        Geom::Point cp = p0(cs[0].ta);
        double p0pt = nearest_time(cp, d2_out);
        len = arcLengthAt(p0pt, d2_out);
    } else {
        if (A > 0) {
            len = radToLen(A * -1, d2_in, d2_out);
        }
    }
    return len;
}

/**
* Convert a satelite length -point position where fillet/chamfer knot be on original curve- to a arc radius of fillet/chamfer
*/
double Satellite::lenToRad(
    double A, Geom::D2<Geom::SBasis> const d2_in,
    Geom::D2<Geom::SBasis> const d2_out,
    Satellite const previousSatellite) const
{
    double time_in = (previousSatellite).time(A, true, d2_in);
    double time_out = timeAtArcLength(A, d2_out);
    Geom::Point startArcPoint = (d2_in).valueAt(time_in);
    Geom::Point endArcPoint = d2_out.valueAt(time_out);
    Geom::Piecewise<Geom::D2<Geom::SBasis> > u;
    u.push_cut(0);
    u.push(d2_in, 1);
    Geom::Curve *C = path_from_piecewise(u, 0.1)[0][0].duplicate();
    Geom::Piecewise<Geom::D2<Geom::SBasis> > u2;
    u2.push_cut(0);
    u2.push(d2_out, 1);
    Geom::Curve *D = path_from_piecewise(u2, 0.1)[0][0].duplicate();
    Geom::Curve *knotCurve1 = C->portion(0, time_in);
    Geom::Curve *knotCurve2 = D->portion(time_out, 1);
    Geom::CubicBezier const *cubic1 =
        dynamic_cast<Geom::CubicBezier const *>(&*knotCurve1);
    Geom::Ray ray1(startArcPoint, (d2_in).valueAt(1));
    if (cubic1) {
        ray1.setPoints((*cubic1)[2], startArcPoint);
    }
    Geom::CubicBezier const *cubic2 =
        dynamic_cast<Geom::CubicBezier const *>(&*knotCurve2);
    Geom::Ray ray2(d2_out.valueAt(0), endArcPoint);
    if (cubic2) {
        ray2.setPoints(endArcPoint, (*cubic2)[1]);
    }
    bool ccwToggle = cross((d2_in).valueAt(1) - startArcPoint,
                           endArcPoint - startArcPoint) < 0;
    double distanceArc =
        Geom::distance(startArcPoint, middle_point(startArcPoint, endArcPoint));
    double angleBetween = angle_between(ray1, ray2, ccwToggle);
    double divisor = std::sin(angleBetween / 2.0);
    if (divisor > 0) {
        return distanceArc / divisor;
    }
    return 0;
}

/**
 * Get the time position of the satellite in d2_in
 */
double Satellite::time(Geom::D2<Geom::SBasis> d2_in) const
{
    double t = amount;
    if (!is_time) {
        t = timeAtArcLength(t, d2_in);
    }
    if (t > 1) {
        t = 1;
    }
    return t;
}

/**.
 * Get the time from a length A in other curve, a bolean I gived to reverse time
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
        return timeAtArcLength(A, d2_in);
    }
    double length_part = Geom::length(d2_in, Geom::EPSILON);
    A = length_part - A;
    return timeAtArcLength(A, d2_in);
}

/**
 * Get the length of the satellite in d2_in
 */
double Satellite::arcDistance(Geom::D2<Geom::SBasis> d2_in) const
{
    double s = amount;
    if (is_time) {
        s = arcLengthAt(s, d2_in);
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
    double A = Geom::nearest_time(p, d2_in);
    if (!is_time) {
        A = arcLengthAt(A, d2_in);
    }
    amount = A;
}

/**
 * Map a satellite type with gchar
 */
void Satellite::setSatelliteType(gchar const *A)
{
    std::map<std::string, SatelliteType> gchar_map_to_satellite_type =
        boost::assign::map_list_of("F", FILLET)("IF", INVERSE_FILLET)("C", CHAMFER)("IC", INVERSE_CHAMFER)("KO", INVALID_SATELLITE);
    std::map<std::string, SatelliteType>::iterator it = gchar_map_to_satellite_type.find(std::string(A));
    if(it != gchar_map_to_satellite_type.end()) {
        satellite_type = it->second;
    }
}

/**
 * Map a gchar with satelliteType
 */
gchar const *Satellite::getSatelliteTypeGchar() const
{
    std::map<SatelliteType, gchar const *> satellite_type_to_gchar_map =
        boost::assign::map_list_of(FILLET, "F")(INVERSE_FILLET, "IF")(CHAMFER, "C")(INVERSE_CHAMFER, "IC")(INVALID_SATELLITE, "KO");
    return satellite_type_to_gchar_map.at(satellite_type);
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
// vim:
// filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99
// :

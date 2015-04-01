/**
 * \file
 * \brief Satellite a per ?node/curve holder of data.
 */ /*
    * Authors:
    * 2015 Jabier Arraiza Cenoz<jabier.arraiza@marker.es>
    *
    * This code is in public domain
    */

#ifndef LIB2GEOM_SEEN_SATELLITE_H
#define LIB2GEOM_SEEN_SATELLITE_H

#include <helper/geom-satellite-enum.h>
#include <2geom/d2.h>
#include <map>
#include <boost/assign.hpp>

namespace Geom {

/**
 * @brief Satellite a per ?node/curve holder of data.
 */
class Satellite {
public:

    Satellite();
    Satellite(SatelliteType satelliteType, bool isTime, bool active,
              bool hasMirror, bool hidden, double amount, double angle,
              size_t steps);

    virtual ~Satellite();

    double toSize(double A, Geom::D2<Geom::SBasis> d2_in) const;
    double toTime(double A, Geom::D2<Geom::SBasis> d2_in) const;
    double lenToRad(double A, boost::optional<Geom::D2<Geom::SBasis> > d2_in,
                    Geom::D2<Geom::SBasis> d2_out,
                    boost::optional<Geom::Satellite> previousSatellite) const;
    double radToLen(double A, boost::optional<Geom::D2<Geom::SBasis> > d2_in,
                    Geom::D2<Geom::SBasis> d2_out,
                    boost::optional<Geom::Satellite> previousSatellite) const;

    double time(Geom::D2<Geom::SBasis> d2_in) const;
    double time(double A, bool I, Geom::D2<Geom::SBasis> d2_in) const;
    double size(Geom::D2<Geom::SBasis> d2_in) const;

    void setPosition(Geom::Point p, Geom::D2<Geom::SBasis> d2_in);
    Geom::Point getPosition(Geom::D2<Geom::SBasis> d2_in) const;

    void setSatelliteType(gchar const *A);
    gchar const *getSatelliteTypeGchar() const;
    //TODO: maybe make after variables protected?
    SatelliteType satelliteType;
    bool isTime;
    bool active;
    bool hasMirror;
    bool hidden;
    double amount;
    double angle;
    size_t steps;
};

} // end namespace Geom

#endif // LIB2GEOM_SEEN_SATELLITE_H

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

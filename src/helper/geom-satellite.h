/**
 * \file
 * \brief Satellite a per ?node/curve holder of data.
 */ /*
    * Authors:
    * 2015 Jabier Arraiza Cenoz<jabier.arraiza@marker.es>
    *
    * This code is in public domain
    */

#ifndef SEEN_SATELLITE_H
#define SEEN_SATELLITE_H

#include <map>
#include <boost/assign.hpp>
#include <2geom/sbasis-geometric.h>
#include "util/enums.h"


enum SatelliteType {
    FILLET = 0, //Fillet
    INVERSE_FILLET,    //Inverse Fillet
    CHAMFER,     //Chamfer
    INVERSE_CHAMFER,    //Inverse Chamfer
    INVALID_SATELLITE     // Invalid Satellite
};
/**
 * @brief Satellite a per ?node/curve holder of data.
 */

class Satellite {
public:

    Satellite();
    Satellite(SatelliteType satellite_type);

    virtual ~Satellite();
    void setIsTime(bool set_is_time)
    {
        is_time = set_is_time;
    }
    void setActive(bool set_active)
    {
        active = set_active;
    }
    void setHasMirror(bool set_has_mirror)
    {
        has_mirror = set_has_mirror;
    }
    void setHidden(bool set_hidden)
    {
        hidden = set_hidden;
    }
    void setAmount(bool set_amount)
    {
        amount = set_amount;
    }
    void setAngle(bool set_angle)
    {
        angle = set_angle;
    }
    void setSteps(bool set_steps)
    {
        steps = set_steps;
    }
    double lenToRad(double const A, Geom::Curve const &curve_in,
                    Geom::Curve const &curve_out,
                    Satellite const previousSatellite) const;
    double radToLen(double const A, Geom::Curve const &curve_in,
                    Geom::Curve const &curve_out) const;

    double time(Geom::Curve const &curve_in, bool const I = false) const;
    double time(double A, bool const I, Geom::Curve const &curve_in) const;
    double arcDistance(Geom::Curve const &curve_in) const;

    void setPosition(Geom::Point const p, Geom::Curve const &curve_in, bool const I = false);
    Geom::Point getPosition(Geom::Curve const &curve_in, bool const I = false) const;

    void setSatelliteType(gchar const *A);
    gchar const *getSatelliteTypeGchar() const;
    SatelliteType satellite_type;
    bool is_time;
    bool active;
    bool has_mirror;
    bool hidden;
    double amount;
    double angle;
    size_t steps;
};
double timeAtArcLength(double const A, Geom::Curve const &curve_in, size_t cache_limit = 0);
double arcLengthAt(double const A, Geom::Curve const &curve_in, size_t cache_limit = 0);

#endif // SEEN_SATELLITE_H

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

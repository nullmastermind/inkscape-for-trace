/**
 * \file
 * \brief Pointwise a class to manage a vector of satellites per piecewise node
 */ /*
    * Authors:
    * Jabiertxof
    * Nathan Hurst
    * Johan Engelen
    * Josh Andler
    * suv
    * Mc-
    * Liam P. White
    * Krzysztof Kosiński
    * This code is in public domain
    */

#ifndef SEEN_POINTWISE_H
#define SEEN_POINTWISE_H

#include <helper/geom-satellite.h>
#include <2geom/path.h>
#include <2geom/pathvector.h>
#include <boost/optional.hpp>

/**
 * @brief Pointwise a class to manage a vector of satellites per curve
 */
typedef std::vector<std::vector<Satellite> > Satellites;
class Pointwise {
public:
    Geom::PathVector getPathVector() const;
    void setPathVector(Geom::PathVector pathv);
    Satellites getSatellites();
    size_t getTotalSatellites();
    void setSatellites(Satellites satellites);
    void recalculateForNewPathVector(Geom::PathVector const pathv, Satellite const S);
private:
    Geom::PathVector _pathvector;
    Satellites _satellites;
};

#endif //SEEN_POINTWISE_H
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

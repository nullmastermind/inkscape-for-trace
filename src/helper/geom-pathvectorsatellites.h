/**
 * \file
 * \brief PathVectorSatellites a class to manage a vector of satellites per piecewise node
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

#ifndef SEEN_PATHVECTORSATELLITES_H
#define SEEN_PATHVECTORSATELLITES_H

#include <helper/geom-satellite.h>
#include <2geom/path.h>
#include <2geom/pathvector.h>

/**
 * @brief PathVectorSatellites a class to manage a vector of satellites per curve
 */
typedef std::vector<std::vector<Satellite> > Satellites;
class PathVectorSatellites {
public:
    Geom::PathVector getPathVector() const;
    void setPathVector(Geom::PathVector pathv);
    Satellites getSatellites();
    void setSatellites(Satellites satellites);
    size_t getTotalSatellites();
    std::pair<size_t, size_t> getIndexData(size_t index);
    void recalculateForNewPathVector(Geom::PathVector const pathv, Satellite const S);
private:
    Geom::PathVector _pathvector;
    Satellites _satellites;
};

#endif //SEEN_PATHVECTORSATELLITES_H
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

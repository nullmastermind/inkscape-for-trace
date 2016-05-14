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

#include <helper/geom-pointwise.h>
Geom::PathVector Pointwise::getPathVector() const
{
    return _pathvector;
}

void Pointwise::setPathVector(Geom::PathVector pathv)
{
    _pathvector = pathv;
}

Satellites Pointwise::getSatellites()
{
    return _satellites;
}

void Pointwise::setSatellites(Satellites satellites)
{
    _satellites = satellites;
}

size_t Pointwise::getTotalSatellites()
{
    size_t counter = 0;
    for (size_t i = 0; i < _satellites.size(); ++i) {
        for (size_t j = 0; j < _satellites[i].size(); ++j) {
            counter++;
        }
    }
    return counter;
}

void Pointwise::recalculateForNewPathVector(Geom::PathVector const pathv, Satellite const &S)
{
    Satellites satellites;
    for (size_t i = 0; i < pathv.size(); i++) {
        std::vector<Satellite> subpath_satellites;
        for (size_t k = 0; k < pathv[i].size_closed(); k++) {
            subpath_satellites.push_back(S);
        }
        satellites.push_back(subpath_satellites);
    }
    setPathVector(pathv);
    setSatellites(satellites);
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

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

#include <helper/geom-pathvectorsatellites.h>
Geom::PathVector PathVectorSatellites::getPathVector() const
{
    return _pathvector;
}

void PathVectorSatellites::setPathVector(Geom::PathVector pathv)
{
    _pathvector = pathv;
}

Satellites PathVectorSatellites::getSatellites()
{
    return _satellites;
}

void PathVectorSatellites::setSatellites(Satellites satellites)
{
    _satellites = satellites;
}

size_t PathVectorSatellites::getTotalSatellites()
{
    size_t counter = 0;
    for (size_t i = 0; i < _satellites.size(); ++i) {
        for (size_t j = 0; j < _satellites[i].size(); ++j) {
            counter++;
        }
    }
    return counter;
}

std::pair<size_t, size_t> PathVectorSatellites::getIndexData(size_t index)
{
    size_t counter = 0;
    for (size_t i = 0; i < _satellites.size(); ++i) {
        for (size_t j = 0; j < _satellites[i].size(); ++j) {
            if (index == counter) {
                return std::make_pair(i,j);
            }
            counter++;
        }
    }
    return std::make_pair(0,0);
}

void PathVectorSatellites::recalculateForNewPathVector(Geom::PathVector const pathv, Satellite const S)
{
    Satellites satellites;
    bool found = false;
    //TODO evaluate fix on nodes at same position
    size_t number_nodes = pathv.nodes().size();
    size_t previous_number_nodes = _pathvector.nodes().size();
    for (size_t i = 0; i < pathv.size(); i++) {
        satellites.reserve(pathv.size());
        std::vector<Satellite> pathsatellites;
        for (size_t j = 0; j < pathv[i].size_closed(); j++) {
            satellites[i].reserve(pathv[i].size_closed());
            found = false;
            for (size_t k = 0; k < _pathvector.size(); k++) {
                for (size_t l = 0; l < _pathvector[k].size_closed(); l++) {
                    if ((l == _pathvector[k].size_closed() -1 &&
                         j == pathv[i].size_closed() -1 &&
                         Geom::are_near(_pathvector[k][l-1].finalPoint(),  pathv[i][j-1].finalPoint())) ||
                        (l == _pathvector[k].size_closed() -1 &&
                         Geom::are_near(_pathvector[k][l-1].finalPoint(),  pathv[i][j].finalPoint())) ||
                        (j == pathv[i].size_closed() -1 &&
                         Geom::are_near(_pathvector[k][l].finalPoint(),  pathv[i][j-1].finalPoint())) ||
                        (Geom::are_near(_pathvector[k][l].initialPoint(),  pathv[i][j].initialPoint())))
                    {
                        pathsatellites.push_back(_satellites[k][l]);
                        std::cout << "dgsgdsgsdgsdgsdgsdgsdgsdgsdgsdgsd\n";
                        found = true;
                        break;
                    }
                }
                if (found) {
                    break;
                }
            }
            
            if (found == false && previous_number_nodes < number_nodes) {
                pathsatellites.push_back(S);
            }
        }
        satellites.push_back(pathsatellites);
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

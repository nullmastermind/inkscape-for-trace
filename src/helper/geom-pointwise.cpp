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
PwD2SBasis Pointwise::getPwd2() const
{
    return _pwd2;
}

Geom::PathVector Pointwise::getPV() const
{
    return _pathvector;
}

void Pointwise::setPwd2(PwD2SBasis const &pwd2_in)
{
    _pwd2 = pwd2_in;
    _pathvector = path_from_piecewise(Geom::remove_short_cuts(_pwd2,0.01),0.01);
}

Satellites Pointwise::getSatellites()
{
    return _satellites;
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

void Pointwise::setSatellites(Satellites const &satellites)
{
    _satellites = satellites;
}

void Pointwise::recalculateForNewPwd2(PwD2SBasis const &A, Geom::PathVector const &B, Satellite const &S)
{
    if (_pwd2.size() > A.size() || _pwd2.size() < A.size()) {
        recalculatePwD2(A, S);
    } else {
        insertDegenerateSatellites(A, B, S);
    }
}

void Pointwise::recalculatePwD2(PwD2SBasis const &A, Satellite const &S)
{
    Satellites satellites;
    Geom::PathVector new_pathv = path_from_piecewise(Geom::remove_short_cuts(A,0.01),0.01);
    Geom::PathVector old_pathv = _pathvector;
    _pathvector.clear();
    size_t new_size = new_pathv.size();
    size_t old_size = old_pathv.size();
    size_t old_increments = old_size;
    for (size_t i = 0; i < new_pathv.size(); i++) {
        bool match = false;
        for (size_t j = 0; j < old_pathv.size(); j++) {
            if ( new_pathv[i] == old_pathv[j]){
                satellites.push_back(_satellites[j]);
                old_pathv.erase(old_pathv.begin() + j);
                new_pathv.erase(new_pathv.begin() + i);
                match = true;
                break;
            }
        }
        if (!match && new_size > old_increments){
            std::vector<Satellite> subpath_satellites;
            for (size_t k = 0; k < new_pathv[i].size_closed(); k++) {
                subpath_satellites.push_back(Satellite(_satellites[0][0].satellite_type));
            }
            satellites.push_back(subpath_satellites);
            old_increments ++;
        }
    }
    if (new_size == old_size) {
        //we asume not change the order of subpaths when remove or add nodes to existing subpaths
        for (size_t l = 0; l < old_pathv.size(); l++) {
            //we assume we only can delete or add nodes not a mix of both
            std::vector<Satellite> subpath_satellites;
            if (old_pathv[l].size() > new_pathv[l].size()){
                //erase nodes
                for (size_t m = 0; m < old_pathv[l].size(); m++) {
                    if (are_near(old_pathv[l][m].initialPoint(), new_pathv[l][m].initialPoint())) {
                        subpath_satellites.push_back(_satellites[l][m]);
                    }
                }
                if (!old_pathv[l].closed() && 
                    are_near(old_pathv[l][old_pathv[l].size() - 1].finalPoint(), new_pathv[l][new_pathv[l].size() - 1].finalPoint())) 
                {
                    subpath_satellites.push_back(_satellites[l][old_pathv[l].size()]);
                }
            } else if (old_pathv[l].size() < new_pathv[l].size()) {
                //add nodes
                for (size_t m = 0; m < old_pathv[l].size(); m++) {
                    if (!are_near(old_pathv[l][m].initialPoint(), new_pathv[l][m].initialPoint())) {
                        _satellites[l].insert(_satellites[l].begin() + m, S);
                    }
                }
                if (!old_pathv[l].closed() && !are_near(old_pathv[l][old_pathv[l].size()-1].finalPoint(), new_pathv[l][old_pathv[l].size()-1].finalPoint())) {
                    _satellites[l].insert(_satellites[l].begin() + old_pathv[l].size(), S);
                }
            } else {
                //never happends
            }
            satellites.push_back(subpath_satellites);
        }
    }
    setPwd2(A);
    setSatellites(satellites);
}

void Pointwise::insertDegenerateSatellites(PwD2SBasis const &A, Geom::PathVector const &B, Satellite const &S)
{
    size_t size_A = A.size();
    size_t size_B = B.curveCount();
    size_t satellite_gap = size_B - size_A;
    if (satellite_gap == 0) {
        return;
    }
    size_t counter_added = 0;
    for (size_t i = 0; i < B.size(); i++) {
        size_t counter = 0;
        if (B[i].empty()) {
            continue;
        }
        for (size_t j = 0; j < B[i].size_closed(); j++) {
            if (B[i][j].isDegenerate() && counter_added < satellite_gap) {
                counter_added++;
                _satellites[i].insert(_satellites[i].begin() + counter + 1 ,S);
            }
            counter++;
        }
    }
    setPwd2(A);
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

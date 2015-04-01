/**
 * \file
 * \brief Pointwise a class to manage a vector of satellites per piecewise curve
 *//*
 * Authors:
 * 2015 Jabier Arraiza Cenoz<jabier.arraiza@marker.es>
 *
 * This code is in public domain
 */

#include <helper/geom-pointwise.h>


namespace Geom {

/**
 * @brief Pointwise a class to manage a vector of satellites per piecewise curve
 *
 * For the moment is a per curve satellite holder not per node. This is ok for
 * much cases but not a real node satellite on open paths 
 * To implement this we can:
 * add extra satellite in open paths, and take notice of current open paths
 * or put extra satellites on back for each open subpath
 *
 * Also maybe the vector of satellites become a vector of
 * optional satellites, and remove the active variable in satellites.
 *
 */
Pointwise::Pointwise(Piecewise<D2<SBasis> > pwd2, std::vector<Satellite> satellites)
        : _pwd2(pwd2), _satellites(satellites), _pathInfo(pwd2)
{
    setStart();
};

Pointwise::~Pointwise(){};

Piecewise<D2<SBasis> >
Pointwise::getPwd2() const
{
    return _pwd2;
}

void
Pointwise::setPwd2(Piecewise<D2<SBasis> > pwd2_in)
{
    _pwd2 = pwd2_in;
    _pathInfo.setPwd2(_pwd2);
}

std::vector<Satellite>
Pointwise::getSatellites() const
{
    return _satellites;
}

void
Pointwise::setSatellites(std::vector<Satellite> sats)
{
    _satellites = sats;
    setStart();
}

/** Update the start satellite on ope/closed paths.
 */
void
Pointwise::setStart()
{
    std::vector<std::pair<size_t, bool> > pathInfo = _pathInfo.getPathInfo();
    for(size_t i = 0; i < pathInfo.size(); i++){
        size_t firstNode = _pathInfo.getFirst(pathInfo[i].first);
        size_t lastNode = _pathInfo.getLast(pathInfo[i].first);
        if(!_pathInfo.getIsClosed(lastNode)){
            _satellites[firstNode].hidden = true;
            _satellites[firstNode].active = false;
        } else {
            _satellites[firstNode].active = true;
            _satellites[firstNode].hidden = _satellites[firstNode + 1].hidden;
        }
    }
}

/** Fired when a path is modified.
 */
void
Pointwise::recalculate_for_new_pwd2(Piecewise<D2<SBasis> > A)
{
    if( _pwd2.size() > A.size()){
        pwd2_sustract(A);
    } else if (_pwd2.size() < A.size()){
        pwd2_append(A);
    }
}

/** Some nodes/subpaths are removed.
 */
void
Pointwise::pwd2_sustract(Piecewise<D2<SBasis> > A)
{
    size_t counter = 0;
    std::vector<Satellite> sats;
    Piecewise<D2<SBasis> > pwd2 = _pwd2;
    setPwd2(A);
    for(size_t i = 0; i < _satellites.size(); i++){
        if(_pathInfo.getLast(i-counter) < i-counter || !are_near(pwd2[i].at0(),A[i-counter].at0(),0.001)){
            counter++;
        } else {
            sats.push_back(_satellites[i-counter]);
        }
    }
    setSatellites(sats);
}

/** Append nodes/subpaths to current pointwise
 */
void 
Pointwise::pwd2_append(Piecewise<D2<SBasis> > A)
{
    size_t counter = 0;
    std::vector<Satellite> sats;
    bool reversed = false;
    bool reorder = false;
    for(size_t i = 0; i < A.size(); i++){
        size_t first = _pathInfo.getFirst(i-counter);
        size_t last = _pathInfo.getLast(i-counter);
        //Check for subpath closed. If a subpath is closed, is not reversed or moved to back
        _pathInfo.setPwd2(A);
        size_t subpathAIndex = _pathInfo.getSubPathIndex(i);
        _pathInfo.setPwd2(_pwd2);
        bool changedSubpath = false;
        if(_pwd2.size() <= i-counter){
            changedSubpath = false;
        } else {
            changedSubpath = subpathAIndex != _pathInfo.getSubPathIndex(i-counter);
        }
        if(!reorder && first == i-counter && !are_near(_pwd2[i-counter].at0(),A[i].at0(),0.001) && !changedSubpath){
            //Send the modified subpath to back
            subpath_to_back(_pathInfo.getSubPathIndex(first));
            reorder = true;
            i--;
            continue;
        }
        if(!reversed && first == i-counter && !are_near(_pwd2[i-counter].at0(),A[i].at0(),0.001) && !changedSubpath){
            subpath_reverse(first, last);
            reversed = true;
        }
        if(_pwd2.size() <= i-counter || !are_near(_pwd2[i-counter].at0(),A[i].at0(),0.001)){
            counter++;
            bool active = true;
            bool hidden = false;
            bool isTime = _satellites[0].isTime;
            bool mirror_knots = _satellites[0].hasMirror;
            double amount = 0.0;
            double degrees = 0.0;
            int steps = 0;
            Satellite sat(_satellites[0].satelliteType, isTime, active, mirror_knots, hidden, amount, degrees, steps);
            sats.push_back(sat);
        } else {
            sats.push_back(_satellites[i-counter]);
        }
    }
    setPwd2(A);
    setSatellites(sats);
}


void
Pointwise::subpath_to_back(size_t subpath){
    std::vector<Geom::Path> path_in = path_from_piecewise(remove_short_cuts(_pwd2,0.1), 0.001);
    size_t nSubpath = 0;
    size_t counter = 0;
    std::vector<Geom::Path> tmp_path;
    Geom::Path rev;
    for (PathVector::const_iterator path_it = path_in.begin(); path_it != path_in.end(); ++path_it) {
        if (path_it->empty()){
            continue;
        }
        Geom::Path::const_iterator curve_it1 = path_it->begin();
        Geom::Path::const_iterator curve_endit = path_it->end_default();
        const Curve &closingline = path_it->back_closed(); 
        if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
            curve_endit = path_it->end_open();
        }
        while (curve_it1 != curve_endit) {
            if(nSubpath == subpath){
                _satellites.push_back(_satellites[counter]);
                _satellites.erase(_satellites.begin() + counter);
            } else {
                counter++;
            }
            ++curve_it1;
        }
        if(nSubpath == subpath){
            rev = *path_it;
        } else {
            tmp_path.push_back(*path_it);
        }
        nSubpath++;
    }
    tmp_path.push_back(rev);
    setPwd2(remove_short_cuts(paths_to_pw(tmp_path),0.01));
}

void
Pointwise::subpath_reverse(size_t start,size_t end){
    start ++;
    for(size_t i = end; i >= start; i--){
        _satellites.push_back(_satellites[i]);
        _satellites.erase(_satellites.begin() + i);
    }
    std::vector<Geom::Path> path_in = path_from_piecewise(remove_short_cuts(_pwd2,0.1), 0.001);
    size_t counter = 0;
    size_t nSubpath = 0;
    size_t subpath = _pathInfo.getSubPathIndex(start);
    std::vector<Geom::Path> tmp_path;
    Geom::Path rev;
    for (PathVector::const_iterator path_it = path_in.begin(); path_it != path_in.end(); ++path_it) {
        if (path_it->empty()){
            continue;
        }
        counter ++;
        if(nSubpath == subpath){
            tmp_path.push_back(path_it->reverse());
        } else {
            tmp_path.push_back(*path_it);
        }
        nSubpath++;
    }
    setPwd2(remove_short_cuts(paths_to_pw(tmp_path),0.01));
}

} // namespace Geom
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

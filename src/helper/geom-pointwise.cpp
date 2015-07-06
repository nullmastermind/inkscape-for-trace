/**
 * \file
 * \brief Pointwise a class to manage a vector of satellites per piecewise curve
 */ /*
    * Authors:
    * Jabiertxof
    * Johan Engelen
    * Josh Andler
    * suv
    * Mc-
    * Liam P. White
    * Nathan Hurst
    * Krzysztof Kosiński
    * This code is in public domain
    */

#include <helper/geom-pointwise.h>

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

pwd2sb Pointwise::getPwd2() const
{
    return _pwd2;
}

void Pointwise::setPwd2(pwd2sb const &pwd2_in)
{
    _pwd2 = pwd2_in;
}

std::vector<Satellite> Pointwise::getSatellites() const
{
    return _satellites;
}

void Pointwise::setSatellites(std::vector<Satellite> const &sats)
{
    _satellites = sats;
}

/** Update the start satellite on open/closed paths.
 */
void Pointwise::setStart()
{
    Geom::PathVector pointwise_pv = path_from_piecewise(Geom::remove_short_cuts(_pwd2,0.01),0.01);
    int counter = 0;
    for (Geom::PathVector::const_iterator path_it = pointwise_pv.begin();
            path_it != pointwise_pv.end(); ++path_it) {
        if (path_it->empty()) {
            continue;
        }
        Geom::Path::const_iterator curve_it = path_it->begin();
        Geom::Path::const_iterator curve_endit = path_it->end_default();
        int index = 0;
        while (curve_it != curve_endit) {
            if(index == 0) {
                if (!path_it->closed()) {
                    _satellites[counter].hidden = true;
                    _satellites[counter].active = false;
                } else {
                    _satellites[counter].active = true;
                    _satellites[counter].hidden = _satellites[counter].hidden;
                }
            }
            ++index;
            ++counter;
            ++curve_it;
        }
    }
}

/** Fired when a path is modified.
 */
void Pointwise::recalculateForNewPwd2(pwd2sb const &A, Geom::PathVector const &B, Satellite const &S)
{
    if (_pwd2.size() > A.size()) {
        pwd2Subtract(A);
    } else if (_pwd2.size() < A.size()) {
        pwd2Append(A, S);
    } else {
        insertDegenerateSatellites(A, B, S);
    }
}

/** Some nodes/subpaths are removed.
 */
void Pointwise::pwd2Subtract(pwd2sb const &A)
{
    size_t counter = 0;
    std::vector<Satellite> sats;
    pwd2sb pwd2 = _pwd2;
    setPwd2(A);
    Geom::PathVector pointwise_pv = path_from_piecewise(Geom::remove_short_cuts(_pwd2,0.01),0.01);
    for (size_t i = 0; i < _satellites.size(); i++) {
        Geom::Path sat_path = pointwise_pv.pathAt(i - counter);
        Geom::PathTime sat_curve_time = sat_path.nearestTime(pointwise_pv.curveAt(i - counter).initialPoint());
        Geom::PathTime sat_curve_time_start = sat_path.nearestTime(sat_path.initialPoint());
        if (sat_curve_time_start.curve_index < sat_curve_time.curve_index||
                !are_near(pwd2[i].at0(), A[i - counter].at0())) {
            counter++;
        } else {
            sats.push_back(_satellites[i - counter]);
        }
    }
    setSatellites(sats);
}

/** Append nodes/subpaths to current pointwise
 */
void Pointwise::pwd2Append(pwd2sb const &A, Satellite const &S)
{
    size_t counter = 0;
    std::vector<Satellite> sats;
    bool reorder = false;
    for (size_t i = 0; i < A.size(); i++) {
        Geom::PathVector pointwise_pv = path_from_piecewise(Geom::remove_short_cuts(_pwd2,0.01),0.01);
        Geom::Path sat_path = pointwise_pv.pathAt(i - counter);
        boost::optional< Geom::PathVectorTime > sat_curve_time_optional = pointwise_pv.nearestTime(pointwise_pv.curveAt(i-counter).initialPoint());
        Geom::PathVectorTime sat_curve_time;
        if(sat_curve_time_optional) {
            sat_curve_time = *sat_curve_time_optional;
        }
        sat_curve_time.normalizeForward(sat_path.size());
        size_t first = Geom::nearest_time(sat_path.initialPoint(),_pwd2);
        size_t last = first + sat_path.size() - 1;
        bool is_start = false;
        if(sat_curve_time.curve_index == 0) {
            is_start = true;
        }
        //Check for subpath closed. If a subpath is closed, is not reversed or moved
        //to back
        size_t old_subpath_index = sat_curve_time.path_index;
        pointwise_pv = path_from_piecewise(Geom::remove_short_cuts(A,0.01),0.01);
        sat_path = pointwise_pv.pathAt(i);
        sat_curve_time_optional = pointwise_pv.nearestTime(pointwise_pv.curveAt(i).initialPoint());
        if(sat_curve_time_optional) {
            sat_curve_time = *sat_curve_time_optional;
        }
        sat_curve_time.normalizeForward(sat_path.size());
        size_t new_subpath_index = sat_curve_time.path_index;
        bool subpath_is_changed = false;
        if (_pwd2.size() > i - counter) {
            subpath_is_changed = old_subpath_index != new_subpath_index;
        }

        if (!reorder && is_start && !are_near(_pwd2[i - counter].at0(), A[i].at0()) && !subpath_is_changed) {
            //Send the modified subpath to back
            subpathToBack(old_subpath_index);
            reorder = true;
            i--;
            continue;
        }

        if (is_start && !are_near(_pwd2[i - counter].at0(), A[i].at0()) && !subpath_is_changed) {
            subpathReverse(first, last);
        }

        if (_pwd2.size() <= i - counter || !are_near(_pwd2[i - counter].at0(), A[i].at0())) {
            counter++;
            sats.push_back(S);
        } else {
            sats.push_back(_satellites[i - counter]);
        }
    }
    setPwd2(A);
    setSatellites(sats);
}

void Pointwise::subpathToBack(size_t subpath)
{
    Geom::PathVector path_in =
        path_from_piecewise(remove_short_cuts(_pwd2, 0.1), 0.001);
    size_t subpath_counter = 0;
    size_t counter = 0;
    Geom::PathVector tmp_path;
    Geom::Path to_back;
    for (Geom::PathVector::const_iterator path_it = path_in.begin();
            path_it != path_in.end(); ++path_it) {
        if (path_it->empty()) {
            continue;
        }
        Geom::Path::const_iterator curve_it1 = path_it->begin();
        Geom::Path::const_iterator curve_endit = path_it->end_default();
        Geom::Curve const &closingline = path_it->back_closed();
        if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
            curve_endit = path_it->end_open();
        }
        while (curve_it1 != curve_endit) {
            if (subpath_counter == subpath) {
                _satellites.push_back(_satellites[counter]);
                _satellites.erase(_satellites.begin() + counter);
            } else {
                counter++;
            }
            ++curve_it1;
        }
        if (subpath_counter == subpath) {
            to_back = *path_it;
        } else {
            tmp_path.push_back(*path_it);
        }
        subpath_counter++;
    }
    tmp_path.push_back(to_back);
    setPwd2(remove_short_cuts(paths_to_pw(tmp_path), 0.01));
}

void Pointwise::subpathReverse(size_t start, size_t end)
{
    start++;
    for (size_t i = end; i >= start; i--) {
        _satellites.insert(_satellites.begin() + end + 1, _satellites[i]);
        _satellites.erase(_satellites.begin() + i);
    }
    Geom::PathVector path_in =
        path_from_piecewise(remove_short_cuts(_pwd2, 0.1), 0.001);
    size_t counter = 0;
    size_t subpath_counter = 0;
    Geom::Path sat_path = path_in.pathAt(start);
    boost::optional< Geom::PathVectorTime > sat_curve_time_optional = path_in.nearestTime(path_in.curveAt(start).initialPoint());
    Geom::PathVectorTime sat_curve_time;
    if(sat_curve_time_optional) {
        sat_curve_time = *sat_curve_time_optional;
    }
    sat_curve_time.normalizeForward(sat_path.size());
    size_t subpath = sat_curve_time.path_index;
    Geom::PathVector tmp_path;
    Geom::Path rev;
    for (Geom::PathVector::const_iterator path_it = path_in.begin();
            path_it != path_in.end(); ++path_it) {
        if (path_it->empty()) {
            continue;
        }
        counter++;
        if (subpath_counter == subpath) {
            tmp_path.push_back(path_it->reversed());
        } else {
            tmp_path.push_back(*path_it);
        }
        subpath_counter++;
    }
    setPwd2(remove_short_cuts(paths_to_pw(tmp_path), 0.01));
}

/** Fired when a path is modified duplicating a node. Piecewise ignore degenerated curves.
 */
void Pointwise::insertDegenerateSatellites(pwd2sb const &A, Geom::PathVector const &B, Satellite const &S)
{
    size_t size_A = A.size();
    size_t size_B = B.curveCount();
    size_t satellite_gap = size_B - size_A;
    if (satellite_gap == 0) {
        return;
    }
    size_t counter = 0;
    size_t counter_added = 0;
    for (Geom::PathVector::const_iterator path_it = B.begin();
            path_it != B.end(); ++path_it) {
        if (path_it->empty()) {
            continue;
        }
        Geom::Path::const_iterator curve_it1 = path_it->begin();
        Geom::Path::const_iterator curve_endit = path_it->end_default();
        if (path_it->closed()) {
            Geom::Curve const &closingline = path_it->back_closed();
            if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
                curve_endit = path_it->end_open();
            }
        }
        while (curve_it1 != curve_endit) {
            if ((*curve_it1).isDegenerate() && counter_added < satellite_gap) {
                counter_added++;
                _satellites.insert(_satellites.begin() + counter + 1 ,S);
            }
            ++curve_it1;
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

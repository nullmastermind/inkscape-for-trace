/**
 * \file
 * \brief Pathinfo iterate a Geom::PathVector and allow get info about it.
 * \Usualy need a curve index to get the results
 * \TODO: migrate more Inkscape loops to use it.
 */ /*
    * Authors:
    * 2015 Jabier Arraiza Cenoz<jabier.arraiza@marker.es>
    *
    * This code is in public domain
    */

#include <helper/geom-pathinfo.h>
#include <2geom/sbasis-to-bezier.h>

/**
 * @brief Pathinfo store the _data of a Geom::PathVector and allow get info about it
 *
 */
Pathinfo::Pathinfo(Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2)
{
    set(pwd2);
}

Pathinfo::Pathinfo(Geom::PathVector path_vector, bool skip_degenerate)
{
    set(path_vector, skip_degenerate);
}


Pathinfo::~Pathinfo() {}


void Pathinfo::set(Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2)
{
    set(path_from_piecewise(remove_short_cuts(pwd2, 0.1), 0.001));
}
/** Store the base path _data
 */
void Pathinfo::set(Geom::PathVector path_vector, bool skip_degenerate)
{
    _data.clear();
    size_t counter = 0;
    for (Geom::PathVector::const_iterator path_it = path_vector.begin();
            path_it != path_vector.end(); ++path_it) 
    {
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
            if(curve_it1->isDegenerate() && skip_degenerate ){
                ++curve_it1;
                continue;
            }
            ++curve_it1;
            counter++;
        }
        if (path_it->closed()) {
            _data.push_back(std::make_pair(counter - 1, true));
        } else {
            _data.push_back(std::make_pair(counter - 1, false));
        }
    }
}

/** Size of pathvector
 */
size_t Pathinfo::size() const
{
    return _data.back().first + 1;
}

/** Size of subpath
 */
size_t Pathinfo::subPathSize(size_t subpath_index) const
{
    size_t size = 0;
    if( _data.size() > subpath_index){
        double prev = 0;
        if(subpath_index != 0){
            prev = _data[subpath_index - 1].first;
        }
        size = prev - _data[subpath_index].first + 1;
    }
    return size;
}

/** Get subpath index from a curve index
 */
size_t Pathinfo::subPathIndex(size_t index) const
{
    for (size_t i = 0; i < _data.size(); i++) {
        if (index <= _data[i].first) {
            return i;
        }
    }
    return 0;
}

/** Get subpath last index given a curve index
 */
size_t Pathinfo::last(size_t index) const
{
    for (size_t i = 0; i < _data.size(); i++) {
        if (index <= _data[i].first) {
            return _data[i].first;
        }
    }
    return 0;
}

/** Get subpath first index given a curve index
 */
size_t Pathinfo::first(size_t index) const
{
    for (size_t i = 0; i < _data.size(); i++) {
        if (index <= _data[i].first) {
            if (i == 0) {
                return 0;
            } else {
                return _data[i - 1].first + 1;
            }
        }
    }
    return 0;
}

/** Get previous index given a curve index
 */
boost::optional<size_t> Pathinfo::previous(size_t index) const
{
    if (first(index) == index && closed(index)) {
        return last(index);
    }
    if (first(index) == index && !closed(index)) {
        return boost::none;
    }
    return index - 1;
}

/** Get next index given a curve index
 */
boost::optional<size_t> Pathinfo::next(size_t index) const
{
    if (last(index) == index && closed(index)) {
        return first(index);
    }
    if (last(index) == index && !closed(index)) {
        return boost::none;
    }
    return index + 1;
}

/** Get if subpath is closed given a curve index
 */
bool Pathinfo::closed(size_t index) const
{
    for (size_t i = 0; i < _data.size(); i++) {
        if (index <= _data[i].first) {
            return _data[i].second;
        }
    }
    return false;
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

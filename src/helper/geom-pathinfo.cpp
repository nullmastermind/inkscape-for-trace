/**
 * \file
 * \brief Pathinfo store the data of a pathvector and allow get info about it
 */ /*
    * Authors:
    * 2015 Jabier Arraiza Cenoz<jabier.arraiza@marker.es>
    *
    * This code is in public domain
    */

#include <helper/geom-pathinfo.h>
#include <2geom/sbasis-to-bezier.h>

namespace Geom {

/**
 * @brief Pathinfo store the data of a pathvector and allow get info about it
 *
 */
Pathinfo::Pathinfo(Piecewise<D2<SBasis> > pwd2) : _pwd2(pwd2)
{
    _setPathInfo(pwd2);
}
;
Pathinfo::Pathinfo(Geom::PathVector path_vector, bool skip_degenerate) : _path_vector(path_vector)
{
    _setPathInfo(path_vector, skip_degenerate);
}
;

Pathinfo::~Pathinfo() {}
;

void Pathinfo::setPwd2(Piecewise<D2<SBasis> > pwd2)
{
    _pwd2 = pwd2;
    _setPathInfo(pwd2);
}

void Pathinfo::setPathVector(Geom::PathVector path_vector, bool skip_degenerate)
{
    _path_vector = path_vector;
    _setPathInfo(path_vector, skip_degenerate);
}

void Pathinfo::_setPathInfo(Piecewise<D2<SBasis> > pwd2)
{
    _setPathInfo(path_from_piecewise(remove_short_cuts(pwd2, 0.1), 0.001));
}
/** Store the base path data
 */
void Pathinfo::_setPathInfo(Geom::PathVector path_vector, bool skip_degenerate)
{
    data.clear();
    Geom::PathVector path_in =
        path_from_piecewise(remove_short_cuts(_pwd2, 0.1), 0.001);
    size_t counter = 0;
    for (PathVector::const_iterator path_it = path_in.begin();
            path_it != path_in.end(); ++path_it) {
        if (path_it->empty()) {
            continue;
        }
        Geom::Path::const_iterator curve_it1 = path_it->begin();
        Geom::Path::const_iterator curve_endit = path_it->end_default();
        if (path_it->closed()) {
            const Curve &closingline = path_it->back_closed();
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
            data.push_back(std::make_pair(counter - 1, true));
        } else {
            data.push_back(std::make_pair(counter - 1, false));
        }
    }
}

size_t Pathinfo::numberCurves() const
{
    return data.back().first;
}

size_t Pathinfo::subPathIndex(size_t index) const
{
    for (size_t i = 0; i < data.size(); i++) {
        if (index <= data[i].first) {
            return i;
        }
    }
    return 0;
}

size_t Pathinfo::last(size_t index) const
{
    for (size_t i = 0; i < data.size(); i++) {
        if (index <= data[i].first) {
            return data[i].first;
        }
    }
    return 0;
}

size_t Pathinfo::first(size_t index) const
{
    for (size_t i = 0; i < data.size(); i++) {
        if (index <= data[i].first) {
            if (i == 0) {
                return 0;
            } else {
                return data[i - 1].first + 1;
            }
        }
    }
    return 0;
}

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

bool Pathinfo::closed(size_t index) const
{
    for (size_t i = 0; i < data.size(); i++) {
        if (index <= data[i].first) {
            return data[i].second;
        }
    }
    return false;
}

}; // namespace Geom

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

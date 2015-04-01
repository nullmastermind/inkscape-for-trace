/**
 * \file
 * \brief Pathinfo store the data of a pathvector and allow get info about it
 *//*
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
Pathinfo::Pathinfo(Piecewise<D2<SBasis> > pwd2)
        : _pwd2(pwd2)
{
    setPathInfo();
};

Pathinfo::~Pathinfo(){};

void
Pathinfo::setPwd2(Piecewise<D2<SBasis> > pwd2_in)
{
    _pwd2 = pwd2_in;
    setPathInfo();
}

/** Store the base path data
 */
void
Pathinfo::setPathInfo()
{
    _pathInfo.clear();
    std::vector<Geom::Path> path_in = path_from_piecewise(remove_short_cuts(_pwd2,0.1), 0.001);
    size_t counter = 0;
    for (PathVector::const_iterator path_it = path_in.begin(); path_it != path_in.end(); ++path_it) {
        if (path_it->empty()){
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
            ++curve_it1;
            counter++;
        }
        if(path_it->closed()){
            _pathInfo.push_back(std::make_pair(counter-1,true));
        } else {
            _pathInfo.push_back(std::make_pair(counter-1,false));
        }
    }
}

size_t
Pathinfo::getSubPathIndex(size_t index) const
{
    for(size_t i = 0; i < _pathInfo.size(); i++){
        if(index <= _pathInfo[i].first){
            return i;
        }
    }
    return 0;
}

size_t
Pathinfo::getLast(size_t index) const
{
    for(size_t i = 0; i < _pathInfo.size(); i++){
        if(index <= _pathInfo[i].first){
            return _pathInfo[i].first;
        }
    }
    return 0;
}

size_t
Pathinfo::getFirst(size_t index) const
{
    for(size_t i = 0; i < _pathInfo.size(); i++){
        if(index <= _pathInfo[i].first){
            if(i==0){
                return 0;
            } else {
                return _pathInfo[i-1].first + 1;
            }
        }
    }
    return 0;
}

boost::optional<size_t>
Pathinfo::getPrevious(size_t index) const
{
    if(getFirst(index) == index && getIsClosed(index)){
        return getLast(index);
    }
    if(getFirst(index) == index && !getIsClosed(index)){
        return boost::none;
    }
    return index - 1;
}

boost::optional<size_t>
Pathinfo::getNext(size_t index) const
{
    if(getLast(index) == index && getIsClosed(index)){
        return getFirst(index);
    }
    if(getLast(index) == index && !getIsClosed(index)){
        return boost::none;
    }
    return index + 1;
}

bool
Pathinfo::getIsClosed(size_t index) const
{
    for(size_t i = 0; i < _pathInfo.size(); i++){
        if(index <= _pathInfo[i].first){
            return _pathInfo[i].second;
        }
    }
    return false;
}

std::vector<std::pair<size_t, bool> >
Pathinfo::getPathInfo() const
{
    return _pathInfo;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

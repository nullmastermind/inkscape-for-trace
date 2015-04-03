/**
 * \file
 * \brief Pathinfo store the data of a pathvector and allow get info about it
 */ /*
    * Authors:
    * 2015 Jabier Arraiza Cenoz<jabier.arraiza@marker.es>
    *
    * This code is in public domain
    */

#ifndef SEEN_GEOM_PATHINFO_H
#define SEEN_GEOM_PATHINFO_H

#include <2geom/path.h>
#include <boost/optional.hpp>

namespace Geom {

/**
 * @brief Pathinfo store the data of a pathvector and allow get info about it
 *
 */
class Pathinfo {
public:
    Pathinfo(Piecewise<D2<SBasis> > pwd2);
    Pathinfo(Geom::PathVector path_vector, bool skip_degenerate = false);
    virtual ~Pathinfo();
    void setPwd2(Piecewise<D2<SBasis> > pwd2);
    void setPathVector(Geom::PathVector path_vector, bool skip_degenerate = false);
    size_t numberCurves() const;
    size_t subPathIndex(size_t index) const;
    size_t last(size_t index) const;
    size_t first(size_t index) const;
    boost::optional<size_t> previous(size_t index) const;
    boost::optional<size_t> next(size_t index) const;
    bool closed(size_t index) const;
    std::vector<std::pair<size_t, bool> > data;

private:
    void _setPathInfo(Piecewise<D2<SBasis> > pwd2);
    void _setPathInfo(Geom::PathVector path_vector, bool skip_degenerate = false);
    Piecewise<D2<SBasis> > _pwd2;
    Geom::PathVector _path_vector;
};

} //namespace Geom

#endif //SEEN_GEOM_PATHINFO_H
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

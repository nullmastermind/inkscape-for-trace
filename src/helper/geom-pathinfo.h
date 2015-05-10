/**
 * \file
 * \brief Pathinfo store data of a Geom::PathVector and allow get info about it
 * \
 */ /*
    * Authors:
    * 2015 Jabier Arraiza Cenoz<jabier.arraiza@marker.es>
    *
    * This code is in public domain
    */

#ifndef SEEN_PATHINFO_H
#define SEEN_PATHINFO_H

#include <2geom/path.h>
#include <boost/optional.hpp>

/**
 * @brief Pathinfo store the data of a Geom::PathVector and allow get info about it
 *
 */

class Pathinfo {
public:
    Pathinfo(Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2);
    Pathinfo(Geom::PathVector path_vector, bool skip_degenerate = false);
    virtual ~Pathinfo();
    void set(Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2);
    void set(Geom::PathVector path_vector, bool skip_degenerate = false);
    std::vector<std::pair<size_t, bool> > get(){return _data;};
    size_t size() const;
    size_t subPathSize(size_t index) const;
    size_t subPathIndex(size_t index) const;
    size_t last(size_t index) const;
    size_t first(size_t index) const;
    boost::optional<size_t> previous(size_t index) const;
    boost::optional<size_t> next(size_t index) const;
    bool closed(size_t index) const;

private:
    std::vector<std::pair<size_t, bool> > _data;
};

#endif //SEEN_PATHINFO_H
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

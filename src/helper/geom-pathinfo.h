/**
 * \file
 * \brief Pathinfo store the data of a pathvector and allow get info about it
 *//*
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
class Pathinfo
{
    public:
        Pathinfo(Piecewise<D2<SBasis> > pwd2);
        virtual ~Pathinfo();
        void setPwd2(Piecewise<D2<SBasis> > pwd2_in);
        size_t getSubPathIndex(size_t index) const;
        size_t getLast(size_t index) const;
        size_t getFirst(size_t index) const;
        boost::optional<size_t> getPrevious(size_t index) const;
        boost::optional<size_t> getNext(size_t index) const;
        bool getIsClosed(size_t index) const;
        std::vector<std::pair<size_t, bool> > getPathInfo() const;

    private:
        void setPathInfo();
        Piecewise<D2<SBasis> > _pwd2;
        std::vector<std::pair<size_t, bool> > _pathInfo;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

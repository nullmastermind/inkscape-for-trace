/**
 * \file
 * \brief Pointwise
 *//*
 * Authors:
 * 2015 Jabier Arraiza Cenoz<jabier.arraiza@marker.es>
 * Copyright 2015  authors
 *
 * Pointwise maintains a set of "Satellite" positions along a curve/pathvector.
 * The positions are specified as arc length distances along the curve or by 
 * time in the curve. Splicing operations automatically update the satellite 
 * positions to preserve the intent.
 * The data is serialised to SVG using a specialiced pointwise LPE parameter to 
 * handle it in th future can be a inkscape based property to paths 
 * //all operations are O(1) per satellite, with the exception of .., .., and .., which
 * //need to use binary search to find the locations.
 * Anywhere a Piecewise is used, a Pointwise can be substituted, allowing
 * existing algorithms to correctly update satellite positions.

 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, output to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 */

#ifndef SEEN_GEOM_POINTWISE_H
#define SEEN_GEOM_POINTWISE_H

#include <vector>
#include <2geom/sbasis.h>
#include <2geom/sbasis-2d.h>
#include <2geom/piecewise.h>
#include <2geom/satellite.h>
#include <2geom/sbasis-to-bezier.h>
#include <2geom/path.h>
#include "helper/geom.h"
#include <boost/optional.hpp>

namespace Geom {
/**
 * %Pointwise function class.
 */

class Pointwise
{
    public:
        Pointwise(Piecewise<D2<SBasis> > pwd2, std::vector<std::pair<size_t,Satellite> > satellites);
        virtual ~Pointwise();
        std::vector<size_t> findSatellites(size_t A, long B = -1) const;
        std::vector<size_t> findPeviousSatellites(size_t A, long  B) const;
        double rad_to_len(double A, std::pair<size_t,Geom::Satellite> sat) const;
        double len_to_rad(double A, std::pair<size_t,Geom::Satellite> sat) const;
        std::vector<std::pair<size_t,Satellite> > getSatellites() const;
        void setSatellites(std::vector<std::pair<size_t,Satellite> > sats);
        Piecewise<D2<SBasis> > getPwd2() const;
        void setPwd2(Piecewise<D2<SBasis> > pwd2_in);
        void recalculate_for_new_pwd2(Piecewise<D2<SBasis> > A);
        void pwd2_append(Piecewise<D2<SBasis> > A);
        void pwd2_sustract(Piecewise<D2<SBasis> > A);
        void set_extremes(bool active, bool hidden, double amount = -1, double angle = -1);
        void deleteSatellites(size_t A);
        void subpath_append_reorder(size_t subpath);
        void reverse(size_t start,size_t end);
        void setPathInfo();
        void setPathInfo(Piecewise<D2<SBasis> >);
        size_t getSubPathIndex(size_t index) const;
        size_t getLast(size_t index) const;
        size_t getFirst(size_t index) const;
        boost::optional<size_t> getPrevious(size_t index) const;
        boost::optional<size_t> getNext(size_t index) const;
        bool getIsClosed(size_t index) const;

    private:
        Piecewise<D2<SBasis> > _pwd2;
        std::vector<std::pair<size_t,Satellite> > _satellites;
        std::vector<std::pair<size_t, bool> > _pathInfo;
};

} // end namespace Geom

#endif //SEEN_GEOM_POINTWISE_H
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

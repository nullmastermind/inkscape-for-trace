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
        Pointwise(Piecewise<D2<SBasis> > pwd2);
        virtual ~Pointwise();
        std::vector<Satellite> findSatellites(unsigned int A, int B = -1) const;
        std::vector<Satellite> findPeviousSatellites(unsigned int A, int B) const;
        double rad_to_len(double A, std::pair<unsigned int,Geom::Satellite> sat) const;
        double len_to_rad(double A, std::pair<unsigned int,Geom::Satellite> sat) const;
        std::vector<std::pair<unsigned int,Satellite> > getSatellites() const;
        void setSatellites(std::vector<std::pair<unsigned int,Satellite> > sats);
        Piecewise<D2<SBasis> > getPwd2() const;
        void setPwd2(Piecewise<D2<SBasis> > pwd2_in);
        void recalculate_for_new_pwd2(Piecewise<D2<SBasis> > A);
        void new_pwd_append(Piecewise<D2<SBasis> > A);
        void new_pwd_sustract(Piecewise<D2<SBasis> > A);
        void set_extremes(bool active, bool hidden, double amount, double angle);
        void setPathInfo();
        void setPathInfo(Piecewise<D2<SBasis> >);
        unsigned int getSubPathIndex(unsigned int index) const;
        unsigned int getLast(unsigned int index) const;
        unsigned int getFirst(unsigned int index) const;
        boost::optional<unsigned int> getPrevious(unsigned int index) const;
        boost::optional<unsigned int> getNext(unsigned int index) const;
        bool getIsClosed(unsigned int index) const;

    private:
        Piecewise<D2<SBasis> > _pwd2;
        std::vector<std::pair<unsigned int,Satellite> > _satellites;
        std::vector<std::pair<unsigned int, bool> > _pathInfo;
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

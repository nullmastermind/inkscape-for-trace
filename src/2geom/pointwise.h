/**
 * \file
 * \brief Pointwise
 *//*
 * Authors:
 *
 * Copyright 2014  authors
 *
 *
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
#include <2geom/affine.h>

namespace Geom {
/**
 * %Pointwise function class.
 */

class Pointwise
{
    public:
        Pointwise(){}
        
        Pointwise(Piecewise<D2<SBasis> > pwd2, std::vector<std::pair<int,satelite> > satellites)
        : _pwd2(pwd2), _satellites(satellites)
        {
        }
        
        virtual ~Pointwise();
        
        std::vector<satelite> findSatellites(int A) const;
        Pointwise recalculate_for_new_pwd2(Piecewise<D2<SBasis> > A);
        Pointwise pwd2_reverse(int index);
        Pointwise pwd2_append(int index);
        Pointwise pwd2_prepend(int index);
        Pointwise pwd2_add(int index);
        Pointwise pwd2_del(int index);
        Pointwise satellite_add(int index,satelite sat);
        Pointwise satellite_del(int index,satelite sat);

    private:
        std::vector<std::pair<int,satelite> > _satellites;
        Piecewise<D2<SBasis> > _pwd2;
};

#endif //SEEN_GEOM_PW_SB_H
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

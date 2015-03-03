/*
 * pointwise.cpp
 * Authors:
 * 2015 Jabier Arraiza Cenoz<jabier.arraiza@marker.es>
 * Copyright 2015  authors
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

#include <2geom/pointwise.h>

namespace Geom {

Pointwise::Pointwise(Piecewise<D2<SBasis> > pwd2, std::vector<std::pair<int,Satellite> > satellites)
        : _pwd2(pwd2), _satellites(satellites)
{
};

Pointwise::~Pointwise(){};

std::vector<std::pair<int,Satellite> > 
Pointwise::getSatellites(){
    return _satellites;
}

void
Pointwise::setSatellites(std::vector<std::pair<int,Satellite> > sat){
    _satellites = sat;
}

Piecewise<D2<SBasis> >
Pointwise::getPwd2(){
    return _pwd2;
}

void
Pointwise::setPwd2(Piecewise<D2<SBasis> > pwd2_in){
    _pwd2 = pwd2_in;
}

boost::optional<Geom::D2<Geom::SBasis> >
Pointwise::getCurveIn(std::pair<int,Satellite> sat){
    //curve out = sat.first;
    std::vector<Geom::Path> path_in_processed = pathv_to_linear_and_cubic_beziers(path_from_piecewise(_pwd2, 0.001));
    int counterTotal = 0;
    for (PathVector::const_iterator path_it = path_in_processed.begin(); path_it != path_in_processed.end(); ++path_it) {
        if (path_it->empty()){
            continue;
        }
        Geom::Path::const_iterator curve_it1 = path_it->begin();
        Geom::Path::const_iterator curve_endit = path_it->end_default();
        if (path_it->closed()) {
          const Curve &closingline = path_it->back_closed(); 
          // the closing line segment is always of type 
          // LineSegment.
          if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
            // closingline.isDegenerate() did not work, because it only checks for
            // *exact* zero length, which goes wrong for relative coordinates and
            // rounding errors...
            // the closing line segment has zero-length. So stop before that one!
            curve_endit = path_it->end_open();
          }
        }
        Geom::Path::const_iterator curve_end = curve_endit;
        --curve_end;
        int counter = 0;
        while (curve_it1 != curve_endit) {
            if(counterTotal == sat.first){
                if (counter==0) {
                    if (path_it->closed()) {
                        return (*curve_end).toSBasis();
                    } else {
                        return boost::none;
                    }
                } else {
                        return (*path_it)[counter - 1].toSBasis();
                }
            }
            ++curve_it1;
            counter++;
            counterTotal++;
        }
    }
    return boost::none;
}

std::vector<Satellite> 
Pointwise::findSatellites(int A, int B) const
{
    std::vector<Satellite> ret;
    int counter = 0;
    for(unsigned i = 0; i < _satellites.size(); i++){
        if(_satellites[i].first == A){
            if(counter >= B && B != -1){
                return ret;
            }
            ret.push_back(_satellites[i].second);
            counter++;
        }
    }
    return ret;
}

std::vector<Satellite> 
Pointwise::findClosingSatellites(int A) const
{
    std::vector<Satellite> ret;
    bool finded = false;
    for(unsigned i = 0; i < _satellites.size(); i++){
        if(finded && _satellites[i].second.getIsStart()){
            return ret;
        }
        if(finded && _satellites[i].second.getIsClosing()){
            ret.push_back(_satellites[i].second);
        }
        if(_satellites[i].first == A){
            finded = true;
        }
    }
    return ret;
}

std::vector<Satellite> 
Pointwise::findPeviousSatellites(int A, int B) const
{
    std::vector<Satellite> ret;
    for(unsigned i = 0; i < _satellites.size(); i++){
        if(_satellites[i].first == A){
            if(!_satellites[i].second.getIsStart()){
                ret = findSatellites(_satellites[i-1].first, B);
            } else {
                ret = findClosingSatellites(_satellites[i].first);
            }
        }
    }
    return ret;
}

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

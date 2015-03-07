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
#include <2geom/ray.h>
#include <2geom/path-intersection.h>
#include <cmath>

namespace Geom {

Pointwise::Pointwise(Piecewise<D2<SBasis> > pwd2, std::vector<std::pair<unsigned int,Satellite> > satellites)
        : _pwd2(pwd2), _satellites(satellites)
{
};

Pointwise::~Pointwise(){};

std::vector<std::pair<unsigned int,Satellite> > 
Pointwise::getSatellites(){
    return _satellites;
}

void
Pointwise::setSatellites(std::vector<std::pair<unsigned int,Satellite> > sat){
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

double 
Pointwise::rad_to_len(double A,  std::pair<unsigned int,Geom::Satellite> satellite)
{
    double len = 0;
    boost::optional<Geom::D2<Geom::SBasis> > d2_in = getCurveIn(satellite);
    if(d2_in){
        Geom::D2<Geom::SBasis> d2_out = _pwd2[satellite.first];
        Piecewise<D2<SBasis> > offset_curve0 = Piecewise<D2<SBasis> >(*d2_in)+rot90(unitVector(derivative(*d2_in)))*(A);
        Piecewise<D2<SBasis> > offset_curve1 = Piecewise<D2<SBasis> >(d2_out)+rot90(unitVector(derivative(d2_out)))*(A);
        Geom::Path p0 = path_from_piecewise(offset_curve0, 0.1)[0];
        Geom::Path p1 = path_from_piecewise(offset_curve1, 0.1)[0];
        Geom::Crossings cs = Geom::crossings(p0, p1);
        if(cs.size() > 0){
            Point cp =p0(cs[0].ta);
            double p0pt = nearest_point(cp, d2_out);
            len = satellite.second.toSize(p0pt,d2_out);
        } else {
            if(A > 0){
                len = rad_to_len(A * -1, satellite);
            }
        }
    }
    return len;
}

double 
Pointwise::len_to_rad(double A,  std::pair<unsigned int,Geom::Satellite> satellite)
{
    boost::optional<Geom::D2<Geom::SBasis> > d2_in = getCurveIn(satellite);
    if(d2_in){
        Geom::D2<Geom::SBasis> d2_out = _pwd2[satellite.first];
        double time_in = satellite.second.getOpositeTime(A, *d2_in);
        double time_out = satellite.second.toTime(A,d2_out);
        Geom::Point startArcPoint = (*d2_in).valueAt(time_in);
        Geom::Point endArcPoint = d2_out.valueAt(time_out);
        Piecewise<D2<SBasis> > u;
        u.push_cut(0);
        u.push((*d2_in), 1);
        Geom::Curve * A = path_from_piecewise(u, 0.1)[0][0].duplicate();
        Piecewise<D2<SBasis> > u2;
        u2.push_cut(0);
        u2.push((d2_out), 1);
        Geom::Curve * B = path_from_piecewise(u2, 0.1)[0][0].duplicate();
        Curve *knotCurve1 = A->portion(0, time_in);
        Curve *knotCurve2 = B->portion(time_out, 1);
        Geom::CubicBezier const *cubic1 = dynamic_cast<Geom::CubicBezier const *>(&*knotCurve1);
        Ray ray1(startArcPoint, (*d2_in).valueAt(1));
        if (cubic1) {
            ray1.setPoints((*cubic1)[2], startArcPoint);
        }
        Geom::CubicBezier const *cubic2 = dynamic_cast<Geom::CubicBezier const *>(&*knotCurve2);
        Ray ray2(d2_out.valueAt(0), endArcPoint);
        if (cubic2) {
            ray2.setPoints(endArcPoint, (*cubic2)[1]);
        }
        bool ccwToggle = cross((*d2_in).valueAt(1) - startArcPoint, endArcPoint - startArcPoint) < 0;
        double distanceArc = Geom::distance(startArcPoint,middle_point(startArcPoint,endArcPoint));
        double angleBetween = angle_between(ray1, ray2, ccwToggle);
        double divisor = std::sin(angleBetween/2.0);
        if(divisor > 0){
            return distanceArc/divisor;
        }
    }
    return 0;
}

boost::optional<Geom::D2<Geom::SBasis> >
Pointwise::getCurveIn(std::pair<unsigned int,Satellite> sat){
    //curve out = sat.first;
    std::vector<Geom::Path> path_in_processed = path_from_piecewise(_pwd2, 0.001);
    unsigned int counterTotal = 0;
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
        unsigned int counter = 0;
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
Pointwise::findSatellites(unsigned int A, int B) const
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
Pointwise::findClosingSatellites(unsigned int A) const
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
Pointwise::findPeviousSatellites(unsigned int A, int B) const
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

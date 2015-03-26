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

Pointwise::Pointwise(Piecewise<D2<SBasis> > pwd2, std::vector<std::pair<size_t,Satellite> > satellites)
        : _pwd2(pwd2),_satellites(satellites),_pathInfo()
{
    setPathInfo();
};

Pointwise::~Pointwise(){};

Piecewise<D2<SBasis> >
Pointwise::getPwd2() const
{
    return _pwd2;
}

void
Pointwise::setPwd2(Piecewise<D2<SBasis> > pwd2_in)
{
    _pwd2 = pwd2_in;
    setPathInfo();
}

std::vector<std::pair<size_t,Satellite> >
Pointwise::getSatellites() const
{
    return _satellites;
}

void
Pointwise::setSatellites(std::vector<std::pair<size_t,Satellite> > sats)
{
    _satellites = sats;
}

//START QUESTION Next functions maybe is beter land outside the class?
void
Pointwise::setPathInfo()
{
    setPathInfo(_pwd2);
}

void
Pointwise::setPathInfo(Piecewise<D2<SBasis> > pwd2)
{
    _pathInfo.clear();
    std::vector<Geom::Path> path_in = path_from_piecewise(remove_short_cuts(pwd2,0.1), 0.001);
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
Pointwise::getSubPathIndex(size_t index) const
{
    for(size_t i = 0; i < _pathInfo.size(); i++){
        if(index <= _pathInfo[i].first){
            return i;
        }
    }
    return 0;
}

size_t
Pointwise::getLast(size_t index) const
{
    for(size_t i = 0; i < _pathInfo.size(); i++){
        if(index <= _pathInfo[i].first){
            return _pathInfo[i].first;
        }
    }
    return 0;
}

size_t
Pointwise::getFirst(size_t index) const
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
Pointwise::getPrevious(size_t index) const
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
Pointwise::getNext(size_t index) const
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
Pointwise::getIsClosed(size_t index) const
{
    for(size_t i = 0; i < _pathInfo.size(); i++){
        if(index <= _pathInfo[i].first){
            return _pathInfo[i].second;
        }
    }
    return false;
}
//END QUESTION

void
Pointwise::recalculate_for_new_pwd2(Piecewise<D2<SBasis> > A)
{
    if( _pwd2.size() > A.size()){
        std::cout << "bbbbbbbbbbbbbbbbbbbbbbbbb\n";
        pwd2_sustract(A);
    } else if ( _pwd2.size() < A.size()){
        pwd2_append(A);
        std::cout << "aaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n";
    }
    std::cout << "cccccccccccccccccccccccccccccccccc\n";
}

void 
Pointwise::pwd2_append(Piecewise<D2<SBasis> > A)
{
    size_t counter = 0;
    std::vector<std::pair<size_t,Satellite> > sats;
    Piecewise<D2<SBasis> > pwd2 = _pwd2;
    setPwd2(A);
    std::cout << A.size() << "ASIZE\n";
    std::cout << pwd2.size() << "PWD2SIZE\n";
    for(size_t i = 0; i < A.size(); i++){
        std::cout << i << "indes\n";
        std::cout << counter << "counter\n";
        if(pwd2.size() <= i-counter || !are_near(pwd2[i-counter].at0(),A[i].at0(),0.001)){
            counter++;
            bool isEndOpen = false;
            bool active = true;
            bool hidden = false;
            bool isTime = _satellites[0].second.getIsTime();
            bool mirror_knots = _satellites[0].second.getHasMirror();
            double amount = 0.0;
            double degrees = 0.0;
            int steps = 0;
            Satellite sat(_satellites[0].second.getSatelliteType(), isTime, isEndOpen, active, mirror_knots, hidden, amount, degrees, steps);
            sats.push_back(std::make_pair(i,sat));
        } else {
            sats.push_back(std::make_pair(i,_satellites[i-counter].second));
        }
    }
    setSatellites(sats);
}

void
Pointwise::pwd2_sustract(Piecewise<D2<SBasis> > A)
{
    size_t counter = 0;
    std::vector<std::pair<size_t,Satellite> > sats;
    for(size_t i = 0; i < _satellites.size(); i++){
        if(_satellites[i].second.getIsEndOpen()){
            _satellites.erase(_satellites.begin() + i);
        }
    }
    Piecewise<D2<SBasis> > pwd2 = _pwd2;
    setPwd2(A);
    for(size_t i = 0; i < _satellites.size(); i++){
        if(getLast(_satellites[i].first-counter) < _satellites[i].first-counter || !are_near(pwd2[_satellites[i].first].at0(),A[_satellites[i].first-counter].at0(),0.001)){
            counter++;
        } else {
            sats.push_back(std::make_pair(_satellites[i].first-counter,_satellites[i].second));
        }
    }
    setSatellites(sats);
}

void
Pointwise::set_extremes(bool endOpenSat,bool active, bool hidden, double amount, double angle)
{
    for(size_t i = 0; i < _pathInfo.size(); i++){
        size_t firstNode = getFirst(_pathInfo[i].first);
        size_t lastNode = getLast(_pathInfo[i].first);
        if(!getIsClosed(lastNode)){
            long lastIndex = -1;
            for(size_t j = 0; j < _satellites.size(); j++){
                if(_satellites[j].first < firstNode || _satellites[j].first > lastNode){
                    continue;
                }
                _satellites[j].second.setIsEndOpen(false);
                if(_satellites[j].first == firstNode){
                    _satellites[j].second.setActive(active);
                    _satellites[j].second.setHidden(hidden);
                    if(amount >= 0){
                        _satellites[j].second.setAmount(amount);
                    }
                    if(angle >= 0){
                        _satellites[j].second.setAngle(angle);
                    }
                }

                if(_satellites[j].first == lastNode && lastIndex != -1){
                    _satellites[j].second.setIsEndOpen(true);
                }

                if(_satellites[j].first == lastNode && lastIndex == -1){
                    lastIndex = j;
                }
            }
            if(endOpenSat){
                Satellite sat(_satellites[0].second.getSatelliteType(), _satellites[0].second.getIsTime(), true, active, _satellites[0].second.getHasMirror(), hidden, 0.0, 0.0, _satellites[0].second.getSteps());
                _satellites.insert(_satellites.begin() + lastIndex + 1, std::make_pair(lastNode,sat));
            }
        }
    }
}

void
Pointwise::reverse(size_t start,size_t end){
    for(size_t i = start; i < end / 2; i++){
      std::pair<size_t,Satellite> tmp = _satellites[i];
      _satellites[i] = _satellites[end - start - i - 1];
      _satellites[end - start - i - 1] = tmp;
    }
}

double 
Pointwise::rad_to_len(double A,  std::pair<size_t,Geom::Satellite> sat) const
{
    double len = 0;
    boost::optional<size_t> d2_prev_index = getPrevious(sat.first);
    if(d2_prev_index){
        Geom::D2<Geom::SBasis> d2_in =  _pwd2[*d2_prev_index];
        Geom::D2<Geom::SBasis> d2_out = _pwd2[sat.first];
        Piecewise<D2<SBasis> > offset_curve0 = Piecewise<D2<SBasis> >(d2_in)+rot90(unitVector(derivative(d2_in)))*(A);
        Piecewise<D2<SBasis> > offset_curve1 = Piecewise<D2<SBasis> >(d2_out)+rot90(unitVector(derivative(d2_out)))*(A);
        Geom::Path p0 = path_from_piecewise(offset_curve0, 0.1)[0];
        Geom::Path p1 = path_from_piecewise(offset_curve1, 0.1)[0];
        Geom::Crossings cs = Geom::crossings(p0, p1);
        if(cs.size() > 0){
            Point cp =p0(cs[0].ta);
            double p0pt = nearest_point(cp, d2_out);
            len = sat.second.toSize(p0pt,d2_out);
        } else {
            if(A > 0){
                len = rad_to_len(A * -1, sat);
            }
        }
    }
    return len;
}

double 
Pointwise::len_to_rad(double A,  std::pair<size_t,Geom::Satellite> sat) const
{
    boost::optional<size_t> d2_prev_index = getPrevious(sat.first);
    if(d2_prev_index){
        Geom::D2<Geom::SBasis> d2_in =  _pwd2[*d2_prev_index];
        Geom::D2<Geom::SBasis> d2_out = _pwd2[sat.first];
        double time_in = sat.second.getOpositeTime(A, d2_in);
        double time_out = sat.second.toTime(A,d2_out);
        Geom::Point startArcPoint = (d2_in).valueAt(time_in);
        Geom::Point endArcPoint = d2_out.valueAt(time_out);
        Piecewise<D2<SBasis> > u;
        u.push_cut(0);
        u.push((d2_in), 1);
        Geom::Curve * A = path_from_piecewise(u, 0.1)[0][0].duplicate();
        Piecewise<D2<SBasis> > u2;
        u2.push_cut(0);
        u2.push((d2_out), 1);
        Geom::Curve * B = path_from_piecewise(u2, 0.1)[0][0].duplicate();
        Curve *knotCurve1 = A->portion(0, time_in);
        Curve *knotCurve2 = B->portion(time_out, 1);
        Geom::CubicBezier const *cubic1 = dynamic_cast<Geom::CubicBezier const *>(&*knotCurve1);
        Ray ray1(startArcPoint, (d2_in).valueAt(1));
        if (cubic1) {
            ray1.setPoints((*cubic1)[2], startArcPoint);
        }
        Geom::CubicBezier const *cubic2 = dynamic_cast<Geom::CubicBezier const *>(&*knotCurve2);
        Ray ray2(d2_out.valueAt(0), endArcPoint);
        if (cubic2) {
            ray2.setPoints(endArcPoint, (*cubic2)[1]);
        }
        bool ccwToggle = cross((d2_in).valueAt(1) - startArcPoint, endArcPoint - startArcPoint) < 0;
        double distanceArc = Geom::distance(startArcPoint,middle_point(startArcPoint,endArcPoint));
        double angleBetween = angle_between(ray1, ray2, ccwToggle);
        double divisor = std::sin(angleBetween/2.0);
        if(divisor > 0){
            return distanceArc/divisor;
        }
    }
    return 0;
}

std::vector<size_t> 
Pointwise::findSatellites(size_t A, long B) const
{
    std::vector<size_t> ret;
    long counter = 0;
    for(size_t i = 0; i < _satellites.size(); i++){
        if(_satellites[i].first == A){
            if(counter >= B && B != (long)-1){
                return ret;
            }
            ret.push_back(i);
            counter++;
        }
    }
    return ret;
}

std::vector<size_t >
Pointwise::findPeviousSatellites(size_t A, long B) const
{
    boost::optional<size_t> previous = getPrevious(A);
    std::vector<size_t> ret;
    if(previous){
        ret = findSatellites(*previous,B);
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

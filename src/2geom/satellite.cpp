/**
 * \file
 * \brief Satellite
 *//*
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
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
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
 */

#include <2geom/satellite.h>
#include <2geom/curve.h>
#include <2geom/nearest-point.h>
#include <2geom/sbasis-geometric.h>


namespace Geom {

Satellite::Satellite(){};

Satellite::Satellite(SatelliteType satellitetype, bool isTime, bool active, bool hasMirror, bool hidden, double size, double time)
        : _satellitetype(satellitetype), _isTime(isTime), _active(active), _hasMirror(hasMirror), _hidden(hidden), _size(size), _time(time){};

Satellite::~Satellite() {};

void
Satellite::setPosition(Geom::Point p, Geom::D2<Geom::SBasis> d2_in)
{
    if(!d2_in.isFinite() ||  d2_in.isZero()){
        _time = 0;
        _size = 0;
        return;
    }
    double lenghtPart = Geom::length(d2_in, Geom::EPSILON);
    _time =  Geom::nearest_point(p, d2_in);
    if (d2_in[0].degreesOfFreedom() != 2) {
        Geom::Piecewise<Geom::D2<Geom::SBasis> > u;
        u.push_cut(0);
        u.push(d2_in, 1);
        u = Geom::portion(u, 0.0, _time);
        _size = Geom::length(u, 0.001);
    } else {
        _size = (_time * lenghtPart);
    }
    if(_time > 0.998){
        _time = 1;
    }
}

double
Satellite::getOpositeTime(Geom::D2<Geom::SBasis> d2_in)
{
    if(!d2_in.isFinite() ||  d2_in.isZero()){
        return 1;
    }
    double t = 0;
    double lenghtPart = Geom::length(d2_in, Geom::EPSILON);
    if(_size == 0){
        return 1;
    }
    double size = lenghtPart - _size;
    if (d2_in[0].degreesOfFreedom() != 2) {
        Geom::Piecewise<Geom::D2<Geom::SBasis> > u;
        u.push_cut(0);
        u.push(d2_in, 1);
        std::vector<double> t_roots = roots(arcLengthSb(u) - size);
        if (t_roots.size() > 0) {
            t = t_roots[0];
        }
    } else {
        if (size < lenghtPart && lenghtPart != 0) {
            t = size / lenghtPart;
        }
    }
    if(_time > 0.999){
        _time = 1;
    }
    return t;
}

void 
Satellite::updateSizeTime(Geom::D2<Geom::SBasis> d2_in)
{
    if(!d2_in.isFinite() ||  d2_in.isZero()){
        _time = 0;
        _size = 0;
        return;
    }
    double lenghtPart = Geom::length(d2_in, Geom::EPSILON);
    if(!_isTime){
        if (_size != 0) {
            if(lenghtPart <= _size){
                _time = 1;
                _size = lenghtPart;
            } else if (d2_in[0].degreesOfFreedom() != 2) {
                Piecewise<D2<SBasis> > u;
                u.push_cut(0);
                u.push(d2_in, 1);
                std::vector<double> t_roots = roots(arcLengthSb(u) - _size);
                if (t_roots.size() > 0) {
                    _time = t_roots[0];
                }
            } else {
                if (_size < lenghtPart && lenghtPart != 0) {
                    _time = _size / lenghtPart;
                }
            }
        } else {
            _time = 0;
        }
    } else {
        if (_time != 0) {
            if (d2_in[0].degreesOfFreedom() != 2) {
                Piecewise<D2<SBasis> > u;
                u.push_cut(0);
                u.push(d2_in, 1);
                u = portion(u, 0, _time);
                _size = length(u, 0.001);
            } else {
                lenghtPart = length(d2_in, EPSILON);
                _size = _time * lenghtPart;
            }
        } else {
            _size = 0;
        }
    }
    if(_time > 0.999){
        _time = 1;
    }
}

Geom::Point 
Satellite::getPosition(Geom::D2<Geom::SBasis> d2_in){
    if(!d2_in.isFinite() ||  d2_in.isZero()){
        return Geom::Point(9999999999.0,9999999999.0);
    }
    updateSizeTime(d2_in);
    return d2_in.valueAt(_time);
}

} // end namespace Geom

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

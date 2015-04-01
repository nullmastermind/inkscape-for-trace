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

#include <helper/geom-satellite.h>
#include <2geom/curve.h>
#include <2geom/nearest-point.h>
#include <2geom/sbasis-geometric.h>
#include <2geom/path-intersection.h>
#include <2geom/sbasis-to-bezier.h>
#include <2geom/ray.h>
#include <boost/optional.hpp>


namespace Geom {

Satellite::Satellite(){};

Satellite::Satellite(SatelliteType satelliteType, bool isTime, bool active, bool hasMirror, bool hidden, double amount, double angle, size_t steps)
        : satelliteType(satelliteType), isTime(isTime), active(active), hasMirror(hasMirror), hidden(hidden), amount(amount), angle(angle), steps(steps){};

Satellite::~Satellite() {};

double
Satellite::toTime(double A,Geom::D2<Geom::SBasis> d2_in) const
{
    if(!d2_in.isFinite() ||  d2_in.isZero() || A == 0){
        return 0;
    }
    double t = 0;
    double lenghtPart = Geom::length(d2_in, Geom::EPSILON);
    if (A > lenghtPart || d2_in[0].degreesOfFreedom() == 2) {
        if (lenghtPart != 0) {
            t = A / lenghtPart;
        }
    } else if (d2_in[0].degreesOfFreedom() != 2) {
        Geom::Piecewise<Geom::D2<Geom::SBasis> > u;
        u.push_cut(0);
        u.push(d2_in, 1);
        std::vector<double> t_roots = roots(arcLengthSb(u) - A);
        if (t_roots.size() > 0) {
            t = t_roots[0];
        }
    }

    return t;
}

double
Satellite::toSize(double A,Geom::D2<Geom::SBasis> d2_in) const
{
    if(!d2_in.isFinite() ||  d2_in.isZero() || A == 0){
        return 0;
    }
    double s = 0;
    double lenghtPart = Geom::length(d2_in, Geom::EPSILON);
    if (A > lenghtPart || d2_in[0].degreesOfFreedom() == 2) {
        s = (A * lenghtPart);
    } else if (d2_in[0].degreesOfFreedom() != 2) {
        Geom::Piecewise<Geom::D2<Geom::SBasis> > u;
        u.push_cut(0);
        u.push(d2_in, 1);
        u = Geom::portion(u, 0.0, A);
        s = Geom::length(u, 0.001);
    }
    return s;
}


double 
Satellite::rad_to_len(double A, boost::optional<Geom::D2<Geom::SBasis> > d2_in, Geom::D2<Geom::SBasis> d2_out, boost::optional<Geom::Satellite> previousSatellite) const
{
    double len = 0;
    if(d2_in && previousSatellite){
        Piecewise<D2<SBasis> > offset_curve0 = Piecewise<D2<SBasis> >(*d2_in)+rot90(unitVector(derivative(*d2_in)))*(A);
        Piecewise<D2<SBasis> > offset_curve1 = Piecewise<D2<SBasis> >(d2_out)+rot90(unitVector(derivative(d2_out)))*(A);
        Geom::Path p0 = path_from_piecewise(offset_curve0, 0.1)[0];
        Geom::Path p1 = path_from_piecewise(offset_curve1, 0.1)[0];
        Geom::Crossings cs = Geom::crossings(p0, p1);
        if(cs.size() > 0){
            Point cp =p0(cs[0].ta);
            double p0pt = nearest_point(cp, d2_out);
            len = (*previousSatellite).toSize(p0pt,d2_out);
        } else {
            if(A > 0){
                len = rad_to_len(A * -1, *d2_in, d2_out, previousSatellite);
            }
        }
    }
    return len;
}

double 
Satellite::len_to_rad(double A, boost::optional<Geom::D2<Geom::SBasis> > d2_in, Geom::D2<Geom::SBasis> d2_out, boost::optional<Geom::Satellite> previousSatellite) const
{
    if(d2_in && previousSatellite){
        double time_in = (*previousSatellite).getOpositeTime(A, *d2_in);
        double time_out = (*previousSatellite).toTime(A,d2_out);
        Geom::Point startArcPoint = (*d2_in).valueAt(time_in);
        Geom::Point endArcPoint = d2_out.valueAt(time_out);
        Piecewise<D2<SBasis> > u;
        u.push_cut(0);
        u.push(*d2_in, 1);
        Geom::Curve * C = path_from_piecewise(u, 0.1)[0][0].duplicate();
        Piecewise<D2<SBasis> > u2;
        u2.push_cut(0);
        u2.push(d2_out, 1);
        Geom::Curve * D = path_from_piecewise(u2, 0.1)[0][0].duplicate();
        Curve *knotCurve1 = C->portion(0, time_in);
        Curve *knotCurve2 = D->portion(time_out, 1);
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

double
Satellite::getOpositeTime(double s, Geom::D2<Geom::SBasis> d2_in) const
{
    if(s == 0){
        return 1;
    }
    double lenghtPart = Geom::length(d2_in, Geom::EPSILON);
    double size = lenghtPart - s;
    return toTime(size, d2_in);
}

double
Satellite::getTime(Geom::D2<Geom::SBasis> d2_in) const
{
    double t = amount;
    if(!isTime){
        t = toTime(t, d2_in);
    }
    if(t > 1){
        t = 1;
    }
    return t;
}

double
Satellite::getSize(Geom::D2<Geom::SBasis> d2_in) const
{
    double s = amount;
    if(isTime){
        s = toSize(s, d2_in);
    }
    return s;
}


Geom::Point 
Satellite::getPosition(Geom::D2<Geom::SBasis> d2_in) const
{
    double t = getTime(d2_in);
    return d2_in.valueAt(t);
}

void
Satellite::setPosition(Geom::Point p, Geom::D2<Geom::SBasis> d2_in)
{
    double A = Geom::nearest_point(p, d2_in);
    if(!isTime){
        A = toSize(A, d2_in);
    }
    amount = A;
}

void 
Satellite::setSatelliteType(gchar const * A)
{
    std::map<std::string,SatelliteType> GcharMapToSatelliteType = boost::assign::map_list_of("F", F)("IF", IF)("C",C)("IC",IC)("KO",KO);
    satelliteType = GcharMapToSatelliteType.find(std::string(A))->second;
}

gchar const *
Satellite::getSatelliteTypeGchar() const
{
    std::map<SatelliteType,gchar const *> SatelliteTypeToGcharMap = boost::assign::map_list_of(F, "F")(IF, "IF")(C,"C")(IC,"IC")(KO,"KO");
    return SatelliteTypeToGcharMap.at(satelliteType);
}

} //namespace Geom

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

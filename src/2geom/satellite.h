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

#ifndef LIB2GEOM_SEEN_SATELLITE_H
#define LIB2GEOM_SEEN_SATELLITE_H

#include <2geom/satellite-enum.h>

namespace Geom {

const Util::EnumData<SatelliteType> SATELLITETypeData[] = {
    // {constant defined in satellite-enum.h, N_("name of satellite_type"), "name of your satellite type on SVG"}
/* 0.92 */
    {FILLET,                N_("Fillet"),               "fillet"},
    {INVERSE_FILLET,        N_("Inverse Fillet"),       "inverse_fillet"},
    {CHAMFER,               N_("Chamfer"),              "chamfer"},
    {INVERSE_CHAMFER,       N_("Inverse Chamfer"),      "inverse_chamfer"},
    {INTERPOLATE_POINTS,    N_("Interpolate points"),   "interpolate_points"},
};
const Util::EnumDataConverter<SatelliteType> SATELLITETypeConverter(SATELLITETypeData, sizeof(SATELLITETypeData)/sizeof(*SATELLITETypeData));


class Satellite
{
  public:
    Satellite()
    {}

    virtual ~Sattelite();

    Satellite(SatelliteType satellitetype, bool isTime, bool active, bool mirror, bool after, bool hidden, double size, double time)
        : _satellitetype(satellitetype), _time(time), _active(active), _mirror(mirror), _after(after), _hidden(hidden), _size(size), _time(time)
    {
    }

    void setSatelliteType(SatelliteType A)
    {
        _satellitetype = A;
    }

    void setActive(bool A)
    {
        _active = A;
    }

    void setHasMirror(bool A)
    {
        _mirror = A;
    }

    void setHidden(bool A)
    {
        _hidden = A;
    }

    void setTime(double A)
    {
        _isTime = true;
        _time = A;
    }

    void setSize(double A)
    {
        _isTime = false;
        _size = A;
    }

    SatelliteType satellitetype() const
    {
        return _ts;
    }

    bool isTime() const
    {
        return _isTime;
    }

    bool active() const
    {
        return _active;
    }

    bool hasMirror() const
    {
        return _mirror;
    }

    bool hidden() const
    {
        return _hidden;
    }

    double size() const
    {
        return _size;
    }

    double time() const
    {
        return _time;
    }

    double time(D2<SBasis> curve) const
    {
        //todo make the process
        return _time;
    }

    void setPosition(Geom::Point p, D2<SBasis> curve){
        _time = Geom::nearestPoint(p, curve);
        if(!_isTime){
            if (curve.degreesOfFreedom() != 2) {
                Piecewise<D2<SBasis> > u;
                u.push_cut(0);
                u.push(curve, 1);
                u = portion(u, 0, _time);
                _size = length(u, 0.001) * -1;
            } else {
                lenghtPart = length(last_pwd2[index], EPSILON);
                _size = (time * lenghtPart) * -1;
            }
        }
    }

    Geom::Point getPosition(D2<SBasis> curve){
       return curve.pointAt(_time);
    }

    bool isDegenerate() const
    {
        return _size = 0 && _time = 0;
    }

  private:
    SatelliteType _satellitetype;
    bool _isTime;
    bool _active;
    bool _hasMirror;
    bool _hidden;
    double _size;
    double _time;
};

} // end namespace Geom

#endif // LIB2GEOM_SEEN_SATELLITE_H

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

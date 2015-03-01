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
#include <2geom/d2.h>
#include <map>
#include <boost/assign.hpp>

namespace Geom {

class Satellite
{
  public:

    Satellite();
    Satellite(SatelliteType satellitetype, bool isTime, bool isClosing, bool isStart, bool active, bool hasMirror, bool hidden, double ammount, double angle);

    virtual ~Satellite();

    void setSatelliteType(SatelliteType A)
    {
        _satellitetype = A;
    }

    void setSatelliteType(gchar const * A)
    {
        std::map<gchar const *,SatelliteType> GcharMapToSatelliteType = boost::assign::map_list_of("F", F)("IF", IF)("C",C)("IC",IC)("KO",KO);
        _satellitetype = GcharMapToSatelliteType[A];
    }

    void setIsTime(bool A)
    {
        _isTime = A;
    }

    void setIsClosing(bool A)
    {
        _isClosing = A;
    }

    void setIsStart(bool A)
    {
        _isStart = A;
    }

    void setActive(bool A)
    {
        _active = A;
    }

    void setHasMirror(bool A)
    {
        _hasMirror = A;
    }

    void setHidden(bool A)
    {
        _hidden = A;
    }
    void setAmmount(double A)
    {
        _ammount = A;
    }

    void setAngle(double A)
    {
        _angle = A;
    }

    SatelliteType getSatelliteType() const
    {
        return _satellitetype;
    }

    gchar const * getSatelliteTypeGchar() const
    {
        std::map<SatelliteType,gchar const *> SatelliteTypeToGcharMap = boost::assign::map_list_of(F, "F")(IF, "IF")(C,"C")(IC,"IC")(KO,"KO");
        return SatelliteTypeToGcharMap[_satellitetype];
    }

    bool getIsTime() const
    {
        return _isTime;
    }

    bool getIsClosing() const
    {
        return _isClosing;
    }

    bool getIsStart() const
    {
        return _isStart;
    }

    bool getActive() const
    {
        return _active;
    }

    bool getHasMirror() const
    {
        return _hasMirror;
    }

    bool getHidden() const
    {
        return _hidden;
    }

    double getAmmount() const
    {
        return _ammount;
    }

    double getAngle() const
    {
        return _angle;
    }

    void setPosition(Geom::Point p, Geom::D2<Geom::SBasis> d2_in);
    Geom::Point getPosition(Geom::D2<Geom::SBasis> curve);
    double getTime(Geom::D2<Geom::SBasis> d2_in);
    double getOpositeTime(double A,Geom::D2<Geom::SBasis> SBasisCurve);
    double toSize(double A,Geom::D2<Geom::SBasis> d2_in);
    double toTime(double A,Geom::D2<Geom::SBasis> d2_in);

  private:

    SatelliteType _satellitetype;
    bool _isTime;
    bool _isClosing;
    bool _isStart;
    bool _active;
    bool _hasMirror;
    bool _hidden;
    double _ammount;
    double _angle;
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

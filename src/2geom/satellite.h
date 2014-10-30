/**
 * \file
 * \brief Satellite
 *//*
 * Authors:
 *
 * Copyright 2014  authors
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

namespace Geom {

class Satellite
{
  public:
    Satellite()
    {}

    virtual ~Sattelite();

    Satellite(typeSatellite ts, bool isTime, bool active, bool after, bool hidden, double size, double time)
        : _ts(ts), _time(time), _active(active), _after(after), _hidden(hidden), _size(size), _time(time)
    {
    }

    void setTypeSatellite(typeSatellite A)
    {
        _ts = A;
    }

    void setActive(bool A)
    {
        _active = A;
    }

    void setAfter(bool A)
    {
        _after = A;
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

    typeSatellite ts() const
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

    bool after() const
    {
        return _after;
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

    bool isDegenerate() const
    {
        return _size = 0 && _time = 0;
    }

  private:
    typeSatellite _ts;
    bool _isTime;
    bool _active;
    bool _after;
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

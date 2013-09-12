/*  This file is part of the libdepixelize project
    Copyright (C) 2013 Vinícius dos Santos Oliveira <vini.ipsmaker@gmail.com>

    GNU Lesser General Public License Usage
    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by the
    Free Software Foundation; either version 2.1 of the License, or (at your
    option) any later version.
    You should have received a copy of the GNU Lesser General Public License
    along with this library.  If not, see <http://www.gnu.org/licenses/>.

    GNU General Public License Usage
    Alternatively, this library may be used under the terms of the GNU General
    Public License as published by the Free Software Foundation, either version
    2 of the License, or (at your option) any later version.
    You should have received a copy of the GNU General Public License along with
    this library.  If not, see <http://www.gnu.org/licenses/>.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
*/

#ifndef LIBDEPIXELIZE_TRACER_UTILS_H
#define LIBDEPIXELIZE_TRACER_UTILS_H

#include "pixelgraph.h"
#include "simplifiedvoronoi.h"
#include <iostream>
#include <ios>
#include <iomanip>

namespace Tracer {
namespace Utils {

template<class T>
Splines to_splines(const SimplifiedVoronoi<T> &diagram)
{
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\""
        " width=\"" << diagram.width()
        << "px\" height=\"" << diagram.height()
        << "px\">";

    for ( typename SimplifiedVoronoi<T>::const_iterator it = diagram.begin()
              , end = diagram.end() ; it != end ; ++it ) {
        typename std::vector< Point<T> >::const_iterator
            it2 = it->vertices.begin(), end2 = it->vertices.end();
        if ( it2 == end2 )
            continue;

        svg << "<path d=\"M";

        for ( ; it2 != end2 ; ++it2 )
            svg << ' ' << it2->x << ' ' << it2->y;

        std::hex(svg);
        svg << " z\" fill=\"#"
            << std::setfill('0')
            << std::setw(2) << int(it->rgba[0])
            << std::setw(2) << int(it->rgba[1])
            << std::setw(2) << int(it->rgba[2])
            << std::setw(0) << std::setfill(' ') << "\" fill-opacity=\""
            << double(it->rgba[3]) / 255.;
        std::dec(svg);

        svg << "\" />";
    }

    svg << "</svg>" << std::endl;
}

} // namespace Utils
} // namespace Tracer

#endif // LIBDEPIXELIZE_TRACER_UTILS_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :

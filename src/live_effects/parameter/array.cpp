/*
 * Copyright (C) Johan Engelen 2008 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/parameter/array.h"

#include "helper-fns.h"

#include <2geom/coord.h>
#include <2geom/point.h>

namespace Inkscape {

namespace LivePathEffect {

template <>
double
ArrayParam<double>::readsvg(const gchar * str)
{
    double newx = Geom::infinity();
    sp_svg_number_read_d(str, &newx);
    return newx;
}

template <>
float
ArrayParam<float>::readsvg(const gchar * str)
{
    float newx = Geom::infinity();
    sp_svg_number_read_f(str, &newx);
    return newx;
}

template <>
Geom::Point
ArrayParam<Geom::Point>::readsvg(const gchar * str)
{
    gchar ** strarray = g_strsplit(str, ",", 2);
    double newx, newy;
    unsigned int success = sp_svg_number_read_d(strarray[0], &newx);
    success += sp_svg_number_read_d(strarray[1], &newy);
    g_strfreev (strarray);
    if (success == 2) {
        return Geom::Point(newx, newy);
    }
    return Geom::Point(Geom::infinity(),Geom::infinity());
}
//TODO: move maybe to svg-lenght.cpp
unsigned int
sp_svg_satellite_read_d(gchar const *str, Geom::Satellite *sat){
    if (!str) {
        return 0;
    }
    gchar ** strarray = g_strsplit(str, "*", 0);
    if(strarray[6] && !strarray[7]){
        std::map< gchar const *, Geom::SatelliteType> gts = sat->GcharMapToSatelliteType;
        sat->setSatelliteType(gts[strarray[0]]);
        sat->setIsTime(helperfns_read_bool(strarray[1], true));
        sat->setActive(helperfns_read_bool(strarray[2], true));
        sat->setHasMirror(helperfns_read_bool(strarray[3], false));
        sat->setHidden(helperfns_read_bool(strarray[4], false));
        double time,size;
        sp_svg_number_read_d(strarray[5], &time);
        sp_svg_number_read_d(strarray[6], &size);
        sat->setTime(time);
        sat->setSize(size);
        g_strfreev (strarray);
        return 1;
    }
    g_strfreev (strarray);
    return 0;
}

template <>
std::pair<int, Geom::Satellite>
ArrayParam<std::pair<int, Geom::Satellite> >::readsvg(const gchar * str)
{
    gchar ** strarray = g_strsplit(str, ",", 2);
    double index;
    std::pair<int, Geom::Satellite> result;
    unsigned int success = (int)sp_svg_number_read_d(strarray[0], &index);
    Geom::Satellite sat;
    success += sp_svg_satellite_read_d(strarray[1], &sat);
    g_strfreev (strarray);
    if (success == 2) {
        return std::make_pair(index, sat);
    }
    return std::make_pair((int)Geom::infinity(),sat);
}

} /* namespace LivePathEffect */

} /* namespace Inkscape */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

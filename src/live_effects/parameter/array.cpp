/*
 * Copyright (C) Johan Engelen 2008 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/parameter/array.h"

#include "svg/svg.h"
#include "svg/stringstream.h"

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
void
sp_svg_satellite_read_d(gchar const *str, satellite *sat){
    gchar ** strarray = g_strsplit(str, "*", 0);
    if(strarray.size() != 6){
        g_strfreev (strarray);
        return NULL;
    }
    sat->setSatelliteType(SatelliteTypeMap[strarray[0]]);
    sat->setActive(helperfns_read_bool(strarray[1], true));
    sat->sethasMirror(helperfns_read_bool(strarray[2], false));
    sat->setHidden(helperfns_read_bool(strarray[3], false));
    sat->setHidden(helperfns_read_bool(strarray[3], false));
    sat->setTime(sp_svg_number_read_d(strarray[4], 0.0));
    sat->setSize(sp_svg_number_read_d(strarray[5], 0.0));
    g_strfreev (strarray);
}

template <>
std::pair<int, satellite>
ArrayParam<Geom::Point>::readsvg(const gchar * str)
{
    gchar ** strarray = g_strsplit(str, ",", 2);
    int index;
    Geom::satellite sat = NULL;
    unsigned int success = sp_svg_number_read_d(strarray[0], &index);
    success += sp_svg_satellite_read_d(strarray[1], &sat);
    g_strfreev (strarray);
    if (success == 2) {
        return std::pair<index, sat>;
    }
    return std::pair<Geom::infinity(),NULL>;
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

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

//TODO: move maybe
unsigned int
sp_svg_satellite_vector_read_d(gchar const *str, std::vector<Satellite> *subpath_satellites){
    if (!str) {
        return 0;
    }
    gchar ** strarray = g_strsplit(str, " @ ", 0);
    gchar ** iter = strarray;
    while (*iter != NULL) {
        gchar ** strsubarray = g_strsplit(*iter, ",", 7);
        if(strlen(str) > 0 && strsubarray[6] && !strsubarray[7]){
            Satellite satellite;
            satellite.setSatelliteType(g_strstrip(strsubarray[0]));
            satellite.is_time = strncmp(strsubarray[1],"1",1) == 0;
            satellite.has_mirror = strncmp(strsubarray[2],"1",1) == 0;
            satellite.hidden = strncmp(strsubarray[3],"1",1) == 0;
            double amount,angle;
            float stepsTmp;
            sp_svg_number_read_d(strsubarray[4], &amount);
            sp_svg_number_read_d(strsubarray[5], &angle);
            sp_svg_number_read_f(strsubarray[6], &stepsTmp);
            unsigned int steps = (unsigned int)stepsTmp;
            satellite.amount = amount;
            satellite.angle = angle;
            satellite.steps = steps;
            subpath_satellites->push_back(satellite);
            g_strfreev (strsubarray);
        }
        iter++;
    }
    g_strfreev (strarray);
    if (!subpath_satellites->empty()){
        return 1;
    }
    return 0;
}


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


template <>
std::vector<Satellite>
ArrayParam<std::vector<Satellite > >::readsvg(const gchar * str)
{
    std::vector<Satellite > subpath_satellites;
    if (sp_svg_satellite_vector_read_d(str, &subpath_satellites)) {
        return subpath_satellites;
    }
    subpath_satellites.push_back(Satellite(FILLET));
    return subpath_satellites;
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

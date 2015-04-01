#ifndef LIB2GEOM_SEEN_SATELLITE_ENUM_H
#define LIB2GEOM_SEEN_SATELLITE_ENUM_H

/**
 * \file
 * \brief Satellite types enum
 */ /*
    * Authors:
    * 2015 Jabier Arraiza Cenoz<jabier.arraiza@marker.es>
    *
    * This code is in public domain
    */

#include "util/enums.h"

namespace Geom {

enum SatelliteType {
    F = 0, //Fillet
    IF,    //Inverse Fillet
    C,     //Chamfer
    IC,    //Inverse Chamfer
    KO     // Invalid Satellite)
};

} //namespace Geom

#endif

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

#ifndef LIB2GEOM_SEEN_SATELLITE_ENUM_H
#define LIB2GEOM_SEEN_SATELLITE_ENUM_H

/*
 *
 *
* Copyright (C) Jabier Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "util/enums.h"
/*#include <boost/bimap.hpp>
*/

namespace Geom {

enum SatelliteType {
    F=0, //Fillet
    IF, //Inverse Fillet
    C, //Chamfer
    IC, //Inverse Chamfer
    KO // Invalid Satellite)
};

/* TODO maybe is best do next by bimap
    typedef boost::bimap< Geom::SatelliteType,gchar const *> map_type ;

    map_type SatelliteTypeBimap;

    SatelliteTypeBimap.insert( map_type::value_type(FILLET, "FILLET"));
    SatelliteTypeBimap.insert( map_type::value_type(INVERSE_FILLET, "INVERSE_FILLET"));
    SatelliteTypeBimap.insert( map_type::value_type(CHAMFER, "CHAMFER")) );
    SatelliteTypeBimap.insert( map_type::value_type(INVERSE_CHAMFER, "INVERSE_CHAMFER"));
    SatelliteTypeBimap.insert( map_type::value_type(INVALID_SATELLITE, "INVALID_SATELLITE"));
*/

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

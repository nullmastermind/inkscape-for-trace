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
    FILLET=0,
    INVERSE_FILLET,
    CHAMFER,
    INVERSE_CHAMFER,
    INVALID_SATELLITE // This must be last)
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

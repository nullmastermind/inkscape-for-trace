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

namespace Geom {

enum SatelliteType {
    FILLET=0,
    INVERSE_FILLET,
    CHAMFER,
    INVERSE_CHAMFER,
    INVALID_SATELLITE // This must be last (I made it such that it is not needed anymore I think..., Don't trust on it being last. - johan)
};
std::map<gchar const *value, SatelliteType> SatelliteTypeMap = boost::map_list_of("FILLET", FILLET)("INVERSE_FILLET", INVERSE_FILLET)("CHAMFER",CHAMFER)("INVERSE_CHAMFER",INVERSE_CHAMFER)("INVALID_SATELLITE",INVALID_SATELLITE);
extern const Util::EnumData<SatelliteType> SATELLITETypeData[];  /// defined in satelite.cpp
extern const Util::EnumDataConverter<SateliteType> SATELLITETypeConverter; /// defined in sattelite.cpp

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

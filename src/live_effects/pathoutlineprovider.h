#ifndef _SEEN_PATH_OUTLINE_H
#define _SEEN_PATH_OUTLINE_H

#include <livarot/Path.h>
#include <livarot/LivarotDefs.h>

enum LineJoinType {
    LINEJOIN_STRAIGHT,
    LINEJOIN_ROUND,
    LINEJOIN_POINTY,
    LINEJOIN_REFLECTED,
    LINEJOIN_EXTRAPOLATED
};

namespace Outline 
{
    unsigned bezierOrder (const Geom::Curve* curve_in);
    std::vector<Geom::Path> PathVectorOutline(std::vector<Geom::Path> const & path_in, double line_width, ButtType linecap_type,
                                              LineJoinType linejoin_type, double miter_limit);

    /*Geom::PathVector outlinePath(const Geom::PathVector& path_in, double line_width, LineJoinType join,
                                 ButtType butt, double miter_lim, bool extrapolate = false);*/
    Geom::Path PathOutsideOutline(Geom::Path const & path_in, double line_width, LineJoinType linejoin_type, double miter_limit);
}

#endif // _SEEN_PATH_OUTLINE_H

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
enum ButtTypeMod {
    BUTT_STRAIGHT,
    BUTT_ROUND,
    BUTT_SQUARE,
    BUTT_POINTY,
    BUTT_LEANED
};
namespace Geom
{
	Geom::CubicBezier sbasis_to_cubicbezier(Geom::D2<Geom::SBasis> const & sbasis_in);
	std::vector<Geom::Path> split_at_cusps(const Geom::Path& in);
}

namespace Outline 
{
    unsigned bezierOrder (const Geom::Curve* curve_in);
    std::vector<Geom::Path> PathVectorOutline(std::vector<Geom::Path> const & path_in, double line_width, ButtTypeMod linecap_type,
                                              LineJoinType linejoin_type, double miter_limit, double start_lean = 0, double end_lean = 0);

    /*Geom::PathVector outlinePath(const Geom::PathVector& path_in, double line_width, LineJoinType join,
                                 ButtTypeMod butt, double miter_lim, bool extrapolate = false);*/
    Geom::Path PathOutsideOutline(Geom::Path const & path_in, double line_width, LineJoinType linejoin_type, double miter_limit);
}

#endif // _SEEN_PATH_OUTLINE_H

/** \file
 * LPE "Transform through 2 points" implementation
 */

/*
 * Authors:
 *   
 *
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/lpe-transform_2pts.h"

namespace Inkscape {
namespace LivePathEffect {

LPETransform2Pts::LPETransform2Pts(LivePathEffectObject *lpeobject) :
    Effect(lpeobject)
{
}

LPETransform2Pts::~LPETransform2Pts()
{
}

std::vector<Geom::Path>
LPETransform2Pts::doEffect_path (std::vector<Geom::Path> const & path_in)
{
    return path_in;
}

/* ######################## */

} //namespace LivePathEffect
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

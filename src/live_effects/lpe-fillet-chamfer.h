#ifndef INKSCAPE_LPE_FILLET_CHAMFER_H
#define INKSCAPE_LPE_FILLET_CHAMFER_H

/*
 * Author(s):
 *     Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2014 Author(s)
 *
 * Special thanks to Johan Engelen for the base of the effect -powerstroke-
 * Also to ScislaC for point me to the idea
 * Also su_v for his construvtive feedback and time
 * and finaly to Liam P. White for his big help on coding, that save me a lot of hours
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#if defined(GLIBMM_DISABLE_DEPRECATED) && defined(HAVE_GLIBMM_THREADS_H)
# include <glibmm/threads.h>
#endif

#include "live_effects/parameter/enum.h"
#include "live_effects/parameter/bool.h"
#include "live_effects/parameter/unit.h"

#include "live_effects/parameter/pointwise.h"
#include "live_effects/effect.h"

namespace Inkscape {
namespace LivePathEffect {


class LPEFilletChamfer : public Effect {
public:
    LPEFilletChamfer(LivePathEffectObject *lpeobject);
    virtual ~LPEFilletChamfer();

    virtual void doOnApply(SPLPEItem const *lpeItem);

    PointwiseArrayParam pointwise_values;

private:

    LPEFilletChamfer(const LPEFilletChamfer &);
    LPEFilletChamfer &operator=(const LPEFilletChamfer &);

};

} //namespace LivePathEffect
} //namespace Inkscape

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

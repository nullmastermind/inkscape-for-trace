#ifndef SP_RADIAL_GRADIENT_FNS_H
#define SP_RADIAL_GRADIENT_FNS_H

/** \file
 * Macros and fn definitions related to radial gradients.
 */

#include <glib-object.h>

#include "sp-radial-gradient.h"

namespace Inkscape {
namespace XML {
class Node;
}
}

struct SPRadialGradient;

#define SP_TYPE_RADIALGRADIENT (sp_radialgradient_get_type())
#define SP_RADIALGRADIENT(obj) ((SPRadialGradient*)obj)
#define SP_IS_RADIALGRADIENT(obj) (dynamic_cast<const SPRadialGradient*>((SPObject*)obj))

GType sp_radialgradient_get_type();

void sp_radialgradient_set_position(SPRadialGradient *rg, gdouble cx, gdouble cy, gdouble fx, gdouble fy, gdouble r);

#endif /* !SP_RADIAL_GRADIENT_FNS_H */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

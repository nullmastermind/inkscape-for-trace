#ifndef SP_LINEAR_GRADIENT_FNS_H
#define SP_LINEAR_GRADIENT_FNS_H

/** \file 
 * Macros and fn declarations related to linear gradients.
 */

#include <glib-object.h>
#include <glib.h>

#include "sp-linear-gradient.h"

namespace Inkscape {
namespace XML {
class Node;
}
}

struct SPLinearGradient;

#define SP_TYPE_LINEARGRADIENT (sp_lineargradient_get_type())
#define SP_LINEARGRADIENT(obj) ((SPLinearGradient*)obj)
#define SP_IS_LINEARGRADIENT(obj) (dynamic_cast<const SPLinearGradient*>((SPObject*)obj))

GType sp_lineargradient_get_type();

void sp_lineargradient_set_position(SPLinearGradient *lg, gdouble x1, gdouble y1, gdouble x2, gdouble y2);

#endif /* !SP_LINEAR_GRADIENT_FNS_H */

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

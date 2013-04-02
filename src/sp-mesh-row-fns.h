#ifndef SP_MESH_ROW_FNS_H
#define SP_MESH_ROW_FNS_H

/** \file
 * Macros and fn definitions related to mesh rows.
 */

#include <glib-object.h>

namespace Inkscape {
namespace XML {
class Node;
}
}

class SPMeshRow;

#define SP_TYPE_MESHROW (sp_meshrow_get_type())
#define SP_MESHROW(obj) ((SPMeshRow*)obj)
#define SP_IS_MESHROW(obj) (dynamic_cast<const SPMeshRow*>((SPObject*)obj))

GType sp_meshrow_get_type();


#endif /* !SP_MESH_ROW_FNS_H */

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

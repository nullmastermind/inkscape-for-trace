#ifndef SP_MESH_PATCH_FNS_H
#define SP_MESH_PATCH_FNS_H

/** \file
 * Macros and fn definitions related to mesh patchs.
 */

#include <glib-object.h>

namespace Inkscape {
namespace XML {
class Node;
}
}

class SPMeshPatch;

#define SP_TYPE_MESHPATCH (sp_meshpatch_get_type())
#define SP_MESHPATCH(obj) ((SPMeshPatch*)obj)
#define SP_IS_MESHPATCH(obj) (dynamic_cast<const SPMeshPatch*>((SPObject*)obj))

GType sp_meshpatch_get_type();


#endif /* !SP_MESH_PATCH_FNS_H */

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

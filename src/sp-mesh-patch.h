#ifndef SEEN_SP_MESHPATCH_H
#define SEEN_SP_MESHPATCH_H

/** \file
 * SPMeshPatch: SVG <meshpatch> implementation.
 */
/*
 * Authors: Tavmjong Bah
 *
 * Copyright (C) 2012 Tavmjong Bah
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <glib.h>
#include <glibmm/ustring.h>
//#include "svg/svg-length.h"
#include "sp-object.h"

class SPObjectClass;

struct SPMeshPatch;
struct SPMeshPatchClass;

#define SP_TYPE_MESHPATCH (sp_meshpatch_get_type())
#define SP_MESHPATCH(obj) ((SPMeshPatch*)obj)
#define SP_IS_MESHPATCH(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPMeshPatch)))

GType sp_meshpatch_get_type();

class CMeshPatch;

/** Gradient MeshPatch. */
class SPMeshPatch : public SPObject {
public:
	SPMeshPatch();
	CMeshPatch* cmeshpatch;

    SPMeshPatch* getNextMeshPatch();
    SPMeshPatch* getPrevMeshPatch();
    Glib::ustring * tensor_string;
    //SVGLength tx[4];  // Tensor points
    //SVGLength ty[4];  // Tensor points
};

/// The SPMeshPatch vtable.
struct SPMeshPatchClass {
    SPObjectClass parent_class;
};


class CMeshPatch : public CObject {
public:
	CMeshPatch(SPMeshPatch* meshpatch);
	virtual ~CMeshPatch();

	virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void set(unsigned int key, const gchar* value);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

protected:
	SPMeshPatch* spmeshpatch;
};


#endif /* !SEEN_SP_MESHPATCH_H */

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

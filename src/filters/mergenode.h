#ifndef SP_FEMERGENODE_H_SEEN
#define SP_FEMERGENODE_H_SEEN

/** \file
 * feMergeNode implementation. A feMergeNode stores information about one
 * input image for feMerge filter primitive.
 */
/*
 * Authors:
 *   Kees Cook <kees@outflux.net>
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2004,2007 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-object.h"

#define SP_TYPE_FEMERGENODE (sp_feMergeNode_get_type())
#define SP_FEMERGENODE(obj) ((SPFeMergeNode*)obj)
#define SP_IS_FEMERGENODE(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPFeMergeNode)))

class CFeMergeNode;

class SPFeMergeNode : public SPObject {
public:
	CFeMergeNode* cfemergenode;

    int input;
};

class CFeMergeNode : public CObject {
public:
	CFeMergeNode(SPFeMergeNode* mergenode);
	virtual ~CFeMergeNode();

	virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void release();

	virtual void set(unsigned int key, const gchar* value);

	virtual void update(SPCtx* ctx, unsigned int flags);

	virtual Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

private:
	SPFeMergeNode* spfemergenode;
};

struct SPFeMergeNodeClass {
    SPObjectClass parent_class;
};

GType sp_feMergeNode_get_type();


#endif /* !SP_FEMERGENODE_H_SEEN */

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

/** \file
 * feMergeNode implementation. A feMergeNode contains the name of one
 * input image for feMerge.
 */
/*
 * Authors:
 *   Kees Cook <kees@outflux.net>
 *   Niko Kiirala <niko@kiirala.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2004,2007 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "attributes.h"
#include "xml/repr.h"
#include "filters/mergenode.h"
#include "filters/merge.h"
#include "display/nr-filter-types.h"

G_DEFINE_TYPE(SPFeMergeNode, sp_feMergeNode, SP_TYPE_OBJECT);

static void
sp_feMergeNode_class_init(SPFeMergeNodeClass *klass)
{
}

CFeMergeNode::CFeMergeNode(SPFeMergeNode* mergenode) : CObject(mergenode) {
	this->spfemergenode = mergenode;
}

CFeMergeNode::~CFeMergeNode() {
}

static void
sp_feMergeNode_init(SPFeMergeNode *feMergeNode)
{
	feMergeNode->cfemergenode = new CFeMergeNode(feMergeNode);
	feMergeNode->typeHierarchy.insert(typeid(SPFeMergeNode));

	delete feMergeNode->cobject;
	feMergeNode->cobject = feMergeNode->cfemergenode;

    feMergeNode->input = Inkscape::Filters::NR_FILTER_SLOT_NOT_SET;
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPFeMergeNode variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void CFeMergeNode::build(SPDocument *document, Inkscape::XML::Node *repr) {
	SPFeMergeNode* object = this->spfemergenode;
	object->readAttr( "in" );
}

/**
 * Drops any allocated memory.
 */
void CFeMergeNode::release() {
	CObject::release();
}

/**
 * Sets a specific value in the SPFeMergeNode.
 */
void CFeMergeNode::set(unsigned int key, gchar const *value) {
	SPFeMergeNode* object = this->spfemergenode;
    SPFeMergeNode *feMergeNode = SP_FEMERGENODE(object);
    SPFeMerge *parent = SP_FEMERGE(object->parent);

    if (key == SP_ATTR_IN) {
        int input = sp_filter_primitive_read_in(parent, value);
        if (input != feMergeNode->input) {
            feMergeNode->input = input;
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
        }
    }

    /* See if any parents need this value. */
    CObject::set(key, value);
}

/**
 * Receives update notifications.
 */
void CFeMergeNode::update(SPCtx *ctx, guint flags) {
	SPFeMergeNode* object = this->spfemergenode;
    //SPFeMergeNode *feMergeNode = SP_FEMERGENODE(object);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
    }

    CObject::update(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* CFeMergeNode::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
	SPFeMergeNode* object = this->spfemergenode;
    //SPFeMergeNode *feMergeNode = SP_FEMERGENODE(object);

    // Inkscape-only object, not copied during an "plain SVG" dump:
    if (flags & SP_OBJECT_WRITE_EXT) {
        if (repr) {
            // is this sane?
            //repr->mergeFrom(object->getRepr(), "id");
        } else {
            repr = object->getRepr()->duplicate(doc);
        }
    }

    CObject::write(doc, repr, flags);

    return repr;
}


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

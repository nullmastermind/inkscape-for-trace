/** \file
 * SVG <feComponentTransfer> implementation.
 *
 */
/*
 * Authors:
 *   hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>

#include "document.h"
#include "attributes.h"
#include "svg/svg.h"
#include "filters/componenttransfer.h"
#include "filters/componenttransfer-funcnode.h"
#include "xml/repr.h"
#include "display/nr-filter.h"
#include "display/nr-filter-component-transfer.h"

/* FeComponentTransfer base class */
G_DEFINE_TYPE(SPFeComponentTransfer, sp_feComponentTransfer, SP_TYPE_FILTER_PRIMITIVE);

static void
sp_feComponentTransfer_class_init(SPFeComponentTransferClass *klass)
{
}

CFeComponentTransfer::CFeComponentTransfer(SPFeComponentTransfer* tr) : CFilterPrimitive(tr) {
	this->spfecomponenttransfer = tr;
}

CFeComponentTransfer::~CFeComponentTransfer() {
}

static void
sp_feComponentTransfer_init(SPFeComponentTransfer *feComponentTransfer)
{
	feComponentTransfer->cfecomponenttransfer = new CFeComponentTransfer(feComponentTransfer);
	feComponentTransfer->typeHierarchy.insert(typeid(SPFeComponentTransfer));

	delete feComponentTransfer->cfilterprimitive;
	feComponentTransfer->cfilterprimitive = feComponentTransfer->cfecomponenttransfer;
	feComponentTransfer->cobject = feComponentTransfer->cfecomponenttransfer;
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPFeComponentTransfer variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void CFeComponentTransfer::build(SPDocument *document, Inkscape::XML::Node *repr) {
	CFilterPrimitive::build(document, repr);

	/*LOAD ATTRIBUTES FROM REPR HERE*/

	//do we need this?
	//document->addResource("feComponentTransfer", object);
}

static void sp_feComponentTransfer_children_modified(SPFeComponentTransfer *sp_componenttransfer)
{
    if (sp_componenttransfer->renderer) {
        bool set[4] = {false, false, false, false};
        SPObject* node = sp_componenttransfer->children;
        for(;node;node=node->next){
            int i=4;
            if (SP_IS_FEFUNCR(node)) i=0;
            if (SP_IS_FEFUNCG(node)) i=1;
            if (SP_IS_FEFUNCB(node)) i=2;
            if (SP_IS_FEFUNCA(node)) i=3;
            if (i==4) {
                g_warning("Unrecognized channel for component transfer.");
                break;
            }
            sp_componenttransfer->renderer->type[i] = ((SPFeFuncNode *) node)->type;
            sp_componenttransfer->renderer->tableValues[i] = ((SPFeFuncNode *) node)->tableValues;
            sp_componenttransfer->renderer->slope[i] = ((SPFeFuncNode *) node)->slope;
            sp_componenttransfer->renderer->intercept[i] = ((SPFeFuncNode *) node)->intercept;
            sp_componenttransfer->renderer->amplitude[i] = ((SPFeFuncNode *) node)->amplitude;
            sp_componenttransfer->renderer->exponent[i] = ((SPFeFuncNode *) node)->exponent;
            sp_componenttransfer->renderer->offset[i] = ((SPFeFuncNode *) node)->offset;
            set[i] = true;
        }
        // Set any types not explicitly set to the identity transform
        for(int i=0;i<4;i++) {
            if (!set[i]) {
                sp_componenttransfer->renderer->type[i] = Inkscape::Filters::COMPONENTTRANSFER_TYPE_IDENTITY;
            }
        }
    }
}

/**
 * Callback for child_added event.
 */
void CFeComponentTransfer::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) {
	SPFeComponentTransfer* object = this->spfecomponenttransfer;

    SPFeComponentTransfer *f = SP_FECOMPONENTTRANSFER(object);

    CFilterPrimitive::child_added(child, ref);

    sp_feComponentTransfer_children_modified(f);
    object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

/**
 * Callback for remove_child event.
 */
void CFeComponentTransfer::remove_child(Inkscape::XML::Node *child) {
	SPFeComponentTransfer* object = this->spfecomponenttransfer;

    SPFeComponentTransfer *f = SP_FECOMPONENTTRANSFER(object);

    CFilterPrimitive::remove_child(child);

    sp_feComponentTransfer_children_modified(f);
    object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

/**
 * Drops any allocated memory.
 */
void CFeComponentTransfer::release() {
	CFilterPrimitive::release();
}

/**
 * Sets a specific value in the SPFeComponentTransfer.
 */
void CFeComponentTransfer::set(unsigned int key, gchar const *value) {
	SPFeComponentTransfer* object = this->spfecomponenttransfer;

    SPFeComponentTransfer *feComponentTransfer = SP_FECOMPONENTTRANSFER(object);
    (void)feComponentTransfer;

    switch(key) {
        /*DEAL WITH SETTING ATTRIBUTES HERE*/
        default:
        	CFilterPrimitive::set(key, value);
            break;
    }
}

/**
 * Receives update notifications.
 */
void CFeComponentTransfer::update(SPCtx *ctx, guint flags) {
	SPFeComponentTransfer* object = this->spfecomponenttransfer;

    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG |
                 SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {

        /* do something to trigger redisplay, updates? */

    }

    CFilterPrimitive::update(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* CFeComponentTransfer::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
	SPFeComponentTransfer* object = this->spfecomponenttransfer;

    /* TODO: Don't just clone, but create a new repr node and write all
     * relevant values into it */
    if (!repr) {
        repr = object->getRepr()->duplicate(doc);
    }

    CFilterPrimitive::write(doc, repr, flags);

    return repr;
}

void CFeComponentTransfer::build_renderer(Inkscape::Filters::Filter* filter) {
	SPFeComponentTransfer* primitive = this->spfecomponenttransfer;

    g_assert(primitive != NULL);
    g_assert(filter != NULL);

    SPFeComponentTransfer *sp_componenttransfer = SP_FECOMPONENTTRANSFER(primitive);

    int primitive_n = filter->add_primitive(Inkscape::Filters::NR_FILTER_COMPONENTTRANSFER);
    Inkscape::Filters::FilterPrimitive *nr_primitive = filter->get_primitive(primitive_n);
    Inkscape::Filters::FilterComponentTransfer *nr_componenttransfer = dynamic_cast<Inkscape::Filters::FilterComponentTransfer*>(nr_primitive);
    g_assert(nr_componenttransfer != NULL);

    sp_componenttransfer->renderer = nr_componenttransfer;
    sp_filter_primitive_renderer_common(primitive, nr_primitive);


    sp_feComponentTransfer_children_modified(sp_componenttransfer);    //do we need it?!
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

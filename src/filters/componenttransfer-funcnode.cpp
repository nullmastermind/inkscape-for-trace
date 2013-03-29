/** \file
 * SVG <funcR>, <funcG>, <funcB> and <funcA> implementations.
 */
/*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Niko Kiirala <niko@kiirala.com>
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006, 2007, 2008 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>

#include "attributes.h"
#include "document.h"
#include "componenttransfer.h"
#include "componenttransfer-funcnode.h"
#include "display/nr-filter-component-transfer.h"
#include "xml/repr.h"
#include "helper-fns.h"

#define SP_MACROS_SILENT
#include "macros.h"

/* FeFuncNode class */

static void sp_fefuncnode_class_init(SPFeFuncNodeClass *klass);
static void sp_fefuncnode_init(SPFeFuncNode *fefuncnode);

static SPObjectClass *feFuncNode_parent_class;

GType
sp_fefuncR_get_type()
{
    static GType fefuncnode_type = 0;

    if (!fefuncnode_type) {
        GTypeInfo fefuncnode_info = {
            sizeof(SPFeFuncNodeClass),
            NULL, NULL,
            (GClassInitFunc) sp_fefuncnode_class_init,
            NULL, NULL,
            sizeof(SPFeFuncNode),
            16,
            (GInstanceInitFunc) sp_fefuncnode_init,
            NULL,    /* value_table */
        };
        fefuncnode_type = g_type_register_static(SP_TYPE_OBJECT, "SPFeFuncR", &fefuncnode_info, (GTypeFlags)0);
    }
    return fefuncnode_type;
}

GType
sp_fefuncG_get_type()
{
    static GType fefuncnode_type = 0;

    if (!fefuncnode_type) {
        GTypeInfo fefuncnode_info = {
            sizeof(SPFeFuncNodeClass),
            NULL, NULL,
            (GClassInitFunc) sp_fefuncnode_class_init,
            NULL, NULL,
            sizeof(SPFeFuncNode),
            16,
            (GInstanceInitFunc) sp_fefuncnode_init,
            NULL,    /* value_table */
        };
        fefuncnode_type = g_type_register_static(SP_TYPE_OBJECT, "SPFeFuncG", &fefuncnode_info, (GTypeFlags)0);
    }
    return fefuncnode_type;
}

GType
sp_fefuncB_get_type()
{
    static GType fefuncnode_type = 0;

    if (!fefuncnode_type) {
        GTypeInfo fefuncnode_info = {
            sizeof(SPFeFuncNodeClass),
            NULL, NULL,
            (GClassInitFunc) sp_fefuncnode_class_init,
            NULL, NULL,
            sizeof(SPFeFuncNode),
            16,
            (GInstanceInitFunc) sp_fefuncnode_init,
            NULL,    /* value_table */
        };
        fefuncnode_type = g_type_register_static(SP_TYPE_OBJECT, "SPFeFuncB", &fefuncnode_info, (GTypeFlags)0);
    }
    return fefuncnode_type;
}

GType
sp_fefuncA_get_type()
{
    static GType fefuncnode_type = 0;

    if (!fefuncnode_type) {
        GTypeInfo fefuncnode_info = {
            sizeof(SPFeFuncNodeClass),
            NULL, NULL,
            (GClassInitFunc) sp_fefuncnode_class_init,
            NULL, NULL,
            sizeof(SPFeFuncNode),
            16,
            (GInstanceInitFunc) sp_fefuncnode_init,
            NULL,    /* value_table */
        };
        fefuncnode_type = g_type_register_static(SP_TYPE_OBJECT, "SPFeFuncA", &fefuncnode_info, (GTypeFlags)0);
    }
    return fefuncnode_type;
}

static void
sp_fefuncnode_class_init(SPFeFuncNodeClass *klass)
{
    SPObjectClass *sp_object_class = (SPObjectClass *)klass;

    feFuncNode_parent_class = (SPObjectClass*)g_type_class_peek_parent(klass);
}

CFeFuncNode::CFeFuncNode(SPFeFuncNode* funcnode) : CObject(funcnode) {
	this->spfefuncnode = funcnode;
}

CFeFuncNode::~CFeFuncNode() {
}

static void
sp_fefuncnode_init(SPFeFuncNode *fefuncnode)
{
	fefuncnode->cfefuncnode = new CFeFuncNode(fefuncnode);

	delete fefuncnode->cobject;
	fefuncnode->cobject = fefuncnode->cfefuncnode;

    fefuncnode->type = Inkscape::Filters::COMPONENTTRANSFER_TYPE_IDENTITY;
    //fefuncnode->tableValues = NULL;
    fefuncnode->slope = 1;
    fefuncnode->intercept = 0;
    fefuncnode->amplitude = 1;
    fefuncnode->exponent = 1;
    fefuncnode->offset = 0;
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPDistantLight variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void CFeFuncNode::build(SPDocument *document, Inkscape::XML::Node *repr) {
	CObject::build(document, repr);

	SPFeFuncNode* object = this->spfefuncnode;

    //Read values of key attributes from XML nodes into object.
    object->readAttr( "type" );
    object->readAttr( "tableValues" );
    object->readAttr( "slope" );
    object->readAttr( "intercept" );
    object->readAttr( "amplitude" );
    object->readAttr( "exponent" );
    object->readAttr( "offset" );


//is this necessary?
    document->addResource("fefuncnode", object); //maybe feFuncR, fefuncG, feFuncB and fefuncA ?
}

/**
 * Drops any allocated memory.
 */
void CFeFuncNode::release() {
	SPFeFuncNode* object = this->spfefuncnode;
    //SPFeFuncNode *fefuncnode = SP_FEFUNCNODE(object);

    if ( object->document ) {
        // Unregister ourselves
        object->document->removeResource("fefuncnode", object);
    }

//TODO: release resources here
}

static Inkscape::Filters::FilterComponentTransferType sp_feComponenttransfer_read_type(gchar const *value){
    if (!value) return Inkscape::Filters::COMPONENTTRANSFER_TYPE_ERROR; //type attribute is REQUIRED.
    switch(value[0]){
        case 'i':
            if (strncmp(value, "identity", 8) == 0) return Inkscape::Filters::COMPONENTTRANSFER_TYPE_IDENTITY;
            break;
        case 't':
            if (strncmp(value, "table", 5) == 0) return Inkscape::Filters::COMPONENTTRANSFER_TYPE_TABLE;
            break;
        case 'd':
            if (strncmp(value, "discrete", 8) == 0) return Inkscape::Filters::COMPONENTTRANSFER_TYPE_DISCRETE;
            break;
        case 'l':
            if (strncmp(value, "linear", 6) == 0) return Inkscape::Filters::COMPONENTTRANSFER_TYPE_LINEAR;
            break;
        case 'g':
            if (strncmp(value, "gamma", 5) == 0) return Inkscape::Filters::COMPONENTTRANSFER_TYPE_GAMMA;
            break;
    }
    return Inkscape::Filters::COMPONENTTRANSFER_TYPE_ERROR; //type attribute is REQUIRED.
}

/**
 * Sets a specific value in the SPFeFuncNode.
 */
void CFeFuncNode::set(unsigned int key, gchar const *value) {
	SPFeFuncNode* object = this->spfefuncnode;

    SPFeFuncNode *feFuncNode = SP_FEFUNCNODE(object);
    Inkscape::Filters::FilterComponentTransferType type;
    double read_num;
    switch(key) {
        case SP_ATTR_TYPE:
            type = sp_feComponenttransfer_read_type(value);
            if(type != feFuncNode->type) {
                feFuncNode->type = type;
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SP_ATTR_TABLEVALUES:
            if (value){
                feFuncNode->tableValues = helperfns_read_vector(value);
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SP_ATTR_SLOPE:
            read_num = value ? helperfns_read_number(value) : 1;
            if (read_num != feFuncNode->slope) {
                feFuncNode->slope = read_num;
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SP_ATTR_INTERCEPT:
            read_num = value ? helperfns_read_number(value) : 0;
            if (read_num != feFuncNode->intercept) {
                feFuncNode->intercept = read_num;
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SP_ATTR_AMPLITUDE:
            read_num = value ? helperfns_read_number(value) : 1;
            if (read_num != feFuncNode->amplitude) {
                feFuncNode->amplitude = read_num;
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SP_ATTR_EXPONENT:
            read_num = value ? helperfns_read_number(value) : 1;
            if (read_num != feFuncNode->exponent) {
                feFuncNode->exponent = read_num;
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SP_ATTR_OFFSET:
            read_num = value ? helperfns_read_number(value) : 0;
            if (read_num != feFuncNode->offset) {
                feFuncNode->offset = read_num;
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        default:
//            if (((SPObjectClass *) feFuncNode_parent_class)->set)
//                ((SPObjectClass *) feFuncNode_parent_class)->set(object, key, value);
        	CObject::set(key, value);
            break;
    }
}

/**
 *  * Receives update notifications.
 *   */
void CFeFuncNode::update(SPCtx *ctx, guint flags) {
	SPFeFuncNode* object = this->spfefuncnode;

    SPFeFuncNode *feFuncNode = SP_FEFUNCNODE(object);
    (void)feFuncNode;

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        /* do something to trigger redisplay, updates? */
        //TODO
        //object->readAttr( "azimuth" );
        //object->readAttr( "elevation" );
    }

    CObject::update(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* CFeFuncNode::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
	SPFeFuncNode* object = this->spfefuncnode;
    SPFeFuncNode *fefuncnode = SP_FEFUNCNODE(object);

    if (!repr) {
        repr = object->getRepr()->duplicate(doc);
    }

    (void)fefuncnode;
    /*
TODO: I'm not sure what to do here...

    if (fefuncnode->azimuth_set)
        sp_repr_set_css_double(repr, "azimuth", fefuncnode->azimuth);
    if (fefuncnode->elevation_set)
        sp_repr_set_css_double(repr, "elevation", fefuncnode->elevation);*/

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

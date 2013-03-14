/*
 * SVG <metadata> implementation
 *
 * Authors:
 *   Kees Cook <kees@outflux.net>
 *
 * Copyright (C) 2004 Kees Cook
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "sp-metadata.h"
#include "xml/node-iterators.h"
#include "document.h"

#include "sp-item-group.h"
#include "sp-root.h"

#define noDEBUG_METADATA
#ifdef DEBUG_METADATA
# define debug(f, a...) { g_print("%s(%d) %s:", \
                                  __FILE__,__LINE__,__FUNCTION__); \
                          g_print(f, ## a); \
                          g_print("\n"); \
                        }
#else
# define debug(f, a...) /**/
#endif

/* Metadata base class */

static void sp_metadata_class_init (SPMetadataClass *klass);
static void sp_metadata_init (SPMetadata *metadata);

static void sp_metadata_build (SPObject * object, SPDocument * document, Inkscape::XML::Node * repr);
static void sp_metadata_release (SPObject *object);
static void sp_metadata_set (SPObject *object, unsigned int key, const gchar *value);
static void sp_metadata_update(SPObject *object, SPCtx *ctx, guint flags);
static Inkscape::XML::Node *sp_metadata_write(SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);

static SPObjectClass *metadata_parent_class;

GType
sp_metadata_get_type (void)
{
    static GType metadata_type = 0;

    if (!metadata_type) {
        GTypeInfo metadata_info = {
            sizeof (SPMetadataClass),
            NULL, NULL,
            (GClassInitFunc) sp_metadata_class_init,
            NULL, NULL,
            sizeof (SPMetadata),
            16,
            (GInstanceInitFunc) sp_metadata_init,
            NULL,    /* value_table */
        };
        metadata_type = g_type_register_static (SP_TYPE_OBJECT, "SPMetadata", &metadata_info, (GTypeFlags)0);
    }
    return metadata_type;
}

static void
sp_metadata_class_init (SPMetadataClass *klass)
{
    //GObjectClass *gobject_class = (GObjectClass *)klass;
    SPObjectClass *sp_object_class = (SPObjectClass *)klass;

    metadata_parent_class = (SPObjectClass*)g_type_class_peek_parent (klass);

    //sp_object_class->build = sp_metadata_build;
    sp_object_class->release = sp_metadata_release;
    sp_object_class->write = sp_metadata_write;
    sp_object_class->set = sp_metadata_set;
    sp_object_class->update = sp_metadata_update;
}

CMetadata::CMetadata(SPMetadata* metadata) : CObject(metadata) {
	this->spmetadata = metadata;
}

CMetadata::~CMetadata() {
}

static void
sp_metadata_init (SPMetadata *metadata)
{
	metadata->cmetadata = new CMetadata(metadata);
	metadata->cobject = metadata->cmetadata;

    (void)metadata;
    debug("0x%08x",(unsigned int)metadata);
}

namespace {

void strip_ids_recursively(Inkscape::XML::Node *node) {
    using Inkscape::XML::NodeSiblingIterator;
    if ( node->type() == Inkscape::XML::ELEMENT_NODE ) {
        node->setAttribute("id", NULL);
    }
    for ( NodeSiblingIterator iter=node->firstChild() ; iter ; ++iter ) {
        strip_ids_recursively(iter);
    }
}

}


void CMetadata::onBuild(SPDocument* doc, Inkscape::XML::Node* repr) {
    using Inkscape::XML::NodeSiblingIterator;

    debug("0x%08x",(unsigned int)object);

    /* clean up our mess from earlier versions; elements under rdf:RDF should not
     * have id= attributes... */
    static GQuark const rdf_root_name=g_quark_from_static_string("rdf:RDF");
    for ( NodeSiblingIterator iter=repr->firstChild() ; iter ; ++iter ) {
        if ( (GQuark)iter->code() == rdf_root_name ) {
            strip_ids_recursively(iter);
        }
    }

    CObject::onBuild(doc, repr);
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPMetadata variables.
 *
 * For this to get called, our name must be associated with
 * a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
static void sp_metadata_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr)
{
	((SPMetadata*)object)->cmetadata->onBuild(document, repr);
}

void CMetadata::onRelease() {
    debug("0x%08x",(unsigned int)object);

    // handle ourself

    CObject::onRelease();
}

/**
 * Drops any allocated memory.
 */
static void sp_metadata_release(SPObject *object)
{
	((SPMetadata*)object)->cmetadata->onRelease();
}

void CMetadata::onSet(unsigned int key, const gchar* value) {
    debug("0x%08x %s(%u): '%s'",(unsigned int)object,
          sp_attribute_name(key),key,value);
    //SP_METADATA(object); // ensures the object is of the proper type.

    // see if any parents need this value
    CObject::onSet(key, value);
}

/**
 * Sets a specific value in the SPMetadata.
 */
static void sp_metadata_set(SPObject *object, unsigned int key, const gchar *value)
{
	((SPMetadata*)object)->cmetadata->onSet(key, value);
}

void CMetadata::onUpdate(SPCtx* ctx, unsigned int flags) {
    debug("0x%08x",(unsigned int)object);
    //SPMetadata *metadata = SP_METADATA(object);

    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG |
                 SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {

        /* do something? */

    }

    // CPPIFY: As CMetadata is derived directly from CObject, this doesn't make no sense.
    // CObject::onUpdate is pure. What was the idea behind these lines?
//    if (((SPObjectClass *) metadata_parent_class)->update)
//    	((SPObjectClass *) metadata_parent_class)->update(object, ctx, flags);
//    CObject::onUpdate(ctx, flags);
}

/**
 * Receives update notifications.
 */
static void sp_metadata_update(SPObject *object, SPCtx *ctx, guint flags)
{
	((SPMetadata*)object)->cmetadata->onUpdate(ctx, flags);
}

Inkscape::XML::Node* CMetadata::onWrite(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags) {
	SPMetadata* object = this->spmetadata;

    debug("0x%08x",(unsigned int)object);
    //SPMetadata *metadata = SP_METADATA(object);

    if ( repr != object->getRepr() ) {
        if (repr) {
            repr->mergeFrom(object->getRepr(), "id");
        } else {
            repr = object->getRepr()->duplicate(doc);
        }
    }

    CObject::onWrite(doc, repr, flags);

    return repr;
}

/**
 * Writes it's settings to an incoming repr object, if any.
 */
static Inkscape::XML::Node *sp_metadata_write(SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags)
{
	return ((SPMetadata*)object)->cmetadata->onWrite(doc, repr, flags);
}

/**
 * Retrieves the metadata object associated with a document.
 */
SPMetadata *sp_document_metadata(SPDocument *document)
{
    SPObject *nv;

    g_return_val_if_fail (document != NULL, NULL);

    nv = sp_item_group_get_child_by_name( document->getRoot(), NULL,
                                        "metadata");
    g_assert (nv != NULL);

    return (SPMetadata *)nv;
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

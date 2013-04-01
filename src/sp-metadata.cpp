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

G_DEFINE_TYPE(SPMetadata, sp_metadata, G_TYPE_OBJECT);

static void
sp_metadata_class_init (SPMetadataClass *klass)
{
}

CMetadata::CMetadata(SPMetadata* metadata) : CObject(metadata) {
	this->spmetadata = metadata;
}

CMetadata::~CMetadata() {
}

SPMetadata::SPMetadata() : SPObject() {
	SPMetadata* metadata = this;

	metadata->cmetadata = new CMetadata(metadata);
	metadata->typeHierarchy.insert(typeid(SPMetadata));

	delete metadata->cobject;
	metadata->cobject = metadata->cmetadata;

    (void)metadata;
    debug("0x%08x",(unsigned int)metadata);
}

static void
sp_metadata_init (SPMetadata *metadata)
{
	new (metadata) SPMetadata();
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


void CMetadata::build(SPDocument* doc, Inkscape::XML::Node* repr) {
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

    CObject::build(doc, repr);
}
void CMetadata::release() {
    debug("0x%08x",(unsigned int)object);

    // handle ourself

    CObject::release();
}

void CMetadata::set(unsigned int key, const gchar* value) {
    debug("0x%08x %s(%u): '%s'",(unsigned int)object,
          sp_attribute_name(key),key,value);

    // see if any parents need this value
    CObject::set(key, value);
}

void CMetadata::update(SPCtx* ctx, unsigned int flags) {
    debug("0x%08x",(unsigned int)object);
    //SPMetadata *metadata = SP_METADATA(object);

    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG |
                 SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {

        /* do something? */

    }

//    CObject::onUpdate(ctx, flags);
}

Inkscape::XML::Node* CMetadata::write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags) {
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

    CObject::write(doc, repr, flags);

    return repr;
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

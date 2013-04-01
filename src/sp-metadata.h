#ifndef __SP_METADATA_H__
#define __SP_METADATA_H__

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

#include "sp-object.h"


/* Metadata base class */

#define SP_TYPE_METADATA (sp_metadata_get_type ())
#define SP_METADATA(obj) ((SPMetadata*)obj)
#define SP_IS_METADATA(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPMetadata)))

class CMetadata;

class SPMetadata : public SPObject {
public:
	CMetadata* cmetadata;
};

struct SPMetadataClass {
	SPObjectClass parent_class;
};


class CMetadata : public CObject {
public:
	CMetadata(SPMetadata* metadata);
	virtual ~CMetadata();

	virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void release();

	virtual void set(unsigned int key, const gchar* value);
	virtual void update(SPCtx* ctx, unsigned int flags);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

protected:
	SPMetadata* spmetadata;
};


GType sp_metadata_get_type (void);

SPMetadata * sp_document_metadata (SPDocument *document);

#endif
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

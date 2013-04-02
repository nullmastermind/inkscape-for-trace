/*
 * SVG <desc> implementation
 *
 * Authors:
 *   Jeff Schiller <codedread@gmail.com>
 *
 * Copyright (C) 2008 Jeff Schiller
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "sp-desc.h"
#include "xml/repr.h"

#include "sp-factory.h"

namespace {
	SPObject* createDesc() {
		return new SPDesc();
	}

	bool descRegistered = SPFactory::instance().registerObject("svg:desc", createDesc);
}

G_DEFINE_TYPE(SPDesc, sp_desc, SP_TYPE_OBJECT);

static void sp_desc_class_init(SPDescClass *klass)
{
    SPObjectClass *sp_object_class = (SPObjectClass *)(klass);

}

CDesc::CDesc(SPDesc* desc) : CObject(desc) {
	this->spdesc = desc;
}

CDesc::~CDesc() {
}

SPDesc::SPDesc() : SPObject() {
	SPDesc* desc = this;

	desc->cdesc = new CDesc(desc);
	desc->typeHierarchy.insert(typeid(SPDesc));

	delete desc->cobject;
	desc->cobject = desc->cdesc;
}

static void sp_desc_init(SPDesc *desc)
{
	new (desc) SPDesc();
}

Inkscape::XML::Node* CDesc::write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags) {
	SPDesc* object = this->spdesc;

    if (!repr) {
        repr = object->getRepr()->duplicate(doc);
    }

    CObject::write(doc, repr, flags);

    return repr;
}

/**
 * Writes it's settings to an incoming repr object, if any.
 */

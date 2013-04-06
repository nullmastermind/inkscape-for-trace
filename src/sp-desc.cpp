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

SPDesc::SPDesc() : SPObject(), CObject(this) {
	delete this->cobject;
	this->cobject = this;
}

SPDesc::~SPDesc() {
}

Inkscape::XML::Node* SPDesc::write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags) {
	SPDesc* object = this;

    if (!repr) {
        repr = object->getRepr()->duplicate(doc);
    }

    CObject::write(doc, repr, flags);

    return repr;
}

/**
 * Writes it's settings to an incoming repr object, if any.
 */

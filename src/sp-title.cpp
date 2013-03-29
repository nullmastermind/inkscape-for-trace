/*
 * SVG <title> implementation
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

#include "sp-title.h"
#include "xml/repr.h"

G_DEFINE_TYPE(SPTitle, sp_title, SP_TYPE_OBJECT);

static void
sp_title_class_init(SPTitleClass *klass)
{
}

CTitle::CTitle(SPTitle* title) : CObject(title) {
	this->sptitle = title;
}

CTitle::~CTitle() {
}

static void
sp_title_init(SPTitle *desc)
{
	desc->ctitle = new CTitle(desc);

	delete desc->cobject;
	desc->cobject = desc->ctitle;
}

Inkscape::XML::Node* CTitle::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	SPTitle* object = this->sptitle;

    if (!repr) {
        repr = object->getRepr()->duplicate(xml_doc);
    }

    CObject::write(xml_doc, repr, flags);

    return repr;
}


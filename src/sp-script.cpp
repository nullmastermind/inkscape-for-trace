/*
 * SVG <script> implementation
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2008 authors
 *
 * Released under GNU GPL version 2 or later, read the file 'COPYING' for more information
 */

#include "sp-script.h"
#include "attributes.h"
#include <cstring>
#include "document.h"

#include "sp-factory.h"

namespace {
	SPObject* createScript() {
		return new SPScript();
	}

	bool scriptRegistered = SPFactory::instance().registerObject("svg:script", createScript);
}

SPScript::SPScript() : SPObject(), CObject(this) {
	delete this->cobject;
	this->cobject = this;

	this->xlinkhref = NULL;
}

SPScript::~SPScript() {
}

void SPScript::build(SPDocument* doc, Inkscape::XML::Node* repr) {
	SPScript* object = this;

    CObject::build(doc, repr);

    //Read values of key attributes from XML nodes into object.
    object->readAttr( "xlink:href" );

    doc->addResource("script", object);
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPScript variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */

void SPScript::release() {
	SPScript* object = this;

    if (object->document) {
        // Unregister ourselves
        object->document->removeResource("script", object);
    }

    CObject::release();
}

void SPScript::update(SPCtx* ctx, unsigned int flags) {
}


void SPScript::modified(unsigned int flags) {
}


void SPScript::set(unsigned int key, const gchar* value) {
	SPScript* object = this;

    SPScript *scr = SP_SCRIPT(object);

    switch (key) {
	case SP_ATTR_XLINK_HREF:
            if (scr->xlinkhref) g_free(scr->xlinkhref);
            scr->xlinkhref = g_strdup(value);
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
	default:
            CObject::set(key, value);
            break;
    }
}

Inkscape::XML::Node* SPScript::write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags) {
	return repr;
}

//static Inkscape::XML::Node *sp_script_write(SPObject *object, Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
//{
/*
TODO:
 code copied from sp-defs
 decide what to do here!

    if (flags & SP_OBJECT_WRITE_BUILD) {

        if (!repr) {
            repr = xml_doc->createElement("svg:script");
        }

        GSList *l = NULL;
        for ( SPObject *child = object->firstChild() ; child; child = child->getNext() ) {
            Inkscape::XML::Node *crepr = child->updateRepr(xml_doc, NULL, flags);
            if (crepr) {
                l = g_slist_prepend(l, crepr);
            }
        }

        while (l) {
            repr->addChild((Inkscape::XML::Node *) l->data, NULL);
            Inkscape::GC::release((Inkscape::XML::Node *) l->data);
            l = g_slist_remove(l, l->data);
        }

    } else {
        for ( SPObject *child = object->firstChild() ; child; child = child->getNext() ) {
            child->updateRepr(flags);
        }
    }

    if (((SPObjectClass *) (parent_class))->write) {
        (* ((SPObjectClass *) (parent_class))->write)(object, xml_doc, repr, flags);
    }
*/
//
//	return ((SPScript*)object)->cscript->onWrite(xml_doc, repr, flags);
//}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

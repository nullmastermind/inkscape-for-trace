/*
 * Abstract base class for non-item groups
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2003 Authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-object-group.h"
#include "xml/repr.h"
#include "document.h"


G_DEFINE_TYPE(SPObjectGroup, sp_objectgroup, SP_TYPE_OBJECT);

static void
sp_objectgroup_class_init(SPObjectGroupClass *klass)
{
}

static void
sp_objectgroup_init(SPObjectGroup * objectgroup)
{
	objectgroup->cobjectgroup = new CObjectGroup(objectgroup);

	delete objectgroup->cobject;
	objectgroup->cobject = objectgroup->cobjectgroup;
}

CObjectGroup::CObjectGroup(SPObjectGroup* gr) : CObject(gr) {
	this->spobjectgroup = gr;
}

CObjectGroup::~CObjectGroup() {
}


void CObjectGroup::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) {
	SPObjectGroup* object = this->spobjectgroup;

	CObject::child_added(child, ref);

	object->requestModified(SP_OBJECT_MODIFIED_FLAG);
}


void CObjectGroup::remove_child(Inkscape::XML::Node *child) {
	SPObjectGroup* object = this->spobjectgroup;

	CObject::remove_child(child);

	object->requestModified(SP_OBJECT_MODIFIED_FLAG);
}


void CObjectGroup::order_changed(Inkscape::XML::Node *child, Inkscape::XML::Node *old_ref, Inkscape::XML::Node *new_ref) {
	SPObjectGroup* object = this->spobjectgroup;

	CObject::order_changed(child, old_ref, new_ref);

	object->requestModified(SP_OBJECT_MODIFIED_FLAG);
}


Inkscape::XML::Node *CObjectGroup::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	SPObjectGroup* object = this->spobjectgroup;

    if (flags & SP_OBJECT_WRITE_BUILD) {
        if (!repr) {
            repr = xml_doc->createElement("svg:g");
        }
        GSList *l = 0;
        for ( SPObject *child = object->firstChild() ; child ; child = child->getNext() ) {
            Inkscape::XML::Node *crepr = child->updateRepr(xml_doc, NULL, flags);
            if (crepr) {
                l = g_slist_prepend(l, crepr);
            }
        }
        while (l) {
            repr->addChild(static_cast<Inkscape::XML::Node *>(l->data), NULL);
            Inkscape::GC::release(static_cast<Inkscape::XML::Node *>(l->data));
            l = g_slist_remove(l, l->data);
        }
    } else {
        for ( SPObject *child = object->firstChild() ; child ; child = child->getNext() ) {
            child->updateRepr(flags);
        }
    }

    CObject::write(xml_doc, repr, flags);

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

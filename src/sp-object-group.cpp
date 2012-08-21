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

SPObjectClass * SPObjectGroupClass::static_parent_class = 0;

GType SPObjectGroup::sp_objectgroup_get_type(void)
{
    static GType objectgroup_type = 0;
    if (!objectgroup_type) {
        GTypeInfo objectgroup_info = {
            sizeof(SPObjectGroupClass),
            NULL,   /* base_init */
            NULL,   /* base_finalize */
            (GClassInitFunc) SPObjectGroupClass::sp_objectgroup_class_init,
            NULL,   /* class_finalize */
            NULL,   /* class_data */
            sizeof(SPObjectGroup),
            16,     /* n_preallocs */
            (GInstanceInitFunc) init,
            NULL,   /* value_table */
        };
        objectgroup_type = g_type_register_static(SP_TYPE_OBJECT, "SPObjectGroup", &objectgroup_info, (GTypeFlags)0);
    }
    return objectgroup_type;
}

void SPObjectGroupClass::sp_objectgroup_class_init(SPObjectGroupClass *klass)
{
    //GObjectClass * object_class = (GObjectClass *) klass;
    SPObjectClass * sp_object_class = (SPObjectClass *) klass;

    static_parent_class = (SPObjectClass *)g_type_class_ref(SP_TYPE_OBJECT);

    sp_object_class->child_added = SPObjectGroup::childAdded;
    sp_object_class->remove_child = SPObjectGroup::removeChild;
    sp_object_class->order_changed = SPObjectGroup::orderChanged;
    sp_object_class->write = SPObjectGroup::write;
}

CObjectGroup::CObjectGroup(SPObjectGroup* gr) : CObject(gr) {
	this->spobjectgroup = gr;
}

CObjectGroup::~CObjectGroup() {
}

void SPObjectGroup::init(SPObjectGroup * objectgroup)
{
	objectgroup->cobjectgroup = new CObjectGroup(objectgroup);
	objectgroup->cobject = objectgroup->cobjectgroup;
}

void CObjectGroup::onChildAdded(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) {
	SPObjectGroup* object = this->spobjectgroup;

	CObject::onChildAdded(child, ref);

	object->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void SPObjectGroup::childAdded(SPObject *object, Inkscape::XML::Node *child, Inkscape::XML::Node *ref)
{
	((SPObjectGroup*)object)->cobjectgroup->onChildAdded(child, ref);
}

void CObjectGroup::onRemoveChild(Inkscape::XML::Node *child) {
	SPObjectGroup* object = this->spobjectgroup;

	CObject::onRemoveChild(child);

	object->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void SPObjectGroup::removeChild(SPObject *object, Inkscape::XML::Node *child)
{
	((SPObjectGroup*)object)->cobjectgroup->onRemoveChild(child);
}

void CObjectGroup::onOrderChanged(Inkscape::XML::Node *child, Inkscape::XML::Node *old_ref, Inkscape::XML::Node *new_ref) {
	SPObjectGroup* object = this->spobjectgroup;

	CObject::onOrderChanged(child, old_ref, new_ref);

	object->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void SPObjectGroup::orderChanged(SPObject *object, Inkscape::XML::Node *child, Inkscape::XML::Node *old_ref, Inkscape::XML::Node *new_ref)
{
	((SPObjectGroup*)object)->cobjectgroup->onOrderChanged(child, old_ref, new_ref);
}

Inkscape::XML::Node *CObjectGroup::onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
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
            repr->addChild((Inkscape::XML::Node *) l->data, NULL);
            Inkscape::GC::release((Inkscape::XML::Node *) l->data);
            l = g_slist_remove(l, l->data);
        }
    } else {
        for ( SPObject *child = object->firstChild() ; child ; child = child->getNext() ) {
            child->updateRepr(flags);
        }
    }

    CObject::onWrite(xml_doc, repr, flags);

    return repr;
}

Inkscape::XML::Node *SPObjectGroup::write(SPObject *object, Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
	return ((SPObjectGroup*)object)->cobjectgroup->onWrite(xml_doc, repr, flags);
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

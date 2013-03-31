/*
 * SVG <defs> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2000-2002 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

/*
 * fixme: We should really check childrens validity - currently everything
 * flips in
 */

#include "sp-defs.h"
#include "xml/repr.h"
#include "document.h"


G_DEFINE_TYPE(SPDefs, sp_defs, SP_TYPE_OBJECT);

static void
sp_defs_class_init(SPDefsClass *dc)
{
    SPObjectClass *sp_object_class = (SPObjectClass *) dc;

}
static void
sp_defs_init(SPDefs* defs)
{
	defs->cdefs = new CDefs(defs);
	defs->typeHierarchy.insert(typeid(SPDefs));

	delete defs->cobject;
	defs->cobject = defs->cdefs;
}

CDefs::CDefs(SPDefs* defs) : CObject(defs) {
	this->spdefs = defs;
}

CDefs::~CDefs() {
}



void CDefs::release() {
	CObject::release();
}

void CDefs::update(SPCtx *ctx, guint flags) {
	SPDefs* object = this->spdefs;

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }

    flags &= SP_OBJECT_MODIFIED_CASCADE;

    GSList *l = g_slist_reverse(object->childList(true));
    while (l) {
        SPObject *child = SP_OBJECT(l->data);
        l = g_slist_remove(l, child);
        if (flags || (child->uflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->updateDisplay(ctx, flags);
        }
        g_object_unref (G_OBJECT (child));
    }
}

void CDefs::modified(unsigned int flags) {
	SPDefs* object = this->spdefs;

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }

    flags &= SP_OBJECT_MODIFIED_CASCADE;

    GSList *l = NULL;
    for ( SPObject *child = object->firstChild() ; child; child = child->getNext() ) {
        g_object_ref(G_OBJECT(child));
        l = g_slist_prepend(l, child);
    }

    l = g_slist_reverse(l);

    while (l) {
        SPObject *child = SP_OBJECT(l->data);
        l = g_slist_remove(l, child);
        if (flags || (child->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->emitModified(flags);
        }
        g_object_unref( G_OBJECT(child) );
    }
}

Inkscape::XML::Node* CDefs::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	SPDefs* object = this->spdefs;

    if (flags & SP_OBJECT_WRITE_BUILD) {

        if (!repr) {
            repr = xml_doc->createElement("svg:defs");
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

/*
 * SVG <mask> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2003 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <cstring>
#include <string>
#include <2geom/transforms.h>

#include "display/drawing.h"
#include "display/drawing-group.h"
#include "xml/repr.h"

#include "enums.h"
#include "attributes.h"
#include "document.h"
#include "document-private.h"
#include "sp-item.h"

#include "sp-mask.h"

struct SPMaskView {
	SPMaskView *next;
	unsigned int key;
	Inkscape::DrawingItem *arenaitem;
	Geom::OptRect bbox;
};

SPMaskView *sp_mask_view_new_prepend (SPMaskView *list, unsigned int key, Inkscape::DrawingItem *arenaitem);
SPMaskView *sp_mask_view_list_remove (SPMaskView *list, SPMaskView *view);

G_DEFINE_TYPE(SPMask, sp_mask, SP_TYPE_OBJECTGROUP);

static void
sp_mask_class_init (SPMaskClass *klass)
{
}

CMask::CMask(SPMask* mask) : CObjectGroup(mask) {
	this->spmask = mask;
}

CMask::~CMask() {
}

static void
sp_mask_init (SPMask *mask)
{
	mask->cmask = new CMask(mask);

	delete mask->cobjectgroup;
	mask->cobjectgroup = mask->cmask;
	mask->cobject = mask->cmask;

	mask->maskUnits_set = FALSE;
	mask->maskUnits = SP_CONTENT_UNITS_OBJECTBOUNDINGBOX;

	mask->maskContentUnits_set = FALSE;
	mask->maskContentUnits = SP_CONTENT_UNITS_USERSPACEONUSE;

	mask->display = NULL;
}

void CMask::onBuild(SPDocument* doc, Inkscape::XML::Node* repr) {
	SPMask* object = this->spmask;

	CObjectGroup::onBuild(doc, repr);

	object->readAttr( "maskUnits" );
	object->readAttr( "maskContentUnits" );

	/* Register ourselves */
	doc->addResource("mask", object);
}

void CMask::onRelease() {
	SPMask* object = this->spmask;

    if (object->document) {
        // Unregister ourselves
        object->document->removeResource("mask", object);
    }

    SPMask *cp = SP_MASK (object);
    while (cp->display) {
        // We simply unref and let item manage this in handler
        cp->display = sp_mask_view_list_remove (cp->display, cp->display);
    }

    CObjectGroup::onRelease();
}

void CMask::onSet(unsigned int key, const gchar* value) {
	SPMask* object = this->spmask;

	SPMask *mask = SP_MASK (object);

	switch (key) {
	case SP_ATTR_MASKUNITS:
		mask->maskUnits = SP_CONTENT_UNITS_OBJECTBOUNDINGBOX;
		mask->maskUnits_set = FALSE;
		if (value) {
			if (!strcmp (value, "userSpaceOnUse")) {
				mask->maskUnits = SP_CONTENT_UNITS_USERSPACEONUSE;
				mask->maskUnits_set = TRUE;
			} else if (!strcmp (value, "objectBoundingBox")) {
				mask->maskUnits_set = TRUE;
			}
		}
		object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_MASKCONTENTUNITS:
		mask->maskContentUnits = SP_CONTENT_UNITS_USERSPACEONUSE;
		mask->maskContentUnits_set = FALSE;
		if (value) {
			if (!strcmp (value, "userSpaceOnUse")) {
				mask->maskContentUnits_set = TRUE;
			} else if (!strcmp (value, "objectBoundingBox")) {
				mask->maskContentUnits = SP_CONTENT_UNITS_OBJECTBOUNDINGBOX;
				mask->maskContentUnits_set = TRUE;
			}
		}
		object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
		break;
	default:
		CObjectGroup::onSet(key, value);
		break;
	}
}

void CMask::onChildAdded(Inkscape::XML::Node* child, Inkscape::XML::Node* ref) {
	SPMask* object = this->spmask;

	/* Invoke SPObjectGroup implementation */
	CObjectGroup::onChildAdded(child, ref);

	/* Show new object */
	SPObject *ochild = object->document->getObjectByRepr(child);
	if (SP_IS_ITEM (ochild)) {
		SPMask *cp = SP_MASK (object);
		for (SPMaskView *v = cp->display; v != NULL; v = v->next) {
			Inkscape::DrawingItem *ac = SP_ITEM (ochild)->invoke_show (							       v->arenaitem->drawing(),
							       v->key,
							       SP_ITEM_REFERENCE_FLAGS);
			if (ac) {
			    v->arenaitem->prependChild(ac);
			}
		}
	}
}


void CMask::onUpdate(SPCtx* ctx, unsigned int flags) {
	SPMask* object = this->spmask;

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
	
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    SPObjectGroup *og = SP_OBJECTGROUP(object);
    GSList *l = NULL;
    for (SPObject *child = og->firstChild(); child; child = child->getNext()) {
        g_object_ref(G_OBJECT (child));
        l = g_slist_prepend (l, child);
    }
    l = g_slist_reverse (l);
    while (l) {
        SPObject *child = SP_OBJECT(l->data);
        l = g_slist_remove(l, child);
        if (flags || (child->uflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->updateDisplay(ctx, flags);
        }
        g_object_unref(G_OBJECT(child));
    }

    SPMask *mask = SP_MASK(object);
    for (SPMaskView *v = mask->display; v != NULL; v = v->next) {
        Inkscape::DrawingGroup *g = dynamic_cast<Inkscape::DrawingGroup *>(v->arenaitem);
        if (mask->maskContentUnits == SP_CONTENT_UNITS_OBJECTBOUNDINGBOX && v->bbox) {
            Geom::Affine t = Geom::Scale(v->bbox->dimensions());
            t.setTranslation(v->bbox->min());
            g->setChildTransform(t);
        } else {
            g->setChildTransform(Geom::identity());
        }
    }
}

void CMask::onModified(unsigned int flags) {
	SPMask* object = this->spmask;

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
	
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    SPObjectGroup *og = SP_OBJECTGROUP(object);
    GSList *l = NULL;
    for (SPObject *child = og->firstChild(); child; child = child->getNext()) {
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
        g_object_unref(G_OBJECT(child));
    }
}

Inkscape::XML::Node* CMask::onWrite(Inkscape::XML::Document* xml_doc, Inkscape::XML::Node* repr, guint flags) {
	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = xml_doc->createElement("svg:mask");
	}

	CObjectGroup::onWrite(xml_doc, repr, flags);

	return repr;
}

// Create a mask element (using passed elements), add it to <defs>
const gchar *
sp_mask_create (GSList *reprs, SPDocument *document, Geom::Affine const* applyTransform)
{
    Inkscape::XML::Node *defsrepr = document->getDefs()->getRepr();

    Inkscape::XML::Document *xml_doc = document->getReprDoc();
    Inkscape::XML::Node *repr = xml_doc->createElement("svg:mask");
    repr->setAttribute("maskUnits", "userSpaceOnUse");
    
    defsrepr->appendChild(repr);
    const gchar *mask_id = repr->attribute("id");
    SPObject *mask_object = document->getObjectById(mask_id);
    
    for (GSList *it = reprs; it != NULL; it = it->next) {
        Inkscape::XML::Node *node = (Inkscape::XML::Node *)(it->data);
        SPItem *item = SP_ITEM(mask_object->appendChildRepr(node));
        
        if (NULL != applyTransform) {
            Geom::Affine transform (item->transform);
            transform *= (*applyTransform);
            item->doWriteTransform(item->getRepr(), transform);
        }
    }

    if (repr != defsrepr->lastChild())
        defsrepr->changeOrder(repr, defsrepr->lastChild()); // workaround for bug 989084
    
    Inkscape::GC::release(repr);
    return mask_id;
}

Inkscape::DrawingItem *sp_mask_show(SPMask *mask, Inkscape::Drawing &drawing, unsigned int key)
{
	g_return_val_if_fail (mask != NULL, NULL);
	g_return_val_if_fail (SP_IS_MASK (mask), NULL);

	Inkscape::DrawingGroup *ai = new Inkscape::DrawingGroup(drawing);
	mask->display = sp_mask_view_new_prepend (mask->display, key, ai);

	for ( SPObject *child = mask->firstChild() ; child; child = child->getNext() ) {
		if (SP_IS_ITEM (child)) {
			Inkscape::DrawingItem *ac = SP_ITEM (child)->invoke_show (drawing, key, SP_ITEM_REFERENCE_FLAGS);
			if (ac) {
				ai->prependChild(ac);
			}
		}
	}

	if (mask->maskContentUnits == SP_CONTENT_UNITS_OBJECTBOUNDINGBOX && mask->display->bbox) {
	    Geom::Affine t = Geom::Scale(mask->display->bbox->dimensions());
	    t.setTranslation(mask->display->bbox->min());
	    ai->setChildTransform(t);
	}

	return ai;
}

void sp_mask_hide(SPMask *cp, unsigned int key)
{
	g_return_if_fail (cp != NULL);
	g_return_if_fail (SP_IS_MASK (cp));

	for ( SPObject *child = cp->firstChild(); child; child = child->getNext()) {
		if (SP_IS_ITEM (child)) {
			SP_ITEM(child)->invoke_hide (key);
		}
	}

	for (SPMaskView *v = cp->display; v != NULL; v = v->next) {
		if (v->key == key) {
			/* We simply unref and let item to manage this in handler */
			cp->display = sp_mask_view_list_remove (cp->display, v);
			return;
		}
	}

	g_assert_not_reached ();
}

void
sp_mask_set_bbox (SPMask *mask, unsigned int key, Geom::OptRect const &bbox)
{
	for (SPMaskView *v = mask->display; v != NULL; v = v->next) {
		if (v->key == key) {
		    v->bbox = bbox;
		    break;
		}
	}
}

/* Mask views */

SPMaskView *
sp_mask_view_new_prepend (SPMaskView *list, unsigned int key, Inkscape::DrawingItem *arenaitem)
{
	SPMaskView *new_mask_view = g_new (SPMaskView, 1);

	new_mask_view->next = list;
	new_mask_view->key = key;
	new_mask_view->arenaitem = arenaitem;
	new_mask_view->bbox = Geom::OptRect();

	return new_mask_view;
}

SPMaskView *
sp_mask_view_list_remove (SPMaskView *list, SPMaskView *view)
{
	if (view == list) {
		list = list->next;
	} else {
		SPMaskView *prev;
		prev = list;
		while (prev->next != view) prev = prev->next;
		prev->next = view->next;
	}

	delete view->arenaitem;
	g_free (view);

	return list;
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

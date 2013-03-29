/*
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <glibmm/i18n.h>

#include <xml/repr.h>
#include "display/curve.h"
#include "sp-shape.h"
#include "sp-text.h"
#include "sp-use.h"
#include "style.h"
#include "document.h"
#include "sp-title.h"
#include "sp-desc.h"

#include "sp-flowregion.h"

#include "display/canvas-bpath.h"


#include "livarot/Path.h"
#include "livarot/Shape.h"

static void sp_flowregion_init (SPFlowregion *group);
static void sp_flowregion_dispose (GObject *object);

G_DEFINE_TYPE(SPFlowregion, sp_flowregion, SP_TYPE_ITEM);

static void sp_flowregionexclude_init (SPFlowregionExclude *group);
static void sp_flowregionexclude_dispose (GObject *object);


static void         GetDest(SPObject* child,Shape **computed);

static void
sp_flowregion_class_init (SPFlowregionClass *klass)
{
	GObjectClass * object_class;
	object_class = (GObjectClass *) klass;
	object_class->dispose = sp_flowregion_dispose;
}

CFlowregion::CFlowregion(SPFlowregion* flowregion) : CItem(flowregion) {
	this->spflowregion = flowregion;
}

CFlowregion::~CFlowregion() {
}

static void
sp_flowregion_init (SPFlowregion *group)
{
	group->cflowregion = new CFlowregion(group);

	delete group->citem;
	group->citem = group->cflowregion;
	group->cobject = group->cflowregion;

	new (&group->computed) std::vector<Shape*>;
}

static void
sp_flowregion_dispose(GObject *object)
{
	SPFlowregion *group=(SPFlowregion *)object;
    for (std::vector<Shape*>::iterator it = group->computed.begin() ; it != group->computed.end() ; ++it)
        delete *it;
    group->computed.~vector<Shape*>();
}

void CFlowregion::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) {
	SPFlowregion* object = this->spflowregion;

	CItem::child_added(child, ref);

	object->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

/* fixme: hide (Lauris) */

void CFlowregion::remove_child(Inkscape::XML::Node * child) {
	SPFlowregion* object = this->spflowregion;

	CItem::remove_child(child);

	object->requestModified(SP_OBJECT_MODIFIED_FLAG);
}


void CFlowregion::update(SPCtx *ctx, unsigned int flags) {
	SPFlowregion* object = this->spflowregion;

    SPFlowregion *group = SP_FLOWREGION(object);

    SPItemCtx *ictx = reinterpret_cast<SPItemCtx *>(ctx);
    SPItemCtx cctx = *ictx;

    CItem::update(ctx, flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    GSList *l = NULL;
    for ( SPObject *child = object->firstChild() ; child ; child = child->getNext() ) {
        g_object_ref( G_OBJECT(child) );
        l = g_slist_prepend(l, child);
    }
    l = g_slist_reverse(l);
    while (l) {
        SPObject *child = SP_OBJECT(l->data);
        l = g_slist_remove(l, child);
        if (flags || (child->uflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            if (SP_IS_ITEM (child)) {
                SPItem const &chi = *SP_ITEM(child);
                cctx.i2doc = chi.transform * ictx->i2doc;
                cctx.i2vp = chi.transform * ictx->i2vp;
                child->updateDisplay((SPCtx *)&cctx, flags);
            } else {
                child->updateDisplay(ctx, flags);
            }
        }
        g_object_unref (G_OBJECT(child));
    }

    group->UpdateComputed();
}

void SPFlowregion::UpdateComputed(void)
{
    for (std::vector<Shape*>::iterator it = computed.begin() ; it != computed.end() ; ++it) {
        delete *it;
    }
    computed.clear();

    for (SPObject* child = firstChild() ; child ; child = child->getNext() ) {
        Shape *shape = 0;
        GetDest(child, &shape);
        computed.push_back(shape);
    }
}

void CFlowregion::modified(guint flags) {
	SPFlowregion* object = this->spflowregion;

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    GSList *l = NULL;
    for ( SPObject *child = object->firstChild() ; child ; child = child->getNext() ) {
        g_object_ref( G_OBJECT(child) );
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

Inkscape::XML::Node *CFlowregion::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	SPFlowregion* object = this->spflowregion;

    if (flags & SP_OBJECT_WRITE_BUILD) {
        if ( repr == NULL ) {
            repr = xml_doc->createElement("svg:flowRegion");
        }

        GSList *l = NULL;
        for ( SPObject *child = object->firstChild() ; child; child = child->getNext() ) {
            if ( !SP_IS_TITLE(child) && !SP_IS_DESC(child) ) {
                Inkscape::XML::Node *crepr = child->updateRepr(xml_doc, NULL, flags);
                if (crepr) {
                    l = g_slist_prepend(l, crepr);
                }
            }
        }

        while (l) {
            repr->addChild((Inkscape::XML::Node *) l->data, NULL);
            Inkscape::GC::release((Inkscape::XML::Node *) l->data);
            l = g_slist_remove(l, l->data);
        }

    } else {
        for ( SPObject *child = object->firstChild() ; child; child = child->getNext() ) {
            if ( !SP_IS_TITLE(child) && !SP_IS_DESC(child) ) {
                child->updateRepr(flags);
            }
        }
    }

    CItem::write(xml_doc, repr, flags);

    return repr;
}

gchar* CFlowregion::description() {
	// TRANSLATORS: "Flow region" is an area where text is allowed to flow
	return g_strdup_printf(_("Flow region"));
}

/*
 *
 */

G_DEFINE_TYPE(SPFlowregionExclude, sp_flowregionexclude, SP_TYPE_ITEM);

static void
sp_flowregionexclude_class_init (SPFlowregionExcludeClass *klass)
{
	GObjectClass * object_class;
	object_class = (GObjectClass *) klass;
	object_class->dispose = sp_flowregionexclude_dispose;
}

CFlowregionExclude::CFlowregionExclude(SPFlowregionExclude* flowregionexclude) : CItem(flowregionexclude) {
	this->spflowregionexclude = flowregionexclude;
}

CFlowregionExclude::~CFlowregionExclude() {
}

static void
sp_flowregionexclude_init (SPFlowregionExclude *group)
{
	group->cflowregionexclude = new CFlowregionExclude(group);

	delete group->citem;
	group->citem = group->cflowregionexclude;
	group->cobject = group->cflowregionexclude;

	group->computed = NULL;
}

static void
sp_flowregionexclude_dispose(GObject *object)
{
	SPFlowregionExclude *group=(SPFlowregionExclude *)object;
    if (group->computed) {
        delete group->computed;
        group->computed = NULL;
    }
}

void CFlowregionExclude::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) {
	SPFlowregionExclude* object = this->spflowregionexclude;

	CItem::child_added(child, ref);

	object->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

/* fixme: hide (Lauris) */

void CFlowregionExclude::remove_child(Inkscape::XML::Node * child) {
	SPFlowregionExclude* object = this->spflowregionexclude;

	CItem::remove_child(child);

	object->requestModified(SP_OBJECT_MODIFIED_FLAG);
}


void CFlowregionExclude::update(SPCtx *ctx, unsigned int flags) {
	SPFlowregionExclude* object = this->spflowregionexclude;

    SPFlowregionExclude *group = SP_FLOWREGIONEXCLUDE (object);

    SPItemCtx *ictx = reinterpret_cast<SPItemCtx *>(ctx);
    SPItemCtx cctx = *ictx;

    CItem::update(ctx, flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    GSList *l = NULL;
    for ( SPObject *child = object->firstChild() ; child ; child = child->getNext() ) {
        g_object_ref( G_OBJECT(child) );
        l = g_slist_prepend(l, child);
    }
    l = g_slist_reverse (l);
    while (l) {
        SPObject *child = SP_OBJECT(l->data);
        l = g_slist_remove(l, child);
        if (flags || (child->uflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            if (SP_IS_ITEM (child)) {
                SPItem const &chi = *SP_ITEM(child);
                cctx.i2doc = chi.transform * ictx->i2doc;
                cctx.i2vp = chi.transform * ictx->i2vp;
                child->updateDisplay((SPCtx *)&cctx, flags);
            } else {
                child->updateDisplay(ctx, flags);
            }
        }
        g_object_unref( G_OBJECT(child) );
    }

    group->UpdateComputed();
}


void SPFlowregionExclude::UpdateComputed(void)
{
    if (computed) {
        delete computed;
        computed = NULL;
    }

    for ( SPObject* child = firstChild() ; child ; child = child->getNext() ) {
        GetDest(child, &computed);
    }
}

void CFlowregionExclude::modified(guint flags) {
	SPFlowregionExclude* object = this->spflowregionexclude;

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    GSList *l = NULL;
    for ( SPObject *child = object->firstChild() ; child ; child = child->getNext() ) {
        g_object_ref( G_OBJECT(child) );
        l = g_slist_prepend(l, child);
    }
    l = g_slist_reverse (l);
    while (l) {
        SPObject *child = SP_OBJECT(l->data);
        l = g_slist_remove(l, child);
        if (flags || (child->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->emitModified(flags);
        }
        g_object_unref( G_OBJECT(child) );
    }
}

Inkscape::XML::Node *CFlowregionExclude::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	SPFlowregionExclude* object = this->spflowregionexclude;

    if (flags & SP_OBJECT_WRITE_BUILD) {
        if ( repr == NULL ) {
            repr = xml_doc->createElement("svg:flowRegionExclude");
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

    CItem::write(xml_doc, repr, flags);

    return repr;
}

gchar* CFlowregionExclude::description() {
	/* TRANSLATORS: A region "cut out of" a flow region; text is not allowed to flow inside the
	 * flow excluded region.  flowRegionExclude in SVG 1.2: see
	 * http://www.w3.org/TR/2004/WD-SVG12-20041027/flow.html#flowRegion-elem and
	 * http://www.w3.org/TR/2004/WD-SVG12-20041027/flow.html#flowRegionExclude-elem. */
	return g_strdup_printf(_("Flow excluded region"));
}

/*
 *
 */

static void         UnionShape(Shape **base_shape, Shape const *add_shape)
{
    if (*base_shape == NULL)
        *base_shape = new Shape;
	if ( (*base_shape)->hasEdges() == false ) {
		(*base_shape)->Copy(const_cast<Shape*>(add_shape));
	} else if ( add_shape->hasEdges() ) {
		Shape* temp=new Shape;
		temp->Booleen(const_cast<Shape*>(add_shape), *base_shape, bool_op_union);
		delete *base_shape;
		*base_shape = temp;
	}
}

static void         GetDest(SPObject* child,Shape **computed)
{
	if ( child == NULL ) return;

	SPCurve *curve=NULL;
	Geom::Affine tr_mat;

	SPObject* u_child=child;
	if ( SP_IS_USE(u_child) ) {
		u_child=SP_USE(u_child)->child;
		tr_mat = SP_ITEM(u_child)->getRelativeTransform(child->parent);
	} else {
		tr_mat = SP_ITEM(u_child)->transform;
	}
	if ( SP_IS_SHAPE (u_child) ) {
		curve = SP_SHAPE (u_child)->getCurve ();
	} else if ( SP_IS_TEXT (u_child) ) {
	curve = SP_TEXT (u_child)->getNormalizedBpath ();
	}

	if ( curve ) {
		Path*   temp=new Path;
        temp->LoadPathVector(curve->get_pathvector(), tr_mat, true);
		Shape*  n_shp=new Shape;
		temp->Convert(0.25);
		temp->Fill(n_shp,0);
		Shape*  uncross=new Shape;
		SPStyle* style = u_child->style;
		if ( style && style->fill_rule.computed == SP_WIND_RULE_EVENODD ) {
			uncross->ConvertToShape(n_shp,fill_oddEven);
		} else {
			uncross->ConvertToShape(n_shp,fill_nonZero);
		}
		UnionShape(computed, uncross);
		delete uncross;
		delete n_shp;
		delete temp;
		curve->unref();
	} else {
//		printf("no curve\n");
	}
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

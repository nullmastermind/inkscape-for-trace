/*
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "xml/repr.h"
//#include "svg/svg.h"

//#include "style.h"

#include "sp-flowdiv.h"
#include "sp-string.h"
#include "document.h"

static void sp_flowdiv_init (SPFlowdiv *group);
static void sp_flowtspan_init (SPFlowtspan *group);
static void sp_flowpara_init (SPFlowpara *group);
static void sp_flowline_init (SPFlowline *group);
static void sp_flowregionbreak_init (SPFlowregionbreak *group);

G_DEFINE_TYPE(SPFlowdiv, sp_flowdiv, SP_TYPE_ITEM);

static void sp_flowdiv_class_init(SPFlowdivClass *klass)
{
}

CFlowdiv::CFlowdiv(SPFlowdiv* flowdiv) : CItem(flowdiv) {
	this->spflowdiv = flowdiv;
}

CFlowdiv::~CFlowdiv() {
}

static void sp_flowdiv_init(SPFlowdiv *group)
{
	group->cflowdiv = new CFlowdiv(group);

	delete group->citem;
	group->citem = group->cflowdiv;
	group->cobject = group->cflowdiv;
}

void CFlowdiv::onRelease() {
	CItem::onRelease();
}


void CFlowdiv::onUpdate(SPCtx *ctx, unsigned int flags) {
	SPFlowdiv* object = this->spflowdiv;
    SPItemCtx *ictx = reinterpret_cast<SPItemCtx *>(ctx);
    SPItemCtx cctx = *ictx;

    CItem::onUpdate(ctx, flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    GSList* l = NULL;
    for (SPObject *child = object->firstChild() ; child ; child = child->getNext() ) {
        g_object_ref( G_OBJECT(child) );
        l = g_slist_prepend(l, child);
    }
    l = g_slist_reverse(l);
    while (l) {
        SPObject *child = SP_OBJECT(l->data);
        l = g_slist_remove(l, child);
        if (flags || (child->uflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            if (SP_IS_ITEM(child)) {
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
}

void CFlowdiv::onModified(unsigned int flags) {
	SPFlowdiv* object = this->spflowdiv;

    CItem::onModified(flags);

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


void CFlowdiv::onBuild(SPDocument *doc, Inkscape::XML::Node *repr) {
	SPFlowdiv* object = this->spflowdiv;

	object->_requireSVGVersion(Inkscape::Version(1, 2));

	CItem::onBuild(doc, repr);
}

void CFlowdiv::onSet(unsigned int key, const gchar* value) {
	CItem::onSet(key, value);
}


Inkscape::XML::Node* CFlowdiv::onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	SPFlowdiv* object = this->spflowdiv;

    if ( flags & SP_OBJECT_WRITE_BUILD ) {
        if ( repr == NULL ) {
            repr = xml_doc->createElement("svg:flowDiv");
        }
        GSList *l = NULL;
        for (SPObject* child = object->firstChild() ; child ; child = child->getNext() ) {
            Inkscape::XML::Node* c_repr = NULL;
            if ( SP_IS_FLOWTSPAN (child) ) {
                c_repr = child->updateRepr(xml_doc, NULL, flags);
            } else if ( SP_IS_FLOWPARA(child) ) {
                c_repr = child->updateRepr(xml_doc, NULL, flags);
            } else if ( SP_IS_STRING(child) ) {
                c_repr = xml_doc->createTextNode(SP_STRING(child)->string.c_str());
            }
            if ( c_repr ) {
                l = g_slist_prepend (l, c_repr);
            }
        }
        while ( l ) {
            repr->addChild((Inkscape::XML::Node *) l->data, NULL);
            Inkscape::GC::release((Inkscape::XML::Node *) l->data);
            l = g_slist_remove(l, l->data);
        }
    } else {
        for ( SPObject* child = object->firstChild() ; child ; child = child->getNext() ) {
            if ( SP_IS_FLOWTSPAN (child) ) {
                child->updateRepr(flags);
            } else if ( SP_IS_FLOWPARA(child) ) {
                child->updateRepr(flags);
            } else if ( SP_IS_STRING(child) ) {
                child->getRepr()->setContent(SP_STRING(child)->string.c_str());
            }
        }
    }

    CItem::onWrite(xml_doc, repr, flags);

    return repr;
}


/*
 *
 */
G_DEFINE_TYPE(SPFlowtspan, sp_flowtspan, SP_TYPE_ITEM);

static void sp_flowtspan_class_init(SPFlowtspanClass *klass)
{
}

CFlowtspan::CFlowtspan(SPFlowtspan* flowtspan) : CItem(flowtspan) {
	this->spflowtspan = flowtspan;
}

CFlowtspan::~CFlowtspan() {
}

static void sp_flowtspan_init(SPFlowtspan *group)
{
	group->cflowtspan = new CFlowtspan(group);

	delete group->citem;
	group->citem = group->cflowtspan;
	group->cobject = group->cflowtspan;
}

void CFlowtspan::onRelease() {
	CItem::onRelease();
}

void CFlowtspan::onUpdate(SPCtx *ctx, unsigned int flags) {
	SPFlowtspan* object = this->spflowtspan;

    SPItemCtx *ictx = reinterpret_cast<SPItemCtx *>(ctx);
    SPItemCtx cctx = *ictx;

    CItem::onUpdate(ctx, flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    GSList* l = NULL;
    for ( SPObject *child = object->firstChild() ; child ; child = child->getNext() ) {
        g_object_ref( G_OBJECT(child) );
        l = g_slist_prepend(l, child);
    }
    l = g_slist_reverse (l);
    while (l) {
        SPObject *child = SP_OBJECT(l->data);
        l = g_slist_remove(l, child);
        if (flags || (child->uflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            if (SP_IS_ITEM(child)) {
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
}

void CFlowtspan::onModified(unsigned int flags) {
	SPFlowtspan* object = this->spflowtspan;

    CItem::onModified(flags);

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


void CFlowtspan::onBuild(SPDocument *doc, Inkscape::XML::Node *repr)
{
	CItem::onBuild(doc, repr);
}

void CFlowtspan::onSet(unsigned int key, const gchar* value) {
	CItem::onSet(key, value);
}

Inkscape::XML::Node *CFlowtspan::onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
	SPFlowtspan* object = this->spflowtspan;

    if ( flags&SP_OBJECT_WRITE_BUILD ) {
        if ( repr == NULL ) {
            repr = xml_doc->createElement("svg:flowSpan");
        }
        GSList *l = NULL;
        for ( SPObject* child = object->firstChild() ; child ; child = child->getNext() ) {
            Inkscape::XML::Node* c_repr = NULL;
            if ( SP_IS_FLOWTSPAN(child) ) {
                c_repr = child->updateRepr(xml_doc, NULL, flags);
            } else if ( SP_IS_FLOWPARA(child) ) {
                c_repr = child->updateRepr(xml_doc, NULL, flags);
            } else if ( SP_IS_STRING(child) ) {
                c_repr = xml_doc->createTextNode(SP_STRING(child)->string.c_str());
            }
            if ( c_repr ) {
                l = g_slist_prepend(l, c_repr);
            }
        }
        while ( l ) {
            repr->addChild((Inkscape::XML::Node *) l->data, NULL);
            Inkscape::GC::release((Inkscape::XML::Node *) l->data);
            l = g_slist_remove(l, l->data);
        }
    } else {
        for ( SPObject* child = object->firstChild() ; child ; child = child->getNext() ) {
            if ( SP_IS_FLOWTSPAN(child) ) {
                child->updateRepr(flags);
            } else if ( SP_IS_FLOWPARA(child) ) {
                child->updateRepr(flags);
            } else if ( SP_IS_STRING(child) ) {
                child->getRepr()->setContent(SP_STRING(child)->string.c_str());
            }
        }
    }

    CItem::onWrite(xml_doc, repr, flags);

    return repr;
}




/*
 *
 */
G_DEFINE_TYPE(SPFlowpara, sp_flowpara, SP_TYPE_ITEM);

static void sp_flowpara_class_init(SPFlowparaClass *klass)
{
}

CFlowpara::CFlowpara(SPFlowpara* flowpara) : CItem(flowpara) {
	this->spflowpara = flowpara;
}

CFlowpara::~CFlowpara() {
}

static void sp_flowpara_init (SPFlowpara *group)
{
	group->cflowpara = new CFlowpara(group);

	delete group->citem;
	group->citem = group->cflowpara;
	group->cobject = group->cflowpara;
}

void CFlowpara::onRelease() {
	CItem::onRelease();
}


void CFlowpara::onUpdate(SPCtx *ctx, unsigned int flags)
{
	SPFlowpara* object = this->spflowpara;

    SPItemCtx *ictx = reinterpret_cast<SPItemCtx *>(ctx);
    SPItemCtx cctx = *ictx;

    CItem::onUpdate(ctx, flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    GSList* l = NULL;
    for ( SPObject *child = object->firstChild() ; child ; child = child->getNext() ) {
        g_object_ref( G_OBJECT(child) );
        l = g_slist_prepend(l, child);
    }
    l = g_slist_reverse (l);
    while (l) {
        SPObject *child = SP_OBJECT(l->data);
        l = g_slist_remove(l, child);
        if (flags || (child->uflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            if (SP_IS_ITEM(child)) {
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
}

void CFlowpara::onModified(unsigned int flags) {
	SPFlowpara* object = this->spflowpara;

    CItem::onModified(flags);

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


void CFlowpara::onBuild(SPDocument *doc, Inkscape::XML::Node *repr)
{
	CItem::onBuild(doc, repr);
}

void CFlowpara::onSet(unsigned int key, const gchar* value) {
	CItem::onSet(key, value);
}

Inkscape::XML::Node *CFlowpara::onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
	SPFlowpara* object = this->spflowpara;

    if ( flags&SP_OBJECT_WRITE_BUILD ) {
        if ( repr == NULL ) repr = xml_doc->createElement("svg:flowPara");
        GSList *l = NULL;
        for ( SPObject* child = object->firstChild() ; child ; child = child->getNext() ) {
            Inkscape::XML::Node* c_repr = NULL;
            if ( SP_IS_FLOWTSPAN(child) ) {
                c_repr = child->updateRepr(xml_doc, NULL, flags);
            } else if ( SP_IS_FLOWPARA(child) ) {
                c_repr = child->updateRepr(xml_doc, NULL, flags);
            } else if ( SP_IS_STRING(child) ) {
                c_repr = xml_doc->createTextNode(SP_STRING(child)->string.c_str());
            }
            if ( c_repr ) {
                l = g_slist_prepend(l, c_repr);
            }
        }
        while ( l ) {
            repr->addChild((Inkscape::XML::Node *) l->data, NULL);
            Inkscape::GC::release((Inkscape::XML::Node *) l->data);
            l = g_slist_remove(l, l->data);
        }
    } else {
        for ( SPObject* child = object->firstChild() ; child ; child = child->getNext() ) {
            if ( SP_IS_FLOWTSPAN(child) ) {
                child->updateRepr(flags);
            } else if ( SP_IS_FLOWPARA(child) ) {
                child->updateRepr(flags);
            } else if ( SP_IS_STRING(child) ) {
                child->getRepr()->setContent(SP_STRING(child)->string.c_str());
            }
        }
    }

    CItem::onWrite(xml_doc, repr, flags);

    return repr;
}


/*
 *
 */
G_DEFINE_TYPE(SPFlowline, sp_flowline, SP_TYPE_OBJECT);

static void sp_flowline_class_init(SPFlowlineClass *klass)
{
}

CFlowline::CFlowline(SPFlowline* flowline) : CObject(flowline) {
	this->spflowline = flowline;
}

CFlowline::~CFlowline() {
}

static void sp_flowline_init(SPFlowline *group)
{
	group->cflowline = new CFlowline(group);

	delete group->cobject;
	group->cobject = group->cflowline;
}

void CFlowline::onRelease() {
	CObject::onRelease();
}


void CFlowline::onModified(unsigned int flags) {
	CObject::onModified(flags);

	if (flags & SP_OBJECT_MODIFIED_FLAG) {
		flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
	}
	flags &= SP_OBJECT_MODIFIED_CASCADE;
}

Inkscape::XML::Node *CFlowline::onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
    if ( flags & SP_OBJECT_WRITE_BUILD ) {
        if ( repr == NULL ) {
            repr = xml_doc->createElement("svg:flowLine");
        }
    } else {
    }

    CObject::onWrite(xml_doc, repr, flags);

    return repr;
}


/*
 *
 */
G_DEFINE_TYPE(SPFlowregionbreak, sp_flowregionbreak, SP_TYPE_OBJECT);

static void sp_flowregionbreak_class_init(SPFlowregionbreakClass *klass)
{
}

CFlowregionbreak::CFlowregionbreak(SPFlowregionbreak* flowregionbreak) : CObject(flowregionbreak) {
	this->spflowregionbreak = flowregionbreak;
}

CFlowregionbreak::~CFlowregionbreak() {
}

static void sp_flowregionbreak_init(SPFlowregionbreak *group)
{
	group->cflowregionbreak = new CFlowregionbreak(group);

	delete group->cobject;
	group->cobject = group->cflowregionbreak;
}

void CFlowregionbreak::onRelease() {
	CObject::onRelease();
}

void CFlowregionbreak::onModified(unsigned int flags) {
	CObject::onModified(flags);

	if (flags & SP_OBJECT_MODIFIED_FLAG) {
		flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
	}
	flags &= SP_OBJECT_MODIFIED_CASCADE;
}

Inkscape::XML::Node *CFlowregionbreak::onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
    if ( flags & SP_OBJECT_WRITE_BUILD ) {
        if ( repr == NULL ) {
            repr = xml_doc->createElement("svg:flowLine");
        }
    } else {
    }

    CObject::onWrite(xml_doc, repr, flags);

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

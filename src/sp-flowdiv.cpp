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

#include "sp-factory.h"

namespace {
	SPObject* createFlowdiv() {
		return new SPFlowdiv();
	}

	SPObject* createFlowtspan() {
		return new SPFlowtspan();
	}

	SPObject* createFlowpara() {
		return new SPFlowpara();
	}

	SPObject* createFlowline() {
		return new SPFlowline();
	}

	SPObject* createFlowregionbreak() {
		return new SPFlowregionbreak();
	}

	bool flowdivRegistered = SPFactory::instance().registerObject("svg:flowDiv", createFlowdiv);
	bool flowtspanRegistered = SPFactory::instance().registerObject("svg:flowSpan", createFlowtspan);
	bool flowparaRegistered = SPFactory::instance().registerObject("svg:flowPara", createFlowpara);
	bool flowlineRegistered = SPFactory::instance().registerObject("svg:flowLine", createFlowline);
	bool flowregionbreakRegistered = SPFactory::instance().registerObject("svg:flowRegionBreak", createFlowregionbreak);
}

G_DEFINE_TYPE(SPFlowdiv, sp_flowdiv, G_TYPE_OBJECT);

static void sp_flowdiv_class_init(SPFlowdivClass *klass)
{
}

CFlowdiv::CFlowdiv(SPFlowdiv* flowdiv) : CItem(flowdiv) {
	this->spflowdiv = flowdiv;
}

CFlowdiv::~CFlowdiv() {
}

SPFlowdiv::SPFlowdiv() : SPItem() {
	SPFlowdiv* group = this;

	group->cflowdiv = new CFlowdiv(group);
	group->typeHierarchy.insert(typeid(SPFlowdiv));

	delete group->citem;
	group->citem = group->cflowdiv;
	group->cobject = group->cflowdiv;
}

static void sp_flowdiv_init(SPFlowdiv *group)
{
	new (group) SPFlowdiv();
}

void CFlowdiv::release() {
	CItem::release();
}


void CFlowdiv::update(SPCtx *ctx, unsigned int flags) {
	SPFlowdiv* object = this->spflowdiv;
    SPItemCtx *ictx = reinterpret_cast<SPItemCtx *>(ctx);
    SPItemCtx cctx = *ictx;

    CItem::update(ctx, flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    GSList* l = NULL;
    for (SPObject *child = object->firstChild() ; child ; child = child->getNext() ) {
        sp_object_ref(child);
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
        sp_object_unref(child);
    }
}

void CFlowdiv::modified(unsigned int flags) {
	SPFlowdiv* object = this->spflowdiv;

    CItem::modified(flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    GSList *l = NULL;
    for ( SPObject *child = object->firstChild() ; child ; child = child->getNext() ) {
        sp_object_ref(child);
        l = g_slist_prepend(l, child);
    }
    l = g_slist_reverse (l);
    while (l) {
        SPObject *child = SP_OBJECT(l->data);
        l = g_slist_remove(l, child);
        if (flags || (child->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->emitModified(flags);
        }
        sp_object_unref(child);
    }
}


void CFlowdiv::build(SPDocument *doc, Inkscape::XML::Node *repr) {
	SPFlowdiv* object = this->spflowdiv;

	object->_requireSVGVersion(Inkscape::Version(1, 2));

	CItem::build(doc, repr);
}

void CFlowdiv::set(unsigned int key, const gchar* value) {
	CItem::set(key, value);
}


Inkscape::XML::Node* CFlowdiv::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
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

    CItem::write(xml_doc, repr, flags);

    return repr;
}


/*
 *
 */
G_DEFINE_TYPE(SPFlowtspan, sp_flowtspan, G_TYPE_OBJECT);

static void sp_flowtspan_class_init(SPFlowtspanClass *klass)
{
}

CFlowtspan::CFlowtspan(SPFlowtspan* flowtspan) : CItem(flowtspan) {
	this->spflowtspan = flowtspan;
}

CFlowtspan::~CFlowtspan() {
}

SPFlowtspan::SPFlowtspan() : SPItem() {
	SPFlowtspan* group = this;

	group->cflowtspan = new CFlowtspan(group);
	group->typeHierarchy.insert(typeid(SPFlowtspan));

	delete group->citem;
	group->citem = group->cflowtspan;
	group->cobject = group->cflowtspan;
}

static void sp_flowtspan_init(SPFlowtspan *group)
{
	new (group) SPFlowtspan();
}

void CFlowtspan::release() {
	CItem::release();
}

void CFlowtspan::update(SPCtx *ctx, unsigned int flags) {
	SPFlowtspan* object = this->spflowtspan;

    SPItemCtx *ictx = reinterpret_cast<SPItemCtx *>(ctx);
    SPItemCtx cctx = *ictx;

    CItem::update(ctx, flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    GSList* l = NULL;
    for ( SPObject *child = object->firstChild() ; child ; child = child->getNext() ) {
        sp_object_ref(child);
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
        sp_object_unref(child);
    }
}

void CFlowtspan::modified(unsigned int flags) {
	SPFlowtspan* object = this->spflowtspan;

    CItem::modified(flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    GSList *l = NULL;
    for ( SPObject *child = object->firstChild() ; child ; child = child->getNext() ) {
        sp_object_ref(child);
        l = g_slist_prepend(l, child);
    }
    l = g_slist_reverse (l);
    while (l) {
        SPObject *child = SP_OBJECT(l->data);
        l = g_slist_remove(l, child);
        if (flags || (child->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->emitModified(flags);
        }
        sp_object_unref(child);
    }
}


void CFlowtspan::build(SPDocument *doc, Inkscape::XML::Node *repr)
{
	CItem::build(doc, repr);
}

void CFlowtspan::set(unsigned int key, const gchar* value) {
	CItem::set(key, value);
}

Inkscape::XML::Node *CFlowtspan::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
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

    CItem::write(xml_doc, repr, flags);

    return repr;
}




/*
 *
 */
G_DEFINE_TYPE(SPFlowpara, sp_flowpara, G_TYPE_OBJECT);

static void sp_flowpara_class_init(SPFlowparaClass *klass)
{
}

CFlowpara::CFlowpara(SPFlowpara* flowpara) : CItem(flowpara) {
	this->spflowpara = flowpara;
}

CFlowpara::~CFlowpara() {
}

SPFlowpara::SPFlowpara() : SPItem() {
	SPFlowpara* group = this;

	group->cflowpara = new CFlowpara(group);
	group->typeHierarchy.insert(typeid(SPFlowpara));

	delete group->citem;
	group->citem = group->cflowpara;
	group->cobject = group->cflowpara;
}

static void sp_flowpara_init (SPFlowpara *group)
{
	new (group) SPFlowpara();
}

void CFlowpara::release() {
	CItem::release();
}


void CFlowpara::update(SPCtx *ctx, unsigned int flags)
{
	SPFlowpara* object = this->spflowpara;

    SPItemCtx *ictx = reinterpret_cast<SPItemCtx *>(ctx);
    SPItemCtx cctx = *ictx;

    CItem::update(ctx, flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    GSList* l = NULL;
    for ( SPObject *child = object->firstChild() ; child ; child = child->getNext() ) {
        sp_object_ref(child);
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
        sp_object_unref(child);
    }
}

void CFlowpara::modified(unsigned int flags) {
	SPFlowpara* object = this->spflowpara;

    CItem::modified(flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    GSList *l = NULL;
    for ( SPObject *child = object->firstChild() ; child ; child = child->getNext() ) {
        sp_object_ref(child);
        l = g_slist_prepend(l, child);
    }
    l = g_slist_reverse (l);
    while (l) {
        SPObject *child = SP_OBJECT(l->data);
        l = g_slist_remove(l, child);
        if (flags || (child->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->emitModified(flags);
        }
        sp_object_unref(child);
    }
}


void CFlowpara::build(SPDocument *doc, Inkscape::XML::Node *repr)
{
	CItem::build(doc, repr);
}

void CFlowpara::set(unsigned int key, const gchar* value) {
	CItem::set(key, value);
}

Inkscape::XML::Node *CFlowpara::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
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

    CItem::write(xml_doc, repr, flags);

    return repr;
}


/*
 *
 */
G_DEFINE_TYPE(SPFlowline, sp_flowline, G_TYPE_OBJECT);

static void sp_flowline_class_init(SPFlowlineClass *klass)
{
}

CFlowline::CFlowline(SPFlowline* flowline) : CObject(flowline) {
	this->spflowline = flowline;
}

CFlowline::~CFlowline() {
}

SPFlowline::SPFlowline() : SPObject() {
	SPFlowline* group = this;

	group->cflowline = new CFlowline(group);
	group->typeHierarchy.insert(typeid(SPFlowline));

	delete group->cobject;
	group->cobject = group->cflowline;
}

static void sp_flowline_init(SPFlowline *group)
{
	new (group) SPFlowline();
}

void CFlowline::release() {
	CObject::release();
}


void CFlowline::modified(unsigned int flags) {
	CObject::modified(flags);

	if (flags & SP_OBJECT_MODIFIED_FLAG) {
		flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
	}
	flags &= SP_OBJECT_MODIFIED_CASCADE;
}

Inkscape::XML::Node *CFlowline::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
    if ( flags & SP_OBJECT_WRITE_BUILD ) {
        if ( repr == NULL ) {
            repr = xml_doc->createElement("svg:flowLine");
        }
    } else {
    }

    CObject::write(xml_doc, repr, flags);

    return repr;
}


/*
 *
 */
G_DEFINE_TYPE(SPFlowregionbreak, sp_flowregionbreak, G_TYPE_OBJECT);

static void sp_flowregionbreak_class_init(SPFlowregionbreakClass *klass)
{
}

CFlowregionbreak::CFlowregionbreak(SPFlowregionbreak* flowregionbreak) : CObject(flowregionbreak) {
	this->spflowregionbreak = flowregionbreak;
}

CFlowregionbreak::~CFlowregionbreak() {
}

SPFlowregionbreak::SPFlowregionbreak() : SPObject() {
	SPFlowregionbreak* group = this;

	group->cflowregionbreak = new CFlowregionbreak(group);
	group->typeHierarchy.insert(typeid(SPFlowregionbreak));

	delete group->cobject;
	group->cobject = group->cflowregionbreak;
}

static void sp_flowregionbreak_init(SPFlowregionbreak *group)
{
	new (group) SPFlowregionbreak();
}

void CFlowregionbreak::release() {
	CObject::release();
}

void CFlowregionbreak::modified(unsigned int flags) {
	CObject::modified(flags);

	if (flags & SP_OBJECT_MODIFIED_FLAG) {
		flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
	}
	flags &= SP_OBJECT_MODIFIED_CASCADE;
}

Inkscape::XML::Node *CFlowregionbreak::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
    if ( flags & SP_OBJECT_WRITE_BUILD ) {
        if ( repr == NULL ) {
            repr = xml_doc->createElement("svg:flowLine");
        }
    } else {
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

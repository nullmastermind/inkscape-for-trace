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

static void sp_flowdiv_class_init (SPFlowdivClass *klass);
static void sp_flowdiv_init (SPFlowdiv *group);
static void sp_flowdiv_release (SPObject *object);
static Inkscape::XML::Node *sp_flowdiv_write (SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);
static void sp_flowdiv_update (SPObject *object, SPCtx *ctx, unsigned int flags);
static void sp_flowdiv_modified (SPObject *object, guint flags);
static void sp_flowdiv_build (SPObject *object, SPDocument *doc, Inkscape::XML::Node *repr);
static void sp_flowdiv_set (SPObject *object, unsigned int key, const gchar *value);

static void sp_flowtspan_class_init (SPFlowtspanClass *klass);
static void sp_flowtspan_init (SPFlowtspan *group);
static void sp_flowtspan_release (SPObject *object);
static Inkscape::XML::Node *sp_flowtspan_write (SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);
static void sp_flowtspan_update (SPObject *object, SPCtx *ctx, unsigned int flags);
static void sp_flowtspan_modified (SPObject *object, guint flags);
static void sp_flowtspan_build (SPObject *object, SPDocument *doc, Inkscape::XML::Node *repr);
static void sp_flowtspan_set (SPObject *object, unsigned int key, const gchar *value);

static void sp_flowpara_class_init (SPFlowparaClass *klass);
static void sp_flowpara_init (SPFlowpara *group);
static void sp_flowpara_release (SPObject *object);
static Inkscape::XML::Node *sp_flowpara_write (SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);
static void sp_flowpara_update (SPObject *object, SPCtx *ctx, unsigned int flags);
static void sp_flowpara_modified (SPObject *object, guint flags);
static void sp_flowpara_build (SPObject *object, SPDocument *doc, Inkscape::XML::Node *repr);
static void sp_flowpara_set (SPObject *object, unsigned int key, const gchar *value);

static void sp_flowline_class_init (SPFlowlineClass *klass);
static void sp_flowline_release (SPObject *object);
static void sp_flowline_init (SPFlowline *group);
static Inkscape::XML::Node *sp_flowline_write (SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);
static void sp_flowline_modified (SPObject *object, guint flags);

static void sp_flowregionbreak_class_init (SPFlowregionbreakClass *klass);
static void sp_flowregionbreak_release (SPObject *object);
static void sp_flowregionbreak_init (SPFlowregionbreak *group);
static Inkscape::XML::Node *sp_flowregionbreak_write (SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);
static void sp_flowregionbreak_modified (SPObject *object, guint flags);

static SPItemClass * flowdiv_parent_class;
static SPItemClass * flowtspan_parent_class;
static SPItemClass * flowpara_parent_class;
static SPObjectClass * flowline_parent_class;
static SPObjectClass * flowregionbreak_parent_class;

GType sp_flowdiv_get_type(void)
{
	static GType group_type = 0;
	if (!group_type) {
		GTypeInfo group_info = {
			sizeof (SPFlowdivClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_flowdiv_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPFlowdiv),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_flowdiv_init,
			NULL,	/* value_table */
		};
		group_type = g_type_register_static (SP_TYPE_ITEM, "SPFlowdiv", &group_info, (GTypeFlags)0);
	}
	return group_type;
}

static void sp_flowdiv_class_init(SPFlowdivClass *klass)
{
    SPObjectClass *sp_object_class = reinterpret_cast<SPObjectClass *>(klass);

    flowdiv_parent_class = reinterpret_cast<SPItemClass *>(g_type_class_ref(SP_TYPE_ITEM));

    //sp_object_class->build = sp_flowdiv_build;
//    sp_object_class->set = sp_flowdiv_set;
//    sp_object_class->release = sp_flowdiv_release;
//    sp_object_class->write = sp_flowdiv_write;
//    sp_object_class->update = sp_flowdiv_update;
//    sp_object_class->modified = sp_flowdiv_modified;
}

CFlowdiv::CFlowdiv(SPFlowdiv* flowdiv) : CItem(flowdiv) {
	this->spflowdiv = flowdiv;
}

CFlowdiv::~CFlowdiv() {
}

static void sp_flowdiv_init(SPFlowdiv *group)
{
	group->cflowdiv = new CFlowdiv(group);
	group->citem = group->cflowdiv;
	group->cobject = group->cflowdiv;
}

void CFlowdiv::onRelease() {
	CItem::onRelease();
}

// CPPIFY: remove
static void sp_flowdiv_release(SPObject *object)
{
	((SPFlowdiv*)object)->cflowdiv->onRelease();
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

// CPPIFY: remove
static void sp_flowdiv_update(SPObject *object, SPCtx *ctx, unsigned int flags)
{
	((SPFlowdiv*)object)->cflowdiv->onUpdate(ctx, flags);
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

// CPPIFY: remove
static void sp_flowdiv_modified(SPObject *object, guint flags)
{
	((SPFlowdiv*)object)->cflowdiv->onModified(flags);
}

void CFlowdiv::onBuild(SPDocument *doc, Inkscape::XML::Node *repr) {
	SPFlowdiv* object = this->spflowdiv;

	object->_requireSVGVersion(Inkscape::Version(1, 2));

	CItem::onBuild(doc, repr);
}

// CPPIFY: remove
static void sp_flowdiv_build(SPObject *object, SPDocument *doc, Inkscape::XML::Node *repr)
{
    ((SPFlowdiv*)object)->cflowdiv->onBuild(doc, repr);
}

void CFlowdiv::onSet(unsigned int key, const gchar* value) {
	CItem::onSet(key, value);
}

// CPPIFY: remove
static void sp_flowdiv_set(SPObject *object, unsigned int key, const gchar *value)
{
    ((SPFlowdiv*)object)->cflowdiv->onSet(key, value);
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

// CPPIFY: remove
static Inkscape::XML::Node *sp_flowdiv_write(SPObject *object, Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
	return ((SPFlowdiv*)object)->cflowdiv->onWrite(xml_doc, repr, flags);
}


/*
 *
 */

GType
sp_flowtspan_get_type (void)
{
	static GType group_type = 0;
	if (!group_type) {
		GTypeInfo group_info = {
			sizeof (SPFlowtspanClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_flowtspan_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPFlowtspan),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_flowtspan_init,
			NULL,	/* value_table */
		};
		group_type = g_type_register_static (SP_TYPE_ITEM, "SPFlowtspan", &group_info, (GTypeFlags)0);
	}
	return group_type;
}

static void sp_flowtspan_class_init(SPFlowtspanClass *klass)
{
    SPObjectClass *sp_object_class = reinterpret_cast<SPObjectClass *>(klass);

    flowtspan_parent_class = reinterpret_cast<SPItemClass *>(g_type_class_ref(SP_TYPE_ITEM));

    //sp_object_class->build = sp_flowtspan_build;
//    sp_object_class->set = sp_flowtspan_set;
//    sp_object_class->release = sp_flowtspan_release;
//    sp_object_class->write = sp_flowtspan_write;
//    sp_object_class->update = sp_flowtspan_update;
//    sp_object_class->modified = sp_flowtspan_modified;
}

CFlowtspan::CFlowtspan(SPFlowtspan* flowtspan) : CItem(flowtspan) {
	this->spflowtspan = flowtspan;
}

CFlowtspan::~CFlowtspan() {
}

static void sp_flowtspan_init(SPFlowtspan *group)
{
	group->cflowtspan = new CFlowtspan(group);
	group->citem = group->cflowtspan;
	group->cobject = group->cflowtspan;
}

void CFlowtspan::onRelease() {
	CItem::onRelease();
}

// CPPIFY: remove
static void sp_flowtspan_release(SPObject *object)
{
    ((SPFlowtspan*)object)->cflowtspan->onRelease();
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

// CPPIFY: remove
static void sp_flowtspan_update(SPObject *object, SPCtx *ctx, unsigned int flags)
{
	((SPFlowtspan*)object)->cflowtspan->onUpdate(ctx, flags);
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

// CPPIFY: remove
static void sp_flowtspan_modified(SPObject *object, guint flags)
{
	((SPFlowtspan*)object)->cflowtspan->onModified(flags);
}

void CFlowtspan::onBuild(SPDocument *doc, Inkscape::XML::Node *repr)
{
	CItem::onBuild(doc, repr);
}

// CPPIFY: remove
static void sp_flowtspan_build(SPObject *object, SPDocument *doc, Inkscape::XML::Node *repr)
{
    ((SPFlowtspan*)object)->cflowtspan->onBuild(doc, repr);
}

void CFlowtspan::onSet(unsigned int key, const gchar* value) {
	CItem::onSet(key, value);
}

// CPPIFY: remove
static void sp_flowtspan_set(SPObject *object, unsigned int key, const gchar *value)
{
    ((SPFlowtspan*)object)->cflowtspan->onSet(key, value);
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

// CPPIFY: remove
static Inkscape::XML::Node *sp_flowtspan_write(SPObject *object, Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
	return ((SPFlowtspan*)object)->cflowtspan->onWrite(xml_doc, repr, flags);
}



/*
 *
 */

GType
sp_flowpara_get_type (void)
{
	static GType group_type = 0;
	if (!group_type) {
		GTypeInfo group_info = {
			sizeof (SPFlowparaClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_flowpara_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPFlowpara),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_flowpara_init,
			NULL,	/* value_table */
		};
		group_type = g_type_register_static (SP_TYPE_ITEM, "SPFlowpara", &group_info, (GTypeFlags)0);
	}
	return group_type;
}

static void sp_flowpara_class_init(SPFlowparaClass *klass)
{
    SPObjectClass *sp_object_class = reinterpret_cast<SPObjectClass *>(klass);

    flowpara_parent_class = reinterpret_cast<SPItemClass *>(g_type_class_ref(SP_TYPE_ITEM));

    //sp_object_class->build = sp_flowpara_build;
//    sp_object_class->set = sp_flowpara_set;
//    sp_object_class->release = sp_flowpara_release;
//    sp_object_class->write = sp_flowpara_write;
//    sp_object_class->update = sp_flowpara_update;
//    sp_object_class->modified = sp_flowpara_modified;
}

CFlowpara::CFlowpara(SPFlowpara* flowpara) : CItem(flowpara) {
	this->spflowpara = flowpara;
}

CFlowpara::~CFlowpara() {
}

static void sp_flowpara_init (SPFlowpara *group)
{
	group->cflowpara = new CFlowpara(group);
	group->citem = group->cflowpara;
	group->cobject = group->cflowpara;
}

void CFlowpara::onRelease() {
	CItem::onRelease();
}

// CPPIFY: remove
static void sp_flowpara_release(SPObject *object)
{
    ((SPFlowpara*)object)->cflowpara->onRelease();
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

// CPPIFY: remove
static void sp_flowpara_update(SPObject *object, SPCtx *ctx, unsigned int flags)
{
	((SPFlowpara*)object)->cflowpara->onUpdate(ctx, flags);
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

// CPPIFY: remove
static void sp_flowpara_modified(SPObject *object, guint flags)
{
	((SPFlowpara*)object)->cflowpara->onModified(flags);
}

void CFlowpara::onBuild(SPDocument *doc, Inkscape::XML::Node *repr)
{
	CItem::onBuild(doc, repr);
}

// CPPIFY: remove
static void sp_flowpara_build(SPObject *object, SPDocument *doc, Inkscape::XML::Node *repr)
{
    ((SPFlowpara*)object)->cflowpara->onBuild(doc, repr);
}

void CFlowpara::onSet(unsigned int key, const gchar* value) {
	CItem::onSet(key, value);
}

// CPPIFY: remove
static void sp_flowpara_set(SPObject *object, unsigned int key, const gchar *value)
{
    ((SPFlowpara*)object)->cflowpara->onSet(key, value);
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

// CPPIFY: remove
static Inkscape::XML::Node *sp_flowpara_write(SPObject *object, Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
	return ((SPFlowpara*)object)->cflowpara->onWrite(xml_doc, repr, flags);
}

/*
 *
 */

GType sp_flowline_get_type(void)
{
	static GType group_type = 0;
	if (!group_type) {
		GTypeInfo group_info = {
			sizeof (SPFlowlineClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_flowline_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPFlowline),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_flowline_init,
			NULL,	/* value_table */
		};
		group_type = g_type_register_static (SP_TYPE_OBJECT, "SPFlowline", &group_info, (GTypeFlags)0);
	}
	return group_type;
}

static void sp_flowline_class_init(SPFlowlineClass *klass)
{
    SPObjectClass *sp_object_class = reinterpret_cast<SPObjectClass *>(klass);

    flowline_parent_class = reinterpret_cast<SPObjectClass *>(g_type_class_ref(SP_TYPE_OBJECT));

//    sp_object_class->release = sp_flowline_release;
//    sp_object_class->write = sp_flowline_write;
//    sp_object_class->modified = sp_flowline_modified;
}

CFlowline::CFlowline(SPFlowline* flowline) : CObject(flowline) {
	this->spflowline = flowline;
}

CFlowline::~CFlowline() {
}

static void sp_flowline_init(SPFlowline *group)
{
	group->cflowline = new CFlowline(group);
	group->cobject = group->cflowline;
}

void CFlowline::onRelease() {
	CObject::onRelease();
}

// CPPIFY: remove
static void sp_flowline_release(SPObject *object)
{
    ((SPFlowline*)object)->cflowline->onRelease();
}

void CFlowline::onModified(unsigned int flags) {
	CObject::onModified(flags);

	if (flags & SP_OBJECT_MODIFIED_FLAG) {
		flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
	}
	flags &= SP_OBJECT_MODIFIED_CASCADE;
}

// CPPIFY: remove
static void sp_flowline_modified(SPObject *object, guint flags)
{
	((SPFlowline*)object)->cflowline->onModified(flags);
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

// CPPIFY: remove
static Inkscape::XML::Node *sp_flowline_write(SPObject *object, Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
	return ((SPFlowline*)object)->cflowline->onWrite(xml_doc, repr, flags);
}

/*
 *
 */

GType sp_flowregionbreak_get_type(void)
{
	static GType group_type = 0;
	if (!group_type) {
		GTypeInfo group_info = {
			sizeof (SPFlowregionbreakClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_flowregionbreak_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPFlowregionbreak),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_flowregionbreak_init,
			NULL,	/* value_table */
		};
		group_type = g_type_register_static (SP_TYPE_OBJECT, "SPFlowregionbreak", &group_info, (GTypeFlags)0);
	}
	return group_type;
}

static void sp_flowregionbreak_class_init(SPFlowregionbreakClass *klass)
{
    SPObjectClass *sp_object_class = reinterpret_cast<SPObjectClass *>(klass);

    flowregionbreak_parent_class = reinterpret_cast<SPObjectClass *>(g_type_class_ref(SP_TYPE_OBJECT));

//    sp_object_class->release = sp_flowregionbreak_release;
//    sp_object_class->write = sp_flowregionbreak_write;
//    sp_object_class->modified = sp_flowregionbreak_modified;
}

CFlowregionbreak::CFlowregionbreak(SPFlowregionbreak* flowregionbreak) : CObject(flowregionbreak) {
	this->spflowregionbreak = flowregionbreak;
}

CFlowregionbreak::~CFlowregionbreak() {
}

static void sp_flowregionbreak_init(SPFlowregionbreak *group)
{
	group->cflowregionbreak = new CFlowregionbreak(group);
	group->cobject = group->cflowregionbreak;
}

void CFlowregionbreak::onRelease() {
	CObject::onRelease();
}

// CPPIFY: remove
static void sp_flowregionbreak_release(SPObject *object)
{
    ((SPFlowregionbreak*)object)->cflowregionbreak->onRelease();
}

void CFlowregionbreak::onModified(unsigned int flags) {
	CObject::onModified(flags);

	if (flags & SP_OBJECT_MODIFIED_FLAG) {
		flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
	}
	flags &= SP_OBJECT_MODIFIED_CASCADE;
}

// CPPIFY: remove
static void sp_flowregionbreak_modified(SPObject *object, guint flags)
{
    ((SPFlowregionbreak*)object)->cflowregionbreak->onModified(flags);
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

// CPPIFY: remove
static Inkscape::XML::Node *sp_flowregionbreak_write(SPObject *object, Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
	return ((SPFlowregionbreak*)object)->cflowregionbreak->onWrite(xml_doc, repr, flags);
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

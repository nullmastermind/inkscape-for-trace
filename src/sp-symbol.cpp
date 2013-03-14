/*
 * SVG <symbol> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1999-2003 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cstring>
#include <string>

#include <2geom/transforms.h>
#include "display/drawing-group.h"
#include "xml/repr.h"
#include "attributes.h"
#include "print.h"
#include "sp-symbol.h"
#include "document.h"

static void sp_symbol_class_init (SPSymbolClass *klass);
static void sp_symbol_init (SPSymbol *symbol);

static void sp_symbol_build (SPObject *object, SPDocument *document, Inkscape::XML::Node *repr);
static void sp_symbol_release (SPObject *object);
static void sp_symbol_set (SPObject *object, unsigned int key, const gchar *value);
static void sp_symbol_child_added (SPObject *object, Inkscape::XML::Node *child, Inkscape::XML::Node *ref);
static void sp_symbol_update (SPObject *object, SPCtx *ctx, guint flags);
static void sp_symbol_modified (SPObject *object, guint flags);
static Inkscape::XML::Node *sp_symbol_write (SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);

static Inkscape::DrawingItem *sp_symbol_show (SPItem *item, Inkscape::Drawing &drawing, unsigned int key, unsigned int flags);
static void sp_symbol_hide (SPItem *item, unsigned int key);
static Geom::OptRect sp_symbol_bbox(SPItem const *item, Geom::Affine const &transform, SPItem::BBoxType type);
static void sp_symbol_print (SPItem *item, SPPrintContext *ctx);

static SPGroupClass *parent_class;

GType
sp_symbol_get_type (void)
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof (SPSymbolClass),
            NULL, NULL,
            (GClassInitFunc) sp_symbol_class_init,
            NULL, NULL,
            sizeof (SPSymbol),
            16,
            (GInstanceInitFunc) sp_symbol_init,
            NULL,	/* value_table */
        };
        type = g_type_register_static (SP_TYPE_GROUP, "SPSymbol", &info, (GTypeFlags)0);
    }
    return type;
}

static void sp_symbol_class_init(SPSymbolClass *klass)
{
    SPObjectClass *sp_object_class = (SPObjectClass *) klass;
    SPItemClass *sp_item_class = (SPItemClass *) klass;

    parent_class = (SPGroupClass *)g_type_class_ref (SP_TYPE_GROUP);

    //sp_object_class->build = sp_symbol_build;
    sp_object_class->release = sp_symbol_release;
    sp_object_class->set = sp_symbol_set;
    sp_object_class->child_added = sp_symbol_child_added;
    sp_object_class->update = sp_symbol_update;
    sp_object_class->modified = sp_symbol_modified;
    sp_object_class->write = sp_symbol_write;

//    sp_item_class->show = sp_symbol_show;
//    sp_item_class->hide = sp_symbol_hide;
//    sp_item_class->bbox = sp_symbol_bbox;
//    sp_item_class->print = sp_symbol_print;
}

CSymbol::CSymbol(SPSymbol* symbol) : CGroup(symbol) {
	this->spsymbol = symbol;
}

CSymbol::~CSymbol() {
}

static void sp_symbol_init(SPSymbol *symbol)
{
	symbol->csymbol = new CSymbol(symbol);
	symbol->cgroup = symbol->csymbol;
	symbol->clpeitem = symbol->csymbol;
	symbol->citem = symbol->csymbol;
	symbol->cobject = symbol->csymbol;

    symbol->viewBox_set = FALSE;
    symbol->c2p = Geom::identity();
}

void CSymbol::onBuild(SPDocument *document, Inkscape::XML::Node *repr) {
	SPSymbol* object = this->spsymbol;

    object->readAttr( "viewBox" );
    object->readAttr( "preserveAspectRatio" );

    CGroup::onBuild(document, repr);
}

// CPPIFY: remove
static void sp_symbol_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr)
{
	((SPSymbol*)object)->csymbol->onBuild(document, repr);
}

void CSymbol::onRelease() {
	CGroup::onRelease();
}

// CPPIFY: remove
static void sp_symbol_release(SPObject *object)
{
	((SPSymbol*)object)->csymbol->onRelease();
}

void CSymbol::onSet(unsigned int key, const gchar* value) {
	SPSymbol* object = this->spsymbol;

    SPSymbol *symbol = SP_SYMBOL(object);

    switch (key) {
    case SP_ATTR_VIEWBOX:
        if (value) {
            double x, y, width, height;
            char *eptr;
            /* fixme: We have to take original item affine into account */
            /* fixme: Think (Lauris) */
            eptr = (gchar *) value;
            x = g_ascii_strtod (eptr, &eptr);
            while (*eptr && ((*eptr == ',') || (*eptr == ' '))) eptr++;
            y = g_ascii_strtod (eptr, &eptr);
            while (*eptr && ((*eptr == ',') || (*eptr == ' '))) eptr++;
            width = g_ascii_strtod (eptr, &eptr);
            while (*eptr && ((*eptr == ',') || (*eptr == ' '))) eptr++;
            height = g_ascii_strtod (eptr, &eptr);
            while (*eptr && ((*eptr == ',') || (*eptr == ' '))) eptr++;
            if ((width > 0) && (height > 0)) {
                /* Set viewbox */
                symbol->viewBox = Geom::Rect::from_xywh(x, y, width, height);
                symbol->viewBox_set = TRUE;
            } else {
                symbol->viewBox_set = FALSE;
            }
        } else {
            symbol->viewBox_set = FALSE;
        }
        object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
        break;
    case SP_ATTR_PRESERVEASPECTRATIO:
        /* Do setup before, so we can use break to escape */
        symbol->aspect_set = FALSE;
        symbol->aspect_align = SP_ASPECT_NONE;
        symbol->aspect_clip = SP_ASPECT_MEET;
        object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
        if (value) {
            int len;
            gchar c[256];
            const gchar *p, *e;
            unsigned int align, clip;
            p = value;
            while (*p && *p == 32) p += 1;
            if (!*p) break;
            e = p;
            while (*e && *e != 32) e += 1;
            len = e - p;
            if (len > 8) break;
            memcpy (c, value, len);
            c[len] = 0;
            /* Now the actual part */
            if (!strcmp (c, "none")) {
                align = SP_ASPECT_NONE;
            } else if (!strcmp (c, "xMinYMin")) {
                align = SP_ASPECT_XMIN_YMIN;
            } else if (!strcmp (c, "xMidYMin")) {
                align = SP_ASPECT_XMID_YMIN;
            } else if (!strcmp (c, "xMaxYMin")) {
                align = SP_ASPECT_XMAX_YMIN;
            } else if (!strcmp (c, "xMinYMid")) {
                align = SP_ASPECT_XMIN_YMID;
            } else if (!strcmp (c, "xMidYMid")) {
                align = SP_ASPECT_XMID_YMID;
            } else if (!strcmp (c, "xMaxYMid")) {
                align = SP_ASPECT_XMAX_YMID;
            } else if (!strcmp (c, "xMinYMax")) {
                align = SP_ASPECT_XMIN_YMAX;
            } else if (!strcmp (c, "xMidYMax")) {
                align = SP_ASPECT_XMID_YMAX;
            } else if (!strcmp (c, "xMaxYMax")) {
                align = SP_ASPECT_XMAX_YMAX;
            } else {
                break;
            }
            clip = SP_ASPECT_MEET;
            while (*e && *e == 32) e += 1;
            if (*e) {
                if (!strcmp (e, "meet")) {
                    clip = SP_ASPECT_MEET;
                } else if (!strcmp (e, "slice")) {
                    clip = SP_ASPECT_SLICE;
                } else {
                    break;
                }
            }
            symbol->aspect_set = TRUE;
            symbol->aspect_align = align;
            symbol->aspect_clip = clip;
        }
        break;
    default:
        CGroup::onSet(key, value);
        break;
    }
}

// CPPIFY: remove
static void sp_symbol_set(SPObject *object, unsigned int key, const gchar *value)
{
	((SPSymbol*)object)->csymbol->onSet(key, value);
}

void CSymbol::onChildAdded(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) {
	CGroup::onChildAdded(child, ref);
}

// CPPIFY: remove
static void sp_symbol_child_added(SPObject *object, Inkscape::XML::Node *child, Inkscape::XML::Node *ref)
{
	((SPSymbol*)object)->csymbol->onChildAdded(child, ref);
}

void CSymbol::onUpdate(SPCtx *ctx, guint flags) {
	SPSymbol* object = this->spsymbol;
    SPSymbol *symbol = object;

    SPItemCtx *ictx = (SPItemCtx *) ctx;
    SPItemCtx rctx;

    if (object->cloned) {
        /* Cloned <symbol> is actually renderable */

        /* fixme: We have to set up clip here too */

        /* Create copy of item context */
        rctx = *ictx;

        /* Calculate child to parent transformation */
        /* Apply parent <use> translation (set up as vewport) */
        symbol->c2p = Geom::Translate(rctx.viewport.min());

        if (symbol->viewBox_set) {
            double x, y, width, height;
            /* Determine actual viewbox in viewport coordinates */
            if (symbol->aspect_align == SP_ASPECT_NONE) {
                x = 0.0;
                y = 0.0;
                width = rctx.viewport.width();
                height = rctx.viewport.height();
            } else {
                double scalex, scaley, scale;
                /* Things are getting interesting */
                scalex = rctx.viewport.width() / symbol->viewBox.width();
                scaley = rctx.viewport.height() / symbol->viewBox.height();
                scale = (symbol->aspect_clip == SP_ASPECT_MEET) ? MIN (scalex, scaley) : MAX (scalex, scaley);
                width = symbol->viewBox.width() * scale;
                height = symbol->viewBox.height() * scale;
                /* Now place viewbox to requested position */
                switch (symbol->aspect_align) {
                case SP_ASPECT_XMIN_YMIN:
                    x = 0.0;
                    y = 0.0;
                    break;
                case SP_ASPECT_XMID_YMIN:
                    x = 0.5 * (rctx.viewport.width() - width);
                    y = 0.0;
                    break;
                case SP_ASPECT_XMAX_YMIN:
                    x = 1.0 * (rctx.viewport.width() - width);
                    y = 0.0;
                    break;
                case SP_ASPECT_XMIN_YMID:
                    x = 0.0;
                    y = 0.5 * (rctx.viewport.height() - height);
                    break;
                case SP_ASPECT_XMID_YMID:
                    x = 0.5 * (rctx.viewport.width() - width);
                    y = 0.5 * (rctx.viewport.height() - height);
                    break;
                case SP_ASPECT_XMAX_YMID:
                    x = 1.0 * (rctx.viewport.width() - width);
                    y = 0.5 * (rctx.viewport.height() - height);
                    break;
                case SP_ASPECT_XMIN_YMAX:
                    x = 0.0;
                    y = 1.0 * (rctx.viewport.height() - height);
                    break;
                case SP_ASPECT_XMID_YMAX:
                    x = 0.5 * (rctx.viewport.width() - width);
                    y = 1.0 * (rctx.viewport.height() - height);
                    break;
                case SP_ASPECT_XMAX_YMAX:
                    x = 1.0 * (rctx.viewport.width() - width);
                    y = 1.0 * (rctx.viewport.height() - height);
                    break;
                default:
                    x = 0.0;
                    y = 0.0;
                    break;
                }
            }
            /* Compose additional transformation from scale and position */
            Geom::Affine q;
            q[0] = width / symbol->viewBox.width();
            q[1] = 0.0;
            q[2] = 0.0;
            q[3] = height / symbol->viewBox.height();
            q[4] = -symbol->viewBox.left() * q[0] + x;
            q[5] = -symbol->viewBox.top() * q[3] + y;
            /* Append viewbox transformation */
            symbol->c2p = q * symbol->c2p;
        }

        rctx.i2doc = symbol->c2p * (Geom::Affine)rctx.i2doc;

        /* If viewBox is set initialize child viewport */
        /* Otherwise <use> has set it up already */
        if (symbol->viewBox_set) {
            rctx.viewport = symbol->viewBox;
            rctx.i2vp = Geom::identity();
        }

        // And invoke parent method
        CGroup::onUpdate((SPCtx *) &rctx, flags);

        // As last step set additional transform of drawing group
        for (SPItemView *v = symbol->display; v != NULL; v = v->next) {
        	Inkscape::DrawingGroup *g = dynamic_cast<Inkscape::DrawingGroup *>(v->arenaitem);
        	g->setChildTransform(symbol->c2p);
        }
    } else {
        // No-op
        CGroup::onUpdate(ctx, flags);
    }
}

// CPPIFY: remove
static void sp_symbol_update(SPObject *object, SPCtx *ctx, guint flags)
{
	((SPSymbol*)object)->csymbol->onUpdate(ctx, flags);
}

void CSymbol::onModified(unsigned int flags) {
	CGroup::onModified(flags);
}

// CPPIFY: remove
static void sp_symbol_modified(SPObject *object, guint flags)
{
	((SPSymbol*)object)->csymbol->onModified(flags);
}

Inkscape::XML::Node* CSymbol::onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	SPSymbol* object = this->spsymbol;

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:symbol");
    }

    //XML Tree being used directly here while it shouldn't be.
    repr->setAttribute("viewBox", object->getRepr()->attribute("viewBox"));
	
    //XML Tree being used directly here while it shouldn't be.
    repr->setAttribute("preserveAspectRatio", object->getRepr()->attribute("preserveAspectRatio"));

    CGroup::onWrite(xml_doc, repr, flags);

    return repr;
}

// CPPIFY: remove
static Inkscape::XML::Node *sp_symbol_write(SPObject *object, Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
	return ((SPSymbol*)object)->csymbol->onWrite(xml_doc, repr, flags);
}

Inkscape::DrawingItem* CSymbol::onShow(Inkscape::Drawing &drawing, unsigned int key, unsigned int flags) {
	SPSymbol* item = this->spsymbol;

    SPSymbol *symbol = SP_SYMBOL(item);
    Inkscape::DrawingItem *ai = 0;

    if (symbol->cloned) {
        // Cloned <symbol> is actually renderable
        ai = CGroup::onShow(drawing, key, flags);
        Inkscape::DrawingGroup *g = dynamic_cast<Inkscape::DrawingGroup *>(ai);
		if (g) {
			g->setChildTransform(symbol->c2p);
		}
    }

    return ai;
}

// CPPIFY: remove
static Inkscape::DrawingItem *sp_symbol_show(SPItem *item, Inkscape::Drawing &drawing, unsigned int key, unsigned int flags)
{
	return ((SPSymbol*)item)->csymbol->onShow(drawing, key, flags);
}

void CSymbol::onHide(unsigned int key) {
	SPSymbol* item = this->spsymbol;

    SPSymbol *symbol = SP_SYMBOL(item);

    if (symbol->cloned) {
        /* Cloned <symbol> is actually renderable */
        CGroup::onHide(key);
    }
}

// CPPIFY: remove
static void sp_symbol_hide(SPItem *item, unsigned int key)
{
	((SPSymbol*)item)->csymbol->onHide(key);
}

Geom::OptRect CSymbol::onBbox(Geom::Affine const &transform, SPItem::BBoxType type) {
	SPSymbol* item = this->spsymbol;

    SPSymbol const *symbol = SP_SYMBOL(item);
    Geom::OptRect bbox;

    if (symbol->cloned) {
        // Cloned <symbol> is actually renderable
    	Geom::Affine const a( symbol->c2p * transform );
    	bbox = CGroup::onBbox(a, type);

    }
    return bbox;
}

// CPPIFY: remove
static Geom::OptRect sp_symbol_bbox(SPItem const *item, Geom::Affine const &transform, SPItem::BBoxType type)
{
	return ((SPSymbol*)item)->csymbol->onBbox(transform, type);
}

void CSymbol::onPrint(SPPrintContext* ctx) {
	SPSymbol* item = this->spsymbol;

    SPSymbol *symbol = SP_SYMBOL(item);
    if (symbol->cloned) {
        // Cloned <symbol> is actually renderable

        sp_print_bind(ctx, symbol->c2p, 1.0);

        CGroup::onPrint(ctx);

        sp_print_release (ctx);
    }
}

// CPPIFY: remove
static void sp_symbol_print(SPItem *item, SPPrintContext *ctx)
{
	((SPSymbol*)item)->csymbol->onPrint(ctx);
}

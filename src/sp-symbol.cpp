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

G_DEFINE_TYPE(SPSymbol, sp_symbol, SP_TYPE_GROUP);

static void sp_symbol_class_init(SPSymbolClass *klass)
{
}

CSymbol::CSymbol(SPSymbol* symbol) : CGroup(symbol) {
	this->spsymbol = symbol;
}

CSymbol::~CSymbol() {
}

static void sp_symbol_init(SPSymbol *symbol)
{
	symbol->csymbol = new CSymbol(symbol);

	delete symbol->cgroup;
	symbol->cgroup = symbol->csymbol;
	symbol->clpeitem = symbol->csymbol;
	symbol->citem = symbol->csymbol;
	symbol->cobject = symbol->csymbol;

    symbol->viewBox_set = FALSE;
    symbol->c2p = Geom::identity();
}

void CSymbol::build(SPDocument *document, Inkscape::XML::Node *repr) {
	SPSymbol* object = this->spsymbol;

    object->readAttr( "viewBox" );
    object->readAttr( "preserveAspectRatio" );

    CGroup::build(document, repr);
}

void CSymbol::release() {
	CGroup::release();
}

void CSymbol::set(unsigned int key, const gchar* value) {
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
        CGroup::set(key, value);
        break;
    }
}

void CSymbol::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) {
	CGroup::child_added(child, ref);
}


void CSymbol::update(SPCtx *ctx, guint flags) {
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
        CGroup::update((SPCtx *) &rctx, flags);

        // As last step set additional transform of drawing group
        for (SPItemView *v = symbol->display; v != NULL; v = v->next) {
        	Inkscape::DrawingGroup *g = dynamic_cast<Inkscape::DrawingGroup *>(v->arenaitem);
        	g->setChildTransform(symbol->c2p);
        }
    } else {
        // No-op
        CGroup::update(ctx, flags);
    }
}

void CSymbol::modified(unsigned int flags) {
	CGroup::modified(flags);
}


Inkscape::XML::Node* CSymbol::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	SPSymbol* object = this->spsymbol;

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:symbol");
    }

    //XML Tree being used directly here while it shouldn't be.
    repr->setAttribute("viewBox", object->getRepr()->attribute("viewBox"));
	
    //XML Tree being used directly here while it shouldn't be.
    repr->setAttribute("preserveAspectRatio", object->getRepr()->attribute("preserveAspectRatio"));

    CGroup::write(xml_doc, repr, flags);

    return repr;
}

Inkscape::DrawingItem* CSymbol::show(Inkscape::Drawing &drawing, unsigned int key, unsigned int flags) {
	SPSymbol* item = this->spsymbol;

    SPSymbol *symbol = SP_SYMBOL(item);
    Inkscape::DrawingItem *ai = 0;

    if (symbol->cloned) {
        // Cloned <symbol> is actually renderable
        ai = CGroup::show(drawing, key, flags);
        Inkscape::DrawingGroup *g = dynamic_cast<Inkscape::DrawingGroup *>(ai);
		if (g) {
			g->setChildTransform(symbol->c2p);
		}
    }

    return ai;
}

void CSymbol::hide(unsigned int key) {
	SPSymbol* item = this->spsymbol;

    SPSymbol *symbol = SP_SYMBOL(item);

    if (symbol->cloned) {
        /* Cloned <symbol> is actually renderable */
        CGroup::hide(key);
    }
}


Geom::OptRect CSymbol::bbox(Geom::Affine const &transform, SPItem::BBoxType type) {
	SPSymbol* item = this->spsymbol;

    SPSymbol const *symbol = SP_SYMBOL(item);
    Geom::OptRect bbox;

    if (symbol->cloned) {
        // Cloned <symbol> is actually renderable
    	Geom::Affine const a( symbol->c2p * transform );
    	bbox = CGroup::bbox(a, type);
    }
    return bbox;
}

void CSymbol::print(SPPrintContext* ctx) {
	SPSymbol* item = this->spsymbol;

    SPSymbol *symbol = SP_SYMBOL(item);
    if (symbol->cloned) {
        // Cloned <symbol> is actually renderable

        sp_print_bind(ctx, symbol->c2p, 1.0);

        CGroup::print(ctx);

        sp_print_release (ctx);
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

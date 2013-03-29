/*
 * SVG <text> and <tspan> implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

/*
 * fixme:
 *
 * These subcomponents should not be items, or alternately
 * we have to invent set of flags to mark, whether standard
 * attributes are applicable to given item (I even like this
 * idea somewhat - Lauris)
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cstring>
#include <string>
#include <glibmm/i18n.h>

#include <livarot/Path.h>
#include "svg/stringstream.h"
#include "attributes.h"
#include "sp-use-reference.h"
#include "sp-tspan.h"
#include "sp-tref.h"
#include "sp-textpath.h"
#include "text-editing.h"
#include "style.h"
#include "xml/repr.h"
#include "document.h"


/*#####################################################
#  SPTSPAN
#####################################################*/
G_DEFINE_TYPE(SPTSpan, sp_tspan, SP_TYPE_ITEM);

static void
sp_tspan_class_init(SPTSpanClass *classname)
{
}

CTSpan::CTSpan(SPTSpan* span) : CItem(span) {
	this->sptspan = span;
}

CTSpan::~CTSpan() {
}

static void
sp_tspan_init(SPTSpan *tspan)
{
	tspan->ctspan = new CTSpan(tspan);

	delete tspan->citem;
	tspan->citem = tspan->ctspan;
	tspan->cobject = tspan->ctspan;

    tspan->role = SP_TSPAN_ROLE_UNSPECIFIED;
    new (&tspan->attributes) TextTagAttributes;
}

void CTSpan::release() {
	SPTSpan* object = this->sptspan;

    SPTSpan *tspan = SP_TSPAN(object);

    tspan->attributes.~TextTagAttributes();

    CItem::release();
}

void CTSpan::build(SPDocument *doc, Inkscape::XML::Node *repr) {
	SPTSpan* object = this->sptspan;

    object->readAttr( "x" );
    object->readAttr( "y" );
    object->readAttr( "dx" );
    object->readAttr( "dy" );
    object->readAttr( "rotate" );
    object->readAttr( "sodipodi:role" );

    CItem::build(doc, repr);
}


void CTSpan::set(unsigned int key, const gchar* value) {
	SPTSpan* object = this->sptspan;

    SPTSpan *tspan = SP_TSPAN(object);

    if (tspan->attributes.readSingleAttribute(key, value)) {
        object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    } else {
        switch (key) {
            case SP_ATTR_SODIPODI_ROLE:
                if (value && (!strcmp(value, "line") || !strcmp(value, "paragraph"))) {
                    tspan->role = SP_TSPAN_ROLE_LINE;
                } else {
                    tspan->role = SP_TSPAN_ROLE_UNSPECIFIED;
                }
                break;
            default:
                CItem::set(key, value);
                break;
        }
    }
}

void CTSpan::update(SPCtx *ctx, guint flags) {
	SPTSpan* object = this->sptspan;

    CItem::update(ctx, flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    for ( SPObject *ochild = object->firstChild() ; ochild ; ochild = ochild->getNext() ) {
        if ( flags || ( ochild->uflags & SP_OBJECT_MODIFIED_FLAG )) {
	    ochild->updateDisplay(ctx, flags);
        }
    }
}

void CTSpan::modified(unsigned int flags) {
	SPTSpan* object = this->sptspan;

//    CItem::onModified(flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    for ( SPObject *ochild = object->firstChild() ; ochild ; ochild = ochild->getNext() ) {
        if (flags || (ochild->mflags & SP_OBJECT_MODIFIED_FLAG)) {
            ochild->emitModified(flags);
        }
    }
}

Geom::OptRect CTSpan::bbox(Geom::Affine const &transform, SPItem::BBoxType type) {
	SPTSpan* item = this->sptspan;

    Geom::OptRect bbox;
    // find out the ancestor text which holds our layout
    SPObject const *parent_text = item;
    while (parent_text && !SP_IS_TEXT(parent_text)) {
        parent_text = parent_text->parent;
    }
    if (parent_text == NULL) {
        return bbox;
    }

    // get the bbox of our portion of the layout
    bbox = SP_TEXT(parent_text)->layout.bounds(transform, sp_text_get_length_upto(parent_text, item), sp_text_get_length_upto(item, NULL) - 1);
    if (!bbox) return bbox;

    // Add stroke width
    // FIXME this code is incorrect
    if (type == SPItem::VISUAL_BBOX && !item->style->stroke.isNone()) {
        double scale = transform.descrim();
        bbox->expandBy(0.5 * item->style->stroke_width.computed * scale);
    }
    return bbox;
}

Inkscape::XML::Node* CTSpan::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	SPTSpan* object = this->sptspan;

    SPTSpan *tspan = SP_TSPAN(object);

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:tspan");
    }

    tspan->attributes.writeTo(repr);

    if ( flags&SP_OBJECT_WRITE_BUILD ) {
        GSList *l = NULL;
        for (SPObject* child = object->firstChild() ; child ; child = child->getNext() ) {
            Inkscape::XML::Node* c_repr=NULL;
            if ( SP_IS_TSPAN(child) || SP_IS_TREF(child) ) {
                c_repr = child->updateRepr(xml_doc, NULL, flags);
            } else if ( SP_IS_TEXTPATH(child) ) {
                //c_repr = child->updateRepr(xml_doc, NULL, flags); // shouldn't happen
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
        for (SPObject* child = object->firstChild() ; child ; child = child->getNext() ) {
            if ( SP_IS_TSPAN(child) || SP_IS_TREF(child) ) {
                child->updateRepr(flags);
            } else if ( SP_IS_TEXTPATH(child) ) {
                //c_repr = child->updateRepr(xml_doc, NULL, flags); // shouldn't happen
            } else if ( SP_IS_STRING(child) ) {
                child->getRepr()->setContent(SP_STRING(child)->string.c_str());
            }
        }
    }

    CItem::write(xml_doc, repr, flags);

    return repr;
}

gchar* CTSpan::description() {
	SPTSpan* item = this->sptspan;

	g_return_val_if_fail(SP_IS_TSPAN(item), NULL);

    return g_strdup(_("<b>Text span</b>"));
}


/*#####################################################
#  SPTEXTPATH
#####################################################*/

static void sp_textpath_finalize(GObject *obj);

void   refresh_textpath_source(SPTextPath* offset);

G_DEFINE_TYPE(SPTextPath, sp_textpath, SP_TYPE_ITEM);

static void
sp_textpath_class_init(SPTextPathClass *classname)
{
    GObjectClass  *gobject_class   = G_OBJECT_CLASS(classname);
    gobject_class->finalize = sp_textpath_finalize;
}


CTextPath::CTextPath(SPTextPath* textpath) : CItem(textpath) {
	this->sptextpath = textpath;
}

CTextPath::~CTextPath() {
}

static void
sp_textpath_init(SPTextPath *textpath)
{
	textpath->ctextpath = new CTextPath(textpath);

	delete textpath->citem;
	textpath->citem = textpath->ctextpath;
	textpath->cobject = textpath->ctextpath;

    new (&textpath->attributes) TextTagAttributes;

    textpath->startOffset._set = false;
    textpath->originalPath = NULL;
    textpath->isUpdating=false;
    // set up the uri reference
    textpath->sourcePath = new SPUsePath(textpath);
    textpath->sourcePath->user_unlink = sp_textpath_to_text;
}

static void
sp_textpath_finalize(GObject *obj)
{
    SPTextPath *textpath = (SPTextPath *) obj;

    delete textpath->sourcePath;
}

void CTextPath::release() {
	SPTextPath* object = this->sptextpath;

    SPTextPath *textpath = SP_TEXTPATH(object);

    textpath->attributes.~TextTagAttributes();

    if (textpath->originalPath) delete textpath->originalPath;
    textpath->originalPath = NULL;

    CItem::release();
}

void CTextPath::build(SPDocument *doc, Inkscape::XML::Node *repr) {
	SPTextPath* object = this->sptextpath;

    object->readAttr( "x" );
    object->readAttr( "y" );
    object->readAttr( "dx" );
    object->readAttr( "dy" );
    object->readAttr( "rotate" );
    object->readAttr( "startOffset" );
    object->readAttr( "xlink:href" );

    bool  no_content = true;
    for (Inkscape::XML::Node* rch = repr->firstChild() ; rch != NULL; rch = rch->next()) {
        if ( rch->type() == Inkscape::XML::TEXT_NODE )
        {
            no_content = false;
            break;
        }
    }

    if ( no_content ) {
        Inkscape::XML::Document *xml_doc = doc->getReprDoc();
        Inkscape::XML::Node* rch = xml_doc->createTextNode("");
        repr->addChild(rch, NULL);
    }

    CItem::build(doc, repr);
}

void CTextPath::set(unsigned int key, const gchar* value) {
	SPTextPath* object = this->sptextpath;

    SPTextPath *textpath = SP_TEXTPATH(object);

    if (textpath->attributes.readSingleAttribute(key, value)) {
        object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    } else {
        switch (key) {
            case SP_ATTR_XLINK_HREF:
                textpath->sourcePath->link((char*)value);
                break;
            case SP_ATTR_STARTOFFSET:
                textpath->startOffset.readOrUnset(value);
                object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
                break;
            default:
                CItem::set(key, value);
                break;
        }
    }
}

void CTextPath::update(SPCtx *ctx, guint flags) {
	SPTextPath* object = this->sptextpath;

    SPTextPath *textpath = SP_TEXTPATH(object);

    textpath->isUpdating = true;
    if ( textpath->sourcePath->sourceDirty ) {
        refresh_textpath_source(textpath);
    }
    textpath->isUpdating = false;

    CItem::update(ctx, flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    for ( SPObject *ochild = object->firstChild() ; ochild ; ochild = ochild->getNext() ) {
        if ( flags || ( ochild->uflags & SP_OBJECT_MODIFIED_FLAG )) {
            ochild->updateDisplay(ctx, flags);
        }
    }
}


void   refresh_textpath_source(SPTextPath* tp)
{
    if ( tp == NULL ) return;
    tp->sourcePath->refresh_source();
    tp->sourcePath->sourceDirty=false;

    // finalisons
    if ( tp->sourcePath->originalPath ) {
        if (tp->originalPath) {
            delete tp->originalPath;
        }
        tp->originalPath = NULL;

        tp->originalPath = new Path;
        tp->originalPath->Copy(tp->sourcePath->originalPath);
        tp->originalPath->ConvertWithBackData(0.01);

    }
}

void CTextPath::modified(unsigned int flags) {
	SPTextPath* object = this->sptextpath;

//    CItem::onModified(flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    for ( SPObject *ochild = object->firstChild() ; ochild ; ochild = ochild->getNext() ) {
        if (flags || (ochild->mflags & SP_OBJECT_MODIFIED_FLAG)) {
            ochild->emitModified(flags);
        }
    }
}

Inkscape::XML::Node* CTextPath::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	SPTextPath* object = this->sptextpath;

    SPTextPath *textpath = SP_TEXTPATH(object);

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:textPath");
    }

    textpath->attributes.writeTo(repr);
    if (textpath->startOffset._set) {
        if (textpath->startOffset.unit == SVGLength::PERCENT) {
	        Inkscape::SVGOStringStream os;
            os << (textpath->startOffset.computed * 100.0) << "%";
            textpath->getRepr()->setAttribute("startOffset", os.str().c_str());
        } else {
            /* FIXME: This logic looks rather undesirable if e.g. startOffset is to be
               in ems. */
            sp_repr_set_svg_double(repr, "startOffset", textpath->startOffset.computed);
        }
    }

    if ( textpath->sourcePath->sourceHref ) repr->setAttribute("xlink:href", textpath->sourcePath->sourceHref);

    if ( flags&SP_OBJECT_WRITE_BUILD ) {
        GSList *l = NULL;
        for (SPObject* child = object->firstChild() ; child ; child = child->getNext() ) {
            Inkscape::XML::Node* c_repr=NULL;
            if ( SP_IS_TSPAN(child) || SP_IS_TREF(child) ) {
                c_repr = child->updateRepr(xml_doc, NULL, flags);
            } else if ( SP_IS_TEXTPATH(child) ) {
                //c_repr = child->updateRepr(xml_doc, NULL, flags); // shouldn't happen
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
        for (SPObject* child = object->firstChild() ; child ; child = child->getNext() ) {
            if ( SP_IS_TSPAN(child) || SP_IS_TREF(child) ) {
                child->updateRepr(flags);
            } else if ( SP_IS_TEXTPATH(child) ) {
                //c_repr = child->updateRepr(xml_doc, NULL, flags); // shouldn't happen
            } else if ( SP_IS_STRING(child) ) {
                child->getRepr()->setContent(SP_STRING(child)->string.c_str());
            }
        }
    }

    CItem::write(xml_doc, repr, flags);

    return repr;
}


SPItem *
sp_textpath_get_path_item(SPTextPath *tp)
{
    if (tp && tp->sourcePath) {
        SPItem *refobj = tp->sourcePath->getObject();
        if (SP_IS_ITEM(refobj))
            return (SPItem *) refobj;
    }
    return NULL;
}

void
sp_textpath_to_text(SPObject *tp)
{
    SPObject *text = tp->parent;

    Geom::OptRect bbox = SP_ITEM(text)->geometricBounds(SP_ITEM(text)->i2doc_affine());
    if (!bbox) return;
    Geom::Point xy = bbox->min();

    // make a list of textpath children
    GSList *tp_reprs = NULL;
    for (SPObject *o = tp->firstChild() ; o != NULL; o = o->next) {
        tp_reprs = g_slist_prepend(tp_reprs, o->getRepr());
    }

    for ( GSList *i = tp_reprs ; i ; i = i->next ) {
        // make a copy of each textpath child
        Inkscape::XML::Node *copy = ((Inkscape::XML::Node *) i->data)->duplicate(text->getRepr()->document());
        // remove the old repr from under textpath
        tp->getRepr()->removeChild((Inkscape::XML::Node *) i->data);
        // put its copy under text
        text->getRepr()->addChild(copy, NULL); // fixme: copy id
    }

    //remove textpath
    tp->deleteObject();
    g_slist_free(tp_reprs);

    // set x/y on text
    /* fixme: Yuck, is this really the right test? */
    if (xy[Geom::X] != 1e18 && xy[Geom::Y] != 1e18) {
        sp_repr_set_svg_double(text->getRepr(), "x", xy[Geom::X]);
        sp_repr_set_svg_double(text->getRepr(), "y", xy[Geom::Y]);
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

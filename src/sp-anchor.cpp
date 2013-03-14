/*
 * SVG <a> element implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define noSP_ANCHOR_VERBOSE

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glibmm/i18n.h>
#include "xml/quote.h"
#include "xml/repr.h"
#include "attributes.h"
#include "sp-anchor.h"
#include "ui/view/view.h"
#include "document.h"

static void sp_anchor_class_init(SPAnchorClass *ac);
static void sp_anchor_init(SPAnchor *anchor);

static void sp_anchor_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr);
static void sp_anchor_release(SPObject *object);
static void sp_anchor_set(SPObject *object, unsigned int key, const gchar *value);
static Inkscape::XML::Node *sp_anchor_write(SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);

static gchar *sp_anchor_description(SPItem *item);
static gint sp_anchor_event(SPItem *item, SPEvent *event);

static SPGroupClass *parent_class;

GType sp_anchor_get_type(void)
{
    static GType type = 0;

    if (!type) {
        GTypeInfo info = {
            sizeof(SPAnchorClass),
            NULL,	/* base_init */
            NULL,	/* base_finalize */
            (GClassInitFunc) sp_anchor_class_init,
            NULL,	/* class_finalize */
            NULL,	/* class_data */
            sizeof(SPAnchor),
            16,	/* n_preallocs */
            (GInstanceInitFunc) sp_anchor_init,
            NULL,	/* value_table */
        };
        type = g_type_register_static(SP_TYPE_GROUP, "SPAnchor", &info, (GTypeFlags) 0);
    }

    return type;
}

static void sp_anchor_class_init(SPAnchorClass *ac)
{
    SPObjectClass *sp_object_class = (SPObjectClass *) ac;
    SPItemClass *item_class = (SPItemClass *) ac;

    parent_class = (SPGroupClass *) g_type_class_ref(SP_TYPE_GROUP);

    //sp_object_class->build = sp_anchor_build;
    sp_object_class->release = sp_anchor_release;
    sp_object_class->set = sp_anchor_set;
    sp_object_class->write = sp_anchor_write;

//    item_class->description = sp_anchor_description;
//    item_class->event = sp_anchor_event;
}

CAnchor::CAnchor(SPAnchor* anchor) : CGroup(anchor) {
	this->spanchor = anchor;
}

CAnchor::~CAnchor() {
}

static void sp_anchor_init(SPAnchor *anchor)
{
	anchor->canchor = new CAnchor(anchor);
	anchor->cgroup = anchor->canchor;
	anchor->clpeitem = anchor->canchor;
	anchor->citem = anchor->canchor;
	anchor->cobject = anchor->canchor;

    anchor->href = NULL;
}

void CAnchor::onBuild(SPDocument *document, Inkscape::XML::Node *repr) {
	SPAnchor* object = this->spanchor;

    CGroup::onBuild(document, repr);

    object->readAttr( "xlink:type" );
    object->readAttr( "xlink:role" );
    object->readAttr( "xlink:arcrole" );
    object->readAttr( "xlink:title" );
    object->readAttr( "xlink:show" );
    object->readAttr( "xlink:actuate" );
    object->readAttr( "xlink:href" );
    object->readAttr( "target" );
}

// CPPIFY: remove
static void sp_anchor_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr)
{
	((SPAnchor*)object)->canchor->onBuild(document, repr);
}

void CAnchor::onRelease() {
    SPAnchor *anchor = this->spanchor;

    if (anchor->href) {
        g_free(anchor->href);
        anchor->href = NULL;
    }

    CGroup::onRelease();
}

// CPPIFY: remove
static void sp_anchor_release(SPObject *object)
{
	((SPAnchor*)object)->canchor->onRelease();
}

void CAnchor::onSet(unsigned int key, const gchar* value) {
    SPAnchor *anchor = this->spanchor;
    SPAnchor* object = anchor;

    switch (key) {
	case SP_ATTR_XLINK_HREF:
            g_free(anchor->href);
            anchor->href = g_strdup(value);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
	case SP_ATTR_XLINK_TYPE:
	case SP_ATTR_XLINK_ROLE:
	case SP_ATTR_XLINK_ARCROLE:
	case SP_ATTR_XLINK_TITLE:
	case SP_ATTR_XLINK_SHOW:
	case SP_ATTR_XLINK_ACTUATE:
	case SP_ATTR_TARGET:
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
	default:
            CGroup::onSet(key, value);
            break;
    }
}

// CPPIFY: remove
static void sp_anchor_set(SPObject *object, unsigned int key, const gchar *value)
{
	((SPAnchor*)object)->canchor->onSet(key, value);
}

#define COPY_ATTR(rd,rs,key) (rd)->setAttribute((key), rs->attribute(key));

Inkscape::XML::Node* CAnchor::onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
    SPAnchor *anchor = this->spanchor;
    SPAnchor* object = anchor;

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:a");
    }

    repr->setAttribute("xlink:href", anchor->href);

    if (repr != object->getRepr()) {
        // XML Tree being directly used while it shouldn't be in the
        //  below COPY_ATTR lines
        COPY_ATTR(repr, object->getRepr(), "xlink:type");
        COPY_ATTR(repr, object->getRepr(), "xlink:role");
        COPY_ATTR(repr, object->getRepr(), "xlink:arcrole");
        COPY_ATTR(repr, object->getRepr(), "xlink:title");
        COPY_ATTR(repr, object->getRepr(), "xlink:show");
        COPY_ATTR(repr, object->getRepr(), "xlink:actuate");
        COPY_ATTR(repr, object->getRepr(), "target");
    }

    CGroup::onWrite(xml_doc, repr, flags);

    return repr;
}

// CPPIFY: remove
static Inkscape::XML::Node *sp_anchor_write(SPObject *object, Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
	return ((SPAnchor*)object)->canchor->onWrite(xml_doc, repr, flags);
}

gchar* CAnchor::onDescription() {
    SPAnchor *anchor = this->spanchor;

    if (anchor->href) {
        char *quoted_href = xml_quote_strdup(anchor->href);
        char *ret = g_strdup_printf(_("<b>Link</b> to %s"), quoted_href);
        g_free(quoted_href);
        return ret;
    } else {
        return g_strdup (_("<b>Link</b> without URI"));
    }
}

// CPPIFY: remove
static gchar *sp_anchor_description(SPItem *item)
{
	return ((SPAnchor*)item)->canchor->onDescription();
}

gint CAnchor::onEvent(SPEvent* event) {
    SPAnchor *anchor = this->spanchor;

    switch (event->type) {
	case SP_EVENT_ACTIVATE:
            if (anchor->href) {
                g_print("Activated xlink:href=\"%s\"\n", anchor->href);
                return TRUE;
            }
            break;
	case SP_EVENT_MOUSEOVER:
            (static_cast<Inkscape::UI::View::View*>(event->data))->mouseover();
            break;
	case SP_EVENT_MOUSEOUT:
            (static_cast<Inkscape::UI::View::View*>(event->data))->mouseout();
            break;
	default:
            break;
    }

    return FALSE;
}

/* fixme: We should forward event to appropriate container/view */

// CPPIFY: remove
static gint sp_anchor_event(SPItem *item, SPEvent *event)
{
	return ((SPAnchor*)item)->canchor->onEvent(event);
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

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG <a> element implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2017 Martin Owens
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#define noSP_ANCHOR_VERBOSE

#include <glibmm/i18n.h>
#include "xml/quote.h"
#include "xml/repr.h"
#include "attributes.h"
#include "sp-anchor.h"
#include "ui/view/svg-view-widget.h"
#include "document.h"

SPAnchor::SPAnchor() : SPGroup() {
    this->href = nullptr;
    this->type = nullptr;
    this->title = nullptr;
    this->page = nullptr;
}

SPAnchor::~SPAnchor() = default;

void SPAnchor::build(SPDocument *document, Inkscape::XML::Node *repr) {
    SPGroup::build(document, repr);

    this->readAttr( "xlink:type" );
    this->readAttr( "xlink:role" );
    this->readAttr( "xlink:arcrole" );
    this->readAttr( "xlink:title" );
    this->readAttr( "xlink:show" );
    this->readAttr( "xlink:actuate" );
    this->readAttr( "xlink:href" );
    this->readAttr( "target" );
}

void SPAnchor::release() {
    if (this->href) {
        g_free(this->href);
        this->href = nullptr;
    }
    if (this->type) {
        g_free(this->type);
        this->type = nullptr;
    }
    if (this->title) {
        g_free(this->title);
        this->title = nullptr;
    }
    if (this->page) {
        g_free(this->page);
        this->page = nullptr;
    }

    SPGroup::release();
}

void SPAnchor::set(SPAttributeEnum key, const gchar* value) {
    switch (key) {
	case SP_ATTR_XLINK_HREF:
            g_free(this->href);
            this->href = g_strdup(value);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            this->updatePageAnchor();
            break;
	case SP_ATTR_XLINK_TYPE:
            g_free(this->type);
            this->type = g_strdup(value);
            this->updatePageAnchor();
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
	case SP_ATTR_XLINK_ROLE:
	case SP_ATTR_XLINK_ARCROLE:
	case SP_ATTR_XLINK_TITLE:
            g_free(this->title);
            this->title = g_strdup(value);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
	case SP_ATTR_XLINK_SHOW:
	case SP_ATTR_XLINK_ACTUATE:
	case SP_ATTR_TARGET:
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;

	default:
            SPGroup::set(key, value);
            break;
    }
}

/*
 * Detect if this anchor qualifies as a page link and append
 * the new page document to this document.
 */
void SPAnchor::updatePageAnchor() {
    if (this->type && !strcmp(this->type, "page")) {
        if (this->href && !this->page) {
             this->page = this->document->createChildDoc(this->href);
        }
    }    
}

#define COPY_ATTR(rd,rs,key) (rd)->setAttribute((key), rs->attribute(key));

Inkscape::XML::Node* SPAnchor::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:a");
    }

    repr->setAttribute("xlink:href", this->href);
    if (this->type) repr->setAttribute("xlink:type", this->type);
    if (this->title) repr->setAttribute("xlink:title", this->title);

    if (repr != this->getRepr()) {
        // XML Tree being directly used while it shouldn't be in the
        //  below COPY_ATTR lines
        COPY_ATTR(repr, this->getRepr(), "xlink:role");
        COPY_ATTR(repr, this->getRepr(), "xlink:arcrole");
        COPY_ATTR(repr, this->getRepr(), "xlink:show");
        COPY_ATTR(repr, this->getRepr(), "xlink:actuate");
        COPY_ATTR(repr, this->getRepr(), "target");
    }

    SPGroup::write(xml_doc, repr, flags);

    return repr;
}

const char* SPAnchor::displayName() const {
    return _("Link");
}

gchar* SPAnchor::description() const {
    if (this->href) {
        char *quoted_href = xml_quote_strdup(this->href);
        char *ret = g_strdup_printf(_("to %s"), quoted_href);
        g_free(quoted_href);
        return ret;
    } else {
        return g_strdup (_("without URI"));
    }
}

/* fixme: We should forward event to appropriate container/view */
/* The only use of SPEvent appears to be here, to change the cursor in Inkview when over a link (and
 * which hasn't worked since at least 0.48). GUI code should not be here. */
int SPAnchor::event(SPEvent* event) {

    switch (event->type) {
	case SPEvent::ACTIVATE:
            if (this->href) {
                // If this actually worked, it could be useful to open a webpage with the link.
                g_print("Activated xlink:href=\"%s\"\n", this->href);
                return TRUE;
            }
            break;

	case SPEvent::MOUSEOVER:
        {
            if (event->view) {
                event->view->mouseover();
            }
            break;
        }

	case SPEvent::MOUSEOUT:
        {
            if (event->view) {
                event->view->mouseout();
            }
            break;
        }

	default:
            break;
    }

    return FALSE;
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

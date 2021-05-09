// SPDX-License-Identifier: GPL-2.0-or-later
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
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <string>
#include <glibmm/i18n.h>
#include <2geom/transforms.h>

#include "display/drawing-group.h"
#include "xml/repr.h"
#include "attributes.h"
#include "print.h"
#include "sp-symbol.h"
#include "document.h"
#include "inkscape.h"
#include "desktop.h"

SPSymbol::SPSymbol() : SPGroup(), SPViewBox() {
}

SPSymbol::~SPSymbol() = default;

void SPSymbol::build(SPDocument *document, Inkscape::XML::Node *repr) {
    this->readAttr(SPAttr::VIEWBOX);
    this->readAttr(SPAttr::PRESERVEASPECTRATIO);

    SPGroup::build(document, repr);
}

void SPSymbol::release() {
	SPGroup::release();
}

void SPSymbol::set(SPAttr key, const gchar* value) {
    switch (key) {
    case SPAttr::VIEWBOX:
        set_viewBox( value );
        // std::cout << "Symbol: ViewBox: " << viewBox << std::endl;
        this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
        break;

    case SPAttr::PRESERVEASPECTRATIO:
        set_preserveAspectRatio( value );
        // std::cout << "Symbol: Preserve aspect ratio: " << aspect_align << ", " << aspect_clip << std::endl;
        this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
        break;

    default:
        SPGroup::set(key, value);
        break;
    }
}

void SPSymbol::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) {
	SPGroup::child_added(child, ref);
}

void SPSymbol::unSymbol()
{
    SPDocument *doc = this->document;
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();
    // Check if something is selected.

    doc->ensureUpToDate();

    // Create new <g> and insert in current layer
    Inkscape::XML::Node *group = xml_doc->createElement("svg:g");
    //TODO: Better handle if no desktop, currently go to defs without it
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if(desktop && desktop->doc() == doc) {
        desktop->currentLayer()->getRepr()->appendChild(group);
    } else {
        parent->getRepr()->appendChild(group);
    }

    // Move all children of symbol to group
    std::vector<SPObject*> children = childList(false);

    // Converting a group to a symbol inserts a group for non-translational transform.
    // In converting a symbol back to a group we strip out the inserted group (or any other
    // group that only adds a transform to the symbol content).
    if( children.size() == 1 ) {
        SPObject *object = children[0];
        if ( dynamic_cast<SPGroup *>( object ) ) {
            if( object->getAttribute("style") == nullptr ||
                object->getAttribute("class") == nullptr ) {

                group->setAttribute("transform", object->getAttribute("transform"));
                children = object->childList(false);
            }
        }
    }
    for (std::vector<SPObject*>::const_reverse_iterator i=children.rbegin();i!=children.rend();++i){
        Inkscape::XML::Node *repr = (*i)->getRepr();
        repr->parent()->removeChild(repr);
        group->addChild(repr,nullptr);
    }

    // Copy relevant attributes
    group->setAttribute("style", getAttribute("style"));
    group->setAttribute("class", getAttribute("class"));
    group->setAttribute("title", getAttribute("title"));
    group->setAttribute("inkscape:transform-center-x",
                        getAttribute("inkscape:transform-center-x"));
    group->setAttribute("inkscape:transform-center-y",
                        getAttribute("inkscape:transform-center-y"));


    // Need to delete <symbol>; all <use> elements that referenced <symbol> should
    // auto-magically reference <g> (if <symbol> deleted after setting <g> 'id').
    Glib::ustring id = getAttribute("id");
    group->setAttribute("id", id);
    
    deleteObject(true);

    // Clean up
    Inkscape::GC::release(group);
}

void SPSymbol::update(SPCtx *ctx, guint flags) {
    if (this->cloned) {

        SPItemCtx *ictx = (SPItemCtx *) ctx;
        SPItemCtx rctx = get_rctx( ictx );

        // And invoke parent method
        SPGroup::update((SPCtx *) &rctx, flags);

        // As last step set additional transform of drawing group
        for (SPItemView *v = this->display; v != nullptr; v = v->next) {
        	Inkscape::DrawingGroup *g = dynamic_cast<Inkscape::DrawingGroup *>(v->arenaitem);
        	g->setChildTransform(this->c2p);
        }
    } else {
        // No-op
        SPGroup::update(ctx, flags);
    }
}

void SPSymbol::modified(unsigned int flags) {
	SPGroup::modified(flags);
}


Inkscape::XML::Node* SPSymbol::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:symbol");
    }

    //XML Tree being used directly here while it shouldn't be.
    repr->setAttribute("viewBox", this->getRepr()->attribute("viewBox"));
	
    //XML Tree being used directly here while it shouldn't be.
    repr->setAttribute("preserveAspectRatio", this->getRepr()->attribute("preserveAspectRatio"));

    SPGroup::write(xml_doc, repr, flags);

    return repr;
}

Inkscape::DrawingItem* SPSymbol::show(Inkscape::Drawing &drawing, unsigned int key, unsigned int flags) {
    Inkscape::DrawingItem *ai = nullptr;

    if (this->cloned) {
        // Cloned <symbol> is actually renderable
        ai = SPGroup::show(drawing, key, flags);
        Inkscape::DrawingGroup *g = dynamic_cast<Inkscape::DrawingGroup *>(ai);

		if (g) {
			g->setChildTransform(this->c2p);
		}
    }

    return ai;
}

void SPSymbol::hide(unsigned int key) {
    if (this->cloned) {
        /* Cloned <symbol> is actually renderable */
        SPGroup::hide(key);
    }
}


Geom::OptRect SPSymbol::bbox(Geom::Affine const &transform, SPItem::BBoxType type) const {
    Geom::OptRect bbox;

    // We don't need a bounding box for Symbols dialog when selecting
    // symbols. They have no canvas location. But cloned symbols are.
    if (this->cloned) {
    	Geom::Affine const a( this->c2p * transform );
    	bbox = SPGroup::bbox(a, type);
    }

    return bbox;
}

void SPSymbol::print(SPPrintContext* ctx) {
    if (this->cloned) {
        // Cloned <symbol> is actually renderable

        ctx->bind(this->c2p, 1.0);

        SPGroup::print(ctx);

        ctx->release ();
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

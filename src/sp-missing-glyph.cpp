#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/*
 * SVG <missing-glyph> element implementation
 *
 * Author:
 *   Felipe C. da S. Sanches <juca@members.fsf.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2008, Felipe C. da S. Sanches
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "xml/repr.h"
#include "attributes.h"
#include "sp-missing-glyph.h"
#include "document.h"

#include "sp-factory.h"

namespace {
	SPObject* createMissingGlyph() {
		return new SPMissingGlyph();
	}

	bool missingGlyphRegistered = SPFactory::instance().registerObject("svg:missing-glyph", createMissingGlyph);
}

SPMissingGlyph::SPMissingGlyph() : SPObject(), CObject(this) {
	delete this->cobject;
	this->cobject = this;

//TODO: correct these values:
    this->d = NULL;
    this->horiz_adv_x = 0;
    this->vert_origin_x = 0;
    this->vert_origin_y = 0;
    this->vert_adv_y = 0;
}

SPMissingGlyph::~SPMissingGlyph() {
}

void SPMissingGlyph::build(SPDocument* doc, Inkscape::XML::Node* repr) {
	SPMissingGlyph* object = this;

    CObject::build(doc, repr);

    object->readAttr( "d" );
    object->readAttr( "horiz-adv-x" );
    object->readAttr( "vert-origin-x" );
    object->readAttr( "vert-origin-y" );
    object->readAttr( "vert-adv-y" );
}

void SPMissingGlyph::release() {
	CObject::release();
}


void SPMissingGlyph::set(unsigned int key, const gchar* value) {
	SPMissingGlyph* object = this;

    SPMissingGlyph *glyph = SP_MISSING_GLYPH(object);

    switch (key) {
        case SP_ATTR_D:
        {
            if (glyph->d) {
                g_free(glyph->d);
            }
            glyph->d = g_strdup(value);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        }
        case SP_ATTR_HORIZ_ADV_X:
        {
            double number = value ? g_ascii_strtod(value, 0) : 0;
            if (number != glyph->horiz_adv_x){
                glyph->horiz_adv_x = number;
                object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SP_ATTR_VERT_ORIGIN_X:
        {
            double number = value ? g_ascii_strtod(value, 0) : 0;
            if (number != glyph->vert_origin_x){
                glyph->vert_origin_x = number;
                object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SP_ATTR_VERT_ORIGIN_Y:
        {
            double number = value ? g_ascii_strtod(value, 0) : 0;
            if (number != glyph->vert_origin_y){
                glyph->vert_origin_y = number;
                object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SP_ATTR_VERT_ADV_Y:
        {
            double number = value ? g_ascii_strtod(value, 0) : 0;
            if (number != glyph->vert_adv_y){
                glyph->vert_adv_y = number;
                object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        default:
        {
            CObject::set(key, value);
            break;
        }
    }
}

#define COPY_ATTR(rd,rs,key) (rd)->setAttribute((key), rs->attribute(key));

Inkscape::XML::Node* SPMissingGlyph::write(Inkscape::XML::Document* xml_doc, Inkscape::XML::Node* repr, guint flags) {
	SPMissingGlyph* object = this;

	//    SPMissingGlyph *glyph = SP_MISSING_GLYPH(object);

	    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
	        repr = xml_doc->createElement("svg:glyph");
	    }

	/* I am commenting out this part because I am not certain how does it work. I will have to study it later. Juca
	    repr->setAttribute("d", glyph->d);
	    sp_repr_set_svg_double(repr, "horiz-adv-x", glyph->horiz_adv_x);
	    sp_repr_set_svg_double(repr, "vert-origin-x", glyph->vert_origin_x);
	    sp_repr_set_svg_double(repr, "vert-origin-y", glyph->vert_origin_y);
	    sp_repr_set_svg_double(repr, "vert-adv-y", glyph->vert_adv_y);
	*/
	    if (repr != object->getRepr()) {

	        // All the COPY_ATTR functions below use
	        //  XML Tree directly while they shouldn't.
	        COPY_ATTR(repr, object->getRepr(), "d");
	        COPY_ATTR(repr, object->getRepr(), "horiz-adv-x");
	        COPY_ATTR(repr, object->getRepr(), "vert-origin-x");
	        COPY_ATTR(repr, object->getRepr(), "vert-origin-y");
	        COPY_ATTR(repr, object->getRepr(), "vert-adv-y");
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

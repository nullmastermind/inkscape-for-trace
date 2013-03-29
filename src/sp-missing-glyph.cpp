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

G_DEFINE_TYPE(SPMissingGlyph, sp_missing_glyph, SP_TYPE_OBJECT);

static void sp_missing_glyph_class_init(SPMissingGlyphClass *gc)
{
}

CMissingGlyph::CMissingGlyph(SPMissingGlyph* mg) : CObject(mg) {
	this->spmissingglyph = mg;
}

CMissingGlyph::~CMissingGlyph() {
}

static void sp_missing_glyph_init(SPMissingGlyph *glyph)
{
	glyph->cmissingglyph = new CMissingGlyph(glyph);

	delete glyph->cobject;
	glyph->cobject = glyph->cmissingglyph;

//TODO: correct these values:
    glyph->d = NULL;
    glyph->horiz_adv_x = 0;
    glyph->vert_origin_x = 0;
    glyph->vert_origin_y = 0;
    glyph->vert_adv_y = 0;
}

void CMissingGlyph::onBuild(SPDocument* doc, Inkscape::XML::Node* repr) {
	SPMissingGlyph* object = this->spmissingglyph;

    CObject::onBuild(doc, repr);

    object->readAttr( "d" );
    object->readAttr( "horiz-adv-x" );
    object->readAttr( "vert-origin-x" );
    object->readAttr( "vert-origin-y" );
    object->readAttr( "vert-adv-y" );
}

void CMissingGlyph::onRelease() {
	CObject::onRelease();
}


void CMissingGlyph::onSet(unsigned int key, const gchar* value) {
	SPMissingGlyph* object = this->spmissingglyph;

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
            CObject::onSet(key, value);
            break;
        }
    }
}

#define COPY_ATTR(rd,rs,key) (rd)->setAttribute((key), rs->attribute(key));

Inkscape::XML::Node* CMissingGlyph::onWrite(Inkscape::XML::Document* xml_doc, Inkscape::XML::Node* repr, guint flags) {
	SPMissingGlyph* object = this->spmissingglyph;

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

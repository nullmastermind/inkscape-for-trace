#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define __SP_ANCHOR_C__

/*
 * SVG <hkern> and <vkern> elements implementation
 * W3C SVG 1.1 spec, page 476, section 20.7
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
#include "sp-glyph-kerning.h"

#include "document.h"
#include <string>
#include <cstring>


SPGlyphKerning::SPGlyphKerning() : SPObject(), CObject(this) {
	delete this->cobject;
	this->cobject = this;

//TODO: correct these values:
    this->u1 = NULL;
    this->g1 = NULL;
    this->u2 = NULL;
    this->g2 = NULL;
    this->k = 0;
}

SPGlyphKerning::~SPGlyphKerning() {
}

void SPGlyphKerning::build(SPDocument *document, Inkscape::XML::Node *repr) {
	SPGlyphKerning* object = this;

	CObject::build(document, repr);

	object->readAttr( "u1" );
	object->readAttr( "g1" );
	object->readAttr( "u2" );
	object->readAttr( "g2" );
	object->readAttr( "k" );
}

void SPGlyphKerning::release() {
	CObject::release();
}

GlyphNames::GlyphNames(const gchar* value){
        if (value) this->names = strdup(value);
}

GlyphNames::~GlyphNames(){
    if (this->names) g_free(this->names);
}

bool GlyphNames::contains(const char* name){
    if (!(this->names) || !name) return false;
    std::istringstream is(this->names);
    std::string str;
    std::string s(name);
    while (is >> str){
        if (str == s) return true;
    }
    return false;
}

void SPGlyphKerning::set(unsigned int key, const gchar *value) {
	SPGlyphKerning* object = this;

    SPGlyphKerning * glyphkern = (SPGlyphKerning*) object; //even if it is a VKern this will work. I did it this way just to avoind warnings.

    switch (key) {
        case SP_ATTR_U1:
        {
            if (glyphkern->u1) {
                delete glyphkern->u1;
            }
            glyphkern->u1 = new UnicodeRange(value);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        }
        case SP_ATTR_U2:
        {
            if (glyphkern->u2) {
                delete glyphkern->u2;
            }
            glyphkern->u2 = new UnicodeRange(value);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        }
        case SP_ATTR_G1:
        {
            if (glyphkern->g1) {
                delete glyphkern->g1;
            }
            glyphkern->g1 = new GlyphNames(value);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        }
        case SP_ATTR_G2:
        {
            if (glyphkern->g2) {
                delete glyphkern->g2;
            }
            glyphkern->g2 = new GlyphNames(value);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
             break;
        }
        case SP_ATTR_K:
        {
            double number = value ? g_ascii_strtod(value, 0) : 0;
            if (number != glyphkern->k){
                glyphkern->k = number;
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

/**
 *  * Receives update notifications.
 *   */
void SPGlyphKerning::update(SPCtx *ctx, guint flags) {
	SPGlyphKerning* object = this;

    SPGlyphKerning *glyph = (SPGlyphKerning *)object;
    (void)glyph;

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        /* do something to trigger redisplay, updates? */
            object->readAttr( "u1" );
            object->readAttr( "u2" );
            object->readAttr( "g2" );
            object->readAttr( "k" );
    }

    CObject::update(ctx, flags);
}

#define COPY_ATTR(rd,rs,key) (rd)->setAttribute((key), rs->attribute(key));

Inkscape::XML::Node* SPGlyphKerning::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	SPGlyphKerning* object = this;

	//    SPGlyphKerning *glyph = SP_GLYPH_KERNING(object);

	    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
	        repr = xml_doc->createElement("svg:glyphkerning");//fix this!
	    }

	/* I am commenting out this part because I am not certain how does it work. I will have to study it later. Juca
	    repr->setAttribute("unicode", glyph->unicode);
	    repr->setAttribute("glyph-name", glyph->glyph_name);
	    repr->setAttribute("d", glyph->d);
	    sp_repr_set_svg_double(repr, "orientation", (double) glyph->orientation);
	    sp_repr_set_svg_double(repr, "arabic-form", (double) glyph->arabic_form);
	    repr->setAttribute("lang", glyph->lang);
	    sp_repr_set_svg_double(repr, "horiz-adv-x", glyph->horiz_adv_x);
	    sp_repr_set_svg_double(repr, "vert-origin-x", glyph->vert_origin_x);
	    sp_repr_set_svg_double(repr, "vert-origin-y", glyph->vert_origin_y);
	    sp_repr_set_svg_double(repr, "vert-adv-y", glyph->vert_adv_y);
	*/
	    if (repr != object->getRepr()) {
	        // All the COPY_ATTR functions below use
	        //   XML Tree directly, while they shouldn't.
	        COPY_ATTR(repr, object->getRepr(), "u1");
	        COPY_ATTR(repr, object->getRepr(), "g1");
	        COPY_ATTR(repr, object->getRepr(), "u2");
	        COPY_ATTR(repr, object->getRepr(), "g2");
	        COPY_ATTR(repr, object->getRepr(), "k");
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

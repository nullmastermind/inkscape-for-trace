/** \file
 * SVG <feImage> implementation.
 *
 */
/*
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2007 Felipe Sanches
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "display/nr-filter-image.h"
#include "uri.h"
#include "uri-references.h"
#include "enums.h"
#include "attributes.h"
#include "svg/svg.h"
#include "image.h"
#include "xml/repr.h"
#include <string.h>

#include "display/nr-filter.h"

#include "sp-factory.h"

namespace {
	SPObject* createImage() {
		return new SPFeImage();
	}

	bool imageRegistered = SPFactory::instance().registerObject("svg:feImage", createImage);
}

SPFeImage::SPFeImage() : SPFilterPrimitive() {
	this->cobject = this;

	this->document = NULL;
	this->href = NULL;
	this->from_element = 0;
	this->SVGElemRef = NULL;
	this->SVGElem = NULL;

    this->aspect_align = SP_ASPECT_XMID_YMID; // Default
    this->aspect_clip = SP_ASPECT_MEET; // Default
}

SPFeImage::~SPFeImage() {
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPFeImage variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void SPFeImage::build(SPDocument *document, Inkscape::XML::Node *repr) {
	SPFeImage* object = this;

    // Save document reference so we can load images with relative paths.
    SPFeImage *feImage = SP_FEIMAGE(object);
    feImage->document = document;

    SPFilterPrimitive::build(document, repr);

    /*LOAD ATTRIBUTES FROM REPR HERE*/

    object->readAttr( "preserveAspectRatio" );
    object->readAttr( "xlink:href" );
}

/**
 * Drops any allocated memory.
 */
void SPFeImage::release() {
	SPFeImage* object = this;

    SPFeImage *feImage = SP_FEIMAGE(object);
    feImage->_image_modified_connection.disconnect();
    feImage->_href_modified_connection.disconnect();
    if (feImage->SVGElemRef) delete feImage->SVGElemRef;

    SPFilterPrimitive::release();
}

static void sp_feImage_elem_modified(SPObject* /*href*/, guint /*flags*/, SPObject* obj)
{
    obj->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

static void sp_feImage_href_modified(SPObject* /*old_elem*/, SPObject* new_elem, SPObject* obj)
{
    SPFeImage *feImage = SP_FEIMAGE(obj);
    feImage->_image_modified_connection.disconnect();
    if (new_elem) {
        feImage->SVGElem = SP_ITEM(new_elem);
        feImage->_image_modified_connection = ((SPObject*) feImage->SVGElem)->connectModified(sigc::bind(sigc::ptr_fun(&sp_feImage_elem_modified), obj));
    } else {
        feImage->SVGElem = 0;
    }

    obj->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

/**
 * Sets a specific value in the SPFeImage.
 */
void SPFeImage::set(unsigned int key, gchar const *value) {
	SPFeImage* object = this;

    SPFeImage *feImage = SP_FEIMAGE(object);
    (void)feImage;
    switch(key) {
    /*DEAL WITH SETTING ATTRIBUTES HERE*/
        case SP_ATTR_XLINK_HREF:
            if (feImage->href) {
                g_free(feImage->href);
            }
            feImage->href = (value) ? g_strdup (value) : NULL;
            if (!feImage->href) return;
            delete feImage->SVGElemRef;
            feImage->SVGElemRef = 0;
            feImage->SVGElem = 0;
            feImage->_image_modified_connection.disconnect();
            feImage->_href_modified_connection.disconnect();
            try{
                Inkscape::URI SVGElem_uri(feImage->href);
                feImage->SVGElemRef = new Inkscape::URIReference(feImage->document);
                feImage->SVGElemRef->attach(SVGElem_uri);
                feImage->from_element = true;
                feImage->_href_modified_connection = feImage->SVGElemRef->changedSignal().connect(sigc::bind(sigc::ptr_fun(&sp_feImage_href_modified), object));
                if (SPObject *elemref = feImage->SVGElemRef->getObject()) {
                    feImage->SVGElem = SP_ITEM(elemref);
                    feImage->_image_modified_connection = ((SPObject*) feImage->SVGElem)->connectModified(sigc::bind(sigc::ptr_fun(&sp_feImage_elem_modified), object));
                    object->requestModified(SP_OBJECT_MODIFIED_FLAG);
                    break;
                } else {
                    g_warning("SVG element URI was not found in the document while loading feImage");
                }
            }
            // catches either MalformedURIException or UnsupportedURIException
            catch(const Inkscape::BadURIException & e)
            {
                feImage->from_element = false;
                /* This occurs when using external image as the source */
                //g_warning("caught Inkscape::BadURIException in sp_feImage_set");
                break;
            }
            break;

        case SP_ATTR_PRESERVEASPECTRATIO:
            /* Copied from sp-image.cpp */
            /* Do setup before, so we can use break to escape */
            feImage->aspect_align = SP_ASPECT_XMID_YMID; // Default
            feImage->aspect_clip = SP_ASPECT_MEET; // Default
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
                    g_warning("Illegal preserveAspectRatio: %s", c);
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
                feImage->aspect_align = align;
                feImage->aspect_clip = clip;
            }
            break;

        default:
        	SPFilterPrimitive::set(key, value);
            break;
    }
}

/**
 * Receives update notifications.
 */
void SPFeImage::update(SPCtx *ctx, guint flags) {
	SPFeImage* object = this;

    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG |
                 SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {

        /* do something to trigger redisplay, updates? */
    }

    SPFilterPrimitive::update(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* SPFeImage::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
	SPFeImage* object = this;

    /* TODO: Don't just clone, but create a new repr node and write all
     * relevant values into it */
    if (!repr) {
        repr = object->getRepr()->duplicate(doc);
    }

    SPFilterPrimitive::write(doc, repr, flags);

    return repr;
}

void SPFeImage::build_renderer(Inkscape::Filters::Filter* filter) {
	SPFeImage* primitive = this;

    g_assert(primitive != NULL);
    g_assert(filter != NULL);

    SPFeImage *sp_image = SP_FEIMAGE(primitive);

    int primitive_n = filter->add_primitive(Inkscape::Filters::NR_FILTER_IMAGE);
    Inkscape::Filters::FilterPrimitive *nr_primitive = filter->get_primitive(primitive_n);
    Inkscape::Filters::FilterImage *nr_image = dynamic_cast<Inkscape::Filters::FilterImage*>(nr_primitive);
    g_assert(nr_image != NULL);

    sp_filter_primitive_renderer_common(primitive, nr_primitive);

    nr_image->from_element = sp_image->from_element;
    nr_image->SVGElem = sp_image->SVGElem;
    nr_image->set_align( sp_image->aspect_align );
    nr_image->set_clip( sp_image->aspect_clip );
    nr_image->set_href(sp_image->href);
    nr_image->set_document(sp_image->document);
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

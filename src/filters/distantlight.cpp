/** \file
 * SVG <fedistantlight> implementation.
 */
/*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Niko Kiirala <niko@kiirala.com>
 *   Jean-Rene Reinhard <jr@komite.net>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006,2007 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>

#include "attributes.h"
#include "document.h"
#include "filters/distantlight.h"
#include "filters/diffuselighting.h"
#include "filters/specularlighting.h"
#include "xml/repr.h"

#define SP_MACROS_SILENT
#include "macros.h"

SPFeDistantLight::SPFeDistantLight() : SPObject() {
    this->azimuth = 0;
    this->elevation = 0;
    this->azimuth_set = FALSE;
    this->elevation_set = FALSE;
}

SPFeDistantLight::~SPFeDistantLight() {
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPDistantLight variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void SPFeDistantLight::build(SPDocument *document, Inkscape::XML::Node *repr) {
	SPObject::build(document, repr);

	SPFeDistantLight* object = this;

    //Read values of key attributes from XML nodes into object.
    object->readAttr( "azimuth" );
    object->readAttr( "elevation" );

//is this necessary?
    document->addResource("fedistantlight", object);
}

/**
 * Drops any allocated memory.
 */
void SPFeDistantLight::release() {
	SPFeDistantLight* object = this;

    //SPFeDistantLight *fedistantlight = SP_FEDISTANTLIGHT(object);

    if ( object->document ) {
        // Unregister ourselves
        object->document->removeResource("fedistantlight", object);
    }

//TODO: release resources here
}

/**
 * Sets a specific value in the SPFeDistantLight.
 */
void SPFeDistantLight::set(unsigned int key, gchar const *value) {
	SPFeDistantLight* object = this;
    SPFeDistantLight *fedistantlight = SP_FEDISTANTLIGHT(object);
    gchar *end_ptr;
    switch (key) {
    case SP_ATTR_AZIMUTH:
        end_ptr =NULL;
        if (value) {
            fedistantlight->azimuth = g_ascii_strtod(value, &end_ptr);
            if (end_ptr) {
                fedistantlight->azimuth_set = TRUE;
            }
        }
        if (!value || !end_ptr) {
                fedistantlight->azimuth_set = FALSE;
                fedistantlight->azimuth = 0;
        }
        if (object->parent &&
                (SP_IS_FEDIFFUSELIGHTING(object->parent) ||
                 SP_IS_FESPECULARLIGHTING(object->parent))) {
            object->parent->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
        }
        break;
    case SP_ATTR_ELEVATION:
        end_ptr =NULL;
        if (value) {
            fedistantlight->elevation = g_ascii_strtod(value, &end_ptr);
            if (end_ptr) {
                fedistantlight->elevation_set = TRUE;
            }
        }
        if (!value || !end_ptr) {
                fedistantlight->elevation_set = FALSE;
                fedistantlight->elevation = 0;
        }
        if (object->parent &&
                (SP_IS_FEDIFFUSELIGHTING(object->parent) ||
                 SP_IS_FESPECULARLIGHTING(object->parent))) {
            object->parent->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
        }
        break;
    default:
        // See if any parents need this value.
    	SPObject::set(key, value);
        break;
    }
}

/**
 *  * Receives update notifications.
 *   */
void SPFeDistantLight::update(SPCtx *ctx, guint flags) {
	SPFeDistantLight* object = this;
    SPFeDistantLight *feDistantLight = SP_FEDISTANTLIGHT(object);
    (void)feDistantLight;

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        /* do something to trigger redisplay, updates? */
        object->readAttr( "azimuth" );
        object->readAttr( "elevation" );
    }

    SPObject::update(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* SPFeDistantLight::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
	SPFeDistantLight* object = this;
    SPFeDistantLight *fedistantlight = SP_FEDISTANTLIGHT(object);

    if (!repr) {
        repr = object->getRepr()->duplicate(doc);
    }

    if (fedistantlight->azimuth_set)
        sp_repr_set_css_double(repr, "azimuth", fedistantlight->azimuth);
    if (fedistantlight->elevation_set)
        sp_repr_set_css_double(repr, "elevation", fedistantlight->elevation);

    SPObject::write(doc, repr, flags);

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

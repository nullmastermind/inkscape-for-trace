/** \file
 * SVG <fespotlight> implementation.
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
#include "filters/spotlight.h"
#include "filters/diffuselighting.h"
#include "filters/specularlighting.h"
#include "xml/repr.h"

#define SP_MACROS_SILENT
#include "macros.h"

#include "sp-factory.h"

namespace {
	SPObject* createSpotLight() {
		return new SPFeSpotLight();
	}

	bool spotLightRegistered = SPFactory::instance().registerObject("svg:feSpotLight", createSpotLight);
}

SPFeSpotLight::SPFeSpotLight() : SPObject(), CObject(this) {
	delete this->cobject;
	this->cobject = this;

    this->x = 0;
    this->y = 0;
    this->z = 0;
    this->pointsAtX = 0;
    this->pointsAtY = 0;
    this->pointsAtZ = 0;
    this->specularExponent = 1;
    this->limitingConeAngle = 90;

    this->x_set = FALSE;
    this->y_set = FALSE;
    this->z_set = FALSE;
    this->pointsAtX_set = FALSE;
    this->pointsAtY_set = FALSE;
    this->pointsAtZ_set = FALSE;
    this->specularExponent_set = FALSE;
    this->limitingConeAngle_set = FALSE;
}

SPFeSpotLight::~SPFeSpotLight() {
}


/**
 * Reads the Inkscape::XML::Node, and initializes SPPointLight variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void SPFeSpotLight::build(SPDocument *document, Inkscape::XML::Node *repr) {
	CObject::build(document, repr);

	SPFeSpotLight* object = this;

    //Read values of key attributes from XML nodes into object.
    object->readAttr( "x" );
    object->readAttr( "y" );
    object->readAttr( "z" );
    object->readAttr( "pointsAtX" );
    object->readAttr( "pointsAtY" );
    object->readAttr( "pointsAtZ" );
    object->readAttr( "specularExponent" );
    object->readAttr( "limitingConeAngle" );

//is this necessary?
    document->addResource("fespotlight", object);
}

/**
 * Drops any allocated memory.
 */
void SPFeSpotLight::release() {
	SPFeSpotLight* object = this;
    //SPFeSpotLight *fespotlight = SP_FESPOTLIGHT(object);

    if ( object->document ) {
        // Unregister ourselves
        object->document->removeResource("fespotlight", object);
    }

//TODO: release resources here
}

/**
 * Sets a specific value in the SPFeSpotLight.
 */
void SPFeSpotLight::set(unsigned int key, gchar const *value) {
	SPFeSpotLight* object = this;

    SPFeSpotLight *fespotlight = SP_FESPOTLIGHT(object);
    gchar *end_ptr;

    switch (key) {
    case SP_ATTR_X:
        end_ptr = NULL;
        if (value) {
            fespotlight->x = g_ascii_strtod(value, &end_ptr);
            if (end_ptr)
                fespotlight->x_set = TRUE;
        }
        if(!value || !end_ptr) {
            fespotlight->x = 0;
            fespotlight->x_set = FALSE;
        }
        if (object->parent &&
                (SP_IS_FEDIFFUSELIGHTING(object->parent) ||
                 SP_IS_FESPECULARLIGHTING(object->parent))) {
            object->parent->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
        }
        break;
    case SP_ATTR_Y:
        end_ptr = NULL;
        if (value) {
            fespotlight->y = g_ascii_strtod(value, &end_ptr);
            if (end_ptr)
                fespotlight->y_set = TRUE;
        }
        if(!value || !end_ptr) {
            fespotlight->y = 0;
            fespotlight->y_set = FALSE;
        }
        if (object->parent &&
                (SP_IS_FEDIFFUSELIGHTING(object->parent) ||
                 SP_IS_FESPECULARLIGHTING(object->parent))) {
            object->parent->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
        }
        break;
    case SP_ATTR_Z:
        end_ptr = NULL;
        if (value) {
            fespotlight->z = g_ascii_strtod(value, &end_ptr);
            if (end_ptr)
                fespotlight->z_set = TRUE;
        }
        if(!value || !end_ptr) {
            fespotlight->z = 0;
            fespotlight->z_set = FALSE;
        }
        if (object->parent &&
                (SP_IS_FEDIFFUSELIGHTING(object->parent) ||
                 SP_IS_FESPECULARLIGHTING(object->parent))) {
            object->parent->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
        }
        break;
    case SP_ATTR_POINTSATX:
        end_ptr = NULL;
        if (value) {
            fespotlight->pointsAtX = g_ascii_strtod(value, &end_ptr);
            if (end_ptr)
                fespotlight->pointsAtX_set = TRUE;
        }
        if(!value || !end_ptr) {
            fespotlight->pointsAtX = 0;
            fespotlight->pointsAtX_set = FALSE;
        }
        if (object->parent &&
                (SP_IS_FEDIFFUSELIGHTING(object->parent) ||
                 SP_IS_FESPECULARLIGHTING(object->parent))) {
            object->parent->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
        }
        break;
    case SP_ATTR_POINTSATY:
        end_ptr = NULL;
        if (value) {
            fespotlight->pointsAtY = g_ascii_strtod(value, &end_ptr);
            if (end_ptr)
                fespotlight->pointsAtY_set = TRUE;
        }
        if(!value || !end_ptr) {
            fespotlight->pointsAtY = 0;
            fespotlight->pointsAtY_set = FALSE;
        }
        if (object->parent &&
                (SP_IS_FEDIFFUSELIGHTING(object->parent) ||
                 SP_IS_FESPECULARLIGHTING(object->parent))) {
            object->parent->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
        }
        break;
    case SP_ATTR_POINTSATZ:
        end_ptr = NULL;
        if (value) {
            fespotlight->pointsAtZ = g_ascii_strtod(value, &end_ptr);
            if (end_ptr)
                fespotlight->pointsAtZ_set = TRUE;
        }
        if(!value || !end_ptr) {
            fespotlight->pointsAtZ = 0;
            fespotlight->pointsAtZ_set = FALSE;
        }
        if (object->parent &&
                (SP_IS_FEDIFFUSELIGHTING(object->parent) ||
                 SP_IS_FESPECULARLIGHTING(object->parent))) {
            object->parent->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
        }
        break;
    case SP_ATTR_SPECULAREXPONENT:
        end_ptr = NULL;
        if (value) {
            fespotlight->specularExponent = g_ascii_strtod(value, &end_ptr);
            if (end_ptr)
                fespotlight->specularExponent_set = TRUE;
        }
        if(!value || !end_ptr) {
            fespotlight->specularExponent = 1;
            fespotlight->specularExponent_set = FALSE;
        }
        if (object->parent &&
                (SP_IS_FEDIFFUSELIGHTING(object->parent) ||
                 SP_IS_FESPECULARLIGHTING(object->parent))) {
            object->parent->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
        }
        break;
    case SP_ATTR_LIMITINGCONEANGLE:
        end_ptr = NULL;
        if (value) {
            fespotlight->limitingConeAngle = g_ascii_strtod(value, &end_ptr);
            if (end_ptr)
                fespotlight->limitingConeAngle_set = TRUE;
        }
        if(!value || !end_ptr) {
            fespotlight->limitingConeAngle = 90;
            fespotlight->limitingConeAngle_set = FALSE;
        }
        if (object->parent &&
                (SP_IS_FEDIFFUSELIGHTING(object->parent) ||
                 SP_IS_FESPECULARLIGHTING(object->parent))) {
            object->parent->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
        }
        break;
    default:
        // See if any parents need this value.
    	CObject::set(key, value);
        break;
    }
}

/**
 *  * Receives update notifications.
 *   */
void SPFeSpotLight::update(SPCtx *ctx, guint flags) {
	SPFeSpotLight* object = this;

    SPFeSpotLight *feSpotLight = SP_FESPOTLIGHT(object);
    (void)feSpotLight;

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        /* do something to trigger redisplay, updates? */
        object->readAttr( "x" );
        object->readAttr( "y" );
        object->readAttr( "z" );
        object->readAttr( "pointsAtX" );
        object->readAttr( "pointsAtY" );
        object->readAttr( "pointsAtZ" );
        object->readAttr( "specularExponent" );
        object->readAttr( "limitingConeAngle" );
    }

    CObject::update(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* SPFeSpotLight::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
	SPFeSpotLight* object = this;
    SPFeSpotLight *fespotlight = SP_FESPOTLIGHT(object);

    if (!repr) {
        repr = object->getRepr()->duplicate(doc);
    }

    if (fespotlight->x_set)
        sp_repr_set_css_double(repr, "x", fespotlight->x);
    if (fespotlight->y_set)
        sp_repr_set_css_double(repr, "y", fespotlight->y);
    if (fespotlight->z_set)
        sp_repr_set_css_double(repr, "z", fespotlight->z);
    if (fespotlight->pointsAtX_set)
        sp_repr_set_css_double(repr, "pointsAtX", fespotlight->pointsAtX);
    if (fespotlight->pointsAtY_set)
        sp_repr_set_css_double(repr, "pointsAtY", fespotlight->pointsAtY);
    if (fespotlight->pointsAtZ_set)
        sp_repr_set_css_double(repr, "pointsAtZ", fespotlight->pointsAtZ);
    if (fespotlight->specularExponent_set)
        sp_repr_set_css_double(repr, "specularExponent", fespotlight->specularExponent);
    if (fespotlight->limitingConeAngle_set)
        sp_repr_set_css_double(repr, "limitingConeAngle", fespotlight->limitingConeAngle);

    CObject::write(doc, repr, flags);

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

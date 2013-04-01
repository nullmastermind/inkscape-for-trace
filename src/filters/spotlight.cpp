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

G_DEFINE_TYPE(SPFeSpotLight, sp_fespotlight, G_TYPE_OBJECT);

static void
sp_fespotlight_class_init(SPFeSpotLightClass *klass)
{
}

CFeSpotLight::CFeSpotLight(SPFeSpotLight* spotlight) : CObject(spotlight) {
	this->spfespotlight = spotlight;
}

CFeSpotLight::~CFeSpotLight() {
}

SPFeSpotLight::SPFeSpotLight() : SPObject() {
	SPFeSpotLight* fespotlight = this;

	fespotlight->cfespotlight = new CFeSpotLight(fespotlight);
	fespotlight->typeHierarchy.insert(typeid(SPFeSpotLight));

	delete fespotlight->cobject;
	fespotlight->cobject = fespotlight->cfespotlight;

    fespotlight->x = 0;
    fespotlight->y = 0;
    fespotlight->z = 0;
    fespotlight->pointsAtX = 0;
    fespotlight->pointsAtY = 0;
    fespotlight->pointsAtZ = 0;
    fespotlight->specularExponent = 1;
    fespotlight->limitingConeAngle = 90;

    fespotlight->x_set = FALSE;
    fespotlight->y_set = FALSE;
    fespotlight->z_set = FALSE;
    fespotlight->pointsAtX_set = FALSE;
    fespotlight->pointsAtY_set = FALSE;
    fespotlight->pointsAtZ_set = FALSE;
    fespotlight->specularExponent_set = FALSE;
    fespotlight->limitingConeAngle_set = FALSE;
}

static void
sp_fespotlight_init(SPFeSpotLight *fespotlight)
{
	new (fespotlight) SPFeSpotLight();
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPPointLight variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void CFeSpotLight::build(SPDocument *document, Inkscape::XML::Node *repr) {
	CObject::build(document, repr);

	SPFeSpotLight* object = this->spfespotlight;

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
void CFeSpotLight::release() {
	SPFeSpotLight* object = this->spfespotlight;
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
void CFeSpotLight::set(unsigned int key, gchar const *value) {
	SPFeSpotLight* object = this->spfespotlight;

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
void CFeSpotLight::update(SPCtx *ctx, guint flags) {
	SPFeSpotLight* object = this->spfespotlight;

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
Inkscape::XML::Node* CFeSpotLight::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
	SPFeSpotLight* object = this->spfespotlight;
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

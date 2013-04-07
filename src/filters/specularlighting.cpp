/** \file
 * SVG <feSpecularLighting> implementation.
 *
 */
/*
 * Authors:
 *   hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Jean-Rene Reinhard <jr@komite.net>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *               2007 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "strneq.h"

#include "attributes.h"
#include "svg/svg.h"
#include "sp-object.h"
#include "svg/svg-color.h"
#include "svg/svg-icc-color.h"
#include "filters/specularlighting.h"
#include "filters/distantlight.h"
#include "filters/pointlight.h"
#include "filters/spotlight.h"
#include "xml/repr.h"
#include "display/nr-filter.h"
#include "display/nr-filter-specularlighting.h"

/* FeSpecularLighting base class */
static void sp_feSpecularLighting_children_modified(SPFeSpecularLighting *sp_specularlighting);

#include "sp-factory.h"

namespace {
	SPObject* createSpecularLighting() {
		return new SPFeSpecularLighting();
	}

	bool specularLightingRegistered = SPFactory::instance().registerObject("svg:feSpecularLighting", createSpecularLighting);
}

SPFeSpecularLighting::SPFeSpecularLighting() : SPFilterPrimitive() {
    this->surfaceScale = 1;
    this->specularConstant = 1;
    this->specularExponent = 1;
    this->lighting_color = 0xffffffff;
    this->icc = NULL;

    //TODO kernelUnit
    this->renderer = NULL;
    
    this->surfaceScale_set = FALSE;
    this->specularConstant_set = FALSE;
    this->specularExponent_set = FALSE;
    this->lighting_color_set = FALSE;
}

SPFeSpecularLighting::~SPFeSpecularLighting() {
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPFeSpecularLighting variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void SPFeSpecularLighting::build(SPDocument *document, Inkscape::XML::Node *repr) {
	SPFeSpecularLighting* object = this;

	SPFilterPrimitive::build(document, repr);

	/*LOAD ATTRIBUTES FROM REPR HERE*/
	object->readAttr( "surfaceScale" );
	object->readAttr( "specularConstant" );
	object->readAttr( "specularExponent" );
	object->readAttr( "kernelUnitLength" );
	object->readAttr( "lighting-color" );
}

/**
 * Drops any allocated memory.
 */
void SPFeSpecularLighting::release() {
	SPFilterPrimitive::release();
}

/**
 * Sets a specific value in the SPFeSpecularLighting.
 */
void SPFeSpecularLighting::set(unsigned int key, gchar const *value) {
	SPFeSpecularLighting* object = this;

    SPFeSpecularLighting *feSpecularLighting = SP_FESPECULARLIGHTING(object);
    gchar const *cend_ptr = NULL;
    gchar *end_ptr = NULL;
    switch(key) {
	/*DEAL WITH SETTING ATTRIBUTES HERE*/
//TODO test forbidden values
        case SP_ATTR_SURFACESCALE:
            end_ptr = NULL;
            if (value) {
                feSpecularLighting->surfaceScale = g_ascii_strtod(value, &end_ptr);
                if (end_ptr) {
                    feSpecularLighting->surfaceScale_set = TRUE;
                } else {
                    g_warning("feSpecularLighting: surfaceScale should be a number ... defaulting to 1");
                }

            }
            //if the attribute is not set or has an unreadable value
            if (!value || !end_ptr) {
                feSpecularLighting->surfaceScale = 1;
                feSpecularLighting->surfaceScale_set = FALSE;
            }
            if (feSpecularLighting->renderer) {
                feSpecularLighting->renderer->surfaceScale = feSpecularLighting->surfaceScale;
            }
            object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_SPECULARCONSTANT:
            end_ptr = NULL;
            if (value) {
                feSpecularLighting->specularConstant = g_ascii_strtod(value, &end_ptr);
                if (end_ptr && feSpecularLighting->specularConstant >= 0) {
                    feSpecularLighting->specularConstant_set = TRUE;
                } else {
                    end_ptr = NULL;
                    g_warning("feSpecularLighting: specularConstant should be a positive number ... defaulting to 1");
                }
            }
            if (!value || !end_ptr) {
                feSpecularLighting->specularConstant = 1;
                feSpecularLighting->specularConstant_set = FALSE;
            }
            if (feSpecularLighting->renderer) {
                feSpecularLighting->renderer->specularConstant = feSpecularLighting->specularConstant;
            }
            object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_SPECULAREXPONENT:
            end_ptr = NULL;
            if (value) {
                feSpecularLighting->specularExponent = g_ascii_strtod(value, &end_ptr);
                if (feSpecularLighting->specularExponent >= 1 && feSpecularLighting->specularExponent <= 128) {
                    feSpecularLighting->specularExponent_set = TRUE;
                } else {
                    end_ptr = NULL;
                    g_warning("feSpecularLighting: specularExponent should be a number in range [1, 128] ... defaulting to 1");
                }
            } 
            if (!value || !end_ptr) {
                feSpecularLighting->specularExponent = 1;
                feSpecularLighting->specularExponent_set = FALSE;
            }
            if (feSpecularLighting->renderer) {
                feSpecularLighting->renderer->specularExponent = feSpecularLighting->specularExponent;
            }
            object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_KERNELUNITLENGTH:
            //TODO kernelUnit
            //feSpecularLighting->kernelUnitLength.set(value);
            /*TODOif (feSpecularLighting->renderer) {
                feSpecularLighting->renderer->surfaceScale = feSpecularLighting->renderer;
            }
            */
            object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_PROP_LIGHTING_COLOR:
            cend_ptr = NULL;
            feSpecularLighting->lighting_color = sp_svg_read_color(value, &cend_ptr, 0xffffffff);
            //if a value was read
            if (cend_ptr) {
                while (g_ascii_isspace(*cend_ptr)) {
                    ++cend_ptr;
                }
                if (strneq(cend_ptr, "icc-color(", 10)) {
                    if (!feSpecularLighting->icc) feSpecularLighting->icc = new SVGICCColor();
                    if ( ! sp_svg_read_icc_color( cend_ptr, feSpecularLighting->icc ) ) {
                        delete feSpecularLighting->icc;
                        feSpecularLighting->icc = NULL;
                    }
                }
                feSpecularLighting->lighting_color_set = TRUE;
            } else {
                //lighting_color already contains the default value
                feSpecularLighting->lighting_color_set = FALSE;
            }
            if (feSpecularLighting->renderer) {
                feSpecularLighting->renderer->lighting_color = feSpecularLighting->lighting_color;
            }
            object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        default:
        	SPFilterPrimitive::set(key, value);
            break;
    }
}

/**
 * Receives update notifications.
 */
void SPFeSpecularLighting::update(SPCtx *ctx, guint flags) {
	SPFeSpecularLighting* object = this;

    if (flags & (SP_OBJECT_MODIFIED_FLAG)) {
        object->readAttr( "surfaceScale" );
        object->readAttr( "specularConstant" );
        object->readAttr( "specularExponent" );
        object->readAttr( "kernelUnitLength" );
        object->readAttr( "lighting-color" );
    }

    SPFilterPrimitive::update(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* SPFeSpecularLighting::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
	SPFeSpecularLighting* object = this;

    SPFeSpecularLighting *fespecularlighting = SP_FESPECULARLIGHTING(object);
    
    /* TODO: Don't just clone, but create a new repr node and write all
     * relevant values _and children_ into it */
    if (!repr) {
        repr = object->getRepr()->duplicate(doc);
        //repr = doc->createElement("svg:feSpecularLighting");
    }

    if (fespecularlighting->surfaceScale_set)
        sp_repr_set_css_double(repr, "surfaceScale", fespecularlighting->surfaceScale);
    if (fespecularlighting->specularConstant_set)
        sp_repr_set_css_double(repr, "specularConstant", fespecularlighting->specularConstant);
    if (fespecularlighting->specularExponent_set)
        sp_repr_set_css_double(repr, "specularExponent", fespecularlighting->specularExponent);
   /*TODO kernelUnits */ 
    if (fespecularlighting->lighting_color_set) {
        gchar c[64];
        sp_svg_write_color(c, sizeof(c), fespecularlighting->lighting_color);
        repr->setAttribute("lighting-color", c);
    }
    SPFilterPrimitive::write(doc, repr, flags);

    return repr;
}

/**
 * Callback for child_added event.
 */
void SPFeSpecularLighting::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) {
	SPFeSpecularLighting* object = this;

    SPFeSpecularLighting *f = SP_FESPECULARLIGHTING(object);

    SPFilterPrimitive::child_added(child, ref);

    sp_feSpecularLighting_children_modified(f);
    object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

/**
 * Callback for remove_child event.
 */
void SPFeSpecularLighting::remove_child(Inkscape::XML::Node *child) {
	SPFeSpecularLighting* object = this;

    SPFeSpecularLighting *f = SP_FESPECULARLIGHTING(object);

    SPFilterPrimitive::remove_child(child);

    sp_feSpecularLighting_children_modified(f);
    object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void SPFeSpecularLighting::order_changed(Inkscape::XML::Node *child, Inkscape::XML::Node *old_ref, Inkscape::XML::Node *new_ref) {
	SPFeSpecularLighting* object = this;

    SPFeSpecularLighting *f = SP_FESPECULARLIGHTING(object);
    SPFilterPrimitive::order_changed(child, old_ref, new_ref);

    sp_feSpecularLighting_children_modified(f);
    object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

static void sp_feSpecularLighting_children_modified(SPFeSpecularLighting *sp_specularlighting)
{
   if (sp_specularlighting->renderer) {
        sp_specularlighting->renderer->light_type = Inkscape::Filters::NO_LIGHT;
        if (SP_IS_FEDISTANTLIGHT(sp_specularlighting->children)) {
            sp_specularlighting->renderer->light_type = Inkscape::Filters::DISTANT_LIGHT;
            sp_specularlighting->renderer->light.distant = SP_FEDISTANTLIGHT(sp_specularlighting->children);
        }
        if (SP_IS_FEPOINTLIGHT(sp_specularlighting->children)) {
            sp_specularlighting->renderer->light_type = Inkscape::Filters::POINT_LIGHT;
            sp_specularlighting->renderer->light.point = SP_FEPOINTLIGHT(sp_specularlighting->children);
        }
        if (SP_IS_FESPOTLIGHT(sp_specularlighting->children)) {
            sp_specularlighting->renderer->light_type = Inkscape::Filters::SPOT_LIGHT;
            sp_specularlighting->renderer->light.spot = SP_FESPOTLIGHT(sp_specularlighting->children);
        }
   }
}

void SPFeSpecularLighting::build_renderer(Inkscape::Filters::Filter* filter) {
	SPFeSpecularLighting* primitive = this;

    g_assert(primitive != NULL);
    g_assert(filter != NULL);

    SPFeSpecularLighting *sp_specularlighting = SP_FESPECULARLIGHTING(primitive);

    int primitive_n = filter->add_primitive(Inkscape::Filters::NR_FILTER_SPECULARLIGHTING);
    Inkscape::Filters::FilterPrimitive *nr_primitive = filter->get_primitive(primitive_n);
    Inkscape::Filters::FilterSpecularLighting *nr_specularlighting = dynamic_cast<Inkscape::Filters::FilterSpecularLighting*>(nr_primitive);
    g_assert(nr_specularlighting != NULL);

    sp_specularlighting->renderer = nr_specularlighting;
    sp_filter_primitive_renderer_common(primitive, nr_primitive);

    nr_specularlighting->specularConstant = sp_specularlighting->specularConstant;
    nr_specularlighting->specularExponent = sp_specularlighting->specularExponent;
    nr_specularlighting->surfaceScale = sp_specularlighting->surfaceScale;
    nr_specularlighting->lighting_color = sp_specularlighting->lighting_color;
    nr_specularlighting->set_icc(sp_specularlighting->icc);

    //We assume there is at most one child
    nr_specularlighting->light_type = Inkscape::Filters::NO_LIGHT;
    if (SP_IS_FEDISTANTLIGHT(primitive->children)) {
        nr_specularlighting->light_type = Inkscape::Filters::DISTANT_LIGHT;
        nr_specularlighting->light.distant = SP_FEDISTANTLIGHT(primitive->children);
    }
    if (SP_IS_FEPOINTLIGHT(primitive->children)) {
        nr_specularlighting->light_type = Inkscape::Filters::POINT_LIGHT;
        nr_specularlighting->light.point = SP_FEPOINTLIGHT(primitive->children);
    }
    if (SP_IS_FESPOTLIGHT(primitive->children)) {
        nr_specularlighting->light_type = Inkscape::Filters::SPOT_LIGHT;
        nr_specularlighting->light.spot = SP_FESPOTLIGHT(primitive->children);
    }
        
    //nr_offset->set_dx(sp_offset->dx);
    //nr_offset->set_dy(sp_offset->dy);
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

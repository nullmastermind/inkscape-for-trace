/** \file
 * SVG <feComposite> implementation.
 *
 */
/*
 * Authors:
 *   hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "attributes.h"
#include "svg/svg.h"
#include "filters/composite.h"
#include "helper-fns.h"
#include "xml/repr.h"
#include "display/nr-filter.h"
#include "display/nr-filter-composite.h"
#include "sp-filter.h"

#include "sp-factory.h"

namespace {
	SPObject* createComposite() {
		return new SPFeComposite();
	}

	bool compositeRegistered = SPFactory::instance().registerObject("svg:feComposite", createComposite);
}

/* FeComposite base class */
G_DEFINE_TYPE(SPFeComposite, sp_feComposite, G_TYPE_OBJECT);

static void
sp_feComposite_class_init(SPFeCompositeClass *klass)
{
}

CFeComposite::CFeComposite(SPFeComposite* comp) : CFilterPrimitive(comp) {
	this->spfecomposite = comp;
}

CFeComposite::~CFeComposite() {
}

SPFeComposite::SPFeComposite() : SPFilterPrimitive() {
	SPFeComposite* feComposite = this;

	feComposite->cfecomposite = new CFeComposite(feComposite);
	feComposite->typeHierarchy.insert(typeid(SPFeComposite));

	delete feComposite->cfilterprimitive;
	feComposite->cfilterprimitive = feComposite->cfecomposite;
	feComposite->cobject = feComposite->cfecomposite;

    feComposite->composite_operator = COMPOSITE_DEFAULT;
    feComposite->k1 = 0;
    feComposite->k2 = 0;
    feComposite->k3 = 0;
    feComposite->k4 = 0;
    feComposite->in2 = Inkscape::Filters::NR_FILTER_SLOT_NOT_SET;
}

static void
sp_feComposite_init(SPFeComposite *feComposite)
{
	new (feComposite) SPFeComposite();
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPFeComposite variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void CFeComposite::build(SPDocument *document, Inkscape::XML::Node *repr) {
	SPFeComposite* object = this->spfecomposite;

	CFilterPrimitive::build(document, repr);

	SPFeComposite *comp = SP_FECOMPOSITE(object);

	object->readAttr( "operator" );
	if (comp->composite_operator == COMPOSITE_ARITHMETIC) {
		object->readAttr( "k1" );
		object->readAttr( "k2" );
		object->readAttr( "k3" );
		object->readAttr( "k4" );
	}
	object->readAttr( "in2" );

	/* Unlike normal in, in2 is required attribute. Make sure, we can call
	 * it by some name. */
	if (comp->in2 == Inkscape::Filters::NR_FILTER_SLOT_NOT_SET ||
		comp->in2 == Inkscape::Filters::NR_FILTER_UNNAMED_SLOT)
	{
		SPFilter *parent = SP_FILTER(object->parent);
		comp->in2 = sp_filter_primitive_name_previous_out(comp);
		repr->setAttribute("in2", sp_filter_name_for_image(parent, comp->in2));
	}
}

/**
 * Drops any allocated memory.
 */
void CFeComposite::release() {
	CFilterPrimitive::release();
}

static FeCompositeOperator
sp_feComposite_read_operator(gchar const *value) {
    if (!value) return COMPOSITE_DEFAULT;

    if (strcmp(value, "over") == 0) return COMPOSITE_OVER;
    else if (strcmp(value, "in") == 0) return COMPOSITE_IN;
    else if (strcmp(value, "out") == 0) return COMPOSITE_OUT;
    else if (strcmp(value, "atop") == 0) return COMPOSITE_ATOP;
    else if (strcmp(value, "xor") == 0) return COMPOSITE_XOR;
    else if (strcmp(value, "arithmetic") == 0) return COMPOSITE_ARITHMETIC;
    return COMPOSITE_DEFAULT;
}

/**
 * Sets a specific value in the SPFeComposite.
 */
void CFeComposite::set(unsigned int key, gchar const *value) {
	SPFeComposite* object = this->spfecomposite;

    SPFeComposite *feComposite = SP_FECOMPOSITE(object);
    (void)feComposite;

    int input;
    FeCompositeOperator op;
    double k_n;
    switch(key) {
	/*DEAL WITH SETTING ATTRIBUTES HERE*/
        case SP_ATTR_OPERATOR:
            op = sp_feComposite_read_operator(value);
            if (op != feComposite->composite_operator) {
                feComposite->composite_operator = op;
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;

        case SP_ATTR_K1:
            k_n = value ? helperfns_read_number(value) : 0;
            if (k_n != feComposite->k1) {
                feComposite->k1 = k_n;
                if (feComposite->composite_operator == COMPOSITE_ARITHMETIC)
                    object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;

        case SP_ATTR_K2:
            k_n = value ? helperfns_read_number(value) : 0;
            if (k_n != feComposite->k2) {
                feComposite->k2 = k_n;
                if (feComposite->composite_operator == COMPOSITE_ARITHMETIC)
                    object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;

        case SP_ATTR_K3:
            k_n = value ? helperfns_read_number(value) : 0;
            if (k_n != feComposite->k3) {
                feComposite->k3 = k_n;
                if (feComposite->composite_operator == COMPOSITE_ARITHMETIC)
                    object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;

        case SP_ATTR_K4:
            k_n = value ? helperfns_read_number(value) : 0;
            if (k_n != feComposite->k4) {
                feComposite->k4 = k_n;
                if (feComposite->composite_operator == COMPOSITE_ARITHMETIC)
                    object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;

        case SP_ATTR_IN2:
            input = sp_filter_primitive_read_in(feComposite, value);
            if (input != feComposite->in2) {
                feComposite->in2 = input;
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;

        default:
        	CFilterPrimitive::set(key, value);
            break;
    }
}

/**
 * Receives update notifications.
 */
void
CFeComposite::update(SPCtx *ctx, guint flags) {
	SPFeComposite* object = this->spfecomposite;

    SPFeComposite *comp = SP_FECOMPOSITE(object);

    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG |
                 SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {

        /* do something to trigger redisplay, updates? */

    }

    /* Unlike normal in, in2 is required attribute. Make sure, we can call
     * it by some name. */
    if (comp->in2 == Inkscape::Filters::NR_FILTER_SLOT_NOT_SET ||
        comp->in2 == Inkscape::Filters::NR_FILTER_UNNAMED_SLOT)
    {
        SPFilter *parent = SP_FILTER(object->parent);
        comp->in2 = sp_filter_primitive_name_previous_out(comp);

		//XML Tree being used directly here while it shouldn't be.
        object->getRepr()->setAttribute("in2", sp_filter_name_for_image(parent, comp->in2));
    }

    CFilterPrimitive::update(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* CFeComposite::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
	SPFeComposite* object = this->spfecomposite;

    SPFeComposite *comp = SP_FECOMPOSITE(object);
    SPFilter *parent = SP_FILTER(object->parent);

    if (!repr) {
        repr = doc->createElement("svg:feComposite");
    }

    gchar const *out_name = sp_filter_name_for_image(parent, comp->in2);
    if (out_name) {
        repr->setAttribute("in2", out_name);
    } else {
        SPObject *i = parent->children;
        while (i && i->next != object) i = i->next;
        SPFilterPrimitive *i_prim = SP_FILTER_PRIMITIVE(i);
        out_name = sp_filter_name_for_image(parent, i_prim->image_out);
        repr->setAttribute("in2", out_name);
        if (!out_name) {
            g_warning("Unable to set in2 for feComposite");
        }
    }

    char const *comp_op;
    switch (comp->composite_operator) {
        case COMPOSITE_OVER:
            comp_op = "over"; break;
        case COMPOSITE_IN:
            comp_op = "in"; break;
        case COMPOSITE_OUT:
            comp_op = "out"; break;
        case COMPOSITE_ATOP:
            comp_op = "atop"; break;
        case COMPOSITE_XOR:
            comp_op = "xor"; break;
        case COMPOSITE_ARITHMETIC:
            comp_op = "arithmetic"; break;
        default:
            comp_op = 0;
    }
    repr->setAttribute("operator", comp_op);

    if (comp->composite_operator == COMPOSITE_ARITHMETIC) {
        sp_repr_set_svg_double(repr, "k1", comp->k1);
        sp_repr_set_svg_double(repr, "k2", comp->k2);
        sp_repr_set_svg_double(repr, "k3", comp->k3);
        sp_repr_set_svg_double(repr, "k4", comp->k4);
    } else {
        repr->setAttribute("k1", 0);
        repr->setAttribute("k2", 0);
        repr->setAttribute("k3", 0);
        repr->setAttribute("k4", 0);
    }

    CFilterPrimitive::write(doc, repr, flags);

    return repr;
}

void CFeComposite::build_renderer(Inkscape::Filters::Filter* filter) {
	SPFeComposite* primitive = this->spfecomposite;

    g_assert(primitive != NULL);
    g_assert(filter != NULL);

    SPFeComposite *sp_composite = SP_FECOMPOSITE(primitive);

    int primitive_n = filter->add_primitive(Inkscape::Filters::NR_FILTER_COMPOSITE);
    Inkscape::Filters::FilterPrimitive *nr_primitive = filter->get_primitive(primitive_n);
    Inkscape::Filters::FilterComposite *nr_composite = dynamic_cast<Inkscape::Filters::FilterComposite*>(nr_primitive);
    g_assert(nr_composite != NULL);

    sp_filter_primitive_renderer_common(primitive, nr_primitive);

    nr_composite->set_operator(sp_composite->composite_operator);
    nr_composite->set_input(1, sp_composite->in2);
    if (sp_composite->composite_operator == COMPOSITE_ARITHMETIC) {
        nr_composite->set_arithmetic(sp_composite->k1, sp_composite->k2,
                                     sp_composite->k3, sp_composite->k4);
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

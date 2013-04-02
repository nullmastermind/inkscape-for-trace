/** \file
 * SVG <gaussianBlur> implementation.
 *
 */
/*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Niko Kiirala <niko@kiirala.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006,2007 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "attributes.h"
#include "svg/svg.h"
#include "filters/gaussian-blur.h"
#include "xml/repr.h"

#include "display/nr-filter.h"
#include "display/nr-filter-primitive.h"
#include "display/nr-filter-gaussian.h"
#include "display/nr-filter-types.h"

//#define SP_MACROS_SILENT
//#include "macros.h"

#include "sp-factory.h"

namespace {
	SPObject* createGaussianBlur() {
		return new SPGaussianBlur();
	}

	bool gaussianBlurRegistered = SPFactory::instance().registerObject("svg:feGaussianBlur", createGaussianBlur);
}

/* GaussianBlur base class */
G_DEFINE_TYPE(SPGaussianBlur, sp_gaussianBlur, G_TYPE_OBJECT);

static void
sp_gaussianBlur_class_init(SPGaussianBlurClass *klass)
{
}

CGaussianBlur::CGaussianBlur(SPGaussianBlur* gb) : CFilterPrimitive(gb) {
	this->spgaussianblur = gb;
}

CGaussianBlur::~CGaussianBlur() {
}

SPGaussianBlur::SPGaussianBlur() : SPFilterPrimitive() {
	SPGaussianBlur* gaussianBlur = this;

	gaussianBlur->cgaussianblur = new CGaussianBlur(gaussianBlur);
	gaussianBlur->typeHierarchy.insert(typeid(SPGaussianBlur));

	delete gaussianBlur->cfilterprimitive;
	gaussianBlur->cfilterprimitive = gaussianBlur->cgaussianblur;
	gaussianBlur->cobject = gaussianBlur->cgaussianblur;
}

static void
sp_gaussianBlur_init(SPGaussianBlur *gaussianBlur)
{
	new (gaussianBlur) SPGaussianBlur();
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPGaussianBlur variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void CGaussianBlur::build(SPDocument *document, Inkscape::XML::Node *repr) {
	CFilterPrimitive::build(document, repr);

	SPGaussianBlur* object = this->spgaussianblur;

    object->readAttr( "stdDeviation" );
}

/**
 * Drops any allocated memory.
 */
void CGaussianBlur::release() {
	CFilterPrimitive::release();
}

/**
 * Sets a specific value in the SPGaussianBlur.
 */
void CGaussianBlur::set(unsigned int key, gchar const *value) {
	SPGaussianBlur* object = this->spgaussianblur;

    SPGaussianBlur *gaussianBlur = SP_GAUSSIANBLUR(object);

    switch(key) {
        case SP_ATTR_STDDEVIATION:
            gaussianBlur->stdDeviation.set(value);
            object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        default:
        	CFilterPrimitive::set(key, value);
            break;
    }
}

/**
 * Receives update notifications.
 */
void CGaussianBlur::update(SPCtx *ctx, guint flags) {
	SPGaussianBlur* object = this->spgaussianblur;

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        object->readAttr( "stdDeviation" );
    }

    CFilterPrimitive::update(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* CGaussianBlur::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
	SPGaussianBlur* object = this->spgaussianblur;

    /* TODO: Don't just clone, but create a new repr node and write all
     * relevant values into it */
    if (!repr) {
        repr = object->getRepr()->duplicate(doc);
    }

    CFilterPrimitive::write(doc, repr, flags);

    return repr;
}

void  sp_gaussianBlur_setDeviation(SPGaussianBlur *blur, float num)
{
    blur->stdDeviation.setNumber(num);
}
void  sp_gaussianBlur_setDeviation(SPGaussianBlur *blur, float num, float optnum)
{
    blur->stdDeviation.setNumber(num);
    blur->stdDeviation.setOptNumber(optnum);
}

void CGaussianBlur::build_renderer(Inkscape::Filters::Filter* filter) {
	SPGaussianBlur* primitive = this->spgaussianblur;

    SPGaussianBlur *sp_blur = SP_GAUSSIANBLUR(primitive);

    int handle = filter->add_primitive(Inkscape::Filters::NR_FILTER_GAUSSIANBLUR);
    Inkscape::Filters::FilterPrimitive *nr_primitive = filter->get_primitive(handle);
    Inkscape::Filters::FilterGaussian *nr_blur = dynamic_cast<Inkscape::Filters::FilterGaussian*>(nr_primitive);

    sp_filter_primitive_renderer_common(primitive, nr_primitive);

    gfloat num = sp_blur->stdDeviation.getNumber();
    if (num >= 0.0) {
        gfloat optnum = sp_blur->stdDeviation.getOptNumber();
        if(optnum >= 0.0)
            nr_blur->set_deviation((double) num, (double) optnum);
        else
            nr_blur->set_deviation((double) num);
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

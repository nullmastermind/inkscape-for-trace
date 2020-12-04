// SPDX-License-Identifier: GPL-2.0-or-later
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
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "gaussian-blur.h"

#include "attributes.h"

#include "display/nr-filter.h"
#include "display/nr-filter-gaussian.h"

#include "svg/svg.h"

#include "xml/repr.h"

SPGaussianBlur::SPGaussianBlur() : SPFilterPrimitive() {
}

SPGaussianBlur::~SPGaussianBlur() = default;

/**
 * Reads the Inkscape::XML::Node, and initializes SPGaussianBlur variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void SPGaussianBlur::build(SPDocument *document, Inkscape::XML::Node *repr) {
	SPFilterPrimitive::build(document, repr);

    this->readAttr(SPAttr::STDDEVIATION);
}

/**
 * Drops any allocated memory.
 */
void SPGaussianBlur::release() {
	SPFilterPrimitive::release();
}

/**
 * Sets a specific value in the SPGaussianBlur.
 */
void SPGaussianBlur::set(SPAttr key, gchar const *value) {
    switch(key) {
        case SPAttr::STDDEVIATION:
            this->stdDeviation.set(value);
            this->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        default:
        	SPFilterPrimitive::set(key, value);
            break;
    }
}

/**
 * Receives update notifications.
 */
void SPGaussianBlur::update(SPCtx *ctx, guint flags) {
    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        this->readAttr(SPAttr::STDDEVIATION);
    }

    SPFilterPrimitive::update(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* SPGaussianBlur::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
    /* TODO: Don't just clone, but create a new repr node and write all
     * relevant values into it */
    if (!repr) {
        repr = this->getRepr()->duplicate(doc);
    }

    SPFilterPrimitive::write(doc, repr, flags);

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

void SPGaussianBlur::build_renderer(Inkscape::Filters::Filter* filter) {
    int handle = filter->add_primitive(Inkscape::Filters::NR_FILTER_GAUSSIANBLUR);
    Inkscape::Filters::FilterPrimitive *nr_primitive = filter->get_primitive(handle);
    Inkscape::Filters::FilterGaussian *nr_blur = dynamic_cast<Inkscape::Filters::FilterGaussian*>(nr_primitive);

    this->renderer_common(nr_primitive);

    gfloat num = this->stdDeviation.getNumber();

    if (num >= 0.0) {
        gfloat optnum = this->stdDeviation.getOptNumber();

        if(optnum >= 0.0) {
            nr_blur->set_deviation((double) num, (double) optnum);
        } else {
            nr_blur->set_deviation((double) num);
        }
    }
}

/* Calculate the region taken up by gaussian blur
 *
 * @param region The original shape's region or previous primitive's region output.
 */
Geom::Rect SPGaussianBlur::calculate_region(Geom::Rect region)
{
    double x = this->stdDeviation.getNumber();
    double y = this->stdDeviation.getOptNumber();
    if (y == -1.0)
        y = x;
    // If not within the default 10% margin (see
    // http://www.w3.org/TR/SVG11/filters.html#FilterEffectsRegion), specify margins
    // The 2.4 is an empirical coefficient: at that distance the cutoff is practically invisible
    // (the opacity at 2.4 * radius is about 3e-3)
    region.expandBy(2.4 * x, 2.4 * y);
    return region;
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

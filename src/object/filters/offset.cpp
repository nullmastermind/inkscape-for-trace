// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <feOffset> implementation.
 *
 */
/*
 * Authors:
 *   hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Niko Kiirala <niko@kiirala.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006,2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/transforms.h>

#include "offset.h"

#include "attributes.h"
#include "helper-fns.h"

#include "display/nr-filter.h"
#include "display/nr-filter-offset.h"

#include "svg/svg.h"

#include "xml/repr.h"

SPFeOffset::SPFeOffset() : SPFilterPrimitive() {
    this->dx = 0;
    this->dy = 0;
}

SPFeOffset::~SPFeOffset() = default;

/**
 * Reads the Inkscape::XML::Node, and initializes SPFeOffset variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void SPFeOffset::build(SPDocument *document, Inkscape::XML::Node *repr) {
	SPFilterPrimitive::build(document, repr);

	this->readAttr(SPAttr::DX);
	this->readAttr(SPAttr::DY);
}

/**
 * Drops any allocated memory.
 */
void SPFeOffset::release() {
	SPFilterPrimitive::release();
}

/**
 * Sets a specific value in the SPFeOffset.
 */
void SPFeOffset::set(SPAttr key, gchar const *value) {
    double read_num;

    switch(key) {
        case SPAttr::DX:
            read_num = value ? helperfns_read_number(value) : 0;

            if (read_num != this->dx) {
                this->dx = read_num;
                this->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SPAttr::DY:
            read_num = value ? helperfns_read_number(value) : 0;

            if (read_num != this->dy) {
                this->dy = read_num;
                this->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
            
	/*DEAL WITH SETTING ATTRIBUTES HERE*/
        default:
        	SPFilterPrimitive::set(key, value);
            break;
    }
}

/**
 * Receives update notifications.
 */
void SPFeOffset::update(SPCtx *ctx, guint flags) {
    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        this->readAttr(SPAttr::DX);
        this->readAttr(SPAttr::DY);
    }

    SPFilterPrimitive::update(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* SPFeOffset::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
    /* TODO: Don't just clone, but create a new repr node and write all
     * relevant values into it */
    if (!repr) {
        repr = this->getRepr()->duplicate(doc);
    }

    SPFilterPrimitive::write(doc, repr, flags);

    return repr;
}

void SPFeOffset::build_renderer(Inkscape::Filters::Filter* filter) {
    g_assert(filter != nullptr);

    int primitive_n = filter->add_primitive(Inkscape::Filters::NR_FILTER_OFFSET);
    Inkscape::Filters::FilterPrimitive *nr_primitive = filter->get_primitive(primitive_n);
    Inkscape::Filters::FilterOffset *nr_offset = dynamic_cast<Inkscape::Filters::FilterOffset*>(nr_primitive);
    g_assert(nr_offset != nullptr);

    this->renderer_common(nr_primitive);

    nr_offset->set_dx(this->dx);
    nr_offset->set_dy(this->dy);
}

/**
 * Calculate the region taken up by an offset
 *
 * @param region The original shape's region or previous primitive's region output.
 */
Geom::Rect SPFeOffset::calculate_region(Geom::Rect region)
{
    region *= Geom::Translate(this->dx, this->dy);
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

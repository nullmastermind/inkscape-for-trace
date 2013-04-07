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
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "attributes.h"
#include "svg/svg.h"
#include "filters/offset.h"
#include "helper-fns.h"
#include "xml/repr.h"
#include "display/nr-filter.h"
#include "display/nr-filter-offset.h"

#include "sp-factory.h"

namespace {
	SPObject* createOffset() {
		return new SPFeOffset();
	}

	bool offsetRegistered = SPFactory::instance().registerObject("svg:feOffset", createOffset);
}

SPFeOffset::SPFeOffset() : SPFilterPrimitive() {
    this->dx = 0;
    this->dy = 0;
}

SPFeOffset::~SPFeOffset() {
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPFeOffset variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void SPFeOffset::build(SPDocument *document, Inkscape::XML::Node *repr) {
	SPFeOffset* object = this;

	SPFilterPrimitive::build(document, repr);

	object->readAttr( "dx" );
	object->readAttr( "dy" );
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
void SPFeOffset::set(unsigned int key, gchar const *value) {
	SPFeOffset* object = this;

    SPFeOffset *feOffset = SP_FEOFFSET(object);

    double read_num;
    switch(key) {
        case SP_ATTR_DX:
            read_num = value ? helperfns_read_number(value) : 0;
            if (read_num != feOffset->dx) {
                feOffset->dx = read_num;
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SP_ATTR_DY:
            read_num = value ? helperfns_read_number(value) : 0;
            if (read_num != feOffset->dy) {
                feOffset->dy = read_num;
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
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
	SPFeOffset* object = this;

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        object->readAttr( "dx" );
        object->readAttr( "dy" );
    }

    SPFilterPrimitive::update(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* SPFeOffset::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
	SPFeOffset* object = this;

    /* TODO: Don't just clone, but create a new repr node and write all
     * relevant values into it */
    if (!repr) {
        repr = object->getRepr()->duplicate(doc);
    }

    SPFilterPrimitive::write(doc, repr, flags);

    return repr;
}

void SPFeOffset::build_renderer(Inkscape::Filters::Filter* filter) {
	SPFeOffset* primitive = this;

    g_assert(primitive != NULL);
    g_assert(filter != NULL);

    SPFeOffset *sp_offset = SP_FEOFFSET(primitive);

    int primitive_n = filter->add_primitive(Inkscape::Filters::NR_FILTER_OFFSET);
    Inkscape::Filters::FilterPrimitive *nr_primitive = filter->get_primitive(primitive_n);
    Inkscape::Filters::FilterOffset *nr_offset = dynamic_cast<Inkscape::Filters::FilterOffset*>(nr_primitive);
    g_assert(nr_offset != NULL);

    sp_filter_primitive_renderer_common(primitive, nr_primitive);

    nr_offset->set_dx(sp_offset->dx);
    nr_offset->set_dy(sp_offset->dy);
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

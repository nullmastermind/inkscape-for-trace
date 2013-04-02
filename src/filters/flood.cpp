/** \file
 * SVG <feFlood> implementation.
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

#include "strneq.h"

#include "attributes.h"
#include "svg/svg.h"
#include "svg/svg-color.h"
#include "filters/flood.h"
#include "xml/repr.h"
#include "helper-fns.h"
#include "display/nr-filter.h"
#include "display/nr-filter-flood.h"

#include "sp-factory.h"

namespace {
	SPObject* createFlood() {
		return new SPFeFlood();
	}

	bool floodRegistered = SPFactory::instance().registerObject("svg:feFlood", createFlood);
}

/* FeFlood base class */
G_DEFINE_TYPE(SPFeFlood, sp_feFlood, G_TYPE_OBJECT);

static void sp_feFlood_class_init(SPFeFloodClass *klass)
{
}

CFeFlood::CFeFlood(SPFeFlood* flood) : CFilterPrimitive(flood) {
	this->spfeflood = flood;
}

CFeFlood::~CFeFlood() {
}

SPFeFlood::SPFeFlood() : SPFilterPrimitive() {
	SPFeFlood* feFlood = this;

	feFlood->cfeflood = new CFeFlood(feFlood);
	feFlood->typeHierarchy.insert(typeid(SPFeFlood));

	delete feFlood->cfilterprimitive;
	feFlood->cfilterprimitive = feFlood->cfeflood;
	feFlood->cobject = feFlood->cfeflood;

	feFlood->color = 0;

    feFlood->opacity = 1;
    feFlood->icc = NULL;
}

static void sp_feFlood_init(SPFeFlood *feFlood)
{
	new (feFlood) SPFeFlood();
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPFeFlood variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void CFeFlood::build(SPDocument *document, Inkscape::XML::Node *repr) {
	CFilterPrimitive::build(document, repr);

	SPFeFlood* object = this->spfeflood;

	/*LOAD ATTRIBUTES FROM REPR HERE*/
	object->readAttr( "flood-opacity" );
	object->readAttr( "flood-color" );
}

/**
 * Drops any allocated memory.
 */
void CFeFlood::release() {
	CFilterPrimitive::release();
}

/**
 * Sets a specific value in the SPFeFlood.
 */
void CFeFlood::set(unsigned int key, gchar const *value) {
	SPFeFlood* object = this->spfeflood;

    SPFeFlood *feFlood = SP_FEFLOOD(object);
    (void)feFlood;
    gchar const *cend_ptr = NULL;
    gchar *end_ptr = NULL;
    guint32 read_color;
    double read_num;
    bool dirty = false;
    
    switch(key) {
	/*DEAL WITH SETTING ATTRIBUTES HERE*/
        case SP_PROP_FLOOD_COLOR:
            cend_ptr = NULL;
            read_color = sp_svg_read_color(value, &cend_ptr, 0xffffffff);

            if (cend_ptr && read_color != feFlood->color){
                feFlood->color = read_color;
                dirty=true;
            }

            if (cend_ptr){
                while (g_ascii_isspace(*cend_ptr)) {
                    ++cend_ptr;
                }
                if (strneq(cend_ptr, "icc-color(", 10)) {
                    if (!feFlood->icc) feFlood->icc = new SVGICCColor();
                    if ( ! sp_svg_read_icc_color( cend_ptr, feFlood->icc ) ) {
                        delete feFlood->icc;
                        feFlood->icc = NULL;
                    }
                    dirty = true;
                }
            }
            if (dirty)
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_PROP_FLOOD_OPACITY:
            if (value) {
                read_num = g_ascii_strtod(value, &end_ptr);
                if (end_ptr != NULL)
                {
                    if (*end_ptr)
                    {
                        g_warning("Unable to convert \"%s\" to number", value);
                        read_num = 1;
                    }
                }
            }
            else {
                read_num = 1;
            }
            if (read_num != feFlood->opacity){
                feFlood->opacity = read_num;
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
void CFeFlood::update(SPCtx *ctx, guint flags) {
	SPFeFlood* object = this->spfeflood;

    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG |
                 SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {

        /* do something to trigger redisplay, updates? */

    }

    CFilterPrimitive::update(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* CFeFlood::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
	SPFeFlood* object = this->spfeflood;

    /* TODO: Don't just clone, but create a new repr node and write all
     * relevant values into it */
    if (!repr) {
        repr = object->getRepr()->duplicate(doc);
    }

    CFilterPrimitive::write(doc, repr, flags);

    return repr;
}

void CFeFlood::build_renderer(Inkscape::Filters::Filter* filter) {
	SPFeFlood* primitive = this->spfeflood;

    g_assert(primitive != NULL);
    g_assert(filter != NULL);

    SPFeFlood *sp_flood = SP_FEFLOOD(primitive);
    (void)sp_flood;

    int primitive_n = filter->add_primitive(Inkscape::Filters::NR_FILTER_FLOOD);
    Inkscape::Filters::FilterPrimitive *nr_primitive = filter->get_primitive(primitive_n);
    Inkscape::Filters::FilterFlood *nr_flood = dynamic_cast<Inkscape::Filters::FilterFlood*>(nr_primitive);
    g_assert(nr_flood != NULL);

    sp_filter_primitive_renderer_common(primitive, nr_primitive);
    
    nr_flood->set_opacity(sp_flood->opacity);
    nr_flood->set_color(sp_flood->color);
    nr_flood->set_icc(sp_flood->icc);
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

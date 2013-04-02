/** \file
 * SVG <feTurbulence> implementation.
 *
 */
/*
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2007 Felipe Sanches
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "attributes.h"
#include "svg/svg.h"
#include "turbulence.h"
#include "helper-fns.h"
#include "xml/repr.h"
#include <string.h>

#include "display/nr-filter.h"
#include "display/nr-filter-turbulence.h"

#include "sp-factory.h"

namespace {
	SPObject* createTurbulence() {
		return new SPFeTurbulence();
	}

	bool turbulenceRegistered = SPFactory::instance().registerObject("svg:feTurbulence", createTurbulence);
}

/* FeTurbulence base class */
G_DEFINE_TYPE(SPFeTurbulence, sp_feTurbulence, G_TYPE_OBJECT);

static void
sp_feTurbulence_class_init(SPFeTurbulenceClass *klass)
{
}

CFeTurbulence::CFeTurbulence(SPFeTurbulence* turb) : CFilterPrimitive(turb) {
	this->spfeturbulence = turb;
}

CFeTurbulence::~CFeTurbulence() {
}

SPFeTurbulence::SPFeTurbulence() : SPFilterPrimitive() {
	SPFeTurbulence* feTurbulence = this;

	feTurbulence->cfeturbulence = new CFeTurbulence(feTurbulence);
	feTurbulence->typeHierarchy.insert(typeid(SPFeTurbulence));

	delete feTurbulence->cfilterprimitive;
	feTurbulence->cfilterprimitive = feTurbulence->cfeturbulence;
	feTurbulence->cobject = feTurbulence->cfeturbulence;

	feTurbulence->stitchTiles = 0;
	feTurbulence->seed = 0;
	feTurbulence->numOctaves = 0;
	feTurbulence->type = Inkscape::Filters::TURBULENCE_FRACTALNOISE;

    feTurbulence->updated=false;
}

static void
sp_feTurbulence_init(SPFeTurbulence *feTurbulence)
{
	new (feTurbulence) SPFeTurbulence();
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPFeTurbulence variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void CFeTurbulence::build(SPDocument *document, Inkscape::XML::Node *repr) {
	SPFeTurbulence* object = this->spfeturbulence;

	CFilterPrimitive::build(document, repr);

	/*LOAD ATTRIBUTES FROM REPR HERE*/
	object->readAttr( "baseFrequency" );
	object->readAttr( "numOctaves" );
	object->readAttr( "seed" );
	object->readAttr( "stitchTiles" );
	object->readAttr( "type" );
}

/**
 * Drops any allocated memory.
 */
void CFeTurbulence::release() {
	CFilterPrimitive::release();
}

static bool sp_feTurbulence_read_stitchTiles(gchar const *value){
    if (!value) return false; // 'noStitch' is default
    switch(value[0]){
        case 's':
            if (strncmp(value, "stitch", 6) == 0) return true;
            break;
        case 'n':
            if (strncmp(value, "noStitch", 8) == 0) return false;
            break;
    }
    return false; // 'noStitch' is default
}

static Inkscape::Filters::FilterTurbulenceType sp_feTurbulence_read_type(gchar const *value){
    if (!value) return Inkscape::Filters::TURBULENCE_TURBULENCE; // 'turbulence' is default
    switch(value[0]){
        case 'f':
            if (strncmp(value, "fractalNoise", 12) == 0) return Inkscape::Filters::TURBULENCE_FRACTALNOISE;
            break;
        case 't':
            if (strncmp(value, "turbulence", 10) == 0) return Inkscape::Filters::TURBULENCE_TURBULENCE;
            break;
    }
    return Inkscape::Filters::TURBULENCE_TURBULENCE; // 'turbulence' is default
}

/**
 * Sets a specific value in the SPFeTurbulence.
 */
void CFeTurbulence::set(unsigned int key, gchar const *value) {
	SPFeTurbulence* object = this->spfeturbulence;

    SPFeTurbulence *feTurbulence = SP_FETURBULENCE(object);
    (void)feTurbulence;

    int read_int;
    double read_num;
    bool read_bool;
    Inkscape::Filters::FilterTurbulenceType read_type;

    switch(key) {
	/*DEAL WITH SETTING ATTRIBUTES HERE*/

        case SP_ATTR_BASEFREQUENCY:
            feTurbulence->baseFrequency.set(value);
                //From SVG spec: If two <number>s are provided, the first number represents a base frequency in the X direction and the second value represents a base frequency in the Y direction. If one number is provided, then that value is used for both X and Y.
            if (feTurbulence->baseFrequency.optNumIsSet() == false)
                feTurbulence->baseFrequency.setOptNumber(feTurbulence->baseFrequency.getNumber());
            feTurbulence->updated = false;
            object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_NUMOCTAVES:
            read_int = value ? (int)floor(helperfns_read_number(value)) : 1;
            if (read_int != feTurbulence->numOctaves){
                feTurbulence->numOctaves = read_int;
                feTurbulence->updated = false;
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SP_ATTR_SEED:
            read_num = value ? helperfns_read_number(value) : 0;
            if (read_num != feTurbulence->seed){
                feTurbulence->seed = read_num;
                feTurbulence->updated = false;
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SP_ATTR_STITCHTILES:
            read_bool = sp_feTurbulence_read_stitchTiles(value);
            if (read_bool != feTurbulence->stitchTiles){
                feTurbulence->stitchTiles = read_bool;
                feTurbulence->updated = false;
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SP_ATTR_TYPE:
            read_type = sp_feTurbulence_read_type(value);
            if (read_type != feTurbulence->type){
                feTurbulence->type = read_type;
                feTurbulence->updated = false;
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
void CFeTurbulence::update(SPCtx *ctx, guint flags) {
	SPFeTurbulence* object = this->spfeturbulence;

    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG |
                 SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {

        /* do something to trigger redisplay, updates? */

    }

    CFilterPrimitive::update(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* CFeTurbulence::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
	SPFeTurbulence* object = this->spfeturbulence;

    /* TODO: Don't just clone, but create a new repr node and write all
     * relevant values into it */
    if (!repr) {
        repr = object->getRepr()->duplicate(doc);
    }

    CFilterPrimitive::write(doc, repr, flags);

    /* turbulence doesn't take input */
    repr->setAttribute("in", 0);

    return repr;
}

void CFeTurbulence::build_renderer(Inkscape::Filters::Filter* filter) {
	SPFeTurbulence* primitive = this->spfeturbulence;

    g_assert(primitive != NULL);
    g_assert(filter != NULL);

    SPFeTurbulence *sp_turbulence = SP_FETURBULENCE(primitive);

    int primitive_n = filter->add_primitive(Inkscape::Filters::NR_FILTER_TURBULENCE);
    Inkscape::Filters::FilterPrimitive *nr_primitive = filter->get_primitive(primitive_n);
    Inkscape::Filters::FilterTurbulence *nr_turbulence = dynamic_cast<Inkscape::Filters::FilterTurbulence*>(nr_primitive);
    g_assert(nr_turbulence != NULL);

    sp_filter_primitive_renderer_common(primitive, nr_primitive);

    nr_turbulence->set_baseFrequency(0, sp_turbulence->baseFrequency.getNumber());
    nr_turbulence->set_baseFrequency(1, sp_turbulence->baseFrequency.getOptNumber());
    nr_turbulence->set_numOctaves(sp_turbulence->numOctaves);
    nr_turbulence->set_seed(sp_turbulence->seed);
    nr_turbulence->set_stitchTiles(sp_turbulence->stitchTiles);
    nr_turbulence->set_type(sp_turbulence->type);
    nr_turbulence->set_updated(sp_turbulence->updated);
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

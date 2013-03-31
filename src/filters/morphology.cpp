/** \file
 * SVG <feMorphology> implementation.
 *
 */
/*
 * Authors:
 *   Felipe Sanches <juca@members.fsf.org>
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>

#include "attributes.h"
#include "svg/svg.h"
#include "morphology.h"
#include "xml/repr.h"
#include "display/nr-filter.h"
#include "display/nr-filter-morphology.h"

/* FeMorphology base class */
G_DEFINE_TYPE(SPFeMorphology, sp_feMorphology, SP_TYPE_FILTER_PRIMITIVE);

static void
sp_feMorphology_class_init(SPFeMorphologyClass *klass)
{
}

CFeMorphology::CFeMorphology(SPFeMorphology* morph) : CFilterPrimitive(morph) {
	this->spfemorphology = morph;
}

CFeMorphology::~CFeMorphology() {
}

static void
sp_feMorphology_init(SPFeMorphology *feMorphology)
{
	feMorphology->cfemorphology = new CFeMorphology(feMorphology);
	feMorphology->typeHierarchy.insert(typeid(SPFeMorphology));

	delete feMorphology->cfilterprimitive;
	feMorphology->cfilterprimitive = feMorphology->cfemorphology;
	feMorphology->cobject = feMorphology->cfemorphology;

    //Setting default values:
    feMorphology->radius.set("0");
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPFeMorphology variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void CFeMorphology::build(SPDocument *document, Inkscape::XML::Node *repr) {
	CFilterPrimitive::build(document, repr);

	SPFeMorphology* object = this->spfemorphology;

	/*LOAD ATTRIBUTES FROM REPR HERE*/
	object->readAttr( "operator" );
	object->readAttr( "radius" );
}

/**
 * Drops any allocated memory.
 */
void CFeMorphology::release() {
	CFilterPrimitive::release();
}

static Inkscape::Filters::FilterMorphologyOperator sp_feMorphology_read_operator(gchar const *value){
    if (!value) return Inkscape::Filters::MORPHOLOGY_OPERATOR_ERODE; //erode is default
    switch(value[0]){
        case 'e':
            if (strncmp(value, "erode", 5) == 0) return Inkscape::Filters::MORPHOLOGY_OPERATOR_ERODE;
            break;
        case 'd':
            if (strncmp(value, "dilate", 6) == 0) return Inkscape::Filters::MORPHOLOGY_OPERATOR_DILATE;
            break;
    }
    return Inkscape::Filters::MORPHOLOGY_OPERATOR_ERODE; //erode is default
}

/**
 * Sets a specific value in the SPFeMorphology.
 */
void CFeMorphology::set(unsigned int key, gchar const *value) {
	SPFeMorphology* object = this->spfemorphology;

    SPFeMorphology *feMorphology = SP_FEMORPHOLOGY(object);
    (void)feMorphology;
    
    Inkscape::Filters::FilterMorphologyOperator read_operator;
    switch(key) {
    /*DEAL WITH SETTING ATTRIBUTES HERE*/
        case SP_ATTR_OPERATOR:
            read_operator = sp_feMorphology_read_operator(value);
            if (read_operator != feMorphology->Operator){
                feMorphology->Operator = read_operator;
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SP_ATTR_RADIUS:
            feMorphology->radius.set(value);
            //From SVG spec: If <y-radius> is not provided, it defaults to <x-radius>.
            if (feMorphology->radius.optNumIsSet() == false)
                feMorphology->radius.setOptNumber(feMorphology->radius.getNumber());
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
void CFeMorphology::update(SPCtx *ctx, guint flags) {
	SPFeMorphology* object = this->spfemorphology;

    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG |
                 SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {

        /* do something to trigger redisplay, updates? */

    }

    CFilterPrimitive::update(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* CFeMorphology::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
	SPFeMorphology* object = this->spfemorphology;

	/* TODO: Don't just clone, but create a new repr node and write all
     * relevant values into it */
    if (!repr) {
        repr = object->getRepr()->duplicate(doc);
    }

    CFilterPrimitive::write(doc, repr, flags);

    return repr;
}

void CFeMorphology::build_renderer(Inkscape::Filters::Filter* filter) {
	SPFeMorphology* primitive = this->spfemorphology;

    g_assert(primitive != NULL);
    g_assert(filter != NULL);

    SPFeMorphology *sp_morphology = SP_FEMORPHOLOGY(primitive);

    int primitive_n = filter->add_primitive(Inkscape::Filters::NR_FILTER_MORPHOLOGY);
    Inkscape::Filters::FilterPrimitive *nr_primitive = filter->get_primitive(primitive_n);
    Inkscape::Filters::FilterMorphology *nr_morphology = dynamic_cast<Inkscape::Filters::FilterMorphology*>(nr_primitive);
    g_assert(nr_morphology != NULL);

    sp_filter_primitive_renderer_common(primitive, nr_primitive); 
    
    nr_morphology->set_operator(sp_morphology->Operator);
    nr_morphology->set_xradius( sp_morphology->radius.getNumber() );
    nr_morphology->set_yradius( sp_morphology->radius.getOptNumber() );
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

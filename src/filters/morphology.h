/** \file
 * @brief SVG morphology filter effect
 *//*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SP_FEMORPHOLOGY_H_SEEN
#define SP_FEMORPHOLOGY_H_SEEN

#include "sp-filter-primitive.h"
#include "number-opt-number.h"
#include "display/nr-filter-morphology.h"

#define SP_TYPE_FEMORPHOLOGY (sp_feMorphology_get_type())
#define SP_FEMORPHOLOGY(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SP_TYPE_FEMORPHOLOGY, SPFeMorphology))
#define SP_FEMORPHOLOGY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), SP_TYPE_FEMORPHOLOGY, SPFeMorphologyClass))
#define SP_IS_FEMORPHOLOGY(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), SP_TYPE_FEMORPHOLOGY))
#define SP_IS_FEMORPHOLOGY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), SP_TYPE_FEMORPHOLOGY))

class CFeMorphology;

class SPFeMorphology : public SPFilterPrimitive {
public:
	CFeMorphology* cfemorphology;

    Inkscape::Filters::FilterMorphologyOperator Operator;
    NumberOptNumber radius;
};

struct SPFeMorphologyClass {
    SPFilterPrimitiveClass parent_class;
};

class CFeMorphology : public CFilterPrimitive {
public:
	CFeMorphology(SPFeMorphology* morph);
	virtual ~CFeMorphology();

	virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void release();

	virtual void set(unsigned int key, const gchar* value);

	virtual void update(SPCtx* ctx, unsigned int flags);

	virtual Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

	virtual void build_renderer(Inkscape::Filters::Filter* filter);

private:
	SPFeMorphology* spfemorphology;
};

GType sp_feMorphology_get_type();


#endif /* !SP_FEMORPHOLOGY_H_SEEN */

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

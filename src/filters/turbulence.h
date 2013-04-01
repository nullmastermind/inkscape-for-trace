/** @file
 * @brief SVG turbulence filter effect
 *//*
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SP_FETURBULENCE_H_SEEN
#define SP_FETURBULENCE_H_SEEN

#include "sp-filter-primitive.h"
#include "number-opt-number.h"
#include "display/nr-filter-turbulence.h"

#define SP_TYPE_FETURBULENCE (sp_feTurbulence_get_type())
#define SP_FETURBULENCE(obj) ((SPFeTurbulence*)obj)
#define SP_IS_FETURBULENCE(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPFeTurbulence)))

/* FeTurbulence base class */

class CFeTurbulence;

class SPFeTurbulence : public SPFilterPrimitive {
public:
	SPFeTurbulence();
	CFeTurbulence* cfeturbulence;

    /** TURBULENCE ATTRIBUTES HERE */
    NumberOptNumber baseFrequency;
    int numOctaves;
    double seed;
    bool stitchTiles;
    Inkscape::Filters::FilterTurbulenceType type;
    SVGLength x, y, height, width;
    bool updated;
};

struct SPFeTurbulenceClass {
    SPFilterPrimitiveClass parent_class;
};

class CFeTurbulence : public CFilterPrimitive {
public:
	CFeTurbulence(SPFeTurbulence* turb);
	virtual ~CFeTurbulence();

	virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void release();

	virtual void set(unsigned int key, const gchar* value);

	virtual void update(SPCtx* ctx, unsigned int flags);

	virtual Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

	virtual void build_renderer(Inkscape::Filters::Filter* filter);

private:
	SPFeTurbulence* spfeturbulence;
};

GType sp_feTurbulence_get_type();


#endif /* !SP_FETURBULENCE_H_SEEN */

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

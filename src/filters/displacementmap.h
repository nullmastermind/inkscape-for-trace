/** \file
 * SVG displacement map filter effect
 *//*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SP_FEDISPLACEMENTMAP_H_SEEN
#define SP_FEDISPLACEMENTMAP_H_SEEN

#include "sp-filter-primitive.h"

#define SP_TYPE_FEDISPLACEMENTMAP (sp_feDisplacementMap_get_type())
#define SP_FEDISPLACEMENTMAP(obj) ((SPFeDisplacementMap*)obj)
#define SP_IS_FEDISPLACEMENTMAP(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPFeDisplacementMap)))

enum FilterDisplacementMapChannelSelector {
    DISPLACEMENTMAP_CHANNEL_RED,
    DISPLACEMENTMAP_CHANNEL_GREEN,
    DISPLACEMENTMAP_CHANNEL_BLUE,
    DISPLACEMENTMAP_CHANNEL_ALPHA,
    DISPLACEMENTMAP_CHANNEL_ENDTYPE
};

class CFeDisplacementMap;

class SPFeDisplacementMap : public SPFilterPrimitive {
public:
	SPFeDisplacementMap();
	CFeDisplacementMap* cfedisplacementmap;

    int in2; 
    double scale;
    FilterDisplacementMapChannelSelector xChannelSelector;
    FilterDisplacementMapChannelSelector yChannelSelector;
};

struct SPFeDisplacementMapClass {
    SPFilterPrimitiveClass parent_class;
};

class CFeDisplacementMap : public CFilterPrimitive {
public:
	CFeDisplacementMap(SPFeDisplacementMap* map);
	virtual ~CFeDisplacementMap();

	virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void release();

	virtual void set(unsigned int key, const gchar* value);

	virtual void update(SPCtx* ctx, unsigned int flags);

	virtual Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

	virtual void build_renderer(Inkscape::Filters::Filter* filter);

private:
	SPFeDisplacementMap* spfedisplacementmap;
};

GType sp_feDisplacementMap_get_type();

#endif /* !SP_FEDISPLACEMENTMAP_H_SEEN */

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

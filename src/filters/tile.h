/** @file
 * @brief SVG tile filter effect
 *//*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SP_FETILE_H_SEEN
#define SP_FETILE_H_SEEN

#include "sp-filter-primitive.h"

#define SP_TYPE_FETILE (sp_feTile_get_type())
#define SP_FETILE(obj) ((SPFeTile*)obj)
#define SP_IS_FETILE(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPFeTile)))

/* FeTile base class */
class CFeTile;

class SPFeTile : public SPFilterPrimitive {
public:
    CFeTile* cfetile;
};

struct SPFeTileClass {
    SPFilterPrimitiveClass parent_class;
};

class CFeTile : public CFilterPrimitive {
public:
	CFeTile(SPFeTile* tile);
	virtual ~CFeTile();

	virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void release();

	virtual void set(unsigned int key, const gchar* value);

	virtual void update(SPCtx* ctx, unsigned int flags);

	virtual Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

	virtual void build_renderer(Inkscape::Filters::Filter* filter);

private:
	SPFeTile* spfetile;
};

GType sp_feTile_get_type();

#endif /* !SP_FETILE_H_SEEN */

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

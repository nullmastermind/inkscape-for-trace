/** @file
 * @brief SVG color matrix filter effect
 *//*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#ifndef SP_FECOLORMATRIX_H_SEEN
#define SP_FECOLORMATRIX_H_SEEN

#include <vector>
#include "sp-filter-primitive.h"
#include "display/nr-filter-colormatrix.h"

#define SP_TYPE_FECOLORMATRIX (sp_feColorMatrix_get_type())
#define SP_FECOLORMATRIX(obj) ((SPFeColorMatrix*)obj)
#define SP_IS_FECOLORMATRIX(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPFeColorMatrix)))

class CFeColorMatrix;

class SPFeColorMatrix : public SPFilterPrimitive {
public:
	SPFeColorMatrix();
	CFeColorMatrix* cfecolormatrix;

    Inkscape::Filters::FilterColorMatrixType type;
    gdouble value;
    std::vector<gdouble> values;
};

struct SPFeColorMatrixClass {
    SPFilterPrimitiveClass parent_class;
};

class CFeColorMatrix : public CFilterPrimitive {
public:
	CFeColorMatrix(SPFeColorMatrix* matrix);
	virtual ~CFeColorMatrix();

	virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void release();

	virtual void set(unsigned int key, const gchar* value);

	virtual void update(SPCtx* ctx, unsigned int flags);

	virtual Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

	virtual void build_renderer(Inkscape::Filters::Filter* filter);

private:
	SPFeColorMatrix* spfecolormatrix;
};

GType sp_feColorMatrix_get_type();

#endif /* !SP_FECOLORMATRIX_H_SEEN */

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

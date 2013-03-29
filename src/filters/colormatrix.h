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
#define SP_FECOLORMATRIX(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SP_TYPE_FECOLORMATRIX, SPFeColorMatrix))
#define SP_FECOLORMATRIX_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), SP_TYPE_FECOLORMATRIX, SPFeColorMatrixClass))
#define SP_IS_FECOLORMATRIX(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), SP_TYPE_FECOLORMATRIX))
#define SP_IS_FECOLORMATRIX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), SP_TYPE_FECOLORMATRIX))

class CFeColorMatrix;

class SPFeColorMatrix : public SPFilterPrimitive {
public:
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

	virtual void onBuild(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void onRelease();

	virtual void onSet(unsigned int key, const gchar* value);

	virtual void onUpdate(SPCtx* ctx, unsigned int flags);

	virtual Inkscape::XML::Node* onWrite(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

	virtual void onBuildRenderer(Inkscape::Filters::Filter* filter);

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

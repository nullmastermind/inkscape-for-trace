#ifndef SP_MESH_GRADIENT_H
#define SP_MESH_GRADIENT_H

/** \file
 * SPMeshGradient: SVG <meshgradient> implementation.
 */

#include "svg/svg-length.h"
#include "sp-gradient.h"
#include "sp-mesh-gradient-fns.h"

class CMeshGradient;

/** Mesh gradient. */
class SPMeshGradient : public SPGradient {
public:
	CMeshGradient* cmeshgradient;

    SVGLength x;  // Upper left corner of mesh
    SVGLength y;  // Upper right corner of mesh
};

/// The SPMeshGradient vtable.
struct SPMeshGradientClass {
    SPGradientClass parent_class;
};


class CMeshGradient : public CGradient {
public:
	CMeshGradient(SPMeshGradient* meshgradient);
	virtual ~CMeshGradient();

	virtual void build(SPDocument *document, Inkscape::XML::Node *repr);
	virtual void set(unsigned key, gchar const *value);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);
	virtual cairo_pattern_t* pattern_new(cairo_t *ct, Geom::OptRect const &bbox, double opacity);

protected:
	SPMeshGradient* spmeshgradient;
};


#endif /* !SP_MESH_GRADIENT_H */

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

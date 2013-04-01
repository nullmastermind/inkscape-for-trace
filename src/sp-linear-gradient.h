#ifndef SP_LINEAR_GRADIENT_H
#define SP_LINEAR_GRADIENT_H

/** \file
 * SPLinearGradient: SVG <lineargradient> implementation
 */

#include "sp-gradient.h"
#include "svg/svg-length.h"
#include "sp-linear-gradient-fns.h"

class CLinearGradient;

/** Linear gradient. */
class SPLinearGradient : public SPGradient {
public:
	SPLinearGradient();
	CLinearGradient* clineargradient;

    SVGLength x1;
    SVGLength y1;
    SVGLength x2;
    SVGLength y2;
};

/// The SPLinearGradient vtable.
struct SPLinearGradientClass {
    SPGradientClass parent_class;
};


class CLinearGradient : public CGradient {
public:
	CLinearGradient(SPLinearGradient* lineargradient);
	virtual ~CLinearGradient();

	virtual void build(SPDocument *document, Inkscape::XML::Node *repr);
	virtual void set(unsigned key, gchar const *value);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);
	virtual cairo_pattern_t* pattern_new(cairo_t *ct, Geom::OptRect const &bbox, double opacity);

protected:
	SPLinearGradient* splineargradient;
};


#endif /* !SP_LINEAR_GRADIENT_H */

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

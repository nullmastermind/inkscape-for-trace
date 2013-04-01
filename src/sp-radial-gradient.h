#ifndef SP_RADIAL_GRADIENT_H
#define SP_RADIAL_GRADIENT_H

/** \file
 * SPRadialGradient: SVG <radialgradient> implementtion.
 */

#include <glib.h>
#include "sp-gradient.h"
#include "svg/svg-length.h"
#include "sp-radial-gradient-fns.h"

class CRadialGradient;

/** Radial gradient. */
class SPRadialGradient : public SPGradient {
public:
	SPRadialGradient();
	CRadialGradient* cradialgradient;

    SVGLength cx;
    SVGLength cy;
    SVGLength r;
    SVGLength fx;
    SVGLength fy;
};

/// The SPRadialGradient vtable.
struct SPRadialGradientClass {
    SPGradientClass parent_class;
};


class CRadialGradient : public CGradient {
public:
	CRadialGradient(SPRadialGradient* radialgradient);
	virtual ~CRadialGradient();

	virtual void build(SPDocument *document, Inkscape::XML::Node *repr);
	virtual void set(unsigned key, gchar const *value);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);
	virtual cairo_pattern_t* pattern_new(cairo_t *ct, Geom::OptRect const &bbox, double opacity);

protected:
	SPRadialGradient* spradialgradient;
};


#endif /* !SP_RADIAL_GRADIENT_H */

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

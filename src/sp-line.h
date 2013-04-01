#ifndef SEEN_SP_LINE_H
#define SEEN_SP_LINE_H

/*
 * SVG <line> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "svg/svg-length.h"
#include "sp-shape.h"



#define SP_TYPE_LINE            (sp_line_get_type())
#define SP_LINE(obj) ((SPLine*)obj)
#define SP_IS_LINE(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPLine)))

class SPLine;
class SPLineClass;
class CLine;

GType sp_line_get_type(void) G_GNUC_CONST;

class SPLine : public SPShape {
public:
	CLine* cline;

    SVGLength x1;
    SVGLength y1;
    SVGLength x2;
    SVGLength y2;

private:
    friend class SPLineClass;
};

class SPLineClass {
public:
    SPShapeClass parent_class;

private:
    friend class SPLine;
};


class CLine : public CShape {
public:
	CLine(SPLine* line);
	virtual ~CLine();

	virtual void build(SPDocument *document, Inkscape::XML::Node *repr);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);
	virtual void set(unsigned int key, gchar const* value);

	virtual gchar* description();
	virtual Geom::Affine set_transform(Geom::Affine const &transform);
	virtual void convert_to_guides();
	virtual void update(SPCtx* ctx, guint flags);

	virtual void set_shape();

protected:
	SPLine* spline;
};


#endif // SEEN_SP_LINE_H
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

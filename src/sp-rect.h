#ifndef SEEN_SP_RECT_H
#define SEEN_SP_RECT_H

/*
 * SVG <rect> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "svg/svg-length.h"
#include "sp-shape.h"
#include <2geom/forward.h>

G_BEGIN_DECLS

#define SP_TYPE_RECT            (sp_rect_get_type ())
#define SP_RECT(obj) ((SPRect*)obj)
#define SP_IS_RECT(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPRect)))

class CRect;

class SPRect : public SPShape {
public:
	CRect* crect;

	SVGLength x;
	SVGLength y;
	SVGLength width;
	SVGLength height;
	SVGLength rx;
	SVGLength ry;

	void setPosition(gdouble x, gdouble y, gdouble width, gdouble height);

	/* If SET if FALSE, VALUE is just ignored */
	void setRx(bool set, gdouble value);
	void setRy(bool set, gdouble value);

	void setVisibleRx(gdouble rx);
	void setVisibleRy(gdouble ry);
	gdouble getVisibleRx() const;
	gdouble getVisibleRy() const;
	Geom::Rect getRect() const;

	void setVisibleWidth(gdouble rx);
	void setVisibleHeight(gdouble ry);
	gdouble getVisibleWidth() const;
	gdouble getVisibleHeight() const;

	void compensateRxRy(Geom::Affine xform);

private:
	static gdouble vectorStretch(Geom::Point p0, Geom::Point p1, Geom::Affine xform);
};

struct SPRectClass {
	SPShapeClass parent_class;
};


class CRect : public CShape {
public:
	CRect(SPRect* sprect);
	virtual ~CRect();

	virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);

	void set(unsigned key, gchar const *value);
	void update(SPCtx* ctx, unsigned int flags);

	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);
	virtual gchar* description();

	void set_shape();
	virtual Geom::Affine set_transform(Geom::Affine const& xform);

	void snappoints(std::vector<Inkscape::SnapCandidatePoint> &p, Inkscape::SnapPreferences const *snapprefs);
	void convert_to_guides();

protected:
	SPRect* sprect;
};


/* Standard GType function */
GType sp_rect_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif // SEEN_SP_RECT_H

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

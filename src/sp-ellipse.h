#ifndef __SP_ELLIPSE_H__
#define __SP_ELLIPSE_H__

/*
 * SVG <ellipse> and related implementations
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Mitsuru Oka
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "svg/svg-length.h"
#include "sp-shape.h"

G_BEGIN_DECLS

/* Common parent class */

#define SP_TYPE_GENERICELLIPSE (sp_genericellipse_get_type ())
#define SP_GENERICELLIPSE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_GENERICELLIPSE, SPGenericEllipse))
#define SP_GENERICELLIPSE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_GENERICELLIPSE, SPGenericEllipseClass))
#define SP_IS_GENERICELLIPSE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_GENERICELLIPSE))
#define SP_IS_GENERICELLIPSE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_GENERICELLIPSE))

class CGenericEllipse;

class SPGenericEllipse : public SPShape {
public:
	CGenericEllipse* cgenericEllipse;

	SVGLength cx;
	SVGLength cy;
	SVGLength rx;
	SVGLength ry;

	unsigned int closed : 1;
	double start, end;
};

struct SPGenericEllipseClass {
	SPShapeClass parent_class;
};


class CGenericEllipse : public CShape {
public:
	CGenericEllipse(SPGenericEllipse* genericEllipse);
	virtual ~CGenericEllipse();

	virtual void update(SPCtx* ctx, unsigned int flags);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);

	virtual void snappoints(std::vector<Inkscape::SnapCandidatePoint> &p, Inkscape::SnapPreferences const *snapprefs);
	virtual void set_shape();

	virtual void update_patheffect(bool write);

protected:
	SPGenericEllipse* spgenericEllipse;
};


GType sp_genericellipse_get_type (void);

/* This is technically priate by we need this in object edit (Lauris) */
void sp_genericellipse_normalize (SPGenericEllipse *ellipse);

/* SVG <ellipse> element */

#define SP_TYPE_ELLIPSE (sp_ellipse_get_type ())
#define SP_ELLIPSE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_ELLIPSE, SPEllipse))
#define SP_ELLIPSE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_ELLIPSE, SPEllipseClass))
#define SP_IS_ELLIPSE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_ELLIPSE))
#define SP_IS_ELLIPSE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_ELLIPSE))

class CEllipse;

class SPEllipse : public SPGenericEllipse {
public:
	CEllipse* cellipse;
};

struct SPEllipseClass {
	SPGenericEllipseClass parent_class;
};


class CEllipse : public CGenericEllipse {
public:
	CEllipse(SPEllipse* ellipse);
	virtual ~CEllipse();

	virtual void build(SPDocument *document, Inkscape::XML::Node *repr);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);
	virtual void set(unsigned int key, gchar const* value);
	virtual gchar* description();

protected:
	SPEllipse* spellipse;
};


GType sp_ellipse_get_type (void);

void sp_ellipse_position_set (SPEllipse * ellipse, gdouble x, gdouble y, gdouble rx, gdouble ry);

/* SVG <circle> element */

#define SP_TYPE_CIRCLE (sp_circle_get_type ())
#define SP_CIRCLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_CIRCLE, SPCircle))
#define SP_CIRCLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_CIRCLE, SPCircleClass))
#define SP_IS_CIRCLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_CIRCLE))
#define SP_IS_CIRCLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_CIRCLE))

class CCircle;

class SPCircle : public SPGenericEllipse {
public:
	CCircle* ccircle;
};

struct SPCircleClass {
	SPGenericEllipseClass parent_class;
};


class CCircle : public CGenericEllipse {
public:
	CCircle(SPCircle* circle);
	virtual ~CCircle();

	virtual void build(SPDocument *document, Inkscape::XML::Node *repr);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);
	virtual void set(unsigned int key, gchar const* value);
	virtual gchar* description();

protected:
	SPCircle* spcircle;
};


GType sp_circle_get_type (void);

/* <path sodipodi:type="arc"> element */

#define SP_TYPE_ARC (sp_arc_get_type ())
#define SP_ARC(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_ARC, SPArc))
#define SP_ARC_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_ARC, SPArcClass))
#define SP_IS_ARC(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_ARC))
#define SP_IS_ARC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_ARC))

class CArc;

class SPArc : public SPGenericEllipse {
public:
	CArc* carc;
};

struct SPArcClass {
	SPGenericEllipseClass parent_class;
};


class CArc : public CGenericEllipse {
public:
	CArc(SPArc* arc);
	virtual ~CArc();

	virtual void build(SPDocument *document, Inkscape::XML::Node *repr);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);
	virtual void set(unsigned int key, gchar const* value);
	virtual gchar* description();
	virtual void modified(unsigned int flags);

protected:
	SPArc* sparc;
};


GType sp_arc_get_type (void);
void sp_arc_position_set (SPArc * arc, gdouble x, gdouble y, gdouble rx, gdouble ry);
Geom::Point sp_arc_get_xy (SPArc *ge, gdouble arg);

G_END_DECLS

#endif

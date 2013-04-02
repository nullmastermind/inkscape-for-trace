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

/* Common parent class */
#define SP_GENERICELLIPSE(obj) ((SPGenericEllipse*)obj)
#define SP_IS_GENERICELLIPSE(obj) (dynamic_cast<const SPGenericEllipse*>((SPObject*)obj))

class CGenericEllipse;

class SPGenericEllipse : public SPShape {
public:
	SPGenericEllipse();
	CGenericEllipse* cgenericEllipse;

	SVGLength cx;
	SVGLength cy;
	SVGLength rx;
	SVGLength ry;

	unsigned int closed : 1;
	double start, end;
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

/* This is technically priate by we need this in object edit (Lauris) */
void sp_genericellipse_normalize (SPGenericEllipse *ellipse);

/* SVG <ellipse> element */
#define SP_ELLIPSE(obj) ((SPEllipse*)obj)
#define SP_IS_ELLIPSE(obj) (dynamic_cast<const SPEllipse*>((SPObject*)obj))

class CEllipse;

class SPEllipse : public SPGenericEllipse {
public:
	SPEllipse();
	CEllipse* cellipse;
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

void sp_ellipse_position_set (SPEllipse * ellipse, gdouble x, gdouble y, gdouble rx, gdouble ry);

/* SVG <circle> element */
#define SP_CIRCLE(obj) ((SPCircle*)obj)
#define SP_IS_CIRCLE(obj) (dynamic_cast<const SPCircle*>((SPObject*)obj))

class CCircle;

class SPCircle : public SPGenericEllipse {
public:
	SPCircle();
	CCircle* ccircle;
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

/* <path sodipodi:type="arc"> element */
#define SP_ARC(obj) ((SPArc*)obj)
#define SP_IS_ARC(obj) (dynamic_cast<const SPArc*>((SPObject*)obj))

class CArc;

class SPArc : public SPGenericEllipse {
public:
	SPArc();
	CArc* carc;
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

void sp_arc_position_set (SPArc * arc, gdouble x, gdouble y, gdouble rx, gdouble ry);
Geom::Point sp_arc_get_xy (SPArc *ge, gdouble arg);

#endif

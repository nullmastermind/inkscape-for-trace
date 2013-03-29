#ifndef __SP_STAR_H__
#define __SP_STAR_H__

/*
 * <sodipodi:star> implementation
 *
 * Authors:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-polygon.h"

#define SP_TYPE_STAR            (sp_star_get_type ())
#define SP_STAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_STAR, SPStar))
#define SP_STAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_STAR, SPStarClass))
#define SP_IS_STAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_STAR))
#define SP_IS_STAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_STAR))

class CStar;
typedef enum {
	SP_STAR_POINT_KNOT1,
	SP_STAR_POINT_KNOT2
} SPStarPoint;

class SPStar : public SPPolygon {
public:
	CStar* cstar;

	gint sides;

	Geom::Point center;
	double r[2];
	double arg[2];
	bool flatsided;

	double rounded;
	double randomized;
};

struct SPStarClass {
	SPPolygonClass parent_class;
};

// CPPIFY: This derivation is a bit weird.
// parent_class = reinterpret_cast<SPShapeClass *>(g_type_class_ref(SP_TYPE_SHAPE));
// So shouldn't star be derived from shape instead of polygon?
// What does polygon have that shape doesn't?
class CStar : public CPolygon {
public:
	CStar(SPStar* star);
	virtual ~CStar();

	virtual void onBuild(SPDocument *document, Inkscape::XML::Node *repr);
	virtual Inkscape::XML::Node* onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);
	virtual void onSet(unsigned int key, gchar const* value);
	virtual void onUpdate(SPCtx* ctx, guint flags);

	virtual gchar* onDescription();
	virtual void onSnappoints(std::vector<Inkscape::SnapCandidatePoint> &p, Inkscape::SnapPreferences const *snapprefs);

	virtual void onUpdatePatheffect(bool write);
	virtual void onSetShape();

protected:
	SPStar* spstar;
};


GType sp_star_get_type (void);

void sp_star_position_set (SPStar *star, gint sides, Geom::Point center, gdouble r1, gdouble r2, gdouble arg1, gdouble arg2, bool isflat, double rounded, double randomized);

Geom::Point sp_star_get_xy (SPStar const *star, SPStarPoint point, gint index, bool randomized = false);



#endif

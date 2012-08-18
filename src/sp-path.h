#ifndef SEEN_SP_PATH_H
#define SEEN_SP_PATH_H

/*
 * SVG <path> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Ximian, Inc.
 *   Johan Engelen
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 1999-2012 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-shape.h"
#include "sp-conn-end-pair.h"

class SPCurve;
class CPath;

#define SP_TYPE_PATH (sp_path_get_type ())
#define SP_PATH(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_PATH, SPPath))
#define SP_IS_PATH(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_PATH))

/**
 * SVG <path> implementation
 */
class SPPath : public SPShape {
public:
	CPath* cpath;

    gint nodesInPath() const;

    // still in lowercase because the names should be clearer on whether curve, curve->copy or curve-ref is returned.
    void     set_original_curve (SPCurve *curve, unsigned int owner, bool write);
    SPCurve* get_original_curve () const;
    SPCurve* get_curve_for_edit () const;
    const SPCurve* get_curve_reference() const;

public: // should be made protected
    SPCurve* get_curve();
    friend class SPConnEndPair;

public:
    SPConnEndPair connEndPair;
};

struct SPPathClass {
    SPShapeClass shape_class;
};


class CPath : public CShape {
public:
	CPath(SPPath* path);
	~CPath();

	virtual void onBuild(SPDocument *document, Inkscape::XML::Node *repr);
	virtual void onRelease();
	virtual void onUpdate(SPCtx* ctx, guint flags);

	virtual void onSet(unsigned int key, gchar const* value);
	virtual Inkscape::XML::Node* onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);

	virtual gchar* onDescription();
	virtual Geom::Affine onSetTransform(Geom::Affine const &transform);
    virtual void onConvertToGuides();

    virtual void onUpdatePatheffect(bool write);

protected:
	SPPath* sppath;
};


GType sp_path_get_type (void);

#endif // SEEN_SP_PATH_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

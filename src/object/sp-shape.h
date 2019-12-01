// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_SHAPE_H
#define SEEN_SP_SHAPE_H

/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *   Johan Engelen
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 1999-2012 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/forward.h>
#include <cstddef>
#include <sigc++/connection.h>

#include "sp-lpe-item.h"
#include "sp-marker-loc.h"

#define SP_SHAPE(obj) (dynamic_cast<SPShape*>((SPObject*)obj))
#define SP_IS_SHAPE(obj) (dynamic_cast<const SPShape*>((SPObject*)obj) != NULL)

#define SP_SHAPE_WRITE_PATH (1 << 2)

class SPDesktop;
class SPMarker;
namespace Inkscape { class DrawingItem; }

/**
 * Base class for shapes, including <path> element
 */
class SPShape : public SPLPEItem {
public:
	SPShape();
	~SPShape() override;

    SPCurve * getCurve (unsigned int owner = FALSE) const;
    SPCurve * getCurveBeforeLPE (unsigned int owner = FALSE) const;
    SPCurve * getCurveForEdit (unsigned int owner = FALSE) const;
    void setCurve (SPCurve *curve, unsigned int owner = FALSE);
    void setCurveBeforeLPE (SPCurve *new_curve, unsigned int owner = FALSE);
    void setCurveInsync (SPCurve *curve, unsigned int owner = FALSE);
    int hasMarkers () const;
    int numberOfMarkers (int type) const;

    // bbox cache
    mutable bool bbox_geom_cache_is_valid = false;
    mutable bool bbox_vis_cache_is_valid = false;
    mutable Geom::Affine bbox_transform_cache;
    mutable Geom::OptRect bbox_geom_cache;
    mutable Geom::OptRect bbox_vis_cache;


public: // temporarily public, until SPPath is properly classed, etc.
    SPCurve *_curve_before_lpe;
    SPCurve *_curve;

public:
    SPMarker *_marker[SP_MARKER_LOC_QTY];
    sigc::connection _release_connect [SP_MARKER_LOC_QTY];
    sigc::connection _modified_connect [SP_MARKER_LOC_QTY];

	void build(SPDocument *document, Inkscape::XML::Node *repr) override;
	void release() override;
	void update(SPCtx* ctx, unsigned int flags) override;
	void modified(unsigned int flags) override;

	void set(SPAttributeEnum key, char const* value) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;

	Geom::OptRect bbox(Geom::Affine const &transform, SPItem::BBoxType bboxtype) const override;
	void print(SPPrintContext* ctx) override;

	Inkscape::DrawingItem* show(Inkscape::Drawing &drawing, unsigned int key, unsigned int flags) override;
	void hide(unsigned int key) override;

	void snappoints(std::vector<Inkscape::SnapCandidatePoint> &p, Inkscape::SnapPreferences const *snapprefs) const override;

	virtual void set_shape();
	void update_patheffect(bool write) override;
};


void sp_shape_set_marker (SPObject *object, unsigned int key, const char *value);

Geom::Affine sp_shape_marker_get_transform(Geom::Curve const & c1, Geom::Curve const & c2);
Geom::Affine sp_shape_marker_get_transform_at_start(Geom::Curve const & c);
Geom::Affine sp_shape_marker_get_transform_at_end(Geom::Curve const & c);

#endif // SEEN_SP_SHAPE_H

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

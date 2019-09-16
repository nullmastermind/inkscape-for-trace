// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_ITEM_FLOWTEXT_H
#define SEEN_SP_ITEM_FLOWTEXT_H

/*
 */

#include <2geom/forward.h>

#include "libnrtype/Layout-TNG.h"
#include "sp-item.h"
#include "desktop.h"

#define SP_FLOWTEXT(obj) (dynamic_cast<SPFlowtext*>((SPObject*)obj))
#define SP_IS_FLOWTEXT(obj) (dynamic_cast<const SPFlowtext*>((SPObject*)obj) != NULL)


namespace Inkscape {

class DrawingGroup;

} // namespace Inkscape

class SPFlowtext : public SPItem {
public:
	SPFlowtext();
	~SPFlowtext() override;

    /** Completely recalculates the layout. */
    void rebuildLayout();

    /** Converts the flowroot in into a \<text\> tree, keeping all the formatting and positioning,
    but losing the automatic wrapping ability. */
    Inkscape::XML::Node *getAsText();

    // TODO check if these should return SPRect instead of SPItem

    SPItem *get_frame(SPItem const *after);

    SPItem const *get_frame(SPItem const *after) const;

    bool has_internal_frame() const;

//semiprivate:  (need to be accessed by the C-style functions still)
    Inkscape::Text::Layout layout;

    /** discards the drawing objects representing this text. */
    void _clearFlow(Inkscape::DrawingGroup* in_arena);

    double par_indent;

    bool _optimizeScaledText;

	/** Converts the text object to its component curves */
	SPCurve *getNormalizedBpath() const {
		return layout.convertToCurves();
	}

    /** Optimize scaled flow text on next set_transform. */
    void optimizeScaledText()
        {_optimizeScaledText = true;}

private:
    /** Recursively walks the xml tree adding tags and their contents. */
    void _buildLayoutInput(SPObject *root, Shape const *exclusion_shape, std::list<Shape> *shapes, SPObject **pending_line_break_object);

    /** calculates the union of all the \<flowregionexclude\> children
    of this flowroot. */
    Shape* _buildExclusionShape() const;

public:
	void build(SPDocument* doc, Inkscape::XML::Node* repr) override;

	void child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref) override;
	void remove_child(Inkscape::XML::Node* child) override;

	void set(SPAttributeEnum key, const char* value) override;
	Geom::Affine set_transform(Geom::Affine const& xform) override;

	void update(SPCtx* ctx, unsigned int flags) override;
	void modified(unsigned int flags) override;
    void fix_overflow_flowregion(bool inverse);

	Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, unsigned int flags) override;

	Geom::OptRect bbox(Geom::Affine const &transform, SPItem::BBoxType type) const override;
	void print(SPPrintContext *ctx) override;
        const char* displayName() const override;
	char* description() const override;
	Inkscape::DrawingItem* show(Inkscape::Drawing &drawing, unsigned int key, unsigned int flags) override;
	void hide(unsigned int key) override;
    void snappoints(std::vector<Inkscape::SnapCandidatePoint> &p, Inkscape::SnapPreferences const *snapprefs) const override;
};

SPItem *create_flowtext_with_internal_frame (SPDesktop *desktop, Geom::Point p1, Geom::Point p2);

#endif // SEEN_SP_ITEM_FLOWTEXT_H

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

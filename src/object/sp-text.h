#ifndef SEEN_SP_TEXT_H
#define SEEN_SP_TEXT_H

/*
 * SVG <text> and <tspan> implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <cstddef>
#include <sigc++/sigc++.h>

#include "libnrtype/Layout-TNG.h"
#include "sp-item.h"
#include "sp-string.h" // Provides many other headers with SP_IS_STRING
#include "text-tag-attributes.h"

#define SP_TEXT(obj) (dynamic_cast<SPText*>((SPObject*)obj))
#define SP_IS_TEXT(obj) (dynamic_cast<const SPText*>((SPObject*)obj) != NULL)

/* Text specific flags */
#define SP_TEXT_CONTENT_MODIFIED_FLAG SP_OBJECT_USER_MODIFIED_FLAG_A
#define SP_TEXT_LAYOUT_MODIFIED_FLAG SP_OBJECT_USER_MODIFIED_FLAG_A


/* SPText */
class SPText : public SPItem {
public:
	SPText();
	~SPText() override;

    /** Converts the text object to its component curves */
    SPCurve *getNormalizedBpath() const
        {return layout.convertToCurves();}

    /** Completely recalculates the layout. */
    void rebuildLayout();

    //semiprivate:  (need to be accessed by the C-style functions still)
    TextTagAttributes attributes;
    Inkscape::Text::Layout layout;

    /** when the object is transformed it's nicer to change the font size
    and coordinates when we can, rather than just applying a matrix
    transform. is_root is used to indicate to the function that it should
    extend zero-length position vectors to length 1 in order to record the
    new position. This is necessary to convert from objects whose position is
    completely specified by transformations. */
    static void _adjustCoordsRecursive(SPItem *item, Geom::Affine const &m, double ex, bool is_root = true);
    static void _adjustFontsizeRecursive(SPItem *item, double ex, bool is_root = true);
	
    /** discards the drawing objects representing this text. */
    void _clearFlow(Inkscape::DrawingGroup *in_arena);

    bool _optimizeTextpathText;

private:

    /** Initializes layout from <text> (i.e. this node). */
    void _buildLayoutInit();

    /** Recursively walks the xml tree adding tags and their contents. The
    non-trivial code does two things: firstly, it manages the positioning
    attributes and their inheritance rules, and secondly it keeps track of line
    breaks and makes sure both that they are assigned the correct SPObject and
    that we don't get a spurious extra one at the end of the flow. */
    unsigned _buildLayoutInput(SPObject *object, Inkscape::Text::Layout::OptionalTextTagAttrs const &parent_optional_attrs, unsigned parent_attrs_offset, bool in_textpath);

    /** Union all exlusion shapes. */
    Shape* _buildExclusionShape() const;

public:
    /** Optimize textpath text on next set_transform. */
    void optimizeTextpathText() {_optimizeTextpathText = true;}

    void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
    void release() override;
    void child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref) override;
    void remove_child(Inkscape::XML::Node* child) override;
    void set(SPAttributeEnum key, const char* value) override;
    void update(SPCtx* ctx, unsigned int flags) override;
    void modified(unsigned int flags) override;
    Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, unsigned int flags) override;

    Geom::OptRect bbox(Geom::Affine const &transform, SPItem::BBoxType type) const override;
    void print(SPPrintContext *ctx) override;
    const char* displayName() const override;
    char* description() const override;
    Inkscape::DrawingItem* show(Inkscape::Drawing &drawing, unsigned int key, unsigned int flags) override;
    void hide(unsigned int key) override;
    void snappoints(std::vector<Inkscape::SnapCandidatePoint> &p, Inkscape::SnapPreferences const *snapprefs) const override;
    Geom::Affine set_transform(Geom::Affine const &transform) override;
};

#endif

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

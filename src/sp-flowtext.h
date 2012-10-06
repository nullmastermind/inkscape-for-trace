#ifndef SEEN_SP_ITEM_FLOWTEXT_H
#define SEEN_SP_ITEM_FLOWTEXT_H

/*
 */

#include "sp-item.h"

#include <2geom/forward.h>
#include "libnrtype/Layout-TNG.h"

#define SP_TYPE_FLOWTEXT            (sp_flowtext_get_type ())
#define SP_FLOWTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_FLOWTEXT, SPFlowtext))
#define SP_FLOWTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_FLOWTEXT, SPFlowtextClass))
#define SP_IS_FLOWTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_FLOWTEXT))
#define SP_IS_FLOWTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_FLOWTEXT))


namespace Inkscape {

class DrawingGroup;

} // namespace Inkscape

class CFlowtext;

class SPFlowtext : public SPItem {
public:
	CFlowtext* cflowtext;

    /** Completely recalculates the layout. */
    void rebuildLayout();

    /** Converts the flowroot in into a \<text\> tree, keeping all the formatting and positioning,
    but losing the automatic wrapping ability. */
    Inkscape::XML::Node *getAsText();

    SPItem *get_frame(SPItem *after);

    bool has_internal_frame();

//semiprivate:  (need to be accessed by the C-style functions still)
    Inkscape::Text::Layout layout;

    /** discards the drawing objects representing this text. */
    void _clearFlow(Inkscape::DrawingGroup* in_arena);

    double par_indent;

private:
    /** Recursively walks the xml tree adding tags and their contents. */
    void _buildLayoutInput(SPObject *root, Shape const *exclusion_shape, std::list<Shape> *shapes, SPObject **pending_line_break_object);

    /** calculates the union of all the \<flowregionexclude\> children
    of this flowroot. */
    Shape* _buildExclusionShape() const;

};

struct SPFlowtextClass {
    SPItemClass parent_class;
};

class CFlowtext : public CItem {
public:
	CFlowtext(SPFlowtext* flowtext);
	virtual ~CFlowtext();

	virtual void onBuild(SPDocument* doc, Inkscape::XML::Node* repr);

	virtual void onChildAdded(Inkscape::XML::Node* child, Inkscape::XML::Node* ref);
	virtual void onRemoveChild(Inkscape::XML::Node* child);

	virtual void onSet(unsigned int key, const gchar* value);

	virtual void onUpdate(SPCtx* ctx, unsigned int flags);
	virtual void onModified(unsigned int flags);

	virtual Inkscape::XML::Node* onWrite(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

	virtual Geom::OptRect onBbox(Geom::Affine const &transform, SPItem::BBoxType type);
	virtual void onPrint(SPPrintContext *ctx);
	virtual gchar* onDescription();
	virtual Inkscape::DrawingItem* onShow(Inkscape::Drawing &drawing, unsigned int key, unsigned int flags);
	virtual void onHide(unsigned int key);
    virtual void onSnappoints(std::vector<Inkscape::SnapCandidatePoint> &p, Inkscape::SnapPreferences const *snapprefs);
protected:
	SPFlowtext* spflowtext;
};


GType sp_flowtext_get_type (void);

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

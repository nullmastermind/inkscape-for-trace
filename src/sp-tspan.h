#ifndef INKSCAPE_SP_TSPAN_H
#define INKSCAPE_SP_TSPAN_H

/*
 * tspan and textpath, based on the flowtext routines
 */

#include <glib.h>
#include "sp-item.h"
#include "text-tag-attributes.h"


#define SP_TYPE_TSPAN (sp_tspan_get_type())
#define SP_TSPAN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SP_TYPE_TSPAN, SPTSpan))
#define SP_TSPAN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), SP_TYPE_TSPAN, SPTSpanClass))
#define SP_IS_TSPAN(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), SP_TYPE_TSPAN))
#define SP_IS_TSPAN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), SP_TYPE_TSPAN))


enum {
    SP_TSPAN_ROLE_UNSPECIFIED,
    SP_TSPAN_ROLE_PARAGRAPH,
    SP_TSPAN_ROLE_LINE
};

class CTSpan;

class SPTSpan : public SPItem {
public:
	CTSpan* ctspan;

    guint role : 2;
    TextTagAttributes attributes;
};

struct SPTSpanClass {
    SPItemClass parent_class;
};


class CTSpan : public CItem {
public:
	CTSpan(SPTSpan* span);
	virtual ~CTSpan();

	virtual void onBuild(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void onRelease();
	virtual void onSet(unsigned int key, const gchar* value);
	virtual void onUpdate(SPCtx* ctx, unsigned int flags);
	virtual void onModified(unsigned int flags);
	virtual Inkscape::XML::Node* onWrite(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

	virtual Geom::OptRect onBbox(Geom::Affine const &transform, SPItem::BBoxType type);
	virtual gchar* onDescription();

protected:
	SPTSpan* sptspan;
};


GType sp_tspan_get_type();


#endif /* !INKSCAPE_SP_TSPAN_H */

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

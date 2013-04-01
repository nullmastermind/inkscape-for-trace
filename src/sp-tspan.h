#ifndef INKSCAPE_SP_TSPAN_H
#define INKSCAPE_SP_TSPAN_H

/*
 * tspan and textpath, based on the flowtext routines
 */

#include <glib.h>
#include "sp-item.h"
#include "text-tag-attributes.h"

G_BEGIN_DECLS

#define SP_TYPE_TSPAN (sp_tspan_get_type())
#define SP_TSPAN(obj) ((SPTSpan*)obj)
#define SP_IS_TSPAN(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPTSpan)))

enum {
    SP_TSPAN_ROLE_UNSPECIFIED,
    SP_TSPAN_ROLE_PARAGRAPH,
    SP_TSPAN_ROLE_LINE
};

class CTSpan;

class SPTSpan : public SPItem {
public:
	SPTSpan();
	CTSpan* ctspan;

    guint role : 2;
    TextTagAttributes attributes;
};

struct SPTSpanClass {
    SPItemClass parent_class;
};

GType sp_tspan_get_type() G_GNUC_CONST;

class CTSpan : public CItem {
public:
	CTSpan(SPTSpan* span);
	virtual ~CTSpan();

	virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void release();
	virtual void set(unsigned int key, const gchar* value);
	virtual void update(SPCtx* ctx, unsigned int flags);
	virtual void modified(unsigned int flags);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

	virtual Geom::OptRect bbox(Geom::Affine const &transform, SPItem::BBoxType type);
	virtual gchar* description();

protected:
	SPTSpan* sptspan;
};

G_END_DECLS

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

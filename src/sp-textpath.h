#ifndef INKSCAPE_SP_TEXTPATH_H
#define INKSCAPE_SP_TEXTPATH_H

#include <glib.h>
#include "svg/svg-length.h"
#include "sp-item.h"
#include "sp-text.h"
class SPUsePath;
class Path;


#define SP_TYPE_TEXTPATH (sp_textpath_get_type())
#define SP_TEXTPATH(obj) ((SPTextPath*)obj)
#define SP_IS_TEXTPATH(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPTextPath)))

class CTextPath;

class SPTextPath : public SPItem {
public:
	CTextPath* ctextpath;

    TextTagAttributes attributes;
    SVGLength startOffset;

    Path *originalPath;
    bool isUpdating;
    SPUsePath *sourcePath;
};

struct SPTextPathClass {
    SPItemClass parent_class;
};


class CTextPath : public CItem {
public:
	CTextPath(SPTextPath* textpath);
	virtual ~CTextPath();

	virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void release();
	virtual void set(unsigned int key, const gchar* value);
	virtual void update(SPCtx* ctx, unsigned int flags);
	virtual void modified(unsigned int flags);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

protected:
	SPTextPath* sptextpath;
};


GType sp_textpath_get_type();

#define SP_IS_TEXT_TEXTPATH(obj) (SP_IS_TEXT(obj) && obj->firstChild() && SP_IS_TEXTPATH(obj->firstChild()))

SPItem *sp_textpath_get_path_item(SPTextPath *tp);
void sp_textpath_to_text(SPObject *tp);


#endif /* !INKSCAPE_SP_TEXTPATH_H */

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

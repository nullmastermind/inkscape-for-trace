#ifndef INKSCAPE_SP_STYLE_ELEM_H
#define INKSCAPE_SP_STYLE_ELEM_H

#include "sp-object.h"
#include "media.h"

#define SP_TYPE_STYLE_ELEM (sp_style_elem_get_type())
#define SP_STYLE_ELEM(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SP_TYPE_STYLE_ELEM, SPStyleElem))
#define SP_STYLE_ELEM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), SP_TYPE_STYLE_ELEM, SPStyleElemClass))
#define SP_IS_STYLE_ELEM(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), SP_TYPE_STYLE_ELEM))
#define SP_IS_STYLE_ELEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), SP_TYPE_STYLE_ELEM))

class CStyleElem;

class SPStyleElem : public SPObject {
public:
	CStyleElem* cstyleelem;

    Media media;
    bool is_css;
};

class SPStyleElemClass : public SPObjectClass {
};


class CStyleElem : public CObject {
public:
	CStyleElem(SPStyleElem* se);
	virtual ~CStyleElem();

	virtual void onBuild(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void onSet(unsigned int key, gchar const* value);
	virtual void onReadContent();
	virtual Inkscape::XML::Node* onWrite(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

protected:
	SPStyleElem* spstyleelem;
};


GType sp_style_elem_get_type();


#endif /* !INKSCAPE_SP_STYLE_ELEM_H */

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

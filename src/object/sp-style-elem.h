#ifndef INKSCAPE_SP_STYLE_ELEM_H
#define INKSCAPE_SP_STYLE_ELEM_H

#include "sp-object.h"
#include "media.h"

class SPStyleElem : public SPObject {
public:
	SPStyleElem();
	~SPStyleElem() override;

    Media media;
    bool is_css;

	void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
	void set(SPAttributeEnum key, char const* value) override;
	void read_content() override;
	Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, unsigned int flags) override;
};


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

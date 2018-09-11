// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_LINE_H 
#define SEEN_SP_LINE_H

/*
 * SVG <line> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "svg/svg-length.h"
#include "sp-shape.h"

#define SP_LINE(obj) (dynamic_cast<SPLine*>((SPObject*)obj))
#define SP_IS_LINE(obj) (dynamic_cast<const SPLine*>((SPObject*)obj) != NULL)

class SPLine : public SPShape {
public:
	SPLine();
	~SPLine() override;

    SVGLength x1;
    SVGLength y1;
    SVGLength x2;
    SVGLength y2;

	void build(SPDocument *document, Inkscape::XML::Node *repr) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
	void set(SPAttributeEnum key, char const* value) override;

	const char* displayName() const override;
	Geom::Affine set_transform(Geom::Affine const &transform) override;
	void convert_to_guides() const override;
	void update(SPCtx* ctx, unsigned int flags) override;

	void set_shape() override;
};

#endif // SEEN_SP_LINE_H
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

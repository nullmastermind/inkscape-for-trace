// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_BOX3D_SIDE_H
#define SEEN_BOX3D_SIDE_H

/*
 * 3D box face implementation
 *
 * Authors:
 *   Maximilian Albert <Anhalter42@gmx.de>
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2007  Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-polygon.h"
#include "axis-manip.h"


class SPBox3D;
class Persp3D;

// FIXME: Would it be better to inherit from SPPath instead?
class Box3DSide : public SPPolygon {
public:
	Box3DSide();
	~Box3DSide() override;

    Box3D::Axis dir1;
    Box3D::Axis dir2;
    Box3D::FrontOrRear front_or_rear;
    int getFaceId();
    static Box3DSide * createBox3DSide(SPBox3D *box);

	void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
	void set(SPAttr key, char const* value) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
	void update(SPCtx *ctx, unsigned int flags) override;

	void set_shape() override;

    void position_set(); // FIXME: Replace this by Box3DSide::set_shape??

    Glib::ustring axes_string() const;

    Persp3D *perspective() const;


    Inkscape::XML::Node *convert_to_path() const;
};

#endif // SEEN_BOX3D_SIDE_H

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

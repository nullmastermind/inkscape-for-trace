// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_POLYLINE_H
#define SEEN_SP_POLYLINE_H

#include "sp-shape.h"

#define SP_POLYLINE(obj) (dynamic_cast<SPPolyLine*>((SPObject*)obj))
#define SP_IS_POLYLINE(obj) (dynamic_cast<const SPPolyLine*>((SPObject*)obj) != NULL)

class SPPolyLine : public SPShape {
public:
	SPPolyLine();
	~SPPolyLine() override;

	void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
	void set(SPAttributeEnum key, char const* value) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;

	char* description() const override;
};

#endif // SEEN_SP_POLYLINE_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

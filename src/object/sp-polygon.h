// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_POLYGON_H
#define SEEN_SP_POLYGON_H

/*
 * SVG <polygon> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-shape.h"

#define SP_POLYGON(obj) (dynamic_cast<SPPolygon*>((SPObject*)obj))
#define SP_IS_POLYGON(obj) (dynamic_cast<const SPPolygon*>((SPObject*)obj) != NULL)

class SPPolygon : public SPShape {
public:
	SPPolygon();
	~SPPolygon() override;

	void build(SPDocument *document, Inkscape::XML::Node *repr) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
	void set(SPAttributeEnum key, char const* value) override;
	char* description() const override;
};

// made 'public' so that SPCurve can set it as friend:
void sp_polygon_set(SPObject *object, unsigned int key, char const*value);

#endif

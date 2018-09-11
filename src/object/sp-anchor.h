// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_ANCHOR_H
#define SEEN_SP_ANCHOR_H

/*
 * SVG <a> element implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-item-group.h"

#define SP_ANCHOR(obj) (dynamic_cast<SPAnchor*>((SPObject*)obj))
#define SP_IS_ANCHOR(obj) (dynamic_cast<const SPAnchor*>((SPObject*)obj) != NULL)

class SPAnchor : public SPGroup {
public:
	SPAnchor();
	~SPAnchor() override;

	char *href;
	char *type;
	char *title;
        SPDocument *page;

	void build(SPDocument *document, Inkscape::XML::Node *repr) override;
	void release() override;
	void set(SPAttributeEnum key, char const* value) override;
        virtual void updatePageAnchor();
	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;

    const char* displayName() const override;
	char* description() const override;
	int event(SPEvent *event) override;
};

#endif

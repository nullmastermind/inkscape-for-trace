// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_TITLE_H
#define SEEN_SP_TITLE_H

/*
 * SVG <title> implementation
 *
 * Authors:
 *   Jeff Schiller <codedread@gmail.com>
 *
 * Copyright (C) 2008 Jeff Schiller
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-object.h"

#define SP_TITLE(obj) (dynamic_cast<SPTitle*>((SPObject*)obj))
#define SP_IS_TITLE(obj) (dynamic_cast<const SPTitle*>((SPObject*)obj) != NULL)

class SPTitle : public SPObject {
public:
	SPTitle();
	~SPTitle() override;

	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
};

#endif

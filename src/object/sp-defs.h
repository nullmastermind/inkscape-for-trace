// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_DEFS_H
#define SEEN_SP_DEFS_H

/*
 * SVG <defs> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2000-2002 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-object.h"

#define SP_DEFS(obj) (dynamic_cast<SPDefs*>((SPObject*)obj))
#define SP_IS_DEFS(obj) (dynamic_cast<const SPDefs*>((SPObject*)obj) != NULL)

class SPDefs : public SPObject {
public:
	SPDefs();
	~SPDefs() override;

protected:
	void release() override;
	void update(SPCtx* ctx, unsigned int flags) override;
	void modified(unsigned int flags) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
};

#endif // !SEEN_SP_DEFS_H

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

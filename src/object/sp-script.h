// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_SCRIPT_H
#define SEEN_SP_SCRIPT_H

/*
 * SVG <script> implementation
 *
 * Author:
 *   Felipe C. da S. Sanches <juca@members.fsf.org>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2008 Author
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-object.h"
#include "document.h"

/* SPScript */
class SPScript : public SPObject {
public:
	SPScript();
	~SPScript() override;

	char *xlinkhref;

protected:
	void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
	void release() override;
	void set(SPAttributeEnum key, char const* value) override;
	void update(SPCtx* ctx, unsigned int flags) override;
	void modified(unsigned int flags) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, unsigned int flags) override;
};

#endif

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

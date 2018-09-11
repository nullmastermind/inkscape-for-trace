// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_METADATA_H
#define SEEN_SP_METADATA_H

/*
 * SVG <metadata> implementation
 *
 * Authors:
 *   Kees Cook <kees@outflux.net>
 *
 * Copyright (C) 2004 Kees Cook
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-object.h"

/* Metadata base class */

#define SP_METADATA(obj) (dynamic_cast<SPMetadata*>((SPObject*)obj))
#define SP_IS_METADATA(obj) (dynamic_cast<const SPMetadata*>((SPObject*)obj) != NULL)

class SPMetadata : public SPObject {
public:
	SPMetadata();
	~SPMetadata() override;

protected:
	void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
	void release() override;

	void set(SPAttributeEnum key, const char* value) override;
	void update(SPCtx* ctx, unsigned int flags) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, unsigned int flags) override;
};

SPMetadata * sp_document_metadata (SPDocument *document);

#endif
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

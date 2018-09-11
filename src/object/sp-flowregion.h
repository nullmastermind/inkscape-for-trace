// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_ITEM_FLOWREGION_H
#define SEEN_SP_ITEM_FLOWREGION_H

/*
 */

#include "sp-item.h"

#define SP_FLOWREGION(obj) (dynamic_cast<SPFlowregion*>((SPObject*)obj))
#define SP_IS_FLOWREGION(obj) (dynamic_cast<const SPFlowregion*>((SPObject*)obj) != NULL)

#define SP_FLOWREGIONEXCLUDE(obj) (dynamic_cast<SPFlowregionExclude*>((SPObject*)obj))
#define SP_IS_FLOWREGIONEXCLUDE(obj) (dynamic_cast<const SPFlowregionExclude*>((SPObject*)obj) != NULL)

class Path;
class Shape;
class flow_dest;
class FloatLigne;

class SPFlowregion : public SPItem {
public:
	SPFlowregion();
	~SPFlowregion() override;

	std::vector<Shape*>     computed;
	
	void             UpdateComputed();

	void child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref) override;
	void remove_child(Inkscape::XML::Node *child) override;
	void update(SPCtx *ctx, unsigned int flags) override;
	void modified(guint flags) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
	const char* displayName() const override;
};

class SPFlowregionExclude : public SPItem {
public:
	SPFlowregionExclude();
	~SPFlowregionExclude() override;

	Shape            *computed;
	
	void             UpdateComputed();

	void child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref) override;
	void remove_child(Inkscape::XML::Node *child) override;
	void update(SPCtx *ctx, unsigned int flags) override;
	void modified(guint flags) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
	const char* displayName() const override;
};

#endif

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_ITEM_FLOWDIV_H
#define SEEN_SP_ITEM_FLOWDIV_H

/*
 */

#include "sp-object.h"
#include "sp-item.h"

#define SP_FLOWDIV(obj) (dynamic_cast<SPFlowdiv*>((SPObject*)obj))
#define SP_IS_FLOWDIV(obj) (dynamic_cast<const SPFlowdiv*>((SPObject*)obj) != NULL)

#define SP_FLOWTSPAN(obj) (dynamic_cast<SPFlowtspan*>((SPObject*)obj))
#define SP_IS_FLOWTSPAN(obj) (dynamic_cast<const SPFlowtspan*>((SPObject*)obj) != NULL)

#define SP_FLOWPARA(obj) (dynamic_cast<SPFlowpara*>((SPObject*)obj))
#define SP_IS_FLOWPARA(obj) (dynamic_cast<const SPFlowpara*>((SPObject*)obj) != NULL)

#define SP_FLOWLINE(obj) (dynamic_cast<SPFlowline*>((SPObject*)obj))
#define SP_IS_FLOWLINE(obj) (dynamic_cast<const SPFlowline*>((SPObject*)obj) != NULL)

#define SP_FLOWREGIONBREAK(obj) (dynamic_cast<SPFlowregionbreak*>((SPObject*)obj))
#define SP_IS_FLOWREGIONBREAK(obj) (dynamic_cast<const SPFlowregionbreak*>((SPObject*)obj) != NULL)

// these 3 are derivatives of SPItem to get the automatic style handling
class SPFlowdiv : public SPItem {
public:
	SPFlowdiv();
	~SPFlowdiv() override;

protected:
	void build(SPDocument *document, Inkscape::XML::Node *repr) override;
	void release() override;
	void update(SPCtx* ctx, unsigned int flags) override;
	void modified(unsigned int flags) override;

	void set(SPAttributeEnum key, char const* value) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
};

class SPFlowtspan : public SPItem {
public:
	SPFlowtspan();
	~SPFlowtspan() override;

protected:
	void build(SPDocument *document, Inkscape::XML::Node *repr) override;
	void release() override;
	void update(SPCtx* ctx, unsigned int flags) override;
	void modified(unsigned int flags) override;

	void set(SPAttributeEnum key, char const* value) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
};

class SPFlowpara : public SPItem {
public:
	SPFlowpara();
	~SPFlowpara() override;

protected:
	void build(SPDocument *document, Inkscape::XML::Node *repr) override;
	void release() override;
	void update(SPCtx* ctx, unsigned int flags) override;
	void modified(unsigned int flags) override;

	void set(SPAttributeEnum key, char const* value) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
};

// these do not need any style
class SPFlowline : public SPObject {
public:
	SPFlowline();
	~SPFlowline() override;

protected:
	void release() override;
	void modified(unsigned int flags) override;

	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
};

class SPFlowregionbreak : public SPObject {
public:
	SPFlowregionbreak();
	~SPFlowregionbreak() override;

protected:
	void release() override;
	void modified(unsigned int flags) override;

	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
};

#endif

#ifndef __SP_SYMBOL_H__
#define __SP_SYMBOL_H__

/*
 * SVG <symbol> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2003 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

/*
 * This is quite similar in logic to <svg>
 * Maybe we should merge them somehow (Lauris)
 */

#define SP_TYPE_SYMBOL (sp_symbol_get_type ())
#define SP_SYMBOL(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_SYMBOL, SPSymbol))
#define SP_IS_SYMBOL(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_SYMBOL))

class SPSymbol;
class SPSymbolClass;
class CSymbol;

#include <2geom/affine.h>
#include "svg/svg-length.h"
#include "enums.h"
#include "sp-item-group.h"

class SPSymbol : public SPGroup {
public:
	CSymbol* csymbol;

	/* viewBox; */
	unsigned int viewBox_set : 1;
	Geom::Rect viewBox;

	/* preserveAspectRatio */
	unsigned int aspect_set : 1;
	unsigned int aspect_align : 4;
	unsigned int aspect_clip : 1;

	/* Child to parent additional transform */
	Geom::Affine c2p;
};

struct SPSymbolClass {
	SPGroupClass parent_class;
};


class CSymbol : public CGroup {
public:
	CSymbol(SPSymbol* symbol);
	virtual ~CSymbol();

	virtual void onBuild(SPDocument *document, Inkscape::XML::Node *repr);
	virtual void onRelease();
	virtual void onSet(unsigned int key, gchar const* value);
	virtual void onUpdate(SPCtx *ctx, guint flags);
	virtual Inkscape::XML::Node* onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);

	virtual void onModified(unsigned int flags);
	virtual void onChildAdded(Inkscape::XML::Node* child, Inkscape::XML::Node* ref);

	virtual Inkscape::DrawingItem* onShow(Inkscape::Drawing &drawing, unsigned int key, unsigned int flags);
	virtual void onPrint(SPPrintContext *ctx);
	virtual Geom::OptRect onBbox(Geom::Affine const &transform, SPItem::BBoxType type);
	virtual void onHide (unsigned int key);

protected:
	SPSymbol* spsymbol;
};


GType sp_symbol_get_type (void);

#endif

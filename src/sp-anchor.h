#ifndef __SP_ANCHOR_H__
#define __SP_ANCHOR_H__

/*
 * SVG <a> element implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-item-group.h"

#define SP_TYPE_ANCHOR (sp_anchor_get_type ())
#define SP_ANCHOR(obj) ((SPAnchor*)obj)
#define SP_IS_ANCHOR(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPAnchor)))

class CAnchor;

class SPAnchor : public SPGroup {
public:
	SPAnchor();
	CAnchor* canchor;

	gchar *href;
};

struct SPAnchorClass {
	SPGroupClass parent_class;
};


class CAnchor : public CGroup {
public:
	CAnchor(SPAnchor* anchor);
	virtual ~CAnchor();

	virtual void build(SPDocument *document, Inkscape::XML::Node *repr);
	virtual void release();
	virtual void set(unsigned int key, gchar const* value);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);

	virtual gchar* description();
	virtual gint event(SPEvent *event);

protected:
	SPAnchor* spanchor;
};


GType sp_anchor_get_type (void);

#endif

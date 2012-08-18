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
#define SP_ANCHOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_ANCHOR, SPAnchor))
#define SP_ANCHOR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_ANCHOR, SPAnchorClass))
#define SP_IS_ANCHOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_ANCHOR))
#define SP_IS_ANCHOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_ANCHOR))

class CAnchor;

class SPAnchor : public SPGroup {
public:
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

	virtual void onBuild(SPDocument *document, Inkscape::XML::Node *repr);
	virtual void onRelease();
	virtual void onSet(unsigned int key, gchar const* value);
	virtual Inkscape::XML::Node* onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);

	virtual gchar* onDescription();
	virtual gint onEvent(SPEvent *event);

protected:
	SPAnchor* spanchor;
};


GType sp_anchor_get_type (void);

#endif

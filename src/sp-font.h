#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef SP_FONT_H_SEEN
#define SP_FONT_H_SEEN

/*
 * SVG <font> element implementation
 *
 * Authors:
 *    Felipe C. da S. Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2008 Felipe C. da S. Sanches
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-object.h"

#define SP_TYPE_FONT (sp_font_get_type ())
#define SP_FONT(obj) ((SPFont*)obj)
#define SP_IS_FONT(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPFont)))

class CFont;

class SPFont : public SPObject {
public:
	SPFont();
	CFont* cfont;

    double horiz_origin_x;
    double horiz_origin_y;
    double horiz_adv_x;
    double vert_origin_x;
    double vert_origin_y;
    double vert_adv_y;
};

class CFont : public CObject {
public:
	CFont(SPFont* font);
	virtual ~CFont();

	virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void release();

	virtual void child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref);
	virtual void remove_child(Inkscape::XML::Node* child);

	virtual void set(unsigned int key, const gchar* value);

	virtual void update(SPCtx* ctx, unsigned int flags);

	virtual Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

private:
	SPFont* spfont;
};

struct SPFontClass {
    SPObjectClass parent_class;
};

GType sp_font_get_type (void);

#endif //#ifndef SP_FONT_H_SEEN

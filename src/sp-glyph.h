#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef __SP_GLYPH_H__
#define __SP_GLYPH_H__

/*
 * SVG <glyph> element implementation
 *
 * Authors:
 *    Felipe C. da S. Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2008 Felipe C. da S. Sanches
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-object.h"

#define SP_TYPE_GLYPH (sp_glyph_get_type ())
#define SP_GLYPH(obj) ((SPGlyph*)obj)
#define SP_IS_GLYPH(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPGlyph)))

enum glyphArabicForm {
    GLYPH_ARABIC_FORM_INITIAL,
    GLYPH_ARABIC_FORM_MEDIAL,
    GLYPH_ARABIC_FORM_TERMINAL,
    GLYPH_ARABIC_FORM_ISOLATED,
};

enum glyphOrientation {
    GLYPH_ORIENTATION_HORIZONTAL,
    GLYPH_ORIENTATION_VERTICAL,
    GLYPH_ORIENTATION_BOTH
};

class CGlyph;

class SPGlyph : public SPObject {
public:
	CGlyph* cglyph;

    Glib::ustring unicode;
    Glib::ustring glyph_name;
    char* d;
    glyphOrientation orientation;
    glyphArabicForm arabic_form;
    char* lang;
    double horiz_adv_x;
    double vert_origin_x;
    double vert_origin_y;
    double vert_adv_y;
};

class CGlyph : public CObject {
public:
	CGlyph(SPGlyph* glyph);
	virtual ~CGlyph();

	virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void release();

	virtual void set(unsigned int key, const gchar* value);

	virtual void update(SPCtx* ctx, unsigned int flags);

	virtual Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

private:
	SPGlyph* spglyph;
};

struct SPGlyphClass {
	SPObjectClass parent_class;
};

GType sp_glyph_get_type (void);

#endif //#ifndef __SP_GLYPH_H__

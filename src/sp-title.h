#ifndef __SP_TITLE_H__
#define __SP_TITLE_H__

/*
 * SVG <title> implementation
 *
 * Authors:
 *   Jeff Schiller <codedread@gmail.com>
 *
 * Copyright (C) 2008 Jeff Schiller
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-object.h"

#define SP_TYPE_TITLE           (sp_title_get_type ())
#define SP_TITLE(obj) ((SPTitle*)obj)
#define SP_IS_TITLE(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPTitle)))

class CTitle;

class SPTitle : public SPObject {
public:
	CTitle* ctitle;
};

struct SPTitleClass {
	SPObjectClass parent_class;
};


class CTitle : public CObject {
public:
	CTitle(SPTitle* title);
	virtual ~CTitle();

	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);

protected:
	SPTitle* sptitle;
};


GType sp_title_get_type (void);

#endif

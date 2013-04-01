#ifndef SEEN_SP_DEFS_H
#define SEEN_SP_DEFS_H

/*
 * SVG <defs> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2000-2002 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-object.h"

#define SP_TYPE_DEFS            (sp_defs_get_type())
#define SP_DEFS(obj) ((SPDefs*)obj)
#define SP_IS_DEFS(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPDefs)))

GType sp_defs_get_type(void) G_GNUC_CONST;

class CDefs;

class SPDefs : public SPObject {
public:
	CDefs* cdefs;

private:
    friend class SPDefsClass;	
};

class SPDefsClass {
public:
    SPObjectClass parent_class;

private:
    friend class SPDefs;	
};


class CDefs : public CObject {
public:
	CDefs(SPDefs* defs);
	virtual ~CDefs();

	virtual void release();
	virtual void update(SPCtx* ctx, unsigned int flags);
	virtual void modified(unsigned int flags);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);

protected:
	SPDefs* spdefs;
};


#endif // !SEEN_SP_DEFS_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

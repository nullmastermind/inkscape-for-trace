#ifndef __SP_SCRIPT_H__
#define __SP_SCRIPT_H__

/*
 * SVG <script> implementation
 *
 * Author:
 *   Felipe C. da S. Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2008 Author
 *
 * Released under GNU GPL version 2 or later, read the file 'COPYING' for more information
 */

#include "sp-item.h"

#define SP_TYPE_SCRIPT (sp_script_get_type())
#define SP_SCRIPT(obj) ((SPScript*)obj)
#define SP_IS_SCRIPT(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPScript)))

/* SPScript */

class CScript;

class SPScript : public SPObject {
public:
	SPScript();
	CScript* cscript;

	gchar *xlinkhref;
};

struct SPScriptClass {
    SPObjectClass parent_class;
};


class CScript : public CObject {
public:
	CScript(SPScript* script);
	virtual ~CScript();

	virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void release();
	virtual void set(unsigned int key, const gchar* value);
	virtual void update(SPCtx* ctx, unsigned int flags);
	virtual void modified(unsigned int flags);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

protected:
	SPScript* spscript;
};


GType sp_script_get_type();

#endif

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

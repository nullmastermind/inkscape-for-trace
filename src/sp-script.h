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
#define SP_SCRIPT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SP_TYPE_SCRIPT, SPScript))
#define SP_SCRIPT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), SP_TYPE_SCRIPT, SPScriptClass))
#define SP_IS_SCRIPT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), SP_TYPE_SCRIPT))
#define SP_IS_SCRIPT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), SP_TYPE_SCRIPT))

/* SPScript */

class CScript;

class SPScript : public SPObject {
public:
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

	virtual void onBuild(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void onRelease();
	virtual void onSet(unsigned int key, const gchar* value);
	virtual void onUpdate(SPCtx* ctx, unsigned int flags);
	virtual void onModified(unsigned int flags);
	virtual Inkscape::XML::Node* onWrite(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

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

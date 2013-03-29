#ifndef __SP_STRING_H__
#define __SP_STRING_H__

/*
 * string elements
 * extracted from sp-text
 */

#include <glibmm/ustring.h>

#include "sp-object.h"

#define SP_TYPE_STRING (sp_string_get_type ())
#define SP_STRING(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_STRING, SPString))
#define SP_STRING_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_STRING, SPStringClass))
#define SP_IS_STRING(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_STRING))
#define SP_IS_STRING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_STRING))

class CString;

class SPString : public SPObject {
public:
	CString* cstring;

    Glib::ustring  string;
};

struct SPStringClass {
	SPObjectClass parent_class;
};


class CString : public CObject {
public:
	CString(SPString* str);
	virtual ~CString();

	virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void release();

	virtual void read_content();

	virtual void update(SPCtx* ctx, unsigned int flags);

protected:
	SPString* spstring;
};


GType sp_string_get_type ();

#endif

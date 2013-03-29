#ifndef SEEN_SP_OBJECTGROUP_H
#define SEEN_SP_OBJECTGROUP_H

/*
 * Abstract base class for non-item groups
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2003 Authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-object.h"

#define SP_TYPE_OBJECTGROUP            (sp_objectgroup_get_type ())
#define SP_OBJECTGROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_OBJECTGROUP, SPObjectGroup))
#define SP_OBJECTGROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_OBJECTGROUP, SPObjectGroupClass))
#define SP_IS_OBJECTGROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_OBJECTGROUP))
#define SP_IS_OBJECTGROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_OBJECTGROUP))

GType sp_objectgroup_get_type() G_GNUC_CONST;
class CObjectGroup;

class SPObjectGroup : public SPObject {
public:
	CObjectGroup* cobjectgroup;

private:
    friend class SPObjectGroupClass;
};

class SPObjectGroupClass {
public:
    SPObjectClass parent_class;

private:
    friend class SPObjectGroup;	
};


class CObjectGroup : public CObject {
public:
	CObjectGroup(SPObjectGroup* gr);
	virtual ~CObjectGroup();

	virtual void onChildAdded(Inkscape::XML::Node* child, Inkscape::XML::Node* ref);
	virtual void onRemoveChild(Inkscape::XML::Node* child);

	virtual void onOrderChanged(Inkscape::XML::Node* child, Inkscape::XML::Node* old, Inkscape::XML::Node* new_repr);

	virtual Inkscape::XML::Node* onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);

protected:
	SPObjectGroup* spobjectgroup;
};


#endif // SEEN_SP_OBJECTGROUP_H
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

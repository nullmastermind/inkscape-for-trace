#ifndef __SP_SWITCH_H__
#define __SP_SWITCH_H__

/*
 * SVG <switch> implementation
 *
 * Authors:
 *   Andrius R. <knutux@gmail.com>
 *
 * Copyright (C) 2006 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-item-group.h"

#include <stddef.h>
#include <sigc++/connection.h>

G_BEGIN_DECLS

#define SP_TYPE_SWITCH            (sp_switch_get_type())
#define SP_SWITCH(obj) ((SPSwitch*)obj)
#define SP_IS_SWITCH(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPSwitch)))

GType sp_switch_get_type() G_GNUC_CONST;
class CSwitch;

class SPSwitch : public SPGroup {
public:
	SPSwitch();
	CSwitch* cswitch;

	static GType getType();

    void resetChildEvaluated() { _reevaluate(); }

    GSList *_childList(bool add_ref, SPObject::Action action);
    void _showChildren (Inkscape::Drawing &drawing, Inkscape::DrawingItem *ai, unsigned int key, unsigned int flags);

    SPObject *_evaluateFirst();
    void _reevaluate(bool add_to_arena = false);
    static void _releaseItem(SPObject *obj, SPSwitch *selection);
    void _releaseLastItem(SPObject *obj);

    SPObject *_cached_item;
    sigc::connection _release_connection;
};

struct SPSwitchClass : public SPGroupClass {
};


/*
 * Virtual methods of SPSwitch
 */
class CSwitch : public CGroup {
public:
    CSwitch(SPSwitch *sw);
    virtual ~CSwitch();

    friend class SPSwitch;

    virtual void child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref);
    virtual void remove_child(Inkscape::XML::Node *child);
    virtual void order_changed(Inkscape::XML::Node *child, Inkscape::XML::Node *old_ref, Inkscape::XML::Node *new_ref);
    virtual gchar *description();

protected:
    SPSwitch* spswitch;
};


G_END_DECLS

#endif

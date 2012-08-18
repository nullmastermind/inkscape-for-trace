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

#define SP_TYPE_SWITCH            (SPSwitch::getType())
#define SP_SWITCH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_SWITCH, SPSwitch))
#define SP_SWITCH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_SWITCH, SPSwitchClass))
#define SP_IS_SWITCH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_SWITCH))
#define SP_IS_SWITCH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_SWITCH))

class CSwitch;

class SPSwitch : public SPGroup {
public:
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

    virtual void onChildAdded(Inkscape::XML::Node* child, Inkscape::XML::Node* ref);
    virtual void onRemoveChild(Inkscape::XML::Node *child);
    virtual void onOrderChanged(Inkscape::XML::Node *child, Inkscape::XML::Node *old_ref, Inkscape::XML::Node *new_ref);
    virtual gchar *onDescription();

protected:
    SPSwitch* spswitch;
};


#endif

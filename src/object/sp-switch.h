// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_SWITCH_H
#define SEEN_SP_SWITCH_H

/*
 * SVG <switch> implementation
 *
 * Authors:
 *   Andrius R. <knutux@gmail.com>
 *
 * Copyright (C) 2006 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstddef>
#include <sigc++/connection.h>

#include "sp-item-group.h"


#define SP_SWITCH(obj) (dynamic_cast<SPSwitch*>((SPObject*)obj))
#define SP_IS_SWITCH(obj) (dynamic_cast<const SPSwitch*>((SPObject*)obj) != NULL)

class SPSwitch : public SPGroup {
public:
	SPSwitch();
	~SPSwitch() override;

    void resetChildEvaluated() { _reevaluate(); }

    std::vector<SPObject*> _childList(bool add_ref, SPObject::Action action);
    void _showChildren (Inkscape::Drawing &drawing, Inkscape::DrawingItem *ai, unsigned int key, unsigned int flags) override;

    SPObject *_evaluateFirst();
    void _reevaluate(bool add_to_arena = false);
    static void _releaseItem(SPObject *obj, SPSwitch *selection);
    void _releaseLastItem(SPObject *obj);

    SPObject *_cached_item;
    sigc::connection _release_connection;

    void child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref) override;
    void remove_child(Inkscape::XML::Node *child) override;
    void order_changed(Inkscape::XML::Node *child, Inkscape::XML::Node *old_ref, Inkscape::XML::Node *new_ref) override;
    const char* displayName() const override;
    gchar *description() const override;
};

#endif

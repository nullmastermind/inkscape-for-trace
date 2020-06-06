// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_CANVAS_GROUP_H
#define SEEN_SP_CANVAS_GROUP_H

/**
 * @file
 * SPCanvasGroup.
 */
/*
 * Authors:
 *   Federico Mena <federico@nuclecu.unam.mx>
 *   Raph Levien <raph@gimp.org>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1998 The Free Software Foundation
 * Copyright (C) 2002 Lauris Kaplinski
 * Copyright (C) 2010 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glib-object.h>

#include "sp-canvas-item.h"


#define SP_TYPE_CANVAS_GROUP (sp_canvas_group_get_type())
#define SP_CANVAS_GROUP(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SP_TYPE_CANVAS_GROUP, SPCanvasGroup))
#define SP_IS_CANVAS_GROUP(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), SP_TYPE_CANVAS_GROUP))

GType sp_canvas_group_get_type();

/**
 * The SPCanvasGroup vtable.
 */
struct SPCanvasGroupClass {
    SPCanvasItemClass parent_class;
};


/**
 * A group of items.
 */
struct SPCanvasGroup {
    /**
     * Adds an item to a canvas group.
     */
    void add(SPCanvasItem *item);

    /**
     * Removes an item from a canvas group.
     */
    void remove(SPCanvasItem *item);

    /**
     * Class initialization function for SPCanvasGroupClass.
     */
    static void classInit(SPCanvasGroupClass *klass);

    /**
     * Callback. Empty.
     */
    static void init(SPCanvasGroup *group);

    /**
     * Callback that destroys all items in group and calls group's virtual
     * destroy() function.
     */
    static void destroy(SPCanvasItem *object);

    /**
     * Update handler for canvas groups.
     */
    static void update(SPCanvasItem *item, Geom::Affine const &affine, unsigned int flags);

    /**
     * Point handler for canvas groups.
     */
    static double point(SPCanvasItem *item, Geom::Point p, SPCanvasItem **actual_item);

    /**
     * Renders all visible canvas group items in buf rectangle.
     */
    static void render(SPCanvasItem *item, SPCanvasBuf *buf);

    static void viewboxChanged(SPCanvasItem *item, Geom::IntRect const &new_area);


    // Data members: ----------------------------------------------------------

    SPCanvasItem item;

    SPCanvasItemList items;
};


#endif // SEEN_SP_CANVAS_GROUP_H

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

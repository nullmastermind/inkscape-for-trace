// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_CANVAS_DEBUG_H
#define SEEN_SP_CANVAS_DEBUG_H

/*
 * A simple surface for debugging the canvas. Shows how tiles are drawn.
 *
 * Author:
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright (C) 2017 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-canvas-item.h"

class SPItem;

#define SP_TYPE_CANVAS_DEBUG (sp_canvas_debug_get_type ())
#define SP_CANVAS_DEBUG(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_CANVAS_DEBUG, SPCanvasDebug))
#define SP_IS_CANVAS_DEBUG(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_CANVAS_DEBUG))

struct SPCanvasDebug : public SPCanvasItem {
};

GType sp_canvas_debug_get_type ();

struct SPCanvasDebugClass : public SPCanvasItemClass{};

#endif // SEEN_SP_CANVAS_DEBUG_H

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

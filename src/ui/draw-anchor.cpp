// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * Anchors implementation.
 */

/*
 * Authors:
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 * Copyright (C) 2004 Monash University
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include "ui/draw-anchor.h"
#include "desktop.h"
#include "ui/tools/tool-base.h"
#include "ui/tools/lpe-tool.h"

#include "display/control/canvas-item-ctrl.h"
#include "display/curve.h"

const guint32 FILL_COLOR_NORMAL    = 0xffffff7f;
const guint32 FILL_COLOR_MOUSEOVER = 0xff0000ff;

/**
 * Creates an anchor object and initializes it.
 */
SPDrawAnchor *sp_draw_anchor_new(Inkscape::UI::Tools::FreehandBase *dc, SPCurve *curve, bool start, Geom::Point delta)
{
    if (SP_IS_LPETOOL_CONTEXT(dc)) {
        // suppress all kinds of anchors in LPEToolContext
        return nullptr;
    }

    SPDrawAnchor *a = new SPDrawAnchor;

    a->dc = dc;
    a->curve = curve->ref();
    a->start = start;
    a->active = FALSE;
    a->dp = delta;
    a->ctrl = new Inkscape::CanvasItemCtrl(dc->getDesktop()->getCanvasControls(), Inkscape::CANVAS_ITEM_CTRL_TYPE_ANCHOR);
    a->ctrl->set_name("CanvasItemCtrl:DrawAnchor");
    a->ctrl->set_fill(FILL_COLOR_NORMAL);
    a->ctrl->set_position(delta);
    a->ctrl->set_pickable(false); // We do our own checking. (TODO: Should be fixed!)

    return a;
}

SPDrawAnchor::~SPDrawAnchor()
{
    if (ctrl) {
        delete (ctrl);
    }
}

/**
 * Destroys the anchor's canvas item and frees the anchor object.
 */
SPDrawAnchor *sp_draw_anchor_destroy(SPDrawAnchor *anchor)
{
    delete anchor;
    return nullptr;
}

/**
 * Test if point is near anchor, if so fill anchor on canvas and return
 * pointer to it or NULL.
 */
SPDrawAnchor *sp_draw_anchor_test(SPDrawAnchor *anchor, Geom::Point w, bool activate)
{
    if ( activate && anchor->ctrl->contains(w)) {
        
        if (!anchor->active) {
            anchor->ctrl->set_size_extra(4);
            anchor->ctrl->set_fill(FILL_COLOR_MOUSEOVER);
            anchor->active = TRUE;
        }
        return anchor;
    }

    if (anchor->active) {
        anchor->ctrl->set_size_extra(0);
        anchor->ctrl->set_fill(FILL_COLOR_NORMAL);
        anchor->active = FALSE;
    }

    return nullptr;
}


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

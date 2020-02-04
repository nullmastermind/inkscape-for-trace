// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_INKSCAPE_CTRLRECT_H
#define SEEN_INKSCAPE_CTRLRECT_H

/**
 * @file
 * Simple non-transformed rectangle, usable for rubberband.
 * Modified to work with rotated canvas.
 */
/*
 * Authors:
 *   Lauris Kaplinski <lauris@ximian.com>
 *   Carl Hetherington <inkscape@carlh.net>
 *   Tavmjong Bah <tavjong@free.fr>
 *
 * Copyright (C) 1999-2001 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2017 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 */

#include <glib.h>
#include "sp-canvas-item.h"
#include <2geom/rect.h>
#include <2geom/int-rect.h>
#include <2geom/transforms.h>

struct SPCanvasBuf;

#define SP_TYPE_CTRLRECT (sp_ctrlrect_get_type ())
#define SP_CTRLRECT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SP_TYPE_CTRLRECT, CtrlRect))
#define SP_CTRLRECT_CLASS(c) (G_TYPE_CHECK_CLASS_CAST((c), SP_TYPE_CTRLRECT, CtrlRectClass))
#define SP_IS_CTRLRECT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_CTRLRECT))
#define SP_IS_CTRLRECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_CTRLRECT))

class CtrlRect : public SPCanvasItem
{
public:

    void init();
    void setColor(guint32 b, bool h, guint f);
    void setShadow(int s, guint c);
    void setInvert(bool invert);
    void setRectangle(Geom::Rect const &r);
    void setDashed(bool d);
    void setCheckerboard(bool d);

    void render(SPCanvasBuf *buf);
    void update(Geom::Affine const &affine, unsigned int flags);
    
private:
    void _requestUpdate();
    
    Geom::Rect _rect;
    Geom::Affine _affine;

    bool _has_fill;
    bool _dashed;
    bool _inverted;
    bool _checkerboard;

    Geom::OptIntRect _area;
    gint _shadow_width;
    guint32 _border_color;
    guint32 _fill_color;
    guint32 _shadow_color;    
};

struct CtrlRectClass : public SPCanvasItemClass {};

GType sp_ctrlrect_get_type();

#endif // SEEN_CTRLRECT_H

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

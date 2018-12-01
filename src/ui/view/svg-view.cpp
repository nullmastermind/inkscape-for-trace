// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Functions and callbacks for generic SVG view and widget.
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Ralf Stephan <ralf@ark.in-berlin.de>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/transforms.h>

#include "ui/view/svg-view.h"

#include "document.h"

#include "display/canvas-arena.h"
#include "display/drawing-group.h"

#include "object/sp-item.h"
#include "object/sp-root.h"

#include "util/units.h"

namespace Inkscape {
namespace UI {
namespace View {

SVGView::SVGView(SPCanvasGroup *parent)
{
    _hscale = 1.0;
    _vscale = 1.0;
    _rescale = FALSE;
    _keepaspect = FALSE;
    _width = 0.0;
    _height = 0.0;

    _dkey = 0;
    _drawing = nullptr;
    _parent = parent;
}

SVGView::~SVGView()
{
    if (doc() && _drawing)
    {
        doc()->getRoot()->invoke_hide(_dkey);
        _drawing = nullptr;
    }
}

void SVGView::setScale(gdouble hscale, gdouble vscale)
{
    if (!_rescale && ((hscale != _hscale) || (vscale != _vscale))) {
        _hscale = hscale;
        _vscale = vscale;
        doRescale (true);
    }
}

void SVGView::setRescale(bool rescale, bool keepaspect, gdouble width, gdouble height)
{
    g_return_if_fail (!rescale || (width >= 0.0));
    g_return_if_fail (!rescale || (height >= 0.0));

    _rescale = rescale;
    _keepaspect = keepaspect;
    _width = width;
    _height = height;

    doRescale (true);
}

void SVGView::doRescale(bool event)
{
    if (!doc()) {
        return;
    }
    if (doc()->getWidth().value("px") < 1e-9) {
        return;
    }
    if (doc()->getHeight().value("px") < 1e-9) {
        return;
    }

    double x_offset = 0.0;
    double y_offset = 0.0;
    if (_rescale) {
        _hscale = _width / doc()->getWidth().value("px");
        _vscale = _height / doc()->getHeight().value("px");
        if (_keepaspect) {
            if (_hscale > _vscale) {
                _hscale = _vscale;
                x_offset = (_width  - doc()->getWidth().value("px")  * _vscale)/2.0;
            } else {
                _vscale = _hscale;
                y_offset = (_height - doc()->getHeight().value("px") * _hscale)/2.0;
            }
        }
    }

    if (_drawing) {
        sp_canvas_item_affine_absolute (_drawing, Geom::Scale(_hscale, _vscale) * Geom::Translate(x_offset, y_offset));
    }

    if (event) {
        emitResized (doc()->getWidth().value("px") * _hscale,
                doc()->getHeight().value("px") * _vscale);
    }
}

void SVGView::mouseover()
{
    GdkDisplay *display = gdk_display_get_default();
    GdkCursor  *cursor  = gdk_cursor_new_for_display(display, GDK_HAND2);
    GdkWindow *window = gtk_widget_get_window (GTK_WIDGET(SP_CANVAS_ITEM(_drawing)->canvas));
    gdk_window_set_cursor(window, cursor);
    g_object_unref(cursor);
}

void SVGView::mouseout()
{
    GdkWindow *window = gtk_widget_get_window (GTK_WIDGET(SP_CANVAS_ITEM(_drawing)->canvas));
    gdk_window_set_cursor(window, nullptr);
}


//----------------------------------------------------------------
/**
 * Callback connected with arena_event.
 */
/// \todo fixme. This hasn't worked since at least 0.48. It should result in a cursor change over <a></a> links.
static gint arena_handler(SPCanvasArena */*arena*/, Inkscape::DrawingItem *ai, GdkEvent *event, SVGView *svgview)
{
	static gdouble x, y;
	static gboolean active = FALSE;
	SPEvent spev;

	SPItem *spitem = (ai) ? ai->getItem() : nullptr;

	switch (event->type) {
            case GDK_BUTTON_PRESS:
                if (event->button.button == 1) {
                    active = TRUE;
                    x = event->button.x;
                    y = event->button.y;
                }
                break;
            case GDK_BUTTON_RELEASE:
                if (event->button.button == 1) {
                    if (active && (event->button.x == x) &&
                        (event->button.y == y)) {
                        spev.type = SPEvent::ACTIVATE;
                        if ( spitem != nullptr )
                        {
                            spitem->emitEvent (spev);
                        }
                    }
		}
		active = FALSE;
		break;
            case GDK_MOTION_NOTIFY:
		active = FALSE;
		break;
            case GDK_ENTER_NOTIFY:
		spev.type = SPEvent::MOUSEOVER;
		spev.view = svgview;
                if ( spitem != nullptr )
		{
                    spitem->emitEvent (spev);
                }
		break;
            case GDK_LEAVE_NOTIFY:
		spev.type = SPEvent::MOUSEOUT;
		spev.view = svgview;
                if ( spitem != nullptr )
		{
                    spitem->emitEvent (spev);
                }
		break;
            default:
		break;
	}

	return TRUE;
}

void SVGView::setDocument(SPDocument *document)
{
    if (doc()) {
        doc()->getRoot()->invoke_hide(_dkey);
    }

    if (!_drawing) {
        _drawing = sp_canvas_item_new (_parent, SP_TYPE_CANVAS_ARENA, nullptr);
        g_signal_connect (G_OBJECT (_drawing), "arena_event", G_CALLBACK (arena_handler), this);
    }

    View::setDocument (document);

    if (document) {
        Inkscape::DrawingItem *ai = document->getRoot()->invoke_show(
                SP_CANVAS_ARENA (_drawing)->drawing,
                _dkey,
                SP_ITEM_SHOW_DISPLAY);

        if (ai) {
            SP_CANVAS_ARENA (_drawing)->drawing.root()->prependChild(ai);
        }

        doRescale (!_rescale);
    }
}

void SVGView::onDocumentResized(gdouble width, gdouble height)
{
    setScale (width, height);
    doRescale (!_rescale);
}

}
}
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

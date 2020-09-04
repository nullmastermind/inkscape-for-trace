// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * A light-weight widget containing an Inkscape canvas for rendering an SVG.
 */
/*
 * Authors:
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Includes code moved from svg-view.cpp authored by:
 *   MenTaLGuy
 *   Ralf Stephan <ralf@ark.in-berlin.de>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2018 Authors
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 * Read the file 'COPYING' for more information.
 *
 */

#include <iostream>

#include "svg-view-widget.h"

#include "document.h"

#include "2geom/transforms.h"

#include "display/drawing.h"
#include "display/control/canvas-item.h"
#include "display/control/canvas-item-drawing.h"
#include "display/control/canvas-item-group.h"

#include "object/sp-item.h"
#include "object/sp-root.h"

#include "ui/widget/canvas.h"

#include "util/units.h"

namespace Inkscape {
namespace UI {
namespace View {

/**
 * Callback connected with drawing_event.
 */
// This hasn't worked since at least 0.48. It should result in a cursor change over <a></a> links.
// There should be a better way of doing this. See note in canvas-arena.cpp.
static bool _drawing_handler(GdkEvent *event, Inkscape::DrawingItem *drawing_item, SVGViewWidget *svgview)
{
    static gdouble x, y;
    static gboolean active = FALSE;
    SPEvent spev;

    SPItem *spitem = (drawing_item) ? drawing_item->getItem() : nullptr;

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

    return true;
}


/**
 * A light-weight widget containing an SPCanvas for rendering an SVG.
 * It's derived from a Gtk::ScrolledWindow like the previous C version, but that doesn't seem to be
 * too useful.
 */
SVGViewWidget::SVGViewWidget(SPDocument* document)
{
    _canvas = Gtk::manage(new Inkscape::UI::Widget::Canvas());
    add(*_canvas);

    _parent = new Inkscape::CanvasItemGroup(_canvas->get_canvas_item_root());
    _drawing = new Inkscape::CanvasItemDrawing(_parent);
    _canvas->set_drawing(_drawing->get_drawing());
    _drawing->connect_drawing_event(sigc::bind(sigc::ptr_fun(_drawing_handler), this));

    setDocument(document);

    show_all();
}

SVGViewWidget::~SVGViewWidget()
{
    if (_document) {
        _document = nullptr;
    }
}

void
SVGViewWidget::setDocument(SPDocument* document)
{
    // Clear old document
    if (_document) {
        _document->getRoot()->invoke_hide(_dkey); // Removed from display tree
    }

    // Add new document
    if (document) {
        _document = document;

        Inkscape::DrawingItem *drawing_item = document->getRoot()->invoke_show(
            *(_drawing->get_drawing()),
            _dkey,
            SP_ITEM_SHOW_DISPLAY);

        if (drawing_item) {
            _drawing->get_drawing()->root()->prependChild(drawing_item);
        }

        doRescale ();
    }
}

void
SVGViewWidget::setResize(int width, int height)
{
    // Triggers size_allocation which calls SVGViewWidget::size_allocate.
    set_size_request(width, height);
    queue_resize();
}

void
SVGViewWidget::on_size_allocate(Gtk::Allocation& allocation)
{
    if (!(_allocation == allocation)) {
        _allocation = allocation;

        double width  = allocation.get_width();
        double height = allocation.get_height();

        if (width < 0.0 || height < 0.0) {
            std::cerr << "SVGViewWidget::size_allocate: negative dimensions!" << std::endl;
            Gtk::ScrolledWindow::on_size_allocate(allocation);
            return;
        }

        _rescale = true;
        _keepaspect = true;
        _width = width;
        _height = height;

        _canvas->redraw_all(); // Must redraw everything!
        doRescale ();
    }

    Gtk::ScrolledWindow::on_size_allocate(allocation);
}

void
SVGViewWidget::doRescale()
{
    if (!_document) {
        std::cerr << "SVGViewWidget::doRescale: No document!" << std::endl;
        return;
    }

    if (_document->getWidth().value("px") < 1e-9) {
        std::cerr << "SVGViewWidget::doRescale: Width too small!" << std::endl;
        return;
    }

    if (_document->getHeight().value("px") < 1e-9) {
        std::cerr << "SVGViewWidget::doRescale: Height too small!" << std::endl;
        return;
    }

    double x_offset = 0.0;
    double y_offset = 0.0;
    if (_rescale) {
        _hscale = _width / _document->getWidth().value("px");
        _vscale = _height / _document->getHeight().value("px");
        if (_keepaspect) {
            if (_hscale > _vscale) {
                _hscale = _vscale;
                x_offset = (_width  - _document->getWidth().value("px")  * _vscale)/2.0;
            } else {
                _vscale = _hscale;
                y_offset = (_height - _document->getHeight().value("px") * _hscale)/2.0;
            }
        }
    }

    if (_drawing) {
        _canvas->set_affine(Geom::Scale(_hscale, _vscale) * Geom::Translate(x_offset, y_offset));
        _canvas->request_update();
    }
}

void
SVGViewWidget::mouseover()
{
    GdkDisplay *display = gdk_display_get_default();
    GdkCursor  *cursor  = gdk_cursor_new_for_display(display, GDK_HAND2);
    GdkWindow *window = gtk_widget_get_window (GTK_WIDGET(_canvas->gobj()));
    gdk_window_set_cursor(window, cursor);
    g_object_unref(cursor);
}

void
SVGViewWidget::mouseout()
{
    GdkWindow *window = gtk_widget_get_window (GTK_WIDGET(_canvas->gobj()));
    gdk_window_set_cursor(window, nullptr);
}

} // Namespace View
} // Namespace UI
} // Namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :

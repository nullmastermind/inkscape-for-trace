/*
 * Author:
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright (C) 2018 Tavmong Bah
 *
 * Released under GNU GPL.  Read the file 'COPYING' for more information.
 */

#include "svg-view-widget.h"
#include "svg-view.h"

#include "document.h"

#include "display/sp-canvas.h"
#include "display/sp-canvas-group.h"
#include "display/sp-canvas-item.h"


namespace Inkscape {
namespace UI {
namespace View {

/**
 * A light-weight widget containing an SPCanvas with a View for rendering an SVG.
 * It's derived from a Gtk::ScrolledWindow like the previous C version, but that doesn't seem to be
 * too useful.
 */
SVGViewWidget::SVGViewWidget(SPDocument* document)
{
  _canvas = SPCanvas::createAA();
  add(*Glib::wrap(_canvas));

  SPCanvasItem* parent =
    sp_canvas_item_new(SP_CANVAS(_canvas)->getRoot(), SP_TYPE_CANVAS_GROUP, nullptr);
  _view = new SVGView(SP_CANVAS_GROUP(parent));
  _view->setDocument(document);

  signal_size_allocate().connect(sigc::mem_fun(*this, &SVGViewWidget::size_allocate));
}

SVGViewWidget::~SVGViewWidget()
{
    delete _view;
}

void
SVGViewWidget::setDocument(SPDocument* document)
{
  _view->setDocument(document);
}

void
SVGViewWidget::setResize(int width, int height)
{
    set_size_request(width, height);
    queue_resize();
}

void
SVGViewWidget::size_allocate(Gtk::Allocation& allocation)
{
  _view->setRescale(true, true, allocation.get_width(), allocation.get_height());
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

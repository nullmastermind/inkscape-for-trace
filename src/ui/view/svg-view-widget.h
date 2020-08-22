// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * A light-weight widget containing an SPCanvas with for rendering an SVG.
 */
/*
 * Authors:
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright (C) 2018 Authors
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 * Read the file 'COPYING' for more information.
 *
 */

#ifndef INKSCAPE_UI_SVG_VIEW_WIDGET_VARIATIONS_H
#define INKSCAPE_UI_SVG_VIEW_WIDGET_VARIATIONS_H


#include <gtkmm.h>

class SPDocument;

namespace Inkscape {

class CanvasItemDrawing;
class CanvasItemGroup;

namespace UI {

namespace Widget {
class Canvas;
}

namespace View {

/**
 * A light-weight widget containing an Inkscape canvas for rendering an SVG.
 */
class SVGViewWidget : public Gtk::ScrolledWindow {

public:
    SVGViewWidget(SPDocument* document);
    ~SVGViewWidget() override;
    void setDocument(  SPDocument* document);
    void setResize( int width, int height);
    void on_size_allocate(Gtk::Allocation& allocation) override;

private:

    Inkscape::UI::Widget::Canvas *_canvas;

// From SVGView ---------------------------------

public:
    SPDocument*     _document = nullptr;
    unsigned int    _dkey     = 0;
    Inkscape::CanvasItemGroup    *_parent   = nullptr;
    Inkscape::CanvasItemDrawing  *_drawing  = nullptr;
    Gtk::Allocation _allocation;
    double          _hscale   = 1.0;     ///< horizontal scale
    double          _vscale   = 1.0;     ///< vertical scale
    bool            _rescale  = false;   ///< whether to rescale automatically
    bool            _keepaspect = false;
    double          _width    = 0.0;
    double          _height   = 0.0;

    /**
     * Helper function that sets rescale ratio.
     */
    void doRescale();

    /**
     * Change cursor (used for links).
     */
    void mouseover();
    void mouseout();

};

} // Namespace View
} // Namespace UI
} // Namespace Inkscape

#endif // INKSCAPE_UI_SVG_VIEW_WIDGET

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

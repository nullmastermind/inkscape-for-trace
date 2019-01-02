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
class SPCanvasGroup;
class SPCanvasItem;

namespace Inkscape {
namespace UI {
namespace View {

/**
 * A light-weight widget containing an SPCanvas for rendering an SVG.
 */
class SVGViewWidget : public Gtk::ScrolledWindow {

public:
    SVGViewWidget(SPDocument* document);
    ~SVGViewWidget() override;
    void setDocument(  SPDocument* document);
    void setResize( int width, int height);

private:
    void size_allocate(Gtk::Allocation& allocation);

    GtkWidget* _canvas;

// From SVGView ---------------------------------

public:
    SPDocument*     _document;
    unsigned int    _dkey;
    SPCanvasGroup  *_parent;
    SPCanvasItem   *_drawing;
    double          _hscale;     ///< horizontal scale
    double          _vscale;     ///< vertical scale
    bool            _rescale;    ///< whether to rescale automatically
    bool            _keepaspect;
    double          _width;
    double          _height;

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

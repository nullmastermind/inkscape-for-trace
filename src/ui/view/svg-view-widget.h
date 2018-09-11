// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * A light-weight widget containing an SPCanvas with a View for rendering an SVG.
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
namespace UI {
namespace View {

class SVGView;

/**
 * A light-weight widget containing an SPCanvas with an SVGView for rendering an SVG.
 */
class SVGViewWidget : public Gtk::ScrolledWindow {

public:
    SVGViewWidget(SPDocument* document);
    ~SVGViewWidget();
    void setDocument(  SPDocument* document);
    void setResize( int width, int height);

private:
    void size_allocate(Gtk::Allocation& allocation);

    SVGView* _view;
    GtkWidget* _canvas;
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

// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SVG_VIEW_H
#define SEEN_SVG_VIEW_H
/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Ralf Stephan <ralf@ark.in-berlin.de>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/view/view.h"

struct SPCanvasGroup;
struct SPCanvasItem;

namespace Inkscape {
namespace UI {
namespace View {

/**
 * Generic SVG view.
 */
class SVGView : public View {
public:	
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
     * Constructs new SPSVGView object and returns pointer to it.
     */
    SVGView(SPCanvasGroup* parent);

    ~SVGView() override;
        
    /**
     * Rescales SPSVGView to given proportions.
     */
    void setScale(double hscale, double vscale);
    
    /**
     * Rescales SPSVGView and keeps aspect ratio.
     */
    void setRescale(bool rescale, bool keepaspect, double width, double height);

    /**
     * Helper function that sets rescale ratio and emits resize event.
     */
    void doRescale(bool event);

    /**
     * Callback connected with set_document signal.
     */
    void setDocument(SPDocument *document) override;

    void mouseover() override;

    void mouseout() override;

    bool shutdown() override { return true; }

private:
    virtual void onPositionSet(double, double) {}
    void onResized(double, double) override {}
    void onRedrawRequested() override {}
    void onStatusMessage(Inkscape::MessageType /*type*/, gchar const */*message*/) override {}
    void onDocumentURISet(gchar const* /*uri*/) override {}

    /**
     * Callback connected with document_resized signal.
     */
    void onDocumentResized(double, double) override;
};

}
}
}

#endif // SEEN_SVG_VIEW_H

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

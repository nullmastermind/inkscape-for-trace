// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Inkscape - An SVG editor.
 */
/*
 * Authors:
 *   Tavmjong Bah
 *
 * Copyright (C) 2018 Authors
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 * Read the file 'COPYING' for more information.
 *
 */

#ifndef INKSCAPE_WINDOW_H
#define INKSCAPE_WINDOW_H

#include <gtkmm.h>

class SPDocument;
class SPDesktop;
class SPDesktopWidget;

namespace Inkscape {
namespace UI {
namespace View {
//  class SVGViewWidget;
}
}
}


class InkscapeWindow : public Gtk::ApplicationWindow {

public:
    InkscapeWindow(SPDocument* document);

    SPDesktop*       get_desktop()        { return _desktop; }
    SPDesktopWidget* get_desktop_widget() { return _desktop_widget; }

private:
    SPDocument* _document;
    SPDesktop*  _desktop;
    SPDesktopWidget* _desktop_widget;

    Gtk::Box*   _mainbox;  // Remove after we don't need set_desktop_widget()

    // Callbacks
    bool key_press(GdkEventKey* event);
};

#endif // INKSCAPE_WINDOW_H

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

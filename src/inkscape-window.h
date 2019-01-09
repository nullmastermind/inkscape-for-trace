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

namespace Inkscape {
namespace UI {
namespace View {
//  class SVGViewWidget;
}
}
}


class InkscapeWindow : public Gtk::ApplicationWindow {

public:
    InkscapeWindow();

private:

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

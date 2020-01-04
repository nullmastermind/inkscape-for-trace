// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_VIEW_VIEWWIDGET_H
#define INKSCAPE_UI_VIEW_VIEWWIDGET_H

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

#include <gtkmm/eventbox.h>


namespace Inkscape {
namespace UI {
namespace View {
class View;
} // namespace View
} // namespace UI
} // namespace Inkscape

/**
 * SPViewWidget is a GUI widget that contain a single View. It is also
 * an abstract base class with little functionality of its own.
 */
class SPViewWidget : public Gtk::EventBox {
    using parent_type = Gtk::EventBox;
    using view_type = Inkscape::UI::View::View;

    view_type *view = nullptr;

  public:
    void on_unrealize() override;

    view_type *getView() { return view; }

    void setView(view_type *view);
};

#endif

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

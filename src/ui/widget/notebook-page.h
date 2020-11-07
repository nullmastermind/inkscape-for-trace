// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Bryce Harrington <bryce@bryceharrington.org>
 *
 * Copyright (C) 2004 Bryce Harrington
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_NOTEBOOK_PAGE_H
#define INKSCAPE_UI_WIDGET_NOTEBOOK_PAGE_H

#include <gtkmm/box.h>

namespace Gtk {
class Grid;
}

namespace Inkscape {
namespace UI {
namespace Widget {

/**
 * A tabbed notebook page for dialogs.
 */
class NotebookPage : public Gtk::Box
{
public:

    /**
     * Construct a NotebookPage.
     */
    NotebookPage(int n_rows, int n_columns, bool expand=false, bool fill=false, guint padding=0);

    Gtk::Grid& table() { return *_table; }

protected:
    Gtk::Grid *_table;
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_WIDGET_NOTEBOOK_PAGE_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

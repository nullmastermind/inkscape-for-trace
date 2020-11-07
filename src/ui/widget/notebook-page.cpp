// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Notebook page widget.
 *
 * Author:
 *   Bryce Harrington <bryce@bryceharrington.org>
 *
 * Copyright (C) 2004 Bryce Harrington
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "notebook-page.h"

# include <gtkmm/grid.h>

namespace Inkscape {
namespace UI {
namespace Widget {

NotebookPage::NotebookPage(int n_rows, int n_columns, bool expand, bool fill, guint padding)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL)
    , _table(Gtk::manage(new Gtk::Grid()))
{
    set_name("NotebookPage");
    set_border_width(4);
    set_spacing(4);

    _table->set_row_spacing(4);
    _table->set_column_spacing(4);

    pack_start(*_table, expand, fill, padding);
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape

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

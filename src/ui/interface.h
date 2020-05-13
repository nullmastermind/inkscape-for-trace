// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_INTERFACE_H
#define SEEN_SP_INTERFACE_H

/*
 * Main UI stuff
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *
 * Copyright (C) 2012 Kris De Gussem
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/ustring.h>

typedef struct _GtkWidget GtkWidget;

namespace Inkscape {
class Verb;

namespace UI {
namespace View {
class View;
} // namespace View
} // namespace UI
} // namespace Inkscape

/**
 * \param widget unused
 */
void sp_ui_close_view (GtkWidget *widget);

void sp_ui_new_view ();

void sp_ui_import_files(gchar *buffer);

/**
 *  This function is called to exit the program, and iterates through all
 *  open document view windows, attempting to close each in turn.  If the
 *  view has unsaved information, the user will be prompted to save,
 *  discard, or cancel.
 *
 *  Returns FALSE if the user cancels the close_all operation, TRUE
 *  otherwise.
 */
unsigned int sp_ui_close_all ();

Glib::ustring getLayoutPrefPath( Inkscape::UI::View::View *view );

/**
 *
 */
void sp_ui_error_dialog (char const* message);
bool sp_ui_overwrite_file (char const* filename);

#endif // SEEN_SP_INTERFACE_H

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

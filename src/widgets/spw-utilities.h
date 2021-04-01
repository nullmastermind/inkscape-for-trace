// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __SPW_UTILITIES_H__
#define __SPW_UTILITIES_H__

/*
 * Inkscape Widget Utilities
 *
 * Author:
 *   Bryce W. Harrington <brycehar@bryceharrington.org>
 *
 * Copyright (C) 2003 Bryce Harrington
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

/* The following are helper routines for making Inkscape dialog widgets.
   All are prefixed with spw_, short for inkscape_widget.  This is not to
   be confused with SPWidget, an existing datatype associated with Inkscape::XML::Node/
   SPObject, that reacts to modification.
*/

#include <glib.h>
#include <glibmm/ustring.h>
#include <gtk/gtk.h>
#include <functional>

namespace Gtk {
  class Label;
  class Grid;
  class HBox;
  class Widget;
}

Gtk::Label * spw_label(Gtk::Grid *table, gchar const *label_text, int col, int row, Gtk::Widget *target);
Gtk::Box * spw_hbox(Gtk::Grid *table, int width, int col, int row);

gpointer sp_search_by_data_recursive(GtkWidget *w, gpointer data);
GtkWidget *sp_search_by_value_recursive(GtkWidget *w, gchar *key, gchar *value);

Gtk::Widget * sp_search_by_name_recursive(Gtk::Widget          *parent,
                                          const Glib::ustring&  name);

Gtk::Widget* sp_traverse_widget_tree(Gtk::Widget* widget, const std::function<bool (Gtk::Widget*)>& eval);

Gtk::Widget* sp_find_focusable_widget(Gtk::Widget* widget);

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

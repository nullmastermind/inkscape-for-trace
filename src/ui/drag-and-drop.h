// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_DRAG_AND_DROP_H
#define SEEN_CANVAS_DRAG_AND_DROP_H

/**
 * @file
 * Drag and drop of drawings onto canvas.
 */

/* Authors:
 *
 * Copyright (C) Tavmjong Bah 2019
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm.h>

void ink_drag_setup(Gtk::Widget* win);

static void ink_drag_data_received(GtkWidget *widget,
                                     GdkDragContext *drag_context,
                                     gint x, gint y,
                                     GtkSelectionData *data,
                                     guint info,
                                     guint event_time,
                                     gpointer user_data);
static void ink_drag_motion( GtkWidget *widget,
                               GdkDragContext *drag_context,
                               gint x, gint y,
                               GtkSelectionData *data,
                               guint info,
                               guint event_time,
                               gpointer user_data );
static void ink_drag_leave( GtkWidget *widget,
                              GdkDragContext *drag_context,
                              guint event_time,
                              gpointer user_data );

#endif // SEEN_CANVAS_DRAG_AND_DROP_H

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

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Generic window implementation
 *//*
 * Authors:
 * see git history
 * Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2017 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm.h>
#include <gtkmm/window.h>

#include "desktop.h"
#include "inkscape.h"
#include "shortcuts.h"
#include "window.h"

static bool on_window_key_press(GdkEventKey* event)
{
    unsigned shortcut = 0;
    shortcut = sp_shortcut_get_for_event(event);
    return sp_shortcut_invoke (shortcut, SP_ACTIVE_DESKTOP);
}

Gtk::Window * Inkscape::UI::window_new (const gchar *title, unsigned int resizeable)
{
    Gtk::Window *window = new Gtk::Window(Gtk::WINDOW_TOPLEVEL);
    window->set_title (title);
    window->set_resizable (resizeable);
    window->signal_key_press_event().connect(sigc::ptr_fun(&on_window_key_press));

    return window;
}

static gboolean sp_window_key_press(GtkWidget *, GdkEventKey *event)
{
    return on_window_key_press(event);
}

GtkWidget * sp_window_new (const gchar *title, unsigned int resizeable)
{
    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title ((GtkWindow *) window, title);
    gtk_window_set_resizable ((GtkWindow *) window, resizeable);
    g_signal_connect_after ((GObject *) window, "key_press_event", (GCallback) sp_window_key_press, NULL);

    return window;
}

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

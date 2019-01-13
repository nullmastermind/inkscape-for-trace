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


#include "inkscape-window.h"
#include "inkscape.h"   // SP_ACTIVE_DESKTOP
#include "shortcuts.h"

#include "widgets/desktop-widget.h"

InkscapeWindow::InkscapeWindow(SPDocument* document)
    : _document(document)
{
    set_resizable(true);

    // Callbacks
    signal_key_press_event().connect(sigc::mem_fun(*this, &InkscapeWindow::key_press));

    // Actions
}

// TEMP: We should be creating the desktop widget and desktop in constructor.
void
InkscapeWindow::set_desktop_widget(SPDesktopWidget* desktop_widget)
{
    gtk_container_add(GTK_CONTAINER(gobj()), GTK_WIDGET(desktop_widget));
    gtk_widget_show(GTK_WIDGET(desktop_widget));
    _desktop = desktop_widget->desktop;
}

bool
InkscapeWindow::key_press(GdkEventKey* event)
{
    unsigned shortcut = sp_shortcut_get_for_event(event);
    return sp_shortcut_invoke (shortcut, _desktop);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Generic button widget
 *//*
 * Authors:
 *  see git history
 *  Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_BUTTON_H
#define SEEN_SP_BUTTON_H

#include <gtk/gtk.h>
#include <sigc++/connection.h>

#define SP_TYPE_BUTTON (sp_button_get_type ())
G_DECLARE_FINAL_TYPE(SPButton, sp_button, SP, BUTTON, GtkToggleButton);

struct SPAction;

namespace Inkscape {
namespace UI {
namespace View {
class View;
}
}
}

enum SPButtonType {
	SP_BUTTON_TYPE_NORMAL,
	SP_BUTTON_TYPE_TOGGLE
};

struct SPBChoiceData {
	guchar *px;
};

struct _SPButton {
    GtkToggleButton parent_instance;

    SPButtonType type;
    GtkIconSize lsize;
    unsigned int psize;
    SPAction *action;
    SPAction *doubleclick_action;

    sigc::connection c_set_active;
    sigc::connection c_set_sensitive;
};

#define SP_BUTTON_IS_DOWN(b) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (b))

GtkWidget *sp_button_new (GtkIconSize size, SPButtonType type, SPAction *action, SPAction *doubleclick_action);

void sp_button_toggle_set_down (SPButton *button, gboolean down);

GtkWidget *sp_button_new_from_data (GtkIconSize  size,
				    SPButtonType type,
				    Inkscape::UI::View::View *view,
				    const gchar *name,
				    const gchar *tip);

#endif // !SEEN_SP_BUTTON_H

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

// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_TOOLBOX_H
#define SEEN_TOOLBOX_H

/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1999-2002 Authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/ustring.h>
#include <gtk/gtk.h>
#include <gtkmm/enums.h>

#include "preferences.h"

#define TOOLBAR_SLIDER_HINT "compact"

class SPDesktop;

namespace Inkscape {
namespace UI {
namespace Tools {

class ToolBase;

}
}
}

namespace Inkscape {
namespace UI {

namespace Widget {
    class UnitTracker;
}

/**
 * Main toolbox source.
 */
class ToolboxFactory
{
public:
    static void setToolboxDesktop(GtkWidget *toolbox, SPDesktop *desktop);
    static void setOrientation(GtkWidget* toolbox, GtkOrientation orientation);
    static void showAuxToolbox(GtkWidget* toolbox);

    static GtkWidget *createToolToolbox();
    static GtkWidget *createAuxToolbox();
    static GtkWidget *createCommandsToolbox();
    static GtkWidget *createSnapToolbox();

    static Glib::ustring getToolboxName(GtkWidget* toolbox);

    static GtkIconSize prefToSize(Glib::ustring const &path, int base = 0 );
    static Gtk::IconSize prefToSize_mm(Glib::ustring const &path, int base = 0);

    ToolboxFactory() = delete;
};



} // namespace UI
} // namespace Inkscape

#endif /* !SEEN_TOOLBOX_H */

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

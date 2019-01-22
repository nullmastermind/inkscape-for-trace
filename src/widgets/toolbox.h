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

#include "preferences.h"

#define TOOLBAR_SLIDER_HINT "compact"

typedef struct _EgeAdjustmentAction      EgeAdjustmentAction;

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

    static void updateSnapToolbox(SPDesktop *desktop, Inkscape::UI::Tools::ToolBase *eventcontext, GtkWidget *toolbox);

    static GtkIconSize prefToSize(Glib::ustring const &path, int base = 0 );
    static Gtk::IconSize prefToSize_mm(Glib::ustring const &path, int base = 0);

  private:
    ToolboxFactory() = delete;
};



} // namespace UI
} // namespace Inkscape


// utility

 EgeAdjustmentAction * create_adjustment_action( gchar const *name,
                                                       gchar const *label, gchar const *shortLabel, gchar const *tooltip,
                                                       Glib::ustring const &path, gdouble def,
                                                       gboolean altx, gchar const *altx_mark,
                                                       gdouble lower, gdouble upper, gdouble step, gdouble page,
                                                       gchar const** descrLabels, gdouble const* descrValues, guint descrCount,
                                                       Inkscape::UI::Widget::UnitTracker *unit_tracker = nullptr,
                                                       gdouble climb = 0.1, guint digits = 3, double factor = 1.0 );

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

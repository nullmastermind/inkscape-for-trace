// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_GRADIENT_TOOLBAR_H
#define SEEN_GRADIENT_TOOLBAR_H

/*
 * Gradient aux toolbar
 *
 * Authors:
 *   bulia byak <bulia@dr.com>
 *
 * Copyright (C) 2005 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "toolbar.h"

#include <gtkmm/adjustment.h>

class InkSelectOneAction;
class SPDesktop;
class SPGradient;

typedef struct _EgeAdjustmentAction EgeAdjustmentAction;
typedef struct _GtkActionGroup GtkActionGroup;
typedef struct _InkAction InkAction;

namespace Inkscape {
class Selection;

namespace UI {
namespace Tools {
class ToolBase;
}

namespace Toolbar {
class GradientToolbar : public Toolbar {
private:
    InkSelectOneAction *_new_type_mode;
    InkSelectOneAction *_new_fillstroke_action;
    InkSelectOneAction *_select_action;
    InkSelectOneAction *_spread_action;
    InkSelectOneAction *_stop_action;

    InkAction *_stops_add_action;
    InkAction *_stops_delete_action;
    InkAction *_stops_reverse_action;

    EgeAdjustmentAction *_offset_action;

    Glib::RefPtr<Gtk::Adjustment> _offset_adj;

    void new_type_changed(int mode);
    void new_fillstroke_changed(int mode);
    void gradient_changed(int active);
    SPGradient * get_selected_gradient();
    void spread_changed(int active);
    void stop_changed(int active);
    void select_dragger_by_stop(SPGradient          *gradient,
                                UI::Tools::ToolBase *ev);
    SPStop * get_selected_stop();
    void stop_set_offset();
    void stop_offset_adjustment_changed();
    static void add_stop(GtkWidget *button, gpointer data);
    static void remove_stop(GtkWidget *button, gpointer data);
    static void reverse(GtkWidget *button, gpointer data);
    static void linked_changed(GtkToggleAction *act, gpointer data);
    void check_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec);
    void selection_changed(Inkscape::Selection *selection);
    int update_stop_list( SPGradient *gradient, SPStop *new_stop, bool gr_multi);
    int select_stop_in_list(SPGradient *gradient, SPStop *new_stop);
    void select_stop_by_draggers(SPGradient *gradient, UI::Tools::ToolBase *ev);
    void selection_modified(Inkscape::Selection *selection, guint flags);
    void drag_selection_changed(gpointer dragger);
    void defs_release(SPObject * defs);
    void defs_modified(SPObject *defs, guint flags);

    sigc::connection _connection_changed;
    sigc::connection _connection_modified;
    sigc::connection _connection_subselection_changed;
    sigc::connection _connection_defs_release;
    sigc::connection _connection_defs_modified;

protected:
    GradientToolbar(SPDesktop *desktop)
        : Toolbar(desktop)
    {}

public:
    static GtkWidget * prep(SPDesktop *desktop, GtkActionGroup* mainActions);
};

}
}
}

#endif /* !SEEN_GRADIENT_TOOLBAR_H */

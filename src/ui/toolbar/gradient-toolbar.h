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

class SPDesktop;
class SPGradient;

namespace Gtk {
class ComboBoxText;
class ToolButton;
class ToolItem;
}

namespace Inkscape {
class Selection;

namespace UI {
namespace Tools {
class ToolBase;
}

namespace Widget {
class ComboToolItem;
class SpinButtonToolItem;
}

namespace Toolbar {
class GradientToolbar : public Toolbar {
private:
    std::vector<Gtk::RadioToolButton *> _new_type_buttons;
    std::vector<Gtk::RadioToolButton *> _new_fillstroke_buttons;
    UI::Widget::ComboToolItem *_select_cb;
    UI::Widget::ComboToolItem *_spread_cb;
    UI::Widget::ComboToolItem *_stop_cb;

    Gtk::ToolButton *_stops_add_item;
    Gtk::ToolButton *_stops_delete_item;
    Gtk::ToolButton *_stops_reverse_item;
    Gtk::ToggleToolButton *_linked_item;

    UI::Widget::SpinButtonToolItem *_offset_item;

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
    void add_stop();
    void remove_stop();
    void reverse();
    void linked_changed();
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
    GradientToolbar(SPDesktop *desktop);

public:
    static GtkWidget * create(SPDesktop *desktop);
};

}
}
}

#endif /* !SEEN_GRADIENT_TOOLBAR_H */

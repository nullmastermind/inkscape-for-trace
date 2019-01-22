// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SELECT_TOOLBAR_H
#define SEEN_SELECT_TOOLBAR_H

/** \file
 * Selector aux toolbar
 */
/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <bulia@dr.com>
 *
 * Copyright (C) 2003 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "toolbar.h"

class SPDesktop;

typedef struct _GtkActionGroup GtkActionGroup;

namespace Inkscape {
class Selection;

namespace UI {

namespace Widget {
class UnitTracker;
}

namespace Toolbar {

class SelectToolbar : public Toolbar {
private:
    std::unique_ptr<UI::Widget::UnitTracker> _tracker;

    Glib::RefPtr<Gtk::Adjustment>  _adj_x;
    Glib::RefPtr<Gtk::Adjustment>  _adj_y;
    Glib::RefPtr<Gtk::Adjustment>  _adj_w;
    Glib::RefPtr<Gtk::Adjustment>  _adj_h;
    GtkToggleAction               *_lock;

    GtkActionGroup *_selection_actions;
    std::vector<GtkAction *> *_context_actions;

    bool _update;

    void any_value_changed(Glib::RefPtr<Gtk::Adjustment>& adj);
    void layout_widget_update(Inkscape::Selection *sel);
    void on_inkscape_selection_modified(Inkscape::Selection *selection, guint flags);
    void on_inkscape_selection_changed(Inkscape::Selection *selection);

protected:
    SelectToolbar(SPDesktop *desktop);

public:
    static GtkWidget * prep(SPDesktop *desktop, GtkActionGroup* mainActions);
};

}
}
}
#endif /* !SEEN_SELECT_TOOLBAR_H */

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

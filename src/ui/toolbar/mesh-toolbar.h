// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_MESH_TOOLBAR_H
#define SEEN_MESH_TOOLBAR_H

/*
 * Mesh aux toolbar
 *
 * Authors:
 *   bulia byak <bulia@dr.com>
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright (C) 2012 authors
 * Copyright (C) 2005 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "toolbar.h"

#include <gtkmm/adjustment.h>

class InkSelectOneAction;
class SPDesktop;
class SPObject;

typedef struct _GtkActionGroup GtkActionGroup;
typedef struct _InkToggleAction InkToggleAction;

namespace Inkscape {
class Selection;

namespace UI {
class PrefPusher;

namespace Tools {
class ToolBase;
}

namespace Toolbar {
class MeshToolbar : public Toolbar {
private:
    InkSelectOneAction *_new_type_mode;
    InkSelectOneAction *_new_fillstroke_mode;
    InkSelectOneAction *_select_type_action;

    Glib::RefPtr<Gtk::Adjustment> _row_adj;
    Glib::RefPtr<Gtk::Adjustment> _col_adj;

    std::unique_ptr<UI::PrefPusher> _edit_fill_pusher;
    std::unique_ptr<UI::PrefPusher> _edit_stroke_pusher;
    std::unique_ptr<UI::PrefPusher> _show_handles_pusher;

    sigc::connection c_selection_changed;
    sigc::connection c_selection_modified;
    sigc::connection c_subselection_changed;
    sigc::connection c_defs_release;
    sigc::connection c_defs_modified;

    void new_geometry_changed(int mode);
    void new_fillstroke_changed(int mode);
    void row_changed();
    void col_changed();
    static void toggle_fill_stroke(InkToggleAction *act, gpointer data);
    void selection_changed(Inkscape::Selection *selection);
    static void toggle_handles();
    void watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec);
    void selection_modified(Inkscape::Selection *selection, guint flags);
    void drag_selection_changed(gpointer dragger);
    void defs_release(SPObject *defs);
    void defs_modified(SPObject *defs, guint flags);
    static void warning_popup();
    void type_changed(int mode);

protected:
    MeshToolbar(SPDesktop *desktop);

public:
    static GtkWidget * prep(SPDesktop *desktop, GtkActionGroup* mainActions);
};

}
}
}

#endif /* !SEEN_MESH_TOOLBAR_H */

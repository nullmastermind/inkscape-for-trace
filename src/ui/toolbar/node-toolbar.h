// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_NODE_TOOLBAR_H
#define SEEN_NODE_TOOLBAR_H

/**
 * @file
 * Node aux toolbar
 */
/* Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Frank Felfe <innerspace@iname.com>
 *   John Cliff <simarilius@yahoo.com>
 *   David Turner <novalis@gnu.org>
 *   Josh Andler <scislac@scislac.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Tavmjong Bah <tavmjong@free.fr>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2003 MenTaLguY
 * Copyright (C) 1999-2011 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "toolbar.h"
#include "2geom/coord.h"

class SPDesktop;

namespace Inkscape {
class Selection;

namespace UI {
class SimplePrefPusher;

namespace Tools {
class ToolBase;
}

namespace Widget {
class SpinButtonToolItem;
class UnitTracker;
}

namespace Toolbar {
class NodeToolbar : public Toolbar {
private:
    std::unique_ptr<UI::Widget::UnitTracker> _tracker;

    std::unique_ptr<UI::SimplePrefPusher> _pusher_show_transform_handles;
    std::unique_ptr<UI::SimplePrefPusher> _pusher_show_handles;
    std::unique_ptr<UI::SimplePrefPusher> _pusher_show_outline;
    std::unique_ptr<UI::SimplePrefPusher> _pusher_edit_clipping_paths;
    std::unique_ptr<UI::SimplePrefPusher> _pusher_edit_masks;

    Gtk::ToggleToolButton *_object_edit_clip_path_item;
    Gtk::ToggleToolButton *_object_edit_mask_path_item;
    Gtk::ToggleToolButton *_show_transform_handles_item;
    Gtk::ToggleToolButton *_show_handles_item;
    Gtk::ToggleToolButton *_show_helper_path_item;

    Gtk::ToolButton *_nodes_lpeedit_item;

    UI::Widget::SpinButtonToolItem *_nodes_x_item;
    UI::Widget::SpinButtonToolItem *_nodes_y_item;

    Glib::RefPtr<Gtk::Adjustment> _nodes_x_adj;
    Glib::RefPtr<Gtk::Adjustment> _nodes_y_adj;

    bool _freeze;

    sigc::connection c_selection_changed;
    sigc::connection c_selection_modified;
    sigc::connection c_subselection_changed;

    void value_changed(Geom::Dim2 d);
    void sel_changed(Inkscape::Selection *selection);
    void sel_modified(Inkscape::Selection *selection, guint /*flags*/);
    void coord_changed(gpointer shape_editor);
    void watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec);
    void edit_add();
    void edit_add_min_x();
    void edit_add_max_x();
    void edit_add_min_y();
    void edit_add_max_y();
    void edit_delete();
    void edit_join();
    void edit_break();
    void edit_join_segment();
    void edit_delete_segment();
    void edit_cusp();
    void edit_smooth();
    void edit_symmetrical();
    void edit_auto();
    void edit_toline();
    void edit_tocurve();
    void on_pref_toggled(Gtk::ToggleToolButton *item,
                         const Glib::ustring&   path);

protected:
    NodeToolbar(SPDesktop *desktop);

public:
    static GtkWidget * create(SPDesktop *desktop);
};
}
}
}
#endif /* !SEEN_SELECT_TOOLBAR_H */

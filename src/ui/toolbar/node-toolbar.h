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

typedef struct _EgeAdjustmentAction EgeAdjustmentAction;
typedef struct _GtkActionGroup GtkActionGroup;
typedef struct _InkAction InkAction;

namespace Inkscape {
class Selection;

namespace UI {
class PrefPusher;

namespace Tools {
class ToolBase;
}

namespace Widget {
class UnitTracker;
}

namespace Toolbar {
class NodeToolbar : public Toolbar {
private:
    UI::Widget::UnitTracker *_tracker;

    PrefPusher *_pusher_show_transform_handles;
    PrefPusher *_pusher_show_handles;
    PrefPusher *_pusher_show_outline;
    PrefPusher *_pusher_edit_clipping_paths;
    PrefPusher *_pusher_edit_masks;

    InkAction *_nodes_lpeedit;

    EgeAdjustmentAction *_nodes_x_action;
    EgeAdjustmentAction *_nodes_y_action;

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

protected:
    NodeToolbar(SPDesktop *desktop);
    ~NodeToolbar();

public:
    static GtkWidget * prep(SPDesktop *desktop, GtkActionGroup* mainActions);
};
}
}
}
#endif /* !SEEN_SELECT_TOOLBAR_H */

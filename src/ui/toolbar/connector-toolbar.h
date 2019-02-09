// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CONNECTOR_TOOLBAR_H
#define SEEN_CONNECTOR_TOOLBAR_H

/**
 * @file
 * Connector aux toolbar
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

#include <gtkmm/adjustment.h>

class SPDesktop;

namespace Gtk {
class ToolButton;
}

namespace Inkscape {
class Selection;

namespace XML {
class Node;
}

namespace UI {
namespace Toolbar {
class ConnectorToolbar : public Toolbar {
private:
    Gtk::ToggleToolButton *_orthogonal;
    Gtk::ToggleToolButton *_directed_item;
    Gtk::ToggleToolButton *_overlap_item;

    Glib::RefPtr<Gtk::Adjustment> _curvature_adj;
    Glib::RefPtr<Gtk::Adjustment> _spacing_adj;
    Glib::RefPtr<Gtk::Adjustment> _length_adj;

    bool _freeze;

    Inkscape::XML::Node *_repr;

    void path_set_avoid();
    void path_set_ignore();
    void orthogonal_toggled();
    void graph_layout();
    void directed_graph_layout_toggled();
    void nooverlaps_graph_layout_toggled();
    void curvature_changed();
    void spacing_changed();
    void length_changed();
    void selection_changed(Inkscape::Selection *selection);

protected:
    ConnectorToolbar(SPDesktop *desktop);

public:
    static GtkWidget * create(SPDesktop *desktop);

    static void event_attr_changed(Inkscape::XML::Node *repr,
                                   gchar const         *name,
                                   gchar const         * /*old_value*/,
                                   gchar const         * /*new_value*/,
                                   bool                  /*is_interactive*/,
                                   gpointer             data);
};

}
}
}

#endif /* !SEEN_CONNECTOR_TOOLBAR_H */

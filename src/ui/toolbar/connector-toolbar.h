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

typedef struct _GtkActionGroup GtkActionGroup;
typedef struct _InkToggleAction InkToggleAction;

namespace Inkscape {
class Selection;

namespace XML {
class Node;
}

namespace UI {
namespace Toolbar {
class ConnectorToolbar : public Toolbar {
private:
    InkToggleAction *_orthogonal;

    Glib::RefPtr<Gtk::Adjustment> _curvature_adj;
    Glib::RefPtr<Gtk::Adjustment> _spacing_adj;
    Glib::RefPtr<Gtk::Adjustment> _length_adj;

    bool _freeze;

    Inkscape::XML::Node *_repr;

    static void path_set_avoid();
    static void path_set_ignore();
    static void orthogonal_toggled(GtkToggleAction *act,
                                   gpointer         data);
    static void graph_layout();
    static void directed_graph_layout_toggled  (GtkToggleAction *act,
                                                gpointer         data);
    static void nooverlaps_graph_layout_toggled(GtkToggleAction *act,
                                                gpointer         data);

    void curvature_changed();
    void spacing_changed();
    void length_changed();
    void selection_changed(Inkscape::Selection *selection);

protected:
    ConnectorToolbar(SPDesktop *desktop)
        : Toolbar(desktop),
          _freeze(false),
          _repr(nullptr)
    {}

public:
    static GtkWidget * prep(SPDesktop *desktop, GtkActionGroup* mainActions);

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

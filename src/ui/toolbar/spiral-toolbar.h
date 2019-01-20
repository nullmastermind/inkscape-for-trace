// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SPIRAL_TOOLBAR_H
#define SEEN_SPIRAL_TOOLBAR_H

/**
 * @file
 * Spiral aux toolbar
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

typedef struct _EgeOutputAction EgeOutputAction;
typedef struct _GtkActionGroup GtkActionGroup;

namespace Inkscape {
class Selection;

namespace XML {
class Node;
}

namespace UI {
namespace Toolbar {
class SpiralToolbar : public Toolbar {
private:
    EgeOutputAction *_mode_action;

    Glib::RefPtr<Gtk::Adjustment> _revolution_adj;
    Glib::RefPtr<Gtk::Adjustment> _expansion_adj;
    Glib::RefPtr<Gtk::Adjustment> _t0_adj;

    bool _freeze;

    XML::Node *_repr;

    void value_changed(Glib::RefPtr<Gtk::Adjustment> &adj,
                       Glib::ustring const           &value_name);
    static void defaults(GtkWidget *widget,
                         GObject   *obj);
    void selection_changed(Inkscape::Selection *selection);

    sigc::connection *_connection;

protected:
    SpiralToolbar(SPDesktop *desktop) :
        Toolbar(desktop),
        _freeze(false),
        _repr(nullptr)
    {}

    ~SpiralToolbar() override;

public:
    static GtkWidget * prep(SPDesktop *desktop, GtkActionGroup* mainActions);

    static void event_attr_changed(Inkscape::XML::Node *repr,
                                   gchar const         *name,
                                   gchar const         *old_value,
                                   gchar const         *new_value,
                                   bool                 is_interactive,
                                   gpointer             data);
};
}
}
}

#endif /* !SEEN_SPIRAL_TOOLBAR_H */

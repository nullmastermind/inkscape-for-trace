// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_RECT_TOOLBAR_H
#define SEEN_RECT_TOOLBAR_H

/**
 * @file
 * Rect aux toolbar
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
class SPItem;
class SPRect;

typedef struct _EgeAdjustmentAction EgeAdjustmentAction;
typedef struct _EgeOutputAction EgeOutputAction;
typedef struct _GtkActionGroup GtkActionGroup;
typedef struct _InkAction InkAction;

namespace Inkscape {
class Selection;

namespace XML {
class Node;
}

namespace UI {
namespace Tools {
class ToolBase;
}

namespace Widget {
class UnitTracker;
}

namespace Toolbar {
class RectToolbar : public Toolbar {
private:
    EgeOutputAction *_mode_action;

    UI::Widget::UnitTracker *_tracker;

    XML::Node *_repr;
    SPItem *_item;

    EgeAdjustmentAction *_width_action;
    EgeAdjustmentAction *_height_action;
    InkAction *_not_rounded;

    Glib::RefPtr<Gtk::Adjustment> _width_adj;
    Glib::RefPtr<Gtk::Adjustment> _height_adj;
    Glib::RefPtr<Gtk::Adjustment> _rx_adj;
    Glib::RefPtr<Gtk::Adjustment> _ry_adj;

    bool _freeze;
    bool _single;

    void value_changed(Glib::RefPtr<Gtk::Adjustment>&  adj,
                       gchar const                    *value_name,
                       void (SPRect::*setter)(gdouble));

    void sensitivize();
    static void defaults(GtkWidget *widget,
                         GObject   *obj);
    void watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec);
    void selection_changed(Inkscape::Selection *selection);

protected:
    RectToolbar(SPDesktop *desktop);
    ~RectToolbar();

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

#endif /* !SEEN_RECT_TOOLBAR_H */

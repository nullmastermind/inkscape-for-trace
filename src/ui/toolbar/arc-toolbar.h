// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_ARC_TOOLBAR_H
#define SEEN_ARC_TOOLBAR_H

/**
 * @file
 * 3d box aux toolbar
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

namespace Gtk {
class RadioToolButton;
class ToolButton;
}

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
class LabelToolItem;
class SpinButtonToolItem;
class UnitTracker;
}

namespace Toolbar {
class ArcToolbar : public Toolbar {
private:
    UI::Widget::UnitTracker *_tracker;

    UI::Widget::SpinButtonToolItem *_rx_item;
    UI::Widget::SpinButtonToolItem *_ry_item;

    UI::Widget::LabelToolItem *_mode_item;

    std::vector<Gtk::RadioToolButton *> _type_buttons;
    Gtk::ToolButton *_make_whole;

    Glib::RefPtr<Gtk::Adjustment> _rx_adj;
    Glib::RefPtr<Gtk::Adjustment> _ry_adj;
    Glib::RefPtr<Gtk::Adjustment> _start_adj;
    Glib::RefPtr<Gtk::Adjustment> _end_adj;

    bool _freeze;
    bool _single;

    XML::Node *_repr;
    SPItem *_item;

    void value_changed(Glib::RefPtr<Gtk::Adjustment>&  adj,
                       gchar const                    *value_name);
    void startend_value_changed(Glib::RefPtr<Gtk::Adjustment>&  adj,
                                gchar const                    *value_name,
                                Glib::RefPtr<Gtk::Adjustment>&  other_adj);
    void type_changed( int type );
    void defaults();
    void sensitivize( double v1, double v2 );
    void check_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec);
    void selection_changed(Inkscape::Selection *selection);

    sigc::connection _changed;

protected:
    ArcToolbar(SPDesktop *desktop);
    ~ArcToolbar() override;

public:
    static GtkWidget * create(SPDesktop *desktop);
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

#endif /* !SEEN_ARC_TOOLBAR_H */

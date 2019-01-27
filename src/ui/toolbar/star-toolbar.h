// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_STAR_TOOLBAR_H
#define SEEN_STAR_TOOLBAR_H

/**
 * @file
 * Star aux toolbar
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
}

namespace Toolbar {
class StarToolbar : public Toolbar {
private:
    UI::Widget::LabelToolItem *_mode_item;
    std::vector<Gtk::RadioToolButton *> _flat_item_buttons;
    UI::Widget::SpinButtonToolItem *_magnitude_item;
    UI::Widget::SpinButtonToolItem *_spoke_item;
    UI::Widget::SpinButtonToolItem *_roundedness_item;
    UI::Widget::SpinButtonToolItem *_randomization_item;
    Gtk::ToolButton *_reset_item;

    XML::Node *_repr;

    Glib::RefPtr<Gtk::Adjustment> _magnitude_adj;
    Glib::RefPtr<Gtk::Adjustment> _spoke_adj;
    Glib::RefPtr<Gtk::Adjustment> _roundedness_adj;
    Glib::RefPtr<Gtk::Adjustment> _randomization_adj;

    bool _freeze;
    sigc::connection _changed;
    
    void side_mode_changed(int mode);
    void magnitude_value_changed();
    void proportion_value_changed();
    void rounded_value_changed();
    void randomized_value_changed();
    void defaults();
    void watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec);
    void selection_changed(Inkscape::Selection *selection);

protected:
    StarToolbar(SPDesktop *desktop);
    ~StarToolbar() override;

public:
    static GtkWidget * create(SPDesktop *desktop);

    static void event_attr_changed(Inkscape::XML::Node *repr,
                                   gchar const         *name,
                                   gchar const         *old_value,
                                   gchar const         *new_value,
                                   bool                 is_interactive,
                                   gpointer             dataPointer);
};

}
}
}

#endif /* !SEEN_SELECT_TOOLBAR_H */

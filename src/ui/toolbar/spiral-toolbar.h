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

namespace Gtk {
class ToolButton;
}

namespace Inkscape {
class Selection;

namespace XML {
class Node;
}

namespace UI {
namespace Widget {
class LabelToolItem;
class SpinButtonToolItem;
}

namespace Toolbar {
class SpiralToolbar : public Toolbar {
private:
    UI::Widget::LabelToolItem *_mode_item;

    UI::Widget::SpinButtonToolItem *_revolution_item;
    UI::Widget::SpinButtonToolItem *_expansion_item;
    UI::Widget::SpinButtonToolItem *_t0_item;

    Gtk::ToolButton *_reset_item;

    Glib::RefPtr<Gtk::Adjustment> _revolution_adj;
    Glib::RefPtr<Gtk::Adjustment> _expansion_adj;
    Glib::RefPtr<Gtk::Adjustment> _t0_adj;

    bool _freeze;

    XML::Node *_repr;

    void value_changed(Glib::RefPtr<Gtk::Adjustment> &adj,
                       Glib::ustring const           &value_name);
    void defaults();
    void selection_changed(Inkscape::Selection *selection);

    std::unique_ptr<sigc::connection> _connection;

protected:
    SpiralToolbar(SPDesktop *desktop);
    ~SpiralToolbar() override;

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

#endif /* !SEEN_SPIRAL_TOOLBAR_H */

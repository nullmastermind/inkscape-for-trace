// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_BOX3D_TOOLBAR_H
#define SEEN_BOX3D_TOOLBAR_H

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

#include "axis-manip.h"

namespace Gtk {
class Adjustment;
}

class Persp3D;
class SPDesktop;

namespace Inkscape {
class Selection;

namespace XML {
class Node;
}

namespace UI {
namespace Widget {
class SpinButtonToolItem;
}

namespace Tools {
class ToolBase;
}

namespace Toolbar {
class Box3DToolbar : public Toolbar {
private:
    UI::Widget::SpinButtonToolItem *_angle_x_item;
    UI::Widget::SpinButtonToolItem *_angle_y_item;
    UI::Widget::SpinButtonToolItem *_angle_z_item;

    Glib::RefPtr<Gtk::Adjustment> _angle_x_adj;
    Glib::RefPtr<Gtk::Adjustment> _angle_y_adj;
    Glib::RefPtr<Gtk::Adjustment> _angle_z_adj;

    Gtk::ToggleToolButton *_vp_x_state_item;
    Gtk::ToggleToolButton *_vp_y_state_item;
    Gtk::ToggleToolButton *_vp_z_state_item;

    XML::Node *_repr;
    bool _freeze;

    void angle_value_changed(Glib::RefPtr<Gtk::Adjustment> &adj,
                             Proj::Axis                     axis);
    void vp_state_changed(Proj::Axis axis);
    void check_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec);
    void selection_changed(Inkscape::Selection *selection);
    void resync_toolbar(Inkscape::XML::Node *persp_repr);
    void set_button_and_adjustment(Persp3D                        *persp,
                                   Proj::Axis                      axis,
                                   Glib::RefPtr<Gtk::Adjustment>&  adj,
                                   UI::Widget::SpinButtonToolItem *spin_btn,
                                   Gtk::ToggleToolButton          *toggle_btn);
    double normalize_angle(double a);

    sigc::connection _changed;

protected:
    Box3DToolbar(SPDesktop *desktop);
    ~Box3DToolbar() override;

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
#endif /* !SEEN_BOX3D_TOOLBAR_H */

// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SPRAY_TOOLBAR_H
#define SEEN_SPRAY_TOOLBAR_H

/**
 * @file
 * Spray aux toolbar
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
 * Copyright (C) 1999-2015 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "toolbar.h"

#include <gtkmm/adjustment.h>

class SPDesktop;

namespace Gtk {
class RadioToolButton;
}

namespace Inkscape {
namespace UI {
class SimplePrefPusher;

namespace Widget {
class SpinButtonToolItem;
}

namespace Toolbar {
class SprayToolbar : public Toolbar {
private:
    Glib::RefPtr<Gtk::Adjustment> _width_adj;
    Glib::RefPtr<Gtk::Adjustment> _mean_adj;
    Glib::RefPtr<Gtk::Adjustment> _sd_adj;
    Glib::RefPtr<Gtk::Adjustment> _population_adj;
    Glib::RefPtr<Gtk::Adjustment> _rotation_adj;
    Glib::RefPtr<Gtk::Adjustment> _offset_adj;
    Glib::RefPtr<Gtk::Adjustment> _scale_adj;

    std::unique_ptr<SimplePrefPusher> _usepressurewidth_pusher;
    std::unique_ptr<SimplePrefPusher> _usepressurepopulation_pusher;

    std::vector<Gtk::RadioToolButton *> _mode_buttons;
    UI::Widget::SpinButtonToolItem *_spray_population;
    UI::Widget::SpinButtonToolItem *_spray_rotation;
    UI::Widget::SpinButtonToolItem *_spray_scale;
    Gtk::ToggleToolButton *_usepressurescale;
    Gtk::ToggleToolButton *_picker;
    Gtk::ToggleToolButton *_pick_center;
    Gtk::ToggleToolButton *_pick_inverse_value;
    Gtk::ToggleToolButton *_pick_fill;
    Gtk::ToggleToolButton *_pick_stroke;
    Gtk::ToggleToolButton *_pick_no_overlap;
    Gtk::ToggleToolButton *_over_transparent;
    Gtk::ToggleToolButton *_over_no_transparent;
    Gtk::ToggleToolButton *_no_overlap;
    UI::Widget::SpinButtonToolItem *_offset;

    void width_value_changed();
    void mean_value_changed();
    void standard_deviation_value_changed();
    void mode_changed(int mode);
    void init();
    void population_value_changed();
    void rotation_value_changed();
    void update_widgets();
    void scale_value_changed();
    void offset_value_changed();
    void on_pref_toggled(Gtk::ToggleToolButton *btn,
                         const Glib::ustring&   path);
    void toggle_no_overlap();
    void toggle_pressure_scale();
    void toggle_picker();

protected:
    SprayToolbar(SPDesktop *desktop);

public:
    static GtkWidget * create(SPDesktop *desktop);

    void set_mode(int mode);
};
}
}
}

#endif /* !SEEN_SELECT_TOOLBAR_H */

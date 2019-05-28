// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_TWEAK_TOOLBAR_H
#define SEEN_TWEAK_TOOLBAR_H

/**
 * @file
 * Tweak aux toolbar
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

class SPDesktop;

namespace Gtk {
class RadioToolButton;
}

namespace Inkscape {
namespace UI {
namespace Widget {
class LabelToolItem;
class SpinButtonToolItem;
}

namespace Toolbar {
class TweakToolbar : public Toolbar {
private:
    UI::Widget::SpinButtonToolItem *_width_item;
    UI::Widget::SpinButtonToolItem *_force_item;
    UI::Widget::SpinButtonToolItem *_fidelity_item;

    Gtk::ToggleToolButton *_pressure_item;

    Glib::RefPtr<Gtk::Adjustment> _width_adj;
    Glib::RefPtr<Gtk::Adjustment> _force_adj;
    Glib::RefPtr<Gtk::Adjustment> _fidelity_adj;

    std::vector<Gtk::RadioToolButton *> _mode_buttons;

    UI::Widget::LabelToolItem *_channels_label;
    Gtk::ToggleToolButton *_doh_item;
    Gtk::ToggleToolButton *_dos_item;
    Gtk::ToggleToolButton *_dol_item;
    Gtk::ToggleToolButton *_doo_item;

    void width_value_changed();
    void force_value_changed();
    void mode_changed(int mode);
    void fidelity_value_changed();
    void pressure_state_changed();
    void toggle_doh();
    void toggle_dos();
    void toggle_dol();
    void toggle_doo();

protected:
    TweakToolbar(SPDesktop *desktop);

public:
    static GtkWidget * create(SPDesktop *desktop);

    void set_mode(int mode);
};
}
}
}

#endif /* !SEEN_SELECT_TOOLBAR_H */

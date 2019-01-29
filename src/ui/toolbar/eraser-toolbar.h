// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_ERASOR_TOOLBAR_H
#define SEEN_ERASOR_TOOLBAR_H

/**
 * @file
 * Erasor aux toolbar
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
class SeparatorToolItem;
}

namespace Inkscape {
namespace UI {
class SimplePrefPusher;

namespace Widget {
class SpinButtonToolItem;
}

namespace Toolbar {
class EraserToolbar : public Toolbar {
private:
    UI::Widget::SpinButtonToolItem *_width;
    UI::Widget::SpinButtonToolItem *_mass;
    UI::Widget::SpinButtonToolItem *_thinning;
    UI::Widget::SpinButtonToolItem *_cap_rounding;
    UI::Widget::SpinButtonToolItem *_tremor;

    Gtk::ToggleToolButton *_usepressure;
    Gtk::ToggleToolButton *_split;

    Glib::RefPtr<Gtk::Adjustment> _width_adj;
    Glib::RefPtr<Gtk::Adjustment> _mass_adj;
    Glib::RefPtr<Gtk::Adjustment> _thinning_adj;
    Glib::RefPtr<Gtk::Adjustment> _cap_rounding_adj;
    Glib::RefPtr<Gtk::Adjustment> _tremor_adj;

    std::unique_ptr<SimplePrefPusher> _pressure_pusher;

    std::vector<Gtk::SeparatorToolItem *> _separators;

    bool _freeze;

    void mode_changed(int mode);
    void set_eraser_mode_visibility(const guint eraser_mode);
    void width_value_changed();
    void mass_value_changed();
    void velthin_value_changed();
    void cap_rounding_value_changed();
    void tremor_value_changed();
    static void update_presets_list(gpointer data);
    void toggle_break_apart();
    void usepressure_toggled();

protected:
    EraserToolbar(SPDesktop *desktop);

public:
    static GtkWidget * create(SPDesktop *desktop);
};

}
}
}

#endif /* !SEEN_ERASOR_TOOLBAR_H */

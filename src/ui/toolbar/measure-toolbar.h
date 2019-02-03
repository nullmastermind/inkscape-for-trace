// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_MEASURE_TOOLBAR_H
#define SEEN_MEASURE_TOOLBAR_H

/**
 * @file
 * Measure aux toolbar
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

namespace Inkscape {
namespace UI {
namespace Widget {
class UnitTracker;
}

namespace Toolbar {
class MeasureToolbar : public Toolbar {
private:
    UI::Widget::UnitTracker *_tracker;
    Glib::RefPtr<Gtk::Adjustment> _font_size_adj;
    Glib::RefPtr<Gtk::Adjustment> _precision_adj;
    Glib::RefPtr<Gtk::Adjustment> _scale_adj;
    Glib::RefPtr<Gtk::Adjustment> _offset_adj;

    Gtk::ToggleToolButton *_only_selected_item;
    Gtk::ToggleToolButton *_ignore_1st_and_last_item;
    Gtk::ToggleToolButton *_inbetween_item;
    Gtk::ToggleToolButton *_show_hidden_item;
    Gtk::ToggleToolButton *_all_layers_item;

    Gtk::ToolButton *_reverse_item;
    Gtk::ToolButton *_to_phantom_item;
    Gtk::ToolButton *_to_guides_item;
    Gtk::ToolButton *_to_item_item;
    Gtk::ToolButton *_mark_dimension_item;

    void fontsize_value_changed();
    void unit_changed(int notUsed);
    void precision_value_changed();
    void scale_value_changed();
    void offset_value_changed();
    void toggle_only_selected();
    void toggle_ignore_1st_and_last();
    void toggle_show_hidden();
    void toggle_show_in_between();
    void toggle_all_layers();
    void reverse_knots();
    void to_phantom();
    void to_guides();
    void to_item();
    void to_mark_dimension();

protected:
    MeasureToolbar(SPDesktop *desktop);

public:
    static GtkWidget * create(SPDesktop *desktop);
};

}
}
}

#endif /* !SEEN_MEASURE_TOOLBAR_H */

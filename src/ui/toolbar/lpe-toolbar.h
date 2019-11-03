// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_LPE_TOOLBAR_H
#define SEEN_LPE_TOOLBAR_H

/**
 * @file
 * LPE aux toolbar
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
class SPLPEItem;

namespace Gtk {
class RadioToolButton;
}

namespace Inkscape {
class Selection;

namespace LivePathEffect {
class Effect;
}

namespace UI {
namespace Tools {
class ToolBase;
}

namespace Widget {
class ComboToolItem;
class UnitTracker;
}

namespace Toolbar {
class LPEToolbar : public Toolbar {
private:
    std::unique_ptr<UI::Widget::UnitTracker> _tracker;
    std::vector<Gtk::RadioToolButton *> _mode_buttons;
    Gtk::ToggleToolButton *_show_bbox_item;
    Gtk::ToggleToolButton *_bbox_from_selection_item;
    Gtk::ToggleToolButton *_measuring_item;
    Gtk::ToggleToolButton *_open_lpe_dialog_item;
    UI::Widget::ComboToolItem *_line_segment_combo;
    UI::Widget::ComboToolItem *_units_item;

    bool _freeze;

    LivePathEffect::Effect *_currentlpe;
    SPLPEItem *_currentlpeitem;

    sigc::connection c_selection_modified;
    sigc::connection c_selection_changed;

    void mode_changed(int mode);
    void unit_changed(int not_used);
    void sel_modified(Inkscape::Selection *selection, guint flags);
    void sel_changed(Inkscape::Selection *selection);
    void change_line_segment_type(int mode);
    void watch_ec(SPDesktop* desktop, UI::Tools::ToolBase* ec);

    void toggle_show_bbox();
    void toggle_set_bbox();
    void toggle_show_measuring_info();
    void open_lpe_dialog();

protected:
    LPEToolbar(SPDesktop *desktop);

public:
    static GtkWidget * create(SPDesktop *desktop);
    void set_mode(int mode);
};

}
}
}

#endif /* !SEEN_LPE_TOOLBAR_H */

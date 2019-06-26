// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_TEXT_TOOLBAR_H
#define SEEN_TEXT_TOOLBAR_H

/**
 * @file
 * Text aux toolbar
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

#include <gtkmm/popover.h>
#include <gtkmm/box.h>
#include <gtkmm/separatortoolitem.h>
#include <gtkmm/adjustment.h>

class SPDesktop;

namespace Gtk {
class ComboBoxText;
class ToggleToolButton;
}

namespace Inkscape {
class Selection;

namespace UI {
namespace Tools {
class ToolBase;
}

namespace Widget {
class ComboBoxEntryToolItem;
class ComboToolItem;
class SpinButtonToolItem;
class UnitTracker;
}

namespace Toolbar {
class TextToolbar : public Toolbar {
private:
    bool _freeze;
    bool _text_style_from_prefs;

    UI::Widget::UnitTracker *_tracker;

    UI::Widget::ComboBoxEntryToolItem *_font_family_item;
    UI::Widget::ComboBoxEntryToolItem *_font_size_item;
    UI::Widget::ComboBoxEntryToolItem *_font_style_item;
    Gtk::Popover *_line_spacing_menu;
    Gtk::Box *_line_spacing_menu_content;
    Gtk::ToolButton *_line_spacing_defaulting;
    Gtk::ToggleToolButton *_line_height_unset_item;
    Gtk::ToggleToolButton *_line_spacing_menu_launcher;
    UI::Widget::ComboToolItem *_line_height_units_item;
    UI::Widget::SpinButtonToolItem *_line_height_item;
    UI::Widget::ComboToolItem *_line_spacing_item;
    Gtk::ToggleToolButton *_superscript_item;
    Gtk::ToggleToolButton *_subscript_item;
    Gtk::ToggleToolButton *_outer_style_item;
    

    UI::Widget::ComboToolItem *_align_item;
    UI::Widget::ComboToolItem *_writing_mode_item;
    UI::Widget::ComboToolItem *_orientation_item;
    UI::Widget::ComboToolItem *_direction_item;

    UI::Widget::SpinButtonToolItem *_word_spacing_item;
    UI::Widget::SpinButtonToolItem *_letter_spacing_item;
    UI::Widget::SpinButtonToolItem *_dx_item;
    UI::Widget::SpinButtonToolItem *_dy_item;
    UI::Widget::SpinButtonToolItem *_rotation_item;

    Glib::RefPtr<Gtk::Adjustment> _line_height_adj;
    Glib::RefPtr<Gtk::Adjustment> _word_spacing_adj;
    Glib::RefPtr<Gtk::Adjustment> _letter_spacing_adj;
    Glib::RefPtr<Gtk::Adjustment> _dx_adj;
    Glib::RefPtr<Gtk::Adjustment> _dy_adj;
    Glib::RefPtr<Gtk::Adjustment> _rotation_adj;

    int _lineheight_unit;

    sigc::connection c_selection_changed;
    sigc::connection c_selection_modified;
    sigc::connection c_subselection_changed;

    void fontfamily_value_changed();
    void fontsize_value_changed();
    void fontstyle_value_changed();
    void script_changed(Gtk::ToggleToolButton *btn);
    void lineheight_unset_changed();
    void outer_style_changed();
    void align_mode_changed(int mode);
    void writing_mode_changed(int mode);
    void orientation_changed(int mode);
    void direction_changed(int mode);
    void lineheight_value_changed();
    void lineheight_unit_changed(int not_used);
    void lineheight_defaulting();
    void line_height_popover_closed();
    void line_spacing_mode_changed(int mode);
    void wordspacing_value_changed();
    void letterspacing_value_changed();
    void dx_value_changed();
    void dy_value_changed();
    void rotation_value_changed();
    void selection_changed(Inkscape::Selection *selection,
                           bool subselection = false);
    void selection_modified(Inkscape::Selection *selection, guint flags);
    void subselection_changed(gpointer tc);
    void watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec);
    void set_sizes(int unit);
    void poptoggle(Gtk::ToggleToolButton *btn);
protected:
    TextToolbar(SPDesktop *desktop);

public:
    static GtkWidget * create(SPDesktop *desktop);
};

}
}
}

#endif /* !SEEN_TEXT_TOOLBAR_H */

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

#include "object/sp-item.h"
#include "object/sp-object.h"
#include "toolbar.h"
#include "text-editing.h"
#include "style.h"
#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>
#include <gtkmm/popover.h>
#include <gtkmm/separatortoolitem.h>
#include <sigc++/connection.h>

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
    UI::Widget::UnitTracker *_tracker_fs;

    UI::Widget::ComboBoxEntryToolItem *_font_family_item;
    UI::Widget::ComboBoxEntryToolItem *_font_size_item;
    UI::Widget::ComboToolItem *_font_size_units_item;
    UI::Widget::ComboBoxEntryToolItem *_font_style_item;
    UI::Widget::ComboToolItem *_line_height_units_item;
    UI::Widget::SpinButtonToolItem *_line_height_item;
    Gtk::ToggleToolButton *_superscript_item;
    Gtk::ToggleToolButton *_subscript_item;

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
    bool _outer;
    SPItem *_sub_active_item;
    int _lineheight_unit;
    Inkscape::Text::Layout::iterator wrap_start;
    Inkscape::Text::Layout::iterator wrap_end;
    bool _updating;
    int _cusor_numbers;
    SPStyle _query_cursor;
    double selection_fontsize;
    sigc::connection c_selection_changed;
    sigc::connection c_selection_modified;
    sigc::connection c_selection_modified_select_tool;
    sigc::connection c_subselection_changed;
    void text_outer_set_style(SPCSSAttr *css);
    void fontfamily_value_changed();
    void fontsize_value_changed();
    void subselection_wrap_toggle(bool start);
    void fontstyle_value_changed();
    void script_changed(Gtk::ToggleToolButton *btn);
    void align_mode_changed(int mode);
    void writing_mode_changed(int mode);
    void orientation_changed(int mode);
    void direction_changed(int mode);
    void lineheight_value_changed();
    void lineheight_unit_changed(int not_used);
    void wordspacing_value_changed();
    void letterspacing_value_changed();
    void dx_value_changed();
    void dy_value_changed();
    void prepare_inner();
    void focus_text();
    void rotation_value_changed();
    void fontsize_unit_changed(int not_used);
    void selection_changed(Inkscape::Selection *selection);
    void selection_modified(Inkscape::Selection *selection, guint flags);
    void selection_modified_select_tool(Inkscape::Selection *selection, guint flags);
    void subselection_changed(gpointer texttool);
    void watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec);
    void set_sizes(int unit);
    Inkscape::XML::Node *unindent_node(Inkscape::XML::Node *repr, Inkscape::XML::Node *before);

  protected:
    TextToolbar(SPDesktop *desktop);

public:
    static GtkWidget * create(SPDesktop *desktop);
};
}
}
}

#endif /* !SEEN_TEXT_TOOLBAR_H */

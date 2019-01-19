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

#include <gtkmm/adjustment.h>

class InkSelectOneAction;
class SPDesktop;

typedef struct _EgeAdjustmentAction EgeAdjustmentAction;
typedef struct _GtkActionGroup GtkActionGroup;
typedef struct _Ink_ComboBoxEntry_Action Ink_ComboBoxEntry_Action;
typedef struct _InkToggleAction InkToggleAction;

namespace Inkscape {
class Selection;

namespace UI {
namespace Tools {
class ToolBase;
}

namespace Widget {
class UnitTracker;
}

namespace Toolbar {
class TextToolbar : public Toolbar {
private:
    bool _freeze;
    bool _text_style_from_prefs;

    UI::Widget::UnitTracker *_tracker;

    Ink_ComboBoxEntry_Action *_font_family_action;
    Ink_ComboBoxEntry_Action *_font_size_action;
    Ink_ComboBoxEntry_Action *_font_style_action;
    InkToggleAction *_superscript_action;
    InkToggleAction *_subscript_action;
    InkToggleAction *_outer_style_action;
    InkToggleAction *_line_height_unset_action;
    InkSelectOneAction *_align_action;
    InkSelectOneAction *_writing_mode_action;
    InkSelectOneAction *_orientation_action;
    InkSelectOneAction *_direction_action;
    InkSelectOneAction *_line_height_units_action;
    InkSelectOneAction *_line_spacing_action;

    EgeAdjustmentAction *_line_height_action;
    EgeAdjustmentAction *_word_spacing_action;
    EgeAdjustmentAction *_letter_spacing_action;
    EgeAdjustmentAction *_dx_action;
    EgeAdjustmentAction *_dy_action;
    EgeAdjustmentAction *_rotation_action;

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

    static void fontfamily_value_changed(Ink_ComboBoxEntry_Action *act,
                                         gpointer                  data);
    static void fontsize_value_changed  (Ink_ComboBoxEntry_Action *act,
                                         gpointer                  data);
    static void fontstyle_value_changed (Ink_ComboBoxEntry_Action *act,
                                         gpointer                  data);
    static void script_changed          (InkToggleAction          *act,
                                         gpointer                  data);
    static void lineheight_unset_changed(InkToggleAction          *act,
                                         gpointer                  data);
    static void outer_style_changed     (InkToggleAction          *act,
                                         gpointer                  data);

    void align_mode_changed(int mode);
    void writing_mode_changed(int mode);
    void orientation_changed(int mode);
    void direction_changed(int mode);
    void lineheight_value_changed();
    void lineheight_unit_changed(int not_used);
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

protected:
    TextToolbar(SPDesktop *desktop);

public:
    static GtkWidget * prep(SPDesktop *desktop, GtkActionGroup* mainActions);
};

}
}
}

#endif /* !SEEN_TEXT_TOOLBAR_H */

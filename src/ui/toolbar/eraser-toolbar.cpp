// SPDX-License-Identifier: GPL-2.0-or-later
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

#include "eraser-toolbar.h"

#include <array>

#include <glibmm/i18n.h>

#include <gtkmm/radiotoolbutton.h>
#include <gtkmm/separatortoolitem.h>

#include "desktop.h"
#include "document-undo.h"
#include "ui/icon-names.h"
#include "ui/simple-pref-pusher.h"
#include "ui/tools/eraser-tool.h"

#include "ui/widget/canvas.h"  // Focus widget
#include "ui/widget/spin-button-tool-item.h"

using Inkscape::DocumentUndo;

namespace Inkscape {
namespace UI {
namespace Toolbar {
EraserToolbar::EraserToolbar(SPDesktop *desktop)
    : Toolbar(desktop),
    _freeze(false)
{
    gint eraser_mode = ERASER_MODE_DELETE;
    auto prefs = Inkscape::Preferences::get();

    // Mode
    {
        add_label(_("Mode:"));

        Gtk::RadioToolButton::Group mode_group;

        std::vector<Gtk::RadioToolButton *> mode_buttons;

        auto delete_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Delete")));
        delete_btn->set_tooltip_text(_("Delete objects touched by eraser"));
        delete_btn->set_icon_name(INKSCAPE_ICON("draw-eraser-delete-objects"));
        mode_buttons.push_back(delete_btn);

        auto cut_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Cut")));
        cut_btn->set_tooltip_text(_("Cut out from paths and shapes"));
        cut_btn->set_icon_name(INKSCAPE_ICON("path-difference"));
        mode_buttons.push_back(cut_btn);

        auto clip_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Clip")));
        clip_btn->set_tooltip_text(_("Clip from objects"));
        clip_btn->set_icon_name(INKSCAPE_ICON("path-intersection"));
        mode_buttons.push_back(clip_btn);

        eraser_mode = prefs->getInt("/tools/eraser/mode", ERASER_MODE_CLIP); // Used at end

        mode_buttons[eraser_mode]->set_active();

        int btn_index = 0;

        for (auto btn : mode_buttons)
        {
            add(*btn);
            btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &EraserToolbar::mode_changed), btn_index++));
        }
    }

    _separators.push_back(Gtk::manage(new Gtk::SeparatorToolItem()));
    add(*_separators.back());

    /* Width */
    {
        std::vector<Glib::ustring> labels = {_("(no width)"), _("(hairline)"), "", "", "", _("(default)"), "", "", "", "", _("(broad stroke)")};
        std::vector<double>        values = {              0,               1,  3,  5, 10,             15, 20, 30, 50, 75,                 100};
        auto width_val = prefs->getDouble("/tools/eraser/width", 15);
        _width_adj = Gtk::Adjustment::create(width_val, 0, 100, 1.0, 10.0);
        _width = Gtk::manage(new UI::Widget::SpinButtonToolItem("eraser-width", _("Width:"), _width_adj, 1, 0));
        _width->set_tooltip_text(_("The width of the eraser pen (relative to the visible canvas area)"));
        _width->set_focus_widget(desktop->canvas);
        _width->set_custom_numeric_menu_data(values, labels);
        _width_adj->signal_value_changed().connect(sigc::mem_fun(*this, &EraserToolbar::width_value_changed));
        // TODO: Allow SpinButtonToolItem to display as a slider
        // ege_adjustment_action_set_appearance( toolbar->_width, TOOLBAR_SLIDER_HINT );
        add(*_width);
        _width->set_sensitive(true);
    }

    /* Use Pressure button */
    {
        _usepressure = add_toggle_button(_("Eraser Pressure"),
                                         _("Use the pressure of the input device to alter the width of the pen"));
        _usepressure->set_icon_name(INKSCAPE_ICON("draw-use-pressure"));
        _pressure_pusher.reset(new UI::SimplePrefPusher(_usepressure, "/tools/eraser/usepressure"));
        _usepressure->signal_toggled().connect(sigc::mem_fun(*this, &EraserToolbar::usepressure_toggled));
    }

    _separators.push_back(Gtk::manage(new Gtk::SeparatorToolItem()));
    add(*_separators.back());

    /* Thinning */
    {
        std::vector<Glib::ustring> labels = {_("(speed blows up stroke)"),  "",  "", _("(slight widening)"), _("(constant width)"), _("(slight thinning, default)"), "", "", _("(speed deflates stroke)")};
        std::vector<double>        values = {                        -100, -40, -20,                    -10,                     0,                              10, 20, 40,                          100};
        auto thinning_val = prefs->getDouble("/tools/eraser/thinning", 10);
        _thinning_adj = Gtk::Adjustment::create(thinning_val, -100, 100, 1, 10.0);
        _thinning = Gtk::manage(new UI::Widget::SpinButtonToolItem("eraser-thinning", _("Thinning:"), _thinning_adj, 1, 0));
        _thinning->set_tooltip_text(_("How much velocity thins the stroke (> 0 makes fast strokes thinner, < 0 makes them broader, 0 makes width independent of velocity)"));
        _thinning->set_custom_numeric_menu_data(values, labels);
        _thinning->set_focus_widget(desktop->canvas);
        _thinning_adj->signal_value_changed().connect(sigc::mem_fun(*this, &EraserToolbar::velthin_value_changed));
        add(*_thinning);
        _thinning->set_sensitive(true);
    }

    _separators.push_back(Gtk::manage(new Gtk::SeparatorToolItem()));
    add(*_separators.back());

    /* Cap Rounding */
    {
        std::vector<Glib::ustring> labels = {_("(blunt caps, default)"), _("(slightly bulging)"),  "",  "", _("(approximately round)"), _("(long protruding caps)")};
        std::vector<double>        values = {                         0,                     0.3, 0.5, 1.0,                        1.4,                         5.0};
        auto cap_rounding_val = prefs->getDouble("/tools/eraser/cap_rounding", 0.0);
        _cap_rounding_adj = Gtk::Adjustment::create(cap_rounding_val, 0.0, 5.0, 0.01, 0.1);
        // TRANSLATORS: "cap" means "end" (both start and finish) here
        _cap_rounding = Gtk::manage(new UI::Widget::SpinButtonToolItem("eraser-cap-rounding", _("Caps:"), _cap_rounding_adj, 0.01, 2));
        _cap_rounding->set_tooltip_text(_("Increase to make caps at the ends of strokes protrude more (0 = no caps, 1 = round caps)"));
        _cap_rounding->set_custom_numeric_menu_data(values, labels);
        _cap_rounding->set_focus_widget(desktop->canvas);
        _cap_rounding_adj->signal_value_changed().connect(sigc::mem_fun(*this, &EraserToolbar::cap_rounding_value_changed));
        add(*_cap_rounding);
        _cap_rounding->set_sensitive(true);
    }

    _separators.push_back(Gtk::manage(new Gtk::SeparatorToolItem()));
    add(*_separators.back());

    /* Tremor */
    {
        std::vector<Glib::ustring> labels = {_("(smooth line)"), _("(slight tremor)"), _("(noticeable tremor)"), "", "", _("(maximum tremor)")};
        std::vector<double>        values = {                 0,                   10,                       20, 40, 60,                   100};
        auto tremor_val = prefs->getDouble("/tools/eraser/tremor", 0.0);
        _tremor_adj = Gtk::Adjustment::create(tremor_val, 0.0, 100, 1, 10.0);
        _tremor = Gtk::manage(new UI::Widget::SpinButtonToolItem("eraser-tremor", _("Tremor:"), _tremor_adj, 1, 0));
        _tremor->set_tooltip_text(_("Increase to make strokes rugged and trembling"));
        _tremor->set_custom_numeric_menu_data(values, labels);
        _tremor->set_focus_widget(desktop->canvas);
        _tremor_adj->signal_value_changed().connect(sigc::mem_fun(*this, &EraserToolbar::tremor_value_changed));

        // TODO: Allow slider appearance
        //ege_adjustment_action_set_appearance( toolbar->_tremor, TOOLBAR_SLIDER_HINT );
        add(*_tremor);
        _tremor->set_sensitive(true);
    }

    _separators.push_back(Gtk::manage(new Gtk::SeparatorToolItem()));
    add(*_separators.back());

    /* Mass */
    {
        std::vector<Glib::ustring> labels = {_("(no inertia)"), _("(slight smoothing, default)"), _("(noticeable lagging)"), "", "", _("(maximum inertia)")};
        std::vector<double>        values = {              0.0,                                2,                        10, 20, 50,                    100};
        auto mass_val = prefs->getDouble("/tools/eraser/mass", 10.0);
        _mass_adj = Gtk::Adjustment::create(mass_val, 0.0, 100, 1, 10.0);
        _mass = Gtk::manage(new UI::Widget::SpinButtonToolItem("eraser-mass", _("Mass:"), _mass_adj, 1, 0));
        _mass->set_tooltip_text(_("Increase to make the eraser drag behind, as if slowed by inertia"));
        _mass->set_custom_numeric_menu_data(values, labels);
        _mass->set_focus_widget(desktop->canvas);
        _mass_adj->signal_value_changed().connect(sigc::mem_fun(*this, &EraserToolbar::mass_value_changed));
        // TODO: Allow slider appearance
        //ege_adjustment_action_set_appearance( toolbar->_mass, TOOLBAR_SLIDER_HINT );
        add(*_mass);
        _mass->set_sensitive(true);
    }

    _separators.push_back(Gtk::manage(new Gtk::SeparatorToolItem()));
    add(*_separators.back());

    /* Overlap */
    {
        _split = add_toggle_button(_("Break apart cut items"),
                                   _("Break apart cut items"));
        _split->set_icon_name(INKSCAPE_ICON("distribute-randomize"));
        _split->set_active( prefs->getBool("/tools/eraser/break_apart", false) );
        _split->signal_toggled().connect(sigc::mem_fun(*this, &EraserToolbar::toggle_break_apart));
    }

    show_all();

    set_eraser_mode_visibility(eraser_mode);
}

GtkWidget *
EraserToolbar::create(SPDesktop *desktop)
{
    auto toolbar = new EraserToolbar(desktop);
    return GTK_WIDGET(toolbar->gobj());
}

void
EraserToolbar::mode_changed(int mode)
{
    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setInt( "/tools/eraser/mode", mode );
    }

    set_eraser_mode_visibility(mode);

    // only take action if run by the attr_changed listener
    if (!_freeze) {
        // in turn, prevent listener from responding
        _freeze = true;

        /*
        if ( eraser_mode != ERASER_MODE_DELETE ) {
        } else {
        }
        */
        // TODO finish implementation

        _freeze =  false;
    }
}

void
EraserToolbar::set_eraser_mode_visibility(const guint eraser_mode)
{
    _split->set_visible((eraser_mode == ERASER_MODE_CUT));

    const gboolean visibility = (eraser_mode != ERASER_MODE_DELETE);

    const std::array<Gtk::Widget *, 6> arr = {_cap_rounding,
                                              _mass,
                                              _thinning,
                                              _tremor,
                                              _usepressure,
                                              _width};
    for (auto widget : arr) {
        widget->set_visible(visibility);
    }

    for (auto separator : _separators) {
        separator->set_visible(visibility);
    }
}

void
EraserToolbar::width_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/eraser/width", _width_adj->get_value() );
}

void
EraserToolbar::mass_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/eraser/mass", _mass_adj->get_value() );
}

void
EraserToolbar::velthin_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble("/tools/eraser/thinning", _thinning_adj->get_value() );
}

void
EraserToolbar::cap_rounding_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/eraser/cap_rounding", _cap_rounding_adj->get_value() );
}

void
EraserToolbar::tremor_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/eraser/tremor", _tremor_adj->get_value() );
}

void
EraserToolbar::toggle_break_apart()
{
    auto prefs = Inkscape::Preferences::get();
    bool active = _split->get_active();
    prefs->setBool("/tools/eraser/break_apart", active);
}

void
EraserToolbar::usepressure_toggled()
{
    auto prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/eraser/usepressure", _usepressure->get_active());
}

}
}
}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

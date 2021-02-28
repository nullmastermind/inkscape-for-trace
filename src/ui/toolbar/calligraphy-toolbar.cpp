// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Calligraphy aux toolbar
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

#include "calligraphy-toolbar.h"

#include <glibmm/i18n.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/separatortoolitem.h>

#include "desktop.h"
#include "document-undo.h"
#include "ui/dialog/calligraphic-profile-rename.h"
#include "ui/icon-names.h"
#include "ui/simple-pref-pusher.h"
#include "ui/uxmanager.h"
#include "ui/widget/canvas.h"
#include "ui/widget/combo-tool-item.h"
#include "ui/widget/spin-button-tool-item.h"
#include "ui/widget/unit-tracker.h"

using Inkscape::DocumentUndo;
using Inkscape::UI::Widget::UnitTracker;
using Inkscape::Util::Quantity;
using Inkscape::Util::Unit;
using Inkscape::Util::unit_table;

std::vector<Glib::ustring> get_presets_list() {

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    std::vector<Glib::ustring> presets = prefs->getAllDirs("/tools/calligraphic/preset");

    return presets;
}

namespace Inkscape {
namespace UI {
namespace Toolbar {

CalligraphyToolbar::CalligraphyToolbar(SPDesktop *desktop)
    : Toolbar(desktop)
    , _tracker(new UnitTracker(Inkscape::Util::UNIT_TYPE_LINEAR))
    , _presets_blocked(false)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _tracker->prependUnit(unit_table.getUnit("px"));
    _tracker->changeLabel("%", 0, true);
    prefs->getBool("/tools/calligraphic/abs_width")
        ? _tracker->setActiveUnitByLabel(prefs->getString("/tools/calligraphic/unit"))
        : _tracker->setActiveUnitByLabel("%");

    /*calligraphic profile */
    {
        _profile_selector_combo = Gtk::manage(new Gtk::ComboBoxText());
        _profile_selector_combo->set_tooltip_text(_("Choose a preset"));

        build_presets_list();

        auto profile_selector_ti = Gtk::manage(new Gtk::ToolItem());
        profile_selector_ti->add(*_profile_selector_combo);
        add(*profile_selector_ti);

        _profile_selector_combo->signal_changed().connect(sigc::mem_fun(*this, &CalligraphyToolbar::change_profile));
    }

    /*calligraphic profile editor */
    {
        auto profile_edit_item = Gtk::manage(new Gtk::ToolButton(_("Add/Edit Profile")));
        profile_edit_item->set_tooltip_text(_("Add or edit calligraphic profile"));
        profile_edit_item->set_icon_name(INKSCAPE_ICON("document-properties"));
        profile_edit_item->signal_clicked().connect(sigc::mem_fun(*this, &CalligraphyToolbar::edit_profile));
        add(*profile_edit_item);
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    {
        /* Width */
        std::vector<Glib::ustring> labels = {_("(hairline)"), "", "", "", _("(default)"), "", "", "", "", _("(broad stroke)")};
        std::vector<double>        values = {              1,  3,  5, 10,             15, 20, 30, 50, 75,                 100};
        auto width_val = prefs->getDouble("/tools/calligraphic/width", 15.118);
        Unit const *unit = unit_table.getUnit(prefs->getString("/tools/calligraphic/unit"));
        _width_adj = Gtk::Adjustment::create(Quantity::convert(width_val, "px", unit), 0.001, 100, 1.0, 10.0);
        auto width_item =
            Gtk::manage(new UI::Widget::SpinButtonToolItem("calligraphy-width", _("Width:"), _width_adj, 0.001, 3));
        width_item->set_tooltip_text(_("The width of the calligraphic pen (relative to the visible canvas area)"));
        width_item->set_custom_numeric_menu_data(values, labels);
        width_item->set_focus_widget(desktop->canvas);
        _width_adj->signal_value_changed().connect(sigc::mem_fun(*this, &CalligraphyToolbar::width_value_changed));
        _widget_map["width"] = G_OBJECT(_width_adj->gobj());
        // ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        add(*width_item);
        _tracker->addAdjustment(_width_adj->gobj());
        width_item->set_sensitive(true);
    }

    /* Unit Menu */
    {
        auto unit_menu_ti = _tracker->create_tool_item(_("Units"), "");
        add(*unit_menu_ti);
        unit_menu_ti->signal_changed_after().connect(sigc::mem_fun(*this, &CalligraphyToolbar::unit_changed));
    }

    /* Use Pressure button */
    {
        _usepressure = add_toggle_button(_("Pressure"),
                                         _("Use the pressure of the input device to alter the width of the pen"));
        _usepressure->set_icon_name(INKSCAPE_ICON("draw-use-pressure"));
        _widget_map["usepressure"] = G_OBJECT(_usepressure->gobj());
        _usepressure_pusher.reset(new SimplePrefPusher(_usepressure, "/tools/calligraphic/usepressure"));
        _usepressure->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &CalligraphyToolbar::on_pref_toggled),
                                                          _usepressure,
                                                          "/tools/calligraphic/usepressure"));
    }

    /* Trace Background button */
    {
        _tracebackground = add_toggle_button(_("Trace Background"),
                                            _("Trace the lightness of the background by the width of the pen (white - minimum width, black - maximum width)"));
        _tracebackground->set_icon_name(INKSCAPE_ICON("draw-trace-background"));
        _widget_map["tracebackground"] = G_OBJECT(_tracebackground->gobj());
        _tracebackground_pusher.reset(new SimplePrefPusher(_tracebackground, "/tools/calligraphic/tracebackground"));
        _tracebackground->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &CalligraphyToolbar::on_pref_toggled),
                                                              _tracebackground,
                                                              "/tools/calligraphic/tracebackground"));
    }

    {
        /* Thinning */
        std::vector<Glib::ustring> labels = {_("(speed blows up stroke)"),  "",  "", _("(slight widening)"), _("(constant width)"), _("(slight thinning, default)"), "", "", _("(speed deflates stroke)")};
        std::vector<double>        values = {                        -100, -40, -20,                    -10,                     0,                              10, 20, 40,                          100};
        auto thinning_val = prefs->getDouble("/tools/calligraphic/thinning", 10);
        _thinning_adj = Gtk::Adjustment::create(thinning_val, -100, 100, 1, 10.0);
        auto thinning_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("calligraphy-thinning", _("Thinning:"), _thinning_adj, 1, 0));
        thinning_item->set_tooltip_text(("How much velocity thins the stroke (> 0 makes fast strokes thinner, < 0 makes them broader, 0 makes width independent of velocity)"));
        thinning_item->set_custom_numeric_menu_data(values, labels);
        thinning_item->set_focus_widget(desktop->canvas);
        _thinning_adj->signal_value_changed().connect(sigc::mem_fun(*this, &CalligraphyToolbar::velthin_value_changed));
        _widget_map["thinning"] = G_OBJECT(_thinning_adj->gobj());
        add(*thinning_item);
        thinning_item->set_sensitive(true);
    }

    {
        /* Mass */
        std::vector<Glib::ustring> labels = {_("(no inertia)"), _("(slight smoothing, default)"), _("(noticeable lagging)"), "", "", _("(maximum inertia)")};
        std::vector<double>        values = {              0.0,                                2,                        10, 20, 50,                    100};
        auto mass_val = prefs->getDouble("/tools/calligraphic/mass", 2.0);
        _mass_adj = Gtk::Adjustment::create(mass_val, 0.0, 100, 1, 10.0);
        auto mass_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("calligraphy-mass", _("Mass:"), _mass_adj, 1, 0));
        mass_item->set_tooltip_text(_("Increase to make the pen drag behind, as if slowed by inertia"));
        mass_item->set_custom_numeric_menu_data(values, labels);
        mass_item->set_focus_widget(desktop->canvas);
        _mass_adj->signal_value_changed().connect(sigc::mem_fun(*this, &CalligraphyToolbar::mass_value_changed));
        _widget_map["mass"] = G_OBJECT(_mass_adj->gobj());
        // ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        add(*mass_item);
        mass_item->set_sensitive(true);
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    {
        /* Angle */
        std::vector<Glib::ustring> labels = {_("(left edge up)"),  "",  "", _("(horizontal)"), _("(default)"), "", _("(right edge up)")};
        std::vector<double>        values = {                -90, -60, -30,                 0,             30, 60,                   90};
        auto angle_val = prefs->getDouble("/tools/calligraphic/angle", 30);
        _angle_adj = Gtk::Adjustment::create(angle_val, -90.0, 90.0, 1.0, 10.0);
        _angle_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("calligraphy-angle", _("Angle:"), _angle_adj, 1, 0));
        _angle_item->set_tooltip_text(_("The angle of the pen's nib (in degrees; 0 = horizontal; has no effect if fixation = 0)"));
        _angle_item->set_custom_numeric_menu_data(values, labels);
        _angle_item->set_focus_widget(desktop->canvas);
        _angle_adj->signal_value_changed().connect(sigc::mem_fun(*this, &CalligraphyToolbar::angle_value_changed));
        _widget_map["angle"] = G_OBJECT(_angle_adj->gobj());
        add(*_angle_item);
        _angle_item->set_sensitive(true);
    }

    /* Use Tilt button */
    {
        _usetilt = add_toggle_button(_("Tilt"),
                                     _("Use the tilt of the input device to alter the angle of the pen's nib"));
        _usetilt->set_icon_name(INKSCAPE_ICON("draw-use-tilt"));
        _widget_map["usetilt"] = G_OBJECT(_usetilt->gobj());
        _usetilt_pusher.reset(new SimplePrefPusher(_usetilt, "/tools/calligraphic/usetilt"));
        _usetilt->signal_toggled().connect(sigc::mem_fun(*this, &CalligraphyToolbar::tilt_state_changed));
        _angle_item->set_sensitive(!prefs->getBool("/tools/calligraphic/usetilt", true));
        _usetilt->set_active(prefs->getBool("/tools/calligraphic/usetilt", true));
    }

    {
        /* Fixation */
        std::vector<Glib::ustring> labels = {_("(perpendicular to stroke, \"brush\")"), "", "", "", _("(almost fixed, default)"), _("(fixed by Angle, \"pen\")")};
        std::vector<double>        values = {                                        0, 20, 40, 60,                           90,                            100};
        auto flatness_val = prefs->getDouble("/tools/calligraphic/flatness", 90);
        _fixation_adj = Gtk::Adjustment::create(flatness_val, 0.0, 100, 1.0, 10.0);
        auto flatness_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("calligraphy-fixation", _("Fixation:"), _fixation_adj, 1, 0));
        flatness_item->set_tooltip_text(_("Angle behavior (0 = nib always perpendicular to stroke direction, 100 = fixed angle)"));
        flatness_item->set_custom_numeric_menu_data(values, labels);
        flatness_item->set_focus_widget(desktop->canvas);
        _fixation_adj->signal_value_changed().connect(sigc::mem_fun(*this, &CalligraphyToolbar::flatness_value_changed));
        _widget_map["flatness"] = G_OBJECT(_fixation_adj->gobj());
        add(*flatness_item);
        flatness_item->set_sensitive(true);
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    {
        /* Cap Rounding */
        std::vector<Glib::ustring> labels = {_("(blunt caps, default)"), _("(slightly bulging)"),  "",  "", _("(approximately round)"), _("(long protruding caps)")};
        std::vector<double>        values = {                         0,                     0.3, 0.5, 1.0,                        1.4,                         5.0};
        auto cap_rounding_val = prefs->getDouble("/tools/calligraphic/cap_rounding", 0.0);
        _cap_rounding_adj = Gtk::Adjustment::create(cap_rounding_val, 0.0, 5.0, 0.01, 0.1);
        auto cap_rounding_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("calligraphy-cap-rounding", _("Caps:"), _cap_rounding_adj, 0.01, 2));

        // TRANSLATORS: "cap" means "end" (both start and finish) here
        cap_rounding_item->set_tooltip_text(_("Increase to make caps at the ends of strokes protrude more (0 = no caps, 1 = round caps)"));
        cap_rounding_item->set_custom_numeric_menu_data(values, labels);
        cap_rounding_item->set_focus_widget(desktop->canvas);
        _cap_rounding_adj->signal_value_changed().connect(sigc::mem_fun(*this, &CalligraphyToolbar::cap_rounding_value_changed));
        _widget_map["cap_rounding"] = G_OBJECT(_cap_rounding_adj->gobj());
        add(*cap_rounding_item);
        cap_rounding_item->set_sensitive(true);
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    {
        /* Tremor */
        std::vector<Glib::ustring> labels = {_("(smooth line)"), _("(slight tremor)"), _("(noticeable tremor)"), "", "", _("(maximum tremor)")};
        std::vector<double>        values = {                 0,                   10,                       20, 40, 60,                   100};
        auto tremor_val = prefs->getDouble("/tools/calligraphic/tremor", 0.0);
        _tremor_adj = Gtk::Adjustment::create(tremor_val, 0.0, 100, 1, 10.0);
        auto tremor_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("calligraphy-tremor", _("Tremor:"), _tremor_adj, 1, 0));
        tremor_item->set_tooltip_text(_("Increase to make strokes rugged and trembling"));
        tremor_item->set_custom_numeric_menu_data(values, labels);
        tremor_item->set_focus_widget(desktop->canvas);
        _tremor_adj->signal_value_changed().connect(sigc::mem_fun(*this, &CalligraphyToolbar::tremor_value_changed));
        _widget_map["tremor"] = G_OBJECT(_tremor_adj->gobj());
        // ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        add(*tremor_item);
        tremor_item->set_sensitive(true);
    }

    {
        /* Wiggle */
        std::vector<Glib::ustring> labels = {_("(no wiggle)"), _("(slight deviation)"), "", "", _("(wild waves and curls)")};
        std::vector<double>        values = {               0,                      20, 40, 60,                         100};
        auto wiggle_val = prefs->getDouble("/tools/calligraphic/wiggle", 0.0);
        _wiggle_adj = Gtk::Adjustment::create(wiggle_val, 0.0, 100, 1, 10.0);
        auto wiggle_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("calligraphy-wiggle", _("Wiggle:"), _wiggle_adj, 1, 0));
        wiggle_item->set_tooltip_text(_("Increase to make the pen waver and wiggle"));
        wiggle_item->set_custom_numeric_menu_data(values, labels);
        wiggle_item->set_focus_widget(desktop->canvas);
        _wiggle_adj->signal_value_changed().connect(sigc::mem_fun(*this, &CalligraphyToolbar::wiggle_value_changed));
        _widget_map["wiggle"] = G_OBJECT(_wiggle_adj->gobj());
        // ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        add(*wiggle_item);
        wiggle_item->set_sensitive(true);
    }

    show_all();
}

GtkWidget *
CalligraphyToolbar::create(SPDesktop *desktop)
{
    auto toolbar = new CalligraphyToolbar(desktop);
    return GTK_WIDGET(toolbar->gobj());
}

void
CalligraphyToolbar::width_value_changed()
{
    Unit const *unit = _tracker->getActiveUnit();
    g_return_if_fail(unit != nullptr);
    auto prefs = Inkscape::Preferences::get();
    _tracker->getCurrentLabel() == "%" ? prefs->setBool("/tools/calligraphic/abs_width", false)
                                         : prefs->setBool("/tools/calligraphic/abs_width", true);
    prefs->setDouble("/tools/calligraphic/width", Quantity::convert(_width_adj->get_value(), unit, "px"));
    update_presets_list();
}

void
CalligraphyToolbar::velthin_value_changed()
{
    auto prefs = Inkscape::Preferences::get();
    prefs->setDouble("/tools/calligraphic/thinning", _thinning_adj->get_value() );
    update_presets_list();
}

void
CalligraphyToolbar::angle_value_changed()
{
    auto prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/calligraphic/angle", _angle_adj->get_value() );
    update_presets_list();
}

void
CalligraphyToolbar::flatness_value_changed()
{
    auto prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/calligraphic/flatness", _fixation_adj->get_value() );
    update_presets_list();
}

void
CalligraphyToolbar::cap_rounding_value_changed()
{
    auto prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/calligraphic/cap_rounding", _cap_rounding_adj->get_value() );
    update_presets_list();
}

void
CalligraphyToolbar::tremor_value_changed()
{
    auto prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/calligraphic/tremor", _tremor_adj->get_value() );
    update_presets_list();
}

void
CalligraphyToolbar::wiggle_value_changed()
{
    auto prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/calligraphic/wiggle", _wiggle_adj->get_value() );
    update_presets_list();
}

void
CalligraphyToolbar::mass_value_changed()
{
    auto prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/calligraphic/mass", _mass_adj->get_value() );
    update_presets_list();
}

void
CalligraphyToolbar::on_pref_toggled(Gtk::ToggleToolButton *item,
                                    const Glib::ustring&   path)
{
    auto prefs = Inkscape::Preferences::get();
    prefs->setBool(path, item->get_active());
    update_presets_list();
}

void
CalligraphyToolbar::update_presets_list()
{
    if (_presets_blocked) {
        return;
    }

    auto prefs = Inkscape::Preferences::get();
    auto presets = get_presets_list();

    int index = 1;  // 0 is for no preset.
    for (auto i = presets.begin(); i != presets.end(); ++i, ++index) {
        bool match = true;

        auto preset = prefs->getAllEntries(*i);
        for (auto & j : preset) {
            Glib::ustring entry_name = j.getEntryName();
            if (entry_name == "id" || entry_name == "name") {
                continue;
            }

            void *widget = _widget_map[entry_name.data()];
            if (widget) {
                if (GTK_IS_ADJUSTMENT(widget)) {
                    double v = j.getDouble();
                    GtkAdjustment* adj = static_cast<GtkAdjustment *>(widget);
                    //std::cout << "compared adj " << attr_name << gtk_adjustment_get_value(adj) << " to " << v << "\n";
                    if (fabs(gtk_adjustment_get_value(adj) - v) > 1e-6) {
                        match = false;
                        break;
                    }
                } else if (GTK_IS_TOGGLE_TOOL_BUTTON(widget)) {
                    bool v = j.getBool();
                    auto toggle = GTK_TOGGLE_TOOL_BUTTON(widget);
                    //std::cout << "compared toggle " << attr_name << gtk_toggle_action_get_active(toggle) << " to " << v << "\n";
                    if ( static_cast<bool>(gtk_toggle_tool_button_get_active(toggle)) != v ) {
                        match = false;
                        break;
                    }
                }
            }
        }

        if (match) {
            // newly added item is at the same index as the
            // save command, so we need to change twice for it to take effect
            _profile_selector_combo->set_active(0);
            _profile_selector_combo->set_active(index);
            return;
        }
    }

    // no match found
    _profile_selector_combo->set_active(0);
}

void
CalligraphyToolbar::tilt_state_changed()
{
    _angle_item->set_sensitive(!_usetilt->get_active());
    on_pref_toggled(_usetilt, "/tools/calligraphic/usetilt");
}

void
CalligraphyToolbar::build_presets_list()
{
    _presets_blocked = true;

    _profile_selector_combo->remove_all();
    _profile_selector_combo->append(_("No preset"));

    // iterate over all presets to populate the list
    auto prefs = Inkscape::Preferences::get();
    auto presets = get_presets_list();

    for (auto & preset : presets) {
        Glib::ustring preset_name = prefs->getString(preset + "/name");

        if (!preset_name.empty()) {
            _profile_selector_combo->append(_(preset_name.data()));
        }
    }

    _presets_blocked = false;

    update_presets_list();
}

void
CalligraphyToolbar::change_profile()
{
    auto mode = _profile_selector_combo->get_active_row_number();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    if (_presets_blocked) {
        return;
    }

    // mode is one-based so we subtract 1
    std::vector<Glib::ustring> presets = get_presets_list();

    Glib::ustring preset_path = "";
    if (mode - 1 < presets.size()) {
        preset_path = presets.at(mode - 1);
    }

    if (!preset_path.empty()) {
        _presets_blocked = true; //temporarily block the selector so no one will updadte it while we're reading it

        std::vector<Inkscape::Preferences::Entry> preset = prefs->getAllEntries(preset_path);

        // Shouldn't this be std::map?
        for (auto & i : preset) {
            Glib::ustring entry_name = i.getEntryName();
            if (entry_name == "id" || entry_name == "name") {
                continue;
            }
            void *widget = _widget_map[entry_name.data()];
            if (widget) {
                if (GTK_IS_ADJUSTMENT(widget)) {
                    GtkAdjustment* adj = static_cast<GtkAdjustment *>(widget);
                    gtk_adjustment_set_value(adj, i.getDouble());
                    //std::cout << "set adj " << attr_name << " to " << v << "\n";
                } else if (GTK_IS_TOGGLE_TOOL_BUTTON(widget)) {
                    auto toggle = GTK_TOGGLE_TOOL_BUTTON(widget);
                    gtk_toggle_tool_button_set_active(toggle, i.getBool());
                    //std::cout << "set toggle " << attr_name << " to " << v << "\n";
                } else {
                    g_warning("Unknown widget type for preset: %s\n", entry_name.data());
                }
            } else {
                g_warning("Bad key found in a preset record: %s\n", entry_name.data());
            }
        }
        _presets_blocked = false;
    }
}

void
CalligraphyToolbar::edit_profile()
{
    save_profile(nullptr);
}

void CalligraphyToolbar::unit_changed(int /* NotUsed */)
{
    Unit const *unit = _tracker->getActiveUnit();
    g_return_if_fail(unit != nullptr);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _tracker->getCurrentLabel() == "%" ? prefs->setBool("/tools/calligraphic/abs_width", false)
                                        : prefs->setBool("/tools/calligraphic/abs_width", true);
    prefs->setDouble("/tools/calligraphic/width",
                     CLAMP(prefs->getDouble("/tools/calligraphic/width"), Quantity::convert(0.001, unit, "px"),
                           Quantity::convert(100, unit, "px")));
    prefs->setString("/tools/calligraphic/unit", unit->abbr);
}

void CalligraphyToolbar::save_profile(GtkWidget * /*widget*/)
{
    using Inkscape::UI::Dialog::CalligraphicProfileRename;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (! _desktop) {
        return;
    }

    if (_presets_blocked) {
        return;
    }

    Glib::ustring current_profile_name = _profile_selector_combo->get_active_text();

    if (current_profile_name == _("No preset")) {
        current_profile_name = "";
    }

    CalligraphicProfileRename::show(_desktop, current_profile_name);
    if ( !CalligraphicProfileRename::applied()) {
        // dialog cancelled
        update_presets_list();
        return;
    }
    Glib::ustring new_profile_name = CalligraphicProfileRename::getProfileName();

    if (new_profile_name.empty()) {
        // empty name entered
        update_presets_list ();
        return;
    }

    _presets_blocked = true;

    // If there's a preset with the given name, find it and set save_path appropriately
    auto presets = get_presets_list();
    int total_presets = presets.size();
    int new_index = -1;
    Glib::ustring save_path; // profile pref path without a trailing slash

    int temp_index = 0;
    for (std::vector<Glib::ustring>::iterator i = presets.begin(); i != presets.end(); ++i, ++temp_index) {
        Glib::ustring name = prefs->getString(*i + "/name");
        if (!name.empty() && (new_profile_name == name || current_profile_name == name)) {
            new_index = temp_index;
            save_path = *i;
            break;
        }
    }

    if ( CalligraphicProfileRename::deleted() && new_index != -1) {
        prefs->remove(save_path);
        _presets_blocked = false;
        build_presets_list();
        return;
    }

    if (new_index == -1) {
        // no preset with this name, create
        new_index = total_presets + 1;
        gchar *profile_id = g_strdup_printf("/dcc%d", new_index);
        save_path = Glib::ustring("/tools/calligraphic/preset") + profile_id;
        g_free(profile_id);
    }

    for (auto map_item : _widget_map) {
        auto widget_name = map_item.first;
        auto widget      = map_item.second;

        if (widget) {
            if (GTK_IS_ADJUSTMENT(widget)) {
                GtkAdjustment* adj = GTK_ADJUSTMENT(widget);
                prefs->setDouble(save_path + "/" + widget_name, gtk_adjustment_get_value(adj));
                //std::cout << "wrote adj " << widget_name << ": " << v << "\n";
            } else if (GTK_IS_TOGGLE_TOOL_BUTTON(widget)) {
                auto toggle = GTK_TOGGLE_TOOL_BUTTON(widget);
                prefs->setBool(save_path + "/" + widget_name, gtk_toggle_tool_button_get_active(toggle));
                //std::cout << "wrote tog " << widget_name << ": " << v << "\n";
            } else {
                g_warning("Unknown widget type for preset: %s\n", widget_name.c_str());
            }
        } else {
            g_warning("Bad key when writing preset: %s\n", widget_name.c_str());
        }
    }
    prefs->setString(save_path + "/name", new_profile_name);

    _presets_blocked = true;
    build_presets_list();
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

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

#include "ui/dialog/calligraphic-profile-rename.h"
#include <glibmm/i18n.h>
#include "calligraphy-toolbar.h"

#include "desktop.h"
#include "document-undo.h"
#include "widgets/ege-adjustment-action.h"
#include "widgets/ink-action.h"
#include "widgets/ink-toggle-action.h"
#include "widgets/toolbox.h"
#include "ui/icon-names.h"
#include "ui/uxmanager.h"
#include "ui/widget/ink-select-one-action.h"

using Inkscape::UI::UXManager;
using Inkscape::DocumentUndo;
using Inkscape::UI::ToolboxFactory;
using Inkscape::UI::PrefPusher;


//########################
//##     Calligraphy    ##
//########################

std::vector<Glib::ustring> get_presets_list() {

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    std::vector<Glib::ustring> presets = prefs->getAllDirs("/tools/calligraphic/preset");

    return presets;
}


#if 0
static gchar const *const widget_names[] = {
    "width",
    "mass",
    "wiggle",
    "angle",
    "thinning",
    "tremor",
    "flatness",
    "cap_rounding",
    "usepressure",
    "tracebackground",
    "usetilt"
};
#endif



namespace Inkscape {
namespace UI {
namespace Toolbar {

CalligraphyToolbar::~CalligraphyToolbar()
{
    if(_tracebackground_pusher) delete _tracebackground_pusher;
    if(_usepressure_pusher)     delete _usepressure_pusher;
    if(_usetilt_pusher)         delete _usetilt_pusher;
}

GtkWidget *
CalligraphyToolbar::prep(SPDesktop *desktop, GtkActionGroup* mainActions)
{
    auto toolbar = new CalligraphyToolbar(desktop);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    {
        toolbar->_presets_blocked = true;

        {
        /* Width */
        gchar const* labels[] = {_("(hairline)"), nullptr, nullptr, nullptr, _("(default)"), nullptr, nullptr, nullptr, nullptr, _("(broad stroke)")};
        gdouble values[] = {1, 3, 5, 10, 15, 20, 30, 50, 75, 100};
        EgeAdjustmentAction *eact = create_adjustment_action( "CalligraphyWidthAction",
                                                              _("Pen Width"), _("Width:"),
                                                              _("The width of the calligraphic pen (relative to the visible canvas area)"),
                                                              "/tools/calligraphic/width", 15,
                                                              GTK_WIDGET(desktop->canvas),
                                                              nullptr, // dataKludge
                                                              TRUE, "altx-calligraphy",
                                                              1, 100, 1.0, 10.0,
                                                              labels, values, G_N_ELEMENTS(labels),
                                                              nullptr, // callback
                                                              nullptr /*unit tracker*/, 1, 0 );
        toolbar->_width_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        toolbar->_width_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &CalligraphyToolbar::width_value_changed));
        toolbar->_widget_map["width"] = G_OBJECT(toolbar->_width_adj->gobj());
        ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        gtk_action_set_sensitive( GTK_ACTION(eact), TRUE );
        }

        {
        /* Thinning */
            gchar const* labels[] = {_("(speed blows up stroke)"), nullptr, nullptr, _("(slight widening)"), _("(constant width)"), _("(slight thinning, default)"), nullptr, nullptr, _("(speed deflates stroke)")};
            gdouble values[] = {-100, -40, -20, -10, 0, 10, 20, 40, 100};
        EgeAdjustmentAction* eact = create_adjustment_action( "ThinningAction",
                                                              _("Stroke Thinning"), _("Thinning:"),
                                                              _("How much velocity thins the stroke (> 0 makes fast strokes thinner, < 0 makes them broader, 0 makes width independent of velocity)"),
                                                              "/tools/calligraphic/thinning", 10,
                                                              GTK_WIDGET(desktop->canvas),
                                                              nullptr, // dataKludge
                                                              FALSE, nullptr,
                                                              -100, 100, 1, 10.0,
                                                              labels, values, G_N_ELEMENTS(labels),
                                                              nullptr, // callback
                                                              nullptr /*unit tracker*/, 1, 0);
        toolbar->_thinning_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        toolbar->_thinning_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &CalligraphyToolbar::velthin_value_changed));
        toolbar->_widget_map["thinning"] = G_OBJECT(toolbar->_thinning_adj->gobj());
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        gtk_action_set_sensitive( GTK_ACTION(eact), TRUE );
        }

        {
        /* Angle */
        gchar const* labels[] = {_("(left edge up)"), nullptr, nullptr, _("(horizontal)"), _("(default)"), nullptr, _("(right edge up)")};
        gdouble values[] = {-90, -60, -30, 0, 30, 60, 90};
        toolbar->_angle_action = create_adjustment_action( "AngleAction",
                                                           _("Pen Angle"), _("Angle:"),
                                                           _("The angle of the pen's nib (in degrees; 0 = horizontal; has no effect if fixation = 0)"),
                                                           "/tools/calligraphic/angle", 30,
                                                           GTK_WIDGET(desktop->canvas),
                                                           nullptr, // dataKludge
                                                           TRUE, "calligraphy-angle",
                                                           -90.0, 90.0, 1.0, 10.0,
                                                           labels, values, G_N_ELEMENTS(labels),
                                                           nullptr, // callback
                                                           nullptr /*unit tracker*/, 1, 0 );
        toolbar->_angle_adj = Glib::wrap(ege_adjustment_action_get_adjustment(toolbar->_angle_action));
        toolbar->_angle_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &CalligraphyToolbar::angle_value_changed));
        toolbar->_widget_map["angle"] = G_OBJECT(toolbar->_angle_adj->gobj());
        gtk_action_group_add_action( mainActions, GTK_ACTION(toolbar->_angle_action) );
        gtk_action_set_sensitive( GTK_ACTION(toolbar->_angle_action), TRUE );
        }

        {
        /* Fixation */
            gchar const* labels[] = {_("(perpendicular to stroke, \"brush\")"), nullptr, nullptr, nullptr, _("(almost fixed, default)"), _("(fixed by Angle, \"pen\")")};
        gdouble values[] = {0, 20, 40, 60, 90, 100};
        EgeAdjustmentAction* eact = create_adjustment_action( "FixationAction",
                                                              _("Fixation"), _("Fixation:"),
                                                              _("Angle behavior (0 = nib always perpendicular to stroke direction, 100 = fixed angle)"),
                                                              "/tools/calligraphic/flatness", 90,
                                                              GTK_WIDGET(desktop->canvas),
                                                              nullptr, // dataKludge
                                                              FALSE, nullptr,
                                                              0.0, 100, 1.0, 10.0,
                                                              labels, values, G_N_ELEMENTS(labels),
                                                              nullptr, // callback
                                                              nullptr /*unit tracker*/, 1, 0);
        toolbar->_fixation_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        toolbar->_fixation_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &CalligraphyToolbar::flatness_value_changed));
        toolbar->_widget_map["flatness"] = G_OBJECT(toolbar->_fixation_adj->gobj());
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        gtk_action_set_sensitive( GTK_ACTION(eact), TRUE );
        }

        {
        /* Cap Rounding */
            gchar const* labels[] = {_("(blunt caps, default)"), _("(slightly bulging)"), nullptr, nullptr, _("(approximately round)"), _("(long protruding caps)")};
        gdouble values[] = {0, 0.3, 0.5, 1.0, 1.4, 5.0};
        // TRANSLATORS: "cap" means "end" (both start and finish) here
        EgeAdjustmentAction* eact = create_adjustment_action( "CapRoundingAction",
                                                              _("Cap rounding"), _("Caps:"),
                                                              _("Increase to make caps at the ends of strokes protrude more (0 = no caps, 1 = round caps)"),
                                                              "/tools/calligraphic/cap_rounding", 0.0,
                                                              GTK_WIDGET(desktop->canvas),
                                                              nullptr, // dataKludge
                                                              FALSE, nullptr,
                                                              0.0, 5.0, 0.01, 0.1,
                                                              labels, values, G_N_ELEMENTS(labels),
                                                              nullptr, // callback
                                                              nullptr /*unit tracker*/, 0.01, 2 );
        toolbar->_cap_rounding_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        toolbar->_cap_rounding_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &CalligraphyToolbar::cap_rounding_value_changed));
        toolbar->_widget_map["cap_rounding"] = G_OBJECT(toolbar->_cap_rounding_adj->gobj());
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        gtk_action_set_sensitive( GTK_ACTION(eact), TRUE );
        }

        {
        /* Tremor */
            gchar const* labels[] = {_("(smooth line)"), _("(slight tremor)"), _("(noticeable tremor)"), nullptr, nullptr, _("(maximum tremor)")};
        gdouble values[] = {0, 10, 20, 40, 60, 100};
        EgeAdjustmentAction* eact = create_adjustment_action( "TremorAction",
                                                              _("Stroke Tremor"), _("Tremor:"),
                                                              _("Increase to make strokes rugged and trembling"),
                                                              "/tools/calligraphic/tremor", 0.0,
                                                              GTK_WIDGET(desktop->canvas),
                                                              nullptr, // dataKludge
                                                              FALSE, nullptr,
                                                              0.0, 100, 1, 10.0,
                                                              labels, values, G_N_ELEMENTS(labels),
                                                              nullptr, // callback
                                                              nullptr /*unit tracker*/, 1, 0);

        toolbar->_tremor_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        toolbar->_tremor_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &CalligraphyToolbar::tremor_value_changed));
        toolbar->_widget_map["tremor"] = G_OBJECT(toolbar->_tremor_adj->gobj());
        ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        gtk_action_set_sensitive( GTK_ACTION(eact), TRUE );
        }

        {
        /* Wiggle */
        gchar const* labels[] = {_("(no wiggle)"), _("(slight deviation)"), nullptr, nullptr, _("(wild waves and curls)")};
        gdouble values[] = {0, 20, 40, 60, 100};
        EgeAdjustmentAction* eact = create_adjustment_action( "WiggleAction",
                                                              _("Pen Wiggle"), _("Wiggle:"),
                                                              _("Increase to make the pen waver and wiggle"),
                                                              "/tools/calligraphic/wiggle", 0.0,
                                                              GTK_WIDGET(desktop->canvas),
                                                              nullptr, // dataKludge
                                                              FALSE, nullptr,
                                                              0.0, 100, 1, 10.0,
                                                              labels, values, G_N_ELEMENTS(labels),
                                                              nullptr, // callback
                                                              nullptr /*unit tracker*/, 1, 0);
        toolbar->_wiggle_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        toolbar->_wiggle_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &CalligraphyToolbar::wiggle_value_changed));
        toolbar->_widget_map["wiggle"] = G_OBJECT(toolbar->_wiggle_adj->gobj());
        ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        gtk_action_set_sensitive( GTK_ACTION(eact), TRUE );
        }

        {
        /* Mass */
            gchar const* labels[] = {_("(no inertia)"), _("(slight smoothing, default)"), _("(noticeable lagging)"), nullptr, nullptr, _("(maximum inertia)")};
        gdouble values[] = {0.0, 2, 10, 20, 50, 100};
        EgeAdjustmentAction* eact = create_adjustment_action( "MassAction",
                                                              _("Pen Mass"), _("Mass:"),
                                                              _("Increase to make the pen drag behind, as if slowed by inertia"),
                                                              "/tools/calligraphic/mass", 2.0,
                                                              GTK_WIDGET(desktop->canvas),
                                                              nullptr, // dataKludge
                                                              FALSE, nullptr,
                                                              0.0, 100, 1, 10.0,
                                                              labels, values, G_N_ELEMENTS(labels),
                                                              nullptr, // callback
                                                              nullptr /*unit tracker*/, 1, 0);
        toolbar->_mass_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        toolbar->_mass_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &CalligraphyToolbar::mass_value_changed));
        toolbar->_widget_map["mass"] = G_OBJECT(toolbar->_mass_adj->gobj());
        ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        gtk_action_set_sensitive( GTK_ACTION(eact), TRUE );
        }


        /* Trace Background button */
        {
            toolbar->_tracebackground = ink_toggle_action_new( "TraceAction",
                                                               _("Trace Background"),
                                                               _("Trace the lightness of the background by the width of the pen (white - minimum width, black - maximum width)"),
                                                               INKSCAPE_ICON("draw-trace-background"),
                                                               GTK_ICON_SIZE_MENU );
            toolbar->_widget_map["tracebackground"] = G_OBJECT(toolbar->_tracebackground);
            gtk_action_group_add_action( mainActions, GTK_ACTION( toolbar->_tracebackground ) );
            toolbar->_tracebackground_pusher = new PrefPusher(GTK_TOGGLE_ACTION(toolbar->_tracebackground), "/tools/calligraphic/tracebackground", update_presets_list, gpointer(toolbar));
        }

        /* Use Pressure button */
        {
            toolbar->_usepressure = ink_toggle_action_new( "PressureAction",
                                                           _("Pressure"),
                                                           _("Use the pressure of the input device to alter the width of the pen"),
                                                           INKSCAPE_ICON("draw-use-pressure"),
                                                           GTK_ICON_SIZE_MENU );
            toolbar->_widget_map["usepressure"] = G_OBJECT(toolbar->_usepressure);
            gtk_action_group_add_action( mainActions, GTK_ACTION( toolbar->_usepressure ) );
            toolbar->_usepressure_pusher = new PrefPusher(GTK_TOGGLE_ACTION(toolbar->_usepressure), "/tools/calligraphic/usepressure", update_presets_list, gpointer(toolbar));
        }

        /* Use Tilt button */
        {
            toolbar->_usetilt = ink_toggle_action_new( "TiltAction",
                                                       _("Tilt"),
                                                       _("Use the tilt of the input device to alter the angle of the pen's nib"),
                                                       INKSCAPE_ICON("draw-use-tilt"),
                                                       GTK_ICON_SIZE_MENU );
            toolbar->_widget_map["usetilt"] = G_OBJECT(toolbar->_usetilt);
            gtk_action_group_add_action( mainActions, GTK_ACTION( toolbar->_usetilt ) );
            toolbar->_usetilt_pusher = new PrefPusher(GTK_TOGGLE_ACTION(toolbar->_usetilt), "/tools/calligraphic/usetilt", update_presets_list, gpointer(toolbar));
            g_signal_connect_after( G_OBJECT(toolbar->_usetilt), "toggled", G_CALLBACK(tilt_state_changed), toolbar );
            gtk_action_set_sensitive( GTK_ACTION(toolbar->_angle_action), !prefs->getBool("/tools/calligraphic/usetilt", true) );
            gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(toolbar->_usetilt), prefs->getBool("/tools/calligraphic/usetilt", true) );
        }

        /*calligraphic profile */
        {
            InkSelectOneActionColumns columns;

            Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

            toolbar->_profile_selector =
                InkSelectOneAction::create( "SetProfileAction",   // Name
                                            "",                   // Label
                                            _("Choose a preset"), // Tooltip
                                            "NotUsed",            // Icon
                                            store );              // Tree store
            toolbar->_profile_selector->use_radio( false );
            toolbar->_profile_selector->use_label( true );

            toolbar->build_presets_list();

            gtk_action_group_add_action(mainActions, GTK_ACTION( toolbar->_profile_selector->gobj() ));

            toolbar->_profile_selector->signal_changed().connect(sigc::mem_fun(*toolbar, &CalligraphyToolbar::change_profile));
        }

        /*calligraphic profile editor */
        {
            InkAction* inky = ink_action_new( "ProfileEditAction",
                                              _("Add/Edit Profile"),
                                              _("Add or edit calligraphic profile"),
                                              INKSCAPE_ICON("document-properties"),
                                              GTK_ICON_SIZE_MENU );
            gtk_action_set_short_label(GTK_ACTION(inky), _("Edit"));
            g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(edit_profile), toolbar );
            gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
        }
    }

    return GTK_WIDGET(toolbar->gobj());
}

void
CalligraphyToolbar::width_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/calligraphic/width", _width_adj->get_value() );
    update_presets_list(this);
}

void
CalligraphyToolbar::velthin_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble("/tools/calligraphic/thinning", _thinning_adj->get_value() );
    update_presets_list(this);
}

void
CalligraphyToolbar::angle_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/calligraphic/angle", _angle_adj->get_value() );
    update_presets_list(this);
}

void
CalligraphyToolbar::flatness_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/calligraphic/flatness", _fixation_adj->get_value() );
    update_presets_list(this);
}

void
CalligraphyToolbar::cap_rounding_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/calligraphic/cap_rounding", _cap_rounding_adj->get_value() );
    update_presets_list(this);
}

void
CalligraphyToolbar::tremor_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/calligraphic/tremor", _tremor_adj->get_value() );
    update_presets_list(this);
}

void
CalligraphyToolbar::wiggle_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/calligraphic/wiggle", _wiggle_adj->get_value() );
    update_presets_list(this);
}

void
CalligraphyToolbar::mass_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/calligraphic/mass", _mass_adj->get_value() );
    update_presets_list(this);
}

void
CalligraphyToolbar::update_presets_list(gpointer data)
{
    auto toolbar = reinterpret_cast<CalligraphyToolbar *>(data);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (toolbar->_presets_blocked) {
        return;
    }

    std::vector<Glib::ustring> presets = get_presets_list();

    int index = 1;  // 0 is for no preset.
    for (std::vector<Glib::ustring>::iterator i = presets.begin(); i != presets.end(); ++i, ++index) {
        bool match = true;

        std::vector<Inkscape::Preferences::Entry> preset = prefs->getAllEntries(*i);
        for (auto & j : preset) {
            Glib::ustring entry_name = j.getEntryName();
            if (entry_name == "id" || entry_name == "name") {
                continue;
            }

            void *widget = toolbar->_widget_map[entry_name.data()];
            if (widget) {
                if (GTK_IS_ADJUSTMENT(widget)) {
                    double v = j.getDouble();
                    GtkAdjustment* adj = static_cast<GtkAdjustment *>(widget);
                    //std::cout << "compared adj " << attr_name << gtk_adjustment_get_value(adj) << " to " << v << "\n";
                    if (fabs(gtk_adjustment_get_value(adj) - v) > 1e-6) {
                        match = false;
                        break;
                    }
                } else if (GTK_IS_TOGGLE_ACTION(widget)) {
                    bool v = j.getBool();
                    GtkToggleAction* toggle = static_cast<GtkToggleAction *>(widget);
                    //std::cout << "compared toggle " << attr_name << gtk_toggle_action_get_active(toggle) << " to " << v << "\n";
                    if ( static_cast<bool>(gtk_toggle_action_get_active(toggle)) != v ) {
                        match = false;
                        break;
                    }
                }
            }
        }

        if (match) {
            // newly added item is at the same index as the
            // save command, so we need to change twice for it to take effect
            toolbar->_profile_selector->set_active(0);
            toolbar->_profile_selector->set_active(index);
            return;
        }
    }

    // no match found
    toolbar->_profile_selector->set_active(0);
}

void
CalligraphyToolbar::tilt_state_changed( GtkToggleAction *act, gpointer data )
{
    auto toolbar = reinterpret_cast<CalligraphyToolbar *>(data);

    // TODO merge into PrefPusher
    if (toolbar->_angle_action) {
        gtk_action_set_sensitive( GTK_ACTION(toolbar->_angle_action), !gtk_toggle_action_get_active( act ) );
    }
}

void
CalligraphyToolbar::build_presets_list()
{
    _presets_blocked = true;

    auto store = _profile_selector->get_store();
    store->clear();

    InkSelectOneActionColumns columns;

    Gtk::TreeModel::Row row;

    row = *(store->append());
    row[columns.col_label    ] = _("No preset");
    row[columns.col_sensitive] = true;

    // iterate over all presets to populate the list
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    std::vector<Glib::ustring> presets = get_presets_list();
    int ii=1;

    for (auto & preset : presets) {
        GtkTreeIter iter;
        Glib::ustring preset_name = prefs->getString(preset + "/name");

        if (!preset_name.empty()) {
            row = *(store->append());
            row[columns.col_label    ] = _(preset_name.data());
            row[columns.col_sensitive] = true;
        }
    }

    _presets_blocked = false;

    update_presets_list(this);
}

void
CalligraphyToolbar::change_profile(int mode)
{
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
                } else if (GTK_IS_TOGGLE_ACTION(widget)) {
                    GtkToggleAction* toggle = static_cast<GtkToggleAction *>(widget);
                    gtk_toggle_action_set_active(toggle, i.getBool());
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
CalligraphyToolbar::edit_profile(GtkAction * /*act*/, gpointer data)
{
    auto toolbar = reinterpret_cast<CalligraphyToolbar *>(data);
    toolbar->save_profile(nullptr);
}

void
CalligraphyToolbar::save_profile(GtkWidget * /*widget*/)
{
    using Inkscape::UI::Dialog::CalligraphicProfileRename;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (! _desktop) {
        return;
    }

    if (_presets_blocked) {
        return;
    }

    Glib::ustring current_profile_name = _profile_selector->get_active_text();

    if (current_profile_name == _("No preset")) {
        current_profile_name = "";
    }

    CalligraphicProfileRename::show(_desktop, current_profile_name);
    if ( !CalligraphicProfileRename::applied()) {
        // dialog cancelled
        update_presets_list(this);
        return;
    }
    Glib::ustring new_profile_name = CalligraphicProfileRename::getProfileName();

    if (new_profile_name.empty()) {
        // empty name entered
        update_presets_list (this);
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
            } else if (GTK_IS_TOGGLE_ACTION(widget)) {
                GtkToggleAction* toggle = GTK_TOGGLE_ACTION(widget);
                prefs->setBool(save_path + "/" + widget_name, gtk_toggle_action_get_active(toggle));
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

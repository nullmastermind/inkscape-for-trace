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
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <glibmm/i18n.h>

#include "tweak-toolbar.h"

#include "desktop.h"
#include "document-undo.h"

#include "ui/icon-names.h"
#include "ui/tools/tweak-tool.h"
#include "ui/widget/ink-select-one-action.h"
#include "ui/widget/spinbutton.h"

#include "widgets/ege-adjustment-action.h"
#include "widgets/ege-output-action.h"
#include "widgets/ink-radio-action.h"
#include "widgets/ink-toggle-action.h"
#include "widgets/toolbox.h"

using Inkscape::DocumentUndo;
using Inkscape::UI::ToolboxFactory;
using Inkscape::UI::PrefPusher;


//########################
//##       Tweak        ##
//########################

static void sp_tweak_width_value_changed( GtkAdjustment *adj, GObject * /*tbl*/ )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/tweak/width",
            gtk_adjustment_get_value(adj) * 0.01 );
}

static void sp_tweak_force_value_changed( GtkAdjustment *adj, GObject * /*tbl*/ )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/tweak/force",
            gtk_adjustment_get_value(adj) * 0.01 );
}

static void sp_tweak_pressure_state_changed( GtkToggleAction *act, gpointer /*data*/ )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/tweak/usepressure", gtk_toggle_action_get_active(act));
}

static void sp_tweak_mode_changed( GObject *tbl, int mode )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt("/tools/tweak/mode", mode);

    static gchar const* names[] = {"tweak_doh", "tweak_dos", "tweak_dol", "tweak_doo", "tweak_channels_label"};
    bool flag = ((mode == Inkscape::UI::Tools::TWEAK_MODE_COLORPAINT) ||
                 (mode == Inkscape::UI::Tools::TWEAK_MODE_COLORJITTER));
    for (size_t i = 0; i < G_N_ELEMENTS(names); ++i) {
        GtkAction *act = GTK_ACTION(g_object_get_data( tbl, names[i] ));
        if (act) {
            gtk_action_set_visible(act, flag);
        }
    }
    GtkAction *fid = GTK_ACTION(g_object_get_data( tbl, "tweak_fidelity"));
    if (fid) {
        gtk_action_set_visible(fid, !flag);
    }
}

static void sp_tweak_fidelity_value_changed( GtkAdjustment *adj, GObject * /*tbl*/ )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/tweak/fidelity",
            gtk_adjustment_get_value(adj) * 0.01 );
}

static void tweak_toggle_doh(GtkToggleAction *act, gpointer /*data*/) {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/tweak/doh", gtk_toggle_action_get_active(act));
}
static void tweak_toggle_dos(GtkToggleAction *act, gpointer /*data*/) {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/tweak/dos", gtk_toggle_action_get_active(act));
}
static void tweak_toggle_dol(GtkToggleAction *act, gpointer /*data*/) {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/tweak/dol", gtk_toggle_action_get_active(act));
}
static void tweak_toggle_doo(GtkToggleAction *act, gpointer /*data*/) {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/tweak/doo", gtk_toggle_action_get_active(act));
}

void sp_tweak_toolbox_prep(SPDesktop *desktop, GtkActionGroup* mainActions, GObject* holder)
{
    GtkIconSize secondarySize = ToolboxFactory::prefToSize("/toolbox/secondary", 1);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    {
        /* Width */
        gchar const* labels[] = {_("(pinch tweak)"), nullptr, nullptr, nullptr, _("(default)"), nullptr, nullptr, nullptr, nullptr, _("(broad tweak)")};
        gdouble values[] = {1, 3, 5, 10, 15, 20, 30, 50, 75, 100};
        EgeAdjustmentAction *eact = create_adjustment_action( "TweakWidthAction",
                                                              _("Width"), _("Width:"), _("The width of the tweak area (relative to the visible canvas area)"),
                                                              "/tools/tweak/width", 15,
                                                              GTK_WIDGET(desktop->canvas), holder, TRUE, "altx-tweak",
                                                              1, 100, 1.0, 10.0,
                                                              labels, values, G_N_ELEMENTS(labels),
                                                              sp_tweak_width_value_changed, nullptr /*unit tracker*/, 0.01, 0, 100 );
        ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        gtk_action_set_sensitive( GTK_ACTION(eact), TRUE );
    }


    {
        /* Force */
        gchar const* labels[] = {_("(minimum force)"), nullptr, nullptr, _("(default)"), nullptr, nullptr, nullptr, _("(maximum force)")};
        gdouble values[] = {1, 5, 10, 20, 30, 50, 70, 100};
        EgeAdjustmentAction *eact = create_adjustment_action( "TweakForceAction",
                                                              _("Force"), _("Force:"), _("The force of the tweak action"),
                                                              "/tools/tweak/force", 20,
                                                              GTK_WIDGET(desktop->canvas), holder, TRUE, "tweak-force",
                                                              1, 100, 1.0, 10.0,
                                                              labels, values, G_N_ELEMENTS(labels),
                                                              sp_tweak_force_value_changed, nullptr /*unit tracker*/, 0.01, 0, 100 );
        ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        gtk_action_set_sensitive( GTK_ACTION(eact), TRUE );
    }

    /* Mode */
    {
        InkSelectOneActionColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = _("Move mode");
        row[columns.col_tooltip  ] = _("Move objects in any direction");
        row[columns.col_icon     ] = INKSCAPE_ICON("object-tweak-push");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Move in/out mode");
        row[columns.col_tooltip  ] = _("Move objects towards cursor; with Shift from cursor");
        row[columns.col_icon     ] = INKSCAPE_ICON("object-tweak-attract");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Move jitter mode");
        row[columns.col_tooltip  ] = _("Move objects in random directions");
        row[columns.col_icon     ] = INKSCAPE_ICON("object-tweak-randomize");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Scale mode");
        row[columns.col_tooltip  ] = _("Shrink objects, with Shift enlarge");
        row[columns.col_icon     ] = INKSCAPE_ICON("object-tweak-shrink");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Rotate mode");
        row[columns.col_tooltip  ] = _("Rotate objects, with Shift counterclockwise");
        row[columns.col_icon     ] = INKSCAPE_ICON("object-tweak-rotate");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Duplicate/delete mode");
        row[columns.col_tooltip  ] = _("Duplicate objects, with Shift delete");
        row[columns.col_icon     ] = INKSCAPE_ICON("object-tweak-duplicate");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Push mode");
        row[columns.col_tooltip  ] = _("Push parts of paths in any direction");
        row[columns.col_icon     ] = INKSCAPE_ICON("path-tweak-push");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Shrink/grow mode");
        row[columns.col_tooltip  ] = _("Shrink (inset) parts of paths; with Shift grow (outset)");
        row[columns.col_icon     ] = INKSCAPE_ICON("path-tweak-shrink");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Attract/repel mode");
        row[columns.col_tooltip  ] = _("Attract parts of paths towards cursor; with Shift from cursor");
        row[columns.col_icon     ] = INKSCAPE_ICON("path-tweak-attract");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Roughen mode");
        row[columns.col_tooltip  ] = _("Roughen parts of paths");
        row[columns.col_icon     ] = INKSCAPE_ICON("path-tweak-roughen");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Color paint mode");
        row[columns.col_tooltip  ] = _("Paint the tool's color upon selected objects");
        row[columns.col_icon     ] = INKSCAPE_ICON("object-tweak-paint");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Color jitter mode");
        row[columns.col_tooltip  ] = _("Jitter the colors of selected objects");
        row[columns.col_icon     ] = INKSCAPE_ICON("object-tweak-jitter-color");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Blur mode");
        row[columns.col_tooltip  ] = _("Blur selected objects more; with Shift, blur less");
        row[columns.col_icon     ] = INKSCAPE_ICON("object-tweak-blur");
        row[columns.col_sensitive] = true;

        InkSelectOneAction* act =
            InkSelectOneAction::create( "TweakModeAction",   // Name
                                        _("Mode"),           // Label
                                        (""),                // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        act->use_radio( true );
        act->use_icon( true );
        act->use_label( false );
        act->use_group_label( true );
        int mode = prefs->getInt("/tools/tweak/mode", 0);
        act->set_active( mode );

        gtk_action_group_add_action( mainActions, GTK_ACTION( act->gobj() ));
        g_object_set_data( holder, "tweak_tool_mode", act );

        act->signal_changed().connect(sigc::bind<0>(sigc::ptr_fun(&sp_tweak_mode_changed), holder));
    }

    guint mode = prefs->getInt("/tools/tweak/mode", 0);

    {
        EgeOutputAction* act = ege_output_action_new( "TweakChannelsLabel", _("Channels:"), "", nullptr );
        ege_output_action_set_use_markup( act, TRUE );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        if (mode != Inkscape::UI::Tools::TWEAK_MODE_COLORPAINT && mode != Inkscape::UI::Tools::TWEAK_MODE_COLORJITTER) {
            gtk_action_set_visible (GTK_ACTION(act), FALSE);
        }
        g_object_set_data( holder, "tweak_channels_label", act);
    }

    {
        InkToggleAction* act = ink_toggle_action_new( "TweakDoH",
                                                      _("Hue"),
                                                      _("In color mode, act on objects' hue"),
                                                      nullptr,
                                                      GTK_ICON_SIZE_MENU );
        //TRANSLATORS:  "H" here stands for hue
        g_object_set( act, "short_label", C_("Hue", "H"), NULL );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(tweak_toggle_doh), desktop );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(act), prefs->getBool("/tools/tweak/doh", true) );
        if (mode != Inkscape::UI::Tools::TWEAK_MODE_COLORPAINT && mode != Inkscape::UI::Tools::TWEAK_MODE_COLORJITTER) {
            gtk_action_set_visible (GTK_ACTION(act), FALSE);
        }
        g_object_set_data( holder, "tweak_doh", act);
    }
    {
        InkToggleAction* act = ink_toggle_action_new( "TweakDoS",
                                                      _("Saturation"),
                                                      _("In color mode, act on objects' saturation"),
                                                      nullptr,
                                                      GTK_ICON_SIZE_MENU );
        //TRANSLATORS: "S" here stands for Saturation
        g_object_set( act, "short_label", C_("Saturation", "S"), NULL );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(tweak_toggle_dos), desktop );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(act), prefs->getBool("/tools/tweak/dos", true) );
        if (mode != Inkscape::UI::Tools::TWEAK_MODE_COLORPAINT && mode != Inkscape::UI::Tools::TWEAK_MODE_COLORJITTER) {
            gtk_action_set_visible (GTK_ACTION(act), FALSE);
        }
        g_object_set_data( holder, "tweak_dos", act );
    }
    {
        InkToggleAction* act = ink_toggle_action_new( "TweakDoL",
                                                      _("Lightness"),
                                                      _("In color mode, act on objects' lightness"),
                                                      nullptr,
                                                      GTK_ICON_SIZE_MENU );
        //TRANSLATORS: "L" here stands for Lightness
        g_object_set( act, "short_label", C_("Lightness", "L"), NULL );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(tweak_toggle_dol), desktop );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(act), prefs->getBool("/tools/tweak/dol", true) );
        if (mode != Inkscape::UI::Tools::TWEAK_MODE_COLORPAINT && mode != Inkscape::UI::Tools::TWEAK_MODE_COLORJITTER) {
            gtk_action_set_visible (GTK_ACTION(act), FALSE);
        }
        g_object_set_data( holder, "tweak_dol", act );
    }
    {
        InkToggleAction* act = ink_toggle_action_new( "TweakDoO",
                                                      _("Opacity"),
                                                      _("In color mode, act on objects' opacity"),
                                                      nullptr,
                                                      GTK_ICON_SIZE_MENU );
        //TRANSLATORS: "O" here stands for Opacity
        g_object_set( act, "short_label", C_("Opacity", "O"), NULL );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(tweak_toggle_doo), desktop );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(act), prefs->getBool("/tools/tweak/doo", true) );
        if (mode != Inkscape::UI::Tools::TWEAK_MODE_COLORPAINT && mode != Inkscape::UI::Tools::TWEAK_MODE_COLORJITTER) {
            gtk_action_set_visible (GTK_ACTION(act), FALSE);
        }
        g_object_set_data( holder, "tweak_doo", act );
    }

    {   /* Fidelity */
        gchar const* labels[] = {_("(rough, simplified)"), nullptr, nullptr, _("(default)"), nullptr, nullptr, _("(fine, but many nodes)")};
        gdouble values[] = {10, 25, 35, 50, 60, 80, 100};
        EgeAdjustmentAction *eact = create_adjustment_action( "TweakFidelityAction",
                                                              _("Fidelity"), _("Fidelity:"),
                                                              _("Low fidelity simplifies paths; high fidelity preserves path features but may generate a lot of new nodes"),
                                                              "/tools/tweak/fidelity", 50,
                                                              GTK_WIDGET(desktop->canvas), holder, TRUE, "tweak-fidelity",
                                                              1, 100, 1.0, 10.0,
                                                              labels, values, G_N_ELEMENTS(labels),
                                                              sp_tweak_fidelity_value_changed, nullptr /*unit tracker*/, 0.01, 0, 100 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        gtk_action_set_visible( GTK_ACTION(eact), TRUE );
        if (mode == Inkscape::UI::Tools::TWEAK_MODE_COLORPAINT || mode == Inkscape::UI::Tools::TWEAK_MODE_COLORJITTER) {
            gtk_action_set_visible (GTK_ACTION(eact), FALSE);
        }
        g_object_set_data( holder, "tweak_fidelity", eact );
    }


    /* Use Pressure button */
    {
        InkToggleAction* act = ink_toggle_action_new( "TweakPressureAction",
                                                      _("Pressure"),
                                                      _("Use the pressure of the input device to alter the force of tweak action"),
                                                      INKSCAPE_ICON("draw-use-pressure"),
                                                      GTK_ICON_SIZE_MENU );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(sp_tweak_pressure_state_changed), NULL);
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(act), prefs->getBool("/tools/tweak/usepressure", true) );
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

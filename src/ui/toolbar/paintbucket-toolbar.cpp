// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Paint bucket aux toolbar
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

#include <glibmm/i18n.h>

#include "paintbucket-toolbar.h"
#include "desktop.h"
#include "document-undo.h"

#include "ui/icon-names.h"
#include "ui/tools/flood-tool.h"
#include "ui/uxmanager.h"
#include "ui/widget/ink-select-one-action.h"
#include "ui/widget/unit-tracker.h"

#include "widgets/ink-action.h"
#include "widgets/ege-adjustment-action.h"
#include "widgets/toolbox.h"

using Inkscape::UI::Widget::UnitTracker;
using Inkscape::UI::UXManager;
using Inkscape::DocumentUndo;
using Inkscape::UI::ToolboxFactory;
using Inkscape::UI::PrefPusher;
using Inkscape::Util::unit_table;


//#########################
//##     Paintbucket     ##
//#########################

static void paintbucket_channels_changed(GObject* /*tbl*/, int channels)
{
    Inkscape::UI::Tools::FloodTool::set_channels(channels);
}

static void paintbucket_threshold_changed(GtkAdjustment *adj, GObject * /*tbl*/)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt("/tools/paintbucket/threshold", (gint)gtk_adjustment_get_value(adj));
}

static void paintbucket_autogap_changed(GObject * /*tbl*/, int autogap)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt("/tools/paintbucket/autogap", autogap);
}

static void paintbucket_offset_changed(GtkAdjustment *adj, GObject *tbl)
{
    UnitTracker* tracker = static_cast<UnitTracker*>(g_object_get_data( tbl, "tracker" ));
    Unit const *unit = tracker->getActiveUnit();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // Don't adjust the offset value because we're saving the
    // unit and it'll be correctly handled on load.
    prefs->setDouble("/tools/paintbucket/offset", (gdouble)gtk_adjustment_get_value(adj));

    g_return_if_fail(unit != nullptr);
    prefs->setString("/tools/paintbucket/offsetunits", unit->abbr);
}

static void paintbucket_defaults(GtkWidget *, GObject *tbl)
{
    // FIXME: make defaults settable via Inkscape Options
    struct KeyValue {
        char const *key;
        double value;
    } const key_values[] = {
        {"threshold", 15},
        {"offset", 0.0}
    };

    for (auto kv : key_values) {
        GtkAdjustment* adj = static_cast<GtkAdjustment *>(g_object_get_data(tbl, kv.key));
        if ( adj ) {
            gtk_adjustment_set_value(adj, kv.value);
        }
    }

    InkSelectOneAction* channels_action =
        static_cast<InkSelectOneAction*>( g_object_get_data( tbl, "channels_action" ) );
    channels_action->set_active( Inkscape::UI::Tools::FLOOD_CHANNELS_RGB );

    InkSelectOneAction* autogap_action =
        static_cast<InkSelectOneAction*>( g_object_get_data( tbl, "autogap_action" ) );
    autogap_action->set_active( 0 );
}

void sp_paintbucket_toolbox_prep(SPDesktop *desktop, GtkActionGroup* mainActions, GObject* holder)
{
    EgeAdjustmentAction* eact = nullptr;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // Channel
    {
        InkSelectOneActionColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        for (auto item: Inkscape::UI::Tools::FloodTool::channel_list) {
            row = *(store->append());
            row[columns.col_label    ] = item;
            row[columns.col_tooltip  ] = ("");
            row[columns.col_icon     ] = "NotUsed";
            row[columns.col_sensitive] = true;
        }

        InkSelectOneAction* act =
            InkSelectOneAction::create( "ChannelsAction",    // Name
                                        _("Fill by"),        // Label
                                        (""),                // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        act->use_radio( false );
        act->use_icon( false );
        act->use_label( true );
        act->use_group_label( true );
        int channels = prefs->getInt("/tools/paintbucket/channels", 0);
        act->set_active( channels );

        gtk_action_group_add_action( mainActions, GTK_ACTION( act->gobj() ));
        g_object_set_data( holder, "channels_action", act );

        act->signal_changed().connect(sigc::bind<0>(sigc::ptr_fun(&paintbucket_channels_changed), holder));
    }

    // Spacing spinbox
    {
        eact = create_adjustment_action(
            "ThresholdAction",
            _("Fill Threshold"), _("Threshold:"),
            _("The maximum allowed difference between the clicked pixel and the neighboring pixels to be counted in the fill"),
            "/tools/paintbucket/threshold", 5, GTK_WIDGET(desktop->canvas), holder, TRUE,
            "inkscape:paintbucket-threshold", 0, 100.0, 1.0, 10.0,
            nullptr, nullptr, 0,
            paintbucket_threshold_changed, nullptr /*unit tracker*/, 1, 0 );

        ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
    }

    // Create the units menu.
    UnitTracker* tracker = new UnitTracker(Inkscape::Util::UNIT_TYPE_LINEAR);
    Glib::ustring stored_unit = prefs->getString("/tools/paintbucket/offsetunits");
    if (!stored_unit.empty()) {
        Unit const *u = unit_table.getUnit(stored_unit);
        tracker->setActiveUnit(u);
    }
    g_object_set_data( holder, "tracker", tracker );

    {
        InkSelectOneAction* act = tracker->createAction( "PaintbucketUnitsAction", _("Units"), ("") );
        gtk_action_group_add_action( mainActions, act->gobj() );
    }

    // Offset spinbox
    {
        eact = create_adjustment_action(
            "OffsetAction",
            _("Grow/shrink by"), _("Grow/shrink by:"),
            _("The amount to grow (positive) or shrink (negative) the created fill path"),
            "/tools/paintbucket/offset", 0, GTK_WIDGET(desktop->canvas), holder, TRUE,
            "inkscape:paintbucket-offset", -1e4, 1e4, 0.1, 0.5,
            nullptr, nullptr, 0,
            paintbucket_offset_changed, tracker, 1, 2);
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
    }

    /* Auto Gap */
    {
        InkSelectOneActionColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        for (auto item: Inkscape::UI::Tools::FloodTool::gap_list) {
            row = *(store->append());
            row[columns.col_label    ] = item;
            row[columns.col_tooltip  ] = ("");
            row[columns.col_icon     ] = "NotUsed";
            row[columns.col_sensitive] = true;
        }

        InkSelectOneAction* act =
            InkSelectOneAction::create( "AutoGapAction",     // Name
                                        _("Close gaps"),     // Label
                                        (""),                // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        act->use_radio( false );
        act->use_icon( false );
        act->use_label( true );
        act->use_group_label( true );
        int autogap = prefs->getInt("/tools/paintbucket/autogap");
        act->set_active( autogap );

        gtk_action_group_add_action( mainActions, GTK_ACTION( act->gobj() ));
        g_object_set_data( holder, "autogap_action", act );

        act->signal_changed().connect(sigc::bind<0>(sigc::ptr_fun(&paintbucket_autogap_changed), holder));
    }

    /* Reset */
    {
        InkAction* inky = ink_action_new( "PaintbucketResetAction",
                                          _("Defaults"),
                                          _("Reset paint bucket parameters to defaults (use Inkscape Preferences > Tools to change defaults)"),
                                          INKSCAPE_ICON("edit-clear"),
                                          GTK_ICON_SIZE_SMALL_TOOLBAR);
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(paintbucket_defaults), holder );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
        gtk_action_set_sensitive( GTK_ACTION(inky), TRUE );
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

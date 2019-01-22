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


namespace Inkscape {
namespace UI {
namespace Toolbar {
PaintbucketToolbar::PaintbucketToolbar(SPDesktop *desktop)
    : Toolbar(desktop),
      _tracker(new UnitTracker(Inkscape::Util::UNIT_TYPE_LINEAR))
{}

GtkWidget *
PaintbucketToolbar::prep(SPDesktop *desktop, GtkActionGroup* mainActions)
{
    auto toolbar = new PaintbucketToolbar(desktop);
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

        toolbar->_channels_action =
            InkSelectOneAction::create( "ChannelsAction",    // Name
                                        _("Fill by"),        // Label
                                        (""),                // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        toolbar->_channels_action->use_radio( false );
        toolbar->_channels_action->use_icon( false );
        toolbar->_channels_action->use_label( true );
        toolbar->_channels_action->use_group_label( true );
        int channels = prefs->getInt("/tools/paintbucket/channels", 0);
        toolbar->_channels_action->set_active( channels );

        gtk_action_group_add_action( mainActions, GTK_ACTION( toolbar->_channels_action->gobj() ));

        toolbar->_channels_action->signal_changed().connect(sigc::mem_fun(*toolbar, &PaintbucketToolbar::channels_changed));
    }

    // Spacing spinbox
    {
        eact = create_adjustment_action(
            "ThresholdAction",
            _("Fill Threshold"), _("Threshold:"),
            _("The maximum allowed difference between the clicked pixel and the neighboring pixels to be counted in the fill"),
            "/tools/paintbucket/threshold", 5,
            TRUE,
            "inkscape:paintbucket-threshold", 0, 100.0, 1.0, 10.0,
            nullptr, nullptr, 0,
            nullptr /*unit tracker*/, 1, 0 );
        ege_adjustment_action_set_focuswidget(eact, GTK_WIDGET(desktop->canvas));

        toolbar->_threshold_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        toolbar->_threshold_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &PaintbucketToolbar::threshold_changed));
        ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
    }

    // Create the units menu.
    Glib::ustring stored_unit = prefs->getString("/tools/paintbucket/offsetunits");
    if (!stored_unit.empty()) {
        Unit const *u = unit_table.getUnit(stored_unit);
        toolbar->_tracker->setActiveUnit(u);
    }

    {
        InkSelectOneAction* act = toolbar->_tracker->createAction( "PaintbucketUnitsAction", _("Units"), ("") );
        gtk_action_group_add_action( mainActions, act->gobj() );
    }

    // Offset spinbox
    {
        eact = create_adjustment_action(
            "OffsetAction",
            _("Grow/shrink by"), _("Grow/shrink by:"),
            _("The amount to grow (positive) or shrink (negative) the created fill path"),
            "/tools/paintbucket/offset", 0,
            TRUE,
            "inkscape:paintbucket-offset", -1e4, 1e4, 0.1, 0.5,
            nullptr, nullptr, 0,
            toolbar->_tracker,
            1, 2);
        ege_adjustment_action_set_focuswidget(eact, GTK_WIDGET(desktop->canvas));
        toolbar->_offset_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        toolbar->_offset_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &PaintbucketToolbar::offset_changed));
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

        toolbar->_autogap_action =
            InkSelectOneAction::create( "AutoGapAction",     // Name
                                        _("Close gaps"),     // Label
                                        (""),                // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        toolbar->_autogap_action->use_radio( false );
        toolbar->_autogap_action->use_icon( false );
        toolbar->_autogap_action->use_label( true );
        toolbar->_autogap_action->use_group_label( true );
        int autogap = prefs->getInt("/tools/paintbucket/autogap");
        toolbar->_autogap_action->set_active( autogap );

        gtk_action_group_add_action( mainActions, GTK_ACTION( toolbar->_autogap_action->gobj() ));

        toolbar->_autogap_action->signal_changed().connect(sigc::mem_fun(*toolbar, &PaintbucketToolbar::autogap_changed));
    }

    /* Reset */
    {
        InkAction* inky = ink_action_new( "PaintbucketResetAction",
                                          _("Defaults"),
                                          _("Reset paint bucket parameters to defaults (use Inkscape Preferences > Tools to change defaults)"),
                                          INKSCAPE_ICON("edit-clear"),
                                          GTK_ICON_SIZE_SMALL_TOOLBAR);
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(defaults), (gpointer)toolbar );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
        gtk_action_set_sensitive( GTK_ACTION(inky), TRUE );
    }

    return GTK_WIDGET(toolbar->gobj());
}

void
PaintbucketToolbar::channels_changed(int channels)
{
    Inkscape::UI::Tools::FloodTool::set_channels(channels);
}

void
PaintbucketToolbar::threshold_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt("/tools/paintbucket/threshold", (gint)_threshold_adj->get_value());
}

void
PaintbucketToolbar::offset_changed()
{
    Unit const *unit = _tracker->getActiveUnit();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // Don't adjust the offset value because we're saving the
    // unit and it'll be correctly handled on load.
    prefs->setDouble("/tools/paintbucket/offset", (gdouble)_offset_adj->get_value());

    g_return_if_fail(unit != nullptr);
    prefs->setString("/tools/paintbucket/offsetunits", unit->abbr);
}

void
PaintbucketToolbar::autogap_changed(int autogap)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt("/tools/paintbucket/autogap", autogap);
}

void
PaintbucketToolbar::defaults(GtkWidget * /* widget */, gpointer data)
{
    auto toolbar = reinterpret_cast<PaintbucketToolbar *>(data);

    // FIXME: make defaults settable via Inkscape Options
    toolbar->_threshold_adj->set_value(15);
    toolbar->_offset_adj->set_value(0.0);

    toolbar->_channels_action->set_active( Inkscape::UI::Tools::FLOOD_CHANNELS_RGB );
    toolbar->_autogap_action->set_active( 0 );
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

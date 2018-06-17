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
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "live_effects/lpe-line_segment.h"
#include "lpe-toolbar.h"

#include "helper/action-context.h"
#include "helper/action.h"

#include "widgets/ink-toggle-action.h"

#include "ui/tools-switch.h"
#include "ui/tools/lpe-tool.h"
#include "ui/widget/ink-select-one-action.h"
#include "ui/widget/unit-tracker.h"

using Inkscape::UI::Widget::UnitTracker;
using Inkscape::Util::Unit;
using Inkscape::Util::Quantity;
using Inkscape::DocumentUndo;
using Inkscape::UI::Tools::ToolBase;
using Inkscape::UI::Tools::LpeTool;


//########################
//##      LPETool       ##
//########################

// the subtools from which the toolbar is built automatically are listed in lpe-tool-context.h

// this is called when the mode is changed via the toolbar (i.e., one of the subtool buttons is pressed)
static void sp_lpetool_mode_changed(GObject *tbl, int mode)
{
    using namespace Inkscape::LivePathEffect;

    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data(tbl, "desktop"));
    ToolBase *ec = desktop->event_context;
    if (!SP_IS_LPETOOL_CONTEXT(ec)) {
        return;
    }

    // only take action if run by the attr_changed listener
    if (!g_object_get_data(tbl, "freeze")) {
        // in turn, prevent listener from responding
        g_object_set_data(tbl, "freeze", GINT_TO_POINTER(TRUE));

        EffectType type = lpesubtools[mode].type;

        LpeTool *lc = SP_LPETOOL_CONTEXT(desktop->event_context);
        bool success = lpetool_try_construction(lc, type);
        if (success) {
            // since the construction was already performed, we set the state back to inactive
            InkSelectOneAction* act =
                static_cast<InkSelectOneAction*>( g_object_get_data( tbl, "lpetool_mode_action" ) );
            act->set_active(0);
            mode = 0;
        } else {
            // switch to the chosen subtool
            SP_LPETOOL_CONTEXT(desktop->event_context)->mode = type;
        }

        if (DocumentUndo::getUndoSensitive(desktop->getDocument())) {
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            prefs->setInt( "/tools/lpetool/mode", mode );
        }

        g_object_set_data(tbl, "freeze", GINT_TO_POINTER(FALSE));
    }
}

static void sp_lpetool_toolbox_sel_modified(Inkscape::Selection *selection, guint /*flags*/, GObject * /*tbl*/)
{
    ToolBase *ec = selection->desktop()->event_context;
    if (SP_IS_LPETOOL_CONTEXT(ec)) {
        lpetool_update_measuring_items(SP_LPETOOL_CONTEXT(ec));
    }
}

static void sp_lpetool_toolbox_sel_changed(Inkscape::Selection *selection, GObject *tbl)
{
    using namespace Inkscape::LivePathEffect;
    ToolBase *ec = selection->desktop()->event_context;
    if (!SP_IS_LPETOOL_CONTEXT(ec)) {
        return;
    }
    LpeTool *lc = SP_LPETOOL_CONTEXT(ec);

    lpetool_delete_measuring_items(lc);
    lpetool_create_measuring_items(lc, selection);

    // activate line segment combo box if a single item with LPELineSegment is selected
    InkSelectOneAction* act =
        static_cast<InkSelectOneAction*>( g_object_get_data( tbl, "lpetool_line_segment_action" ) );

    SPItem *item = selection->singleItem();
    if (item && SP_IS_LPE_ITEM(item) && lpetool_item_has_construction(lc, item)) {

        SPLPEItem *lpeitem = SP_LPE_ITEM(item);
        Effect* lpe = lpeitem->getCurrentLPE();
        if (lpe && lpe->effectType() == LINE_SEGMENT) {
            LPELineSegment *lpels = static_cast<LPELineSegment*>(lpe);
            g_object_set_data(tbl, "currentlpe", lpe);
            g_object_set_data(tbl, "currentlpeitem", lpeitem);
            act->set_sensitive(true);
            act->set_active( lpels->end_type.get_value() );
        } else {
            g_object_set_data(tbl, "currentlpe", NULL);
            g_object_set_data(tbl, "currentlpeitem", NULL);
            act->set_sensitive(false);
        }

    } else {
        g_object_set_data(tbl, "currentlpe", NULL);
        g_object_set_data(tbl, "currentlpeitem", NULL);
        act->set_sensitive(false);
    }
}

static void lpetool_toggle_show_bbox(GtkToggleAction *act, gpointer data) {
    SPDesktop *desktop = static_cast<SPDesktop *>(data);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    bool show = gtk_toggle_action_get_active( act );
    prefs->setBool("/tools/lpetool/show_bbox",  show);

    if (tools_isactive(desktop, TOOLS_LPETOOL)) {
        LpeTool *lc = SP_LPETOOL_CONTEXT(desktop->event_context);
        lpetool_context_reset_limiting_bbox(lc);
    }
}

static void lpetool_toggle_show_measuring_info(GtkToggleAction *act, GObject *tbl)
{
    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data(tbl, "desktop"));
    if (!tools_isactive(desktop, TOOLS_LPETOOL)) {
        return;
    }

    bool show = gtk_toggle_action_get_active( act );

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/lpetool/show_measuring_info",  show);

    LpeTool *lc = SP_LPETOOL_CONTEXT(desktop->event_context);
    lpetool_show_measuring_info(lc, show);

    InkSelectOneAction* unitact =
        static_cast<InkSelectOneAction*>(g_object_get_data(tbl, "lpetool_units_action"));
    unitact->set_sensitive( show );
}

static void lpetool_unit_changed(GObject* tbl, int /* NotUsed */)
{
    UnitTracker* tracker = reinterpret_cast<UnitTracker*>(g_object_get_data(tbl, "tracker"));
    Unit const *unit = tracker->getActiveUnit();
    g_return_if_fail(unit != NULL);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setString("/tools/lpetool/unit", unit->abbr);

    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data( tbl, "desktop" ));
    if (SP_IS_LPETOOL_CONTEXT(desktop->event_context)) {
        LpeTool *lc = SP_LPETOOL_CONTEXT(desktop->event_context);
        lpetool_delete_measuring_items(lc);
        lpetool_create_measuring_items(lc);
    }
}

static void lpetool_toggle_set_bbox(GtkToggleAction *act, gpointer data)
{
    SPDesktop *desktop = static_cast<SPDesktop *>(data);
    Inkscape::Selection *selection = desktop->selection;

    Geom::OptRect bbox = selection->visualBounds();

    if (bbox) {
        Geom::Point A(bbox->min());
        Geom::Point B(bbox->max());

        A *= desktop->doc2dt();
        B *= desktop->doc2dt();

        // TODO: should we provide a way to store points in prefs?
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setDouble("/tools/lpetool/bbox_upperleftx", A[Geom::X]);
        prefs->setDouble("/tools/lpetool/bbox_upperlefty", A[Geom::Y]);
        prefs->setDouble("/tools/lpetool/bbox_lowerrightx", B[Geom::X]);
        prefs->setDouble("/tools/lpetool/bbox_lowerrighty", B[Geom::Y]);

        lpetool_context_reset_limiting_bbox(SP_LPETOOL_CONTEXT(desktop->event_context));
    }

    gtk_toggle_action_set_active(act, false);
}

static void sp_lpetool_change_line_segment_type(GObject* tbl, int mode)
{
    using namespace Inkscape::LivePathEffect;

    // quit if run by the attr_changed listener
    if (g_object_get_data(tbl, "freeze")) {
        return;
    }

    // in turn, prevent listener from responding
    g_object_set_data(tbl, "freeze", GINT_TO_POINTER(TRUE));

    LPELineSegment *lpe = static_cast<LPELineSegment *>(g_object_get_data(tbl, "currentlpe"));
    SPLPEItem *lpeitem = static_cast<SPLPEItem *>(g_object_get_data(tbl, "currentlpeitem"));
    if (lpeitem) {
        SPLPEItem *lpeitem = static_cast<SPLPEItem *>(g_object_get_data(tbl, "currentlpeitem"));
        lpe->end_type.param_set_value(static_cast<Inkscape::LivePathEffect::EndType>(mode));
        sp_lpe_item_update_patheffect(lpeitem, true, true);
    }

    g_object_set_data( tbl, "freeze", GINT_TO_POINTER(FALSE) );
}

static void lpetool_open_lpe_dialog(GtkToggleAction *act, gpointer data)
{
    SPDesktop *desktop = static_cast<SPDesktop *>(data);

    if (tools_isactive(desktop, TOOLS_LPETOOL)) {
        sp_action_perform(Inkscape::Verb::get(SP_VERB_DIALOG_LIVE_PATH_EFFECT)->get_action(Inkscape::ActionContext(desktop)), NULL);
    }
    gtk_toggle_action_set_active(act, false);
}

static void lpetool_toolbox_watch_ec(SPDesktop* dt, Inkscape::UI::Tools::ToolBase* ec, GObject* holder);

void sp_lpetool_toolbox_prep(SPDesktop *desktop, GtkActionGroup* mainActions, GObject* holder)
{
    UnitTracker* tracker = new UnitTracker(Inkscape::Util::UNIT_TYPE_LINEAR);
    tracker->setActiveUnit(desktop->getNamedView()->display_units);
    g_object_set_data(holder, "tracker", tracker);
    Unit const *unit = tracker->getActiveUnit();
    g_return_if_fail(unit != NULL);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setString("/tools/lpetool/unit", unit->abbr);

    /** Automatically create a list of LPEs that get added to the toolbar **/
    {
        InkSelectOneActionColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        // The first toggle button represents the state that no subtool is active.
        row = *(store->append());
        row[columns.col_label    ] = _("All inactive");
        row[columns.col_tooltip  ] = _("No geometric tool is active");
        row[columns.col_icon     ] = "draw-geometry-inactive";
        row[columns.col_sensitive] = true;

        Inkscape::LivePathEffect::EffectType type;
        for (int i = 1; i < num_subtools; ++i) { // i == 0 ia INVALIDE_LPE.

            type =  lpesubtools[i].type;

            row = *(store->append());
            row[columns.col_label    ] = Inkscape::LivePathEffect::LPETypeConverter.get_label(type);
            row[columns.col_tooltip  ] = _(Inkscape::LivePathEffect::LPETypeConverter.get_label(type).c_str());
            row[columns.col_icon     ] = lpesubtools[i].icon_name;
            row[columns.col_sensitive] = true;
        }

        InkSelectOneAction* act =
            InkSelectOneAction::create( "LPEToolModeAction",    // Name
                                        (""),                // Label
                                        (""),                // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        act->use_radio( true );
        act->use_icon( true );
        act->use_label( false );
        int mode = prefs->getInt("/tools/lpetool/mode", 0);
        act->set_active( mode );

        gtk_action_group_add_action( mainActions, act->gobj() );
        g_object_set_data( holder, "lpetool_mode_action", act );

        act->signal_changed().connect(sigc::bind<0>(sigc::ptr_fun(&sp_lpetool_mode_changed), holder));
    }

    /* Show limiting bounding box */
    {
        InkToggleAction* act = ink_toggle_action_new( "LPEShowBBoxAction",
                                                      _("Show limiting bounding box"),
                                                      _("Show bounding box (used to cut infinite lines)"),
                                                      "show-bounding-box",
                                                      GTK_ICON_SIZE_MENU );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(lpetool_toggle_show_bbox), desktop );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(act), prefs->getBool( "/tools/lpetool/show_bbox", true ) );
    }

    /* Set limiting bounding box to bbox of current selection */
    {
        InkToggleAction* act = ink_toggle_action_new( "LPEBBoxFromSelectionAction",
                                                      _("Get limiting bounding box from selection"),
                                                      _("Set limiting bounding box (used to cut infinite lines) to the bounding box of current selection"),
                                                      "draw-geometry-set-bounding-box",
                                                      GTK_ICON_SIZE_MENU );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(lpetool_toggle_set_bbox), desktop );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(act), FALSE );
    }


    /* Combo box to choose line segment type */
    {
        InkSelectOneActionColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = _("Closed");
        row[columns.col_tooltip  ] = ("");
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Open start");
        row[columns.col_tooltip  ] = ("");
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Open end");
        row[columns.col_tooltip  ] = ("");
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Open both");
        row[columns.col_tooltip  ] = ("");
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;

        InkSelectOneAction* act =
            InkSelectOneAction::create( "LPELineSegmentAction", // Name
                                        (""),                   // Label
                                        _("Choose a line segment type"), // Tooltip
                                        "Not Used",             // Icon
                                        store );                // Tree store

        act->use_radio( false );
        act->use_icon( false );
        act->use_label( true );
        act->set_sensitive( false );

        gtk_action_group_add_action( mainActions, GTK_ACTION( act->gobj() ));
        g_object_set_data (holder, "lpetool_line_segment_action", act );

        act->signal_changed().connect(sigc::bind<0>(sigc::ptr_fun(sp_lpetool_change_line_segment_type), holder));
    }

    /* Display measuring info for selected items */
    {
        InkToggleAction* act = ink_toggle_action_new( "LPEMeasuringAction",
                                                      _("Display measuring info"),
                                                      _("Display measuring info for selected items"),
                                                      "draw-geometry-show-measuring-info",
                                                      GTK_ICON_SIZE_MENU );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(lpetool_toggle_show_measuring_info), holder );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(act), prefs->getBool( "/tools/lpetool/show_measuring_info", true ) );
    }

    // Add the units menu
    {
        InkSelectOneAction* act = tracker->createAction( "LPEToolUnitsAction", _("Units"), ("") );
        gtk_action_group_add_action( mainActions, act->gobj() );
        act->signal_changed_after().connect(sigc::bind<0>(sigc::ptr_fun(&lpetool_unit_changed), holder));
        g_object_set_data(holder, "lpetool_units_action", act);
        act->set_sensitive( prefs->getBool("/tools/lpetool/show_measuring_info", true));
    }

    /* Open LPE dialog (to adapt parameters numerically) */
    {
        InkToggleAction* act = ink_toggle_action_new( "LPEOpenLPEDialogAction",
                                                      _("Open LPE dialog"),
                                                      _("Open LPE dialog (to adapt parameters numerically)"),
                                                      "dialog-geometry",
                                                      GTK_ICON_SIZE_MENU );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(lpetool_open_lpe_dialog), desktop );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(act), FALSE );
    }

    desktop->connectEventContextChanged(sigc::bind(sigc::ptr_fun(lpetool_toolbox_watch_ec), holder));
}

static void lpetool_toolbox_watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec, GObject* holder)
{
    static sigc::connection c_selection_modified;
    static sigc::connection c_selection_changed;

    if (SP_IS_LPETOOL_CONTEXT(ec)) {
        // Watch selection
        c_selection_modified = desktop->getSelection()->connectModified(sigc::bind(sigc::ptr_fun(sp_lpetool_toolbox_sel_modified), holder));
        c_selection_changed = desktop->getSelection()->connectChanged(sigc::bind(sigc::ptr_fun(sp_lpetool_toolbox_sel_changed), holder));
        sp_lpetool_toolbox_sel_changed(desktop->getSelection(), holder);
    } else {
        if (c_selection_modified)
            c_selection_modified.disconnect();
        if (c_selection_changed)
            c_selection_changed.disconnect();
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :

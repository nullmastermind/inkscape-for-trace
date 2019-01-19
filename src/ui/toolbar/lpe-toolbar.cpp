// SPDX-License-Identifier: GPL-2.0-or-later
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
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

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

namespace Inkscape {
namespace UI {
namespace Toolbar {
LPEToolbar::LPEToolbar(SPDesktop *desktop)
    : Toolbar(desktop),
      _tracker(new UnitTracker(Util::UNIT_TYPE_LINEAR)),
      _freeze(false),
      _currentlpe(nullptr),
      _currentlpeitem(nullptr)
{
    _tracker->setActiveUnit(_desktop->getNamedView()->display_units);
}

LPEToolbar::~LPEToolbar()
{
    delete _tracker;
}

GtkWidget *
LPEToolbar::prep(SPDesktop *desktop, GtkActionGroup* mainActions)
{
    auto toolbar = new LPEToolbar(desktop);
    Unit const *unit = toolbar->_tracker->getActiveUnit();
    g_return_val_if_fail(unit != nullptr, nullptr);

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

        toolbar->_mode_action =
            InkSelectOneAction::create( "LPEToolModeAction",    // Name
                                        (""),                // Label
                                        (""),                // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        toolbar->_mode_action->use_radio( true );
        toolbar->_mode_action->use_icon( true );
        toolbar->_mode_action->use_label( false );
        int mode = prefs->getInt("/tools/lpetool/mode", 0);
        toolbar->_mode_action->set_active( mode );

        gtk_action_group_add_action( mainActions, toolbar->_mode_action->gobj() );

        toolbar->_mode_action->signal_changed().connect(sigc::mem_fun(*toolbar, &LPEToolbar::mode_changed));
    }

    /* Show limiting bounding box */
    {
        InkToggleAction* act = ink_toggle_action_new( "LPEShowBBoxAction",
                                                      _("Show limiting bounding box"),
                                                      _("Show bounding box (used to cut infinite lines)"),
                                                      "show-bounding-box",
                                                      GTK_ICON_SIZE_MENU );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_show_bbox), desktop );
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
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_set_bbox), desktop );
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

        toolbar->_line_segment_action =
            InkSelectOneAction::create( "LPELineSegmentAction", // Name
                                        (""),                   // Label
                                        _("Choose a line segment type"), // Tooltip
                                        "Not Used",             // Icon
                                        store );                // Tree store

        toolbar->_line_segment_action->use_radio( false );
        toolbar->_line_segment_action->use_icon( false );
        toolbar->_line_segment_action->use_label( true );
        toolbar->_line_segment_action->set_sensitive( false );

        gtk_action_group_add_action( mainActions, GTK_ACTION( toolbar->_line_segment_action->gobj() ));

        toolbar->_line_segment_action->signal_changed().connect(sigc::mem_fun(*toolbar, &LPEToolbar::change_line_segment_type));
    }

    /* Display measuring info for selected items */
    {
        InkToggleAction* act = ink_toggle_action_new( "LPEMeasuringAction",
                                                      _("Display measuring info"),
                                                      _("Display measuring info for selected items"),
                                                      "draw-geometry-show-measuring-info",
                                                      GTK_ICON_SIZE_MENU );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_show_measuring_info), toolbar );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(act), prefs->getBool( "/tools/lpetool/show_measuring_info", true ) );
    }

    // Add the units menu
    {
        toolbar->_units_action = toolbar->_tracker->createAction( "LPEToolUnitsAction", _("Units"), ("") );
        gtk_action_group_add_action( mainActions, toolbar->_units_action->gobj() );
        toolbar->_units_action->signal_changed_after().connect(sigc::mem_fun(*toolbar, &LPEToolbar::unit_changed));
        toolbar->_units_action->set_sensitive( prefs->getBool("/tools/lpetool/show_measuring_info", true));
    }

    /* Open LPE dialog (to adapt parameters numerically) */
    {
        InkToggleAction* act = ink_toggle_action_new( "LPEOpenLPEDialogAction",
                                                      _("Open LPE dialog"),
                                                      _("Open LPE dialog (to adapt parameters numerically)"),
                                                      "dialog-geometry",
                                                      GTK_ICON_SIZE_MENU );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(open_lpe_dialog), desktop );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(act), FALSE );
    }

    desktop->connectEventContextChanged(sigc::mem_fun(*toolbar, &LPEToolbar::watch_ec));

    return GTK_WIDGET(toolbar->gobj());
}

// this is called when the mode is changed via the toolbar (i.e., one of the subtool buttons is pressed)
void
LPEToolbar::mode_changed(int mode)
{
    using namespace Inkscape::LivePathEffect;

    ToolBase *ec = _desktop->event_context;
    if (!SP_IS_LPETOOL_CONTEXT(ec)) {
        return;
    }

    // only take action if run by the attr_changed listener
    if (!_freeze) {
        // in turn, prevent listener from responding
        _freeze = true;

        EffectType type = lpesubtools[mode].type;

        LpeTool *lc = SP_LPETOOL_CONTEXT(_desktop->event_context);
        bool success = lpetool_try_construction(lc, type);
        if (success) {
            // since the construction was already performed, we set the state back to inactive
            _mode_action->set_active(0);
            mode = 0;
        } else {
            // switch to the chosen subtool
            SP_LPETOOL_CONTEXT(_desktop->event_context)->mode = type;
        }

        if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            prefs->setInt( "/tools/lpetool/mode", mode );
        }

        _freeze = false;
    }
}

void
LPEToolbar::toggle_show_bbox(GtkToggleAction *act, gpointer data) {
    SPDesktop *desktop = static_cast<SPDesktop *>(data);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    bool show = gtk_toggle_action_get_active( act );
    prefs->setBool("/tools/lpetool/show_bbox",  show);

    if (tools_isactive(desktop, TOOLS_LPETOOL)) {
        LpeTool *lc = SP_LPETOOL_CONTEXT(desktop->event_context);
        lpetool_context_reset_limiting_bbox(lc);
    }
}

void
LPEToolbar::toggle_set_bbox(GtkToggleAction *act, gpointer data)
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

void
LPEToolbar::change_line_segment_type(int mode)
{
    using namespace Inkscape::LivePathEffect;

    // quit if run by the attr_changed listener
    if (_freeze) {
        return;
    }

    // in turn, prevent listener from responding
    _freeze = true;
    auto line_seg = dynamic_cast<LPELineSegment *>(_currentlpe);

    if (_currentlpeitem && line_seg) {
        line_seg->end_type.param_set_value(static_cast<Inkscape::LivePathEffect::EndType>(mode));
        sp_lpe_item_update_patheffect(_currentlpeitem, true, true);
    }

    _freeze = false;
}

void
LPEToolbar::toggle_show_measuring_info(GtkToggleAction *act, gpointer data)
{
    auto toolbar = reinterpret_cast<LPEToolbar *>(data);

    if (!tools_isactive(toolbar->_desktop, TOOLS_LPETOOL)) {
        return;
    }

    bool show = gtk_toggle_action_get_active( act );

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/lpetool/show_measuring_info",  show);

    LpeTool *lc = SP_LPETOOL_CONTEXT(toolbar->_desktop->event_context);
    lpetool_show_measuring_info(lc, show);

    toolbar->_units_action->set_sensitive( show );
}

void
LPEToolbar::unit_changed(int /* NotUsed */)
{
    Unit const *unit = _tracker->getActiveUnit();
    g_return_if_fail(unit != nullptr);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setString("/tools/lpetool/unit", unit->abbr);

    if (SP_IS_LPETOOL_CONTEXT(_desktop->event_context)) {
        LpeTool *lc = SP_LPETOOL_CONTEXT(_desktop->event_context);
        lpetool_delete_measuring_items(lc);
        lpetool_create_measuring_items(lc);
    }
}

void
LPEToolbar::open_lpe_dialog(GtkToggleAction *act, gpointer data)
{
    SPDesktop *desktop = static_cast<SPDesktop *>(data);

    if (tools_isactive(desktop, TOOLS_LPETOOL)) {
        sp_action_perform(Inkscape::Verb::get(SP_VERB_DIALOG_LIVE_PATH_EFFECT)->get_action(Inkscape::ActionContext(desktop)), nullptr);
    }
    gtk_toggle_action_set_active(act, false);
}

void
LPEToolbar::watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec)
{
    if (SP_IS_LPETOOL_CONTEXT(ec)) {
        // Watch selection
        c_selection_modified = desktop->getSelection()->connectModified(sigc::mem_fun(*this, &LPEToolbar::sel_modified));
        c_selection_changed = desktop->getSelection()->connectChanged(sigc::mem_fun(*this, &LPEToolbar::sel_changed));
        sel_changed(desktop->getSelection());
    } else {
        if (c_selection_modified)
            c_selection_modified.disconnect();
        if (c_selection_changed)
            c_selection_changed.disconnect();
    }
}

void
LPEToolbar::sel_modified(Inkscape::Selection *selection, guint /*flags*/)
{
    ToolBase *ec = selection->desktop()->event_context;
    if (SP_IS_LPETOOL_CONTEXT(ec)) {
        lpetool_update_measuring_items(SP_LPETOOL_CONTEXT(ec));
    }
}

void
LPEToolbar::sel_changed(Inkscape::Selection *selection)
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
    SPItem *item = selection->singleItem();
    if (item && SP_IS_LPE_ITEM(item) && lpetool_item_has_construction(lc, item)) {

        SPLPEItem *lpeitem = SP_LPE_ITEM(item);
        Effect* lpe = lpeitem->getCurrentLPE();
        if (lpe && lpe->effectType() == LINE_SEGMENT) {
            LPELineSegment *lpels = static_cast<LPELineSegment*>(lpe);
            _currentlpe = lpe;
            _currentlpeitem = lpeitem;
            _line_segment_action->set_sensitive(true);
            _line_segment_action->set_active( lpels->end_type.get_value() );
        } else {
            _currentlpe = nullptr;
            _currentlpeitem = nullptr;
            _line_segment_action->set_sensitive(false);
        }

    } else {
        _currentlpe = nullptr;
        _currentlpeitem = nullptr;
        _line_segment_action->set_sensitive(false);
    }
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :

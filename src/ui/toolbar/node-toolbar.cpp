// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Node aux toolbar
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

#include "node-toolbar.h"

#include <glibmm/i18n.h>

#include <gtkmm/adjustment.h>

#include "desktop.h"
#include "document-undo.h"
#include "widgets/ink-toggle-action.h"
#include "widgets/ink-tool-menu-action.h"
#include "widgets/toolbox.h"
#include "inkscape.h"
#include "selection-chemistry.h"
#include "verbs.h"

#include "object/sp-namedview.h"

#include "ui/icon-names.h"
#include "ui/tool/control-point-selection.h"
#include "ui/tool/multi-path-manipulator.h"
#include "ui/tools/node-tool.h"
#include "ui/widget/ink-select-one-action.h"
#include "ui/widget/unit-tracker.h"

#include "widgets/ege-adjustment-action.h"
#include "widgets/widget-sizes.h"

using Inkscape::UI::Widget::UnitTracker;
using Inkscape::Util::Unit;
using Inkscape::Util::Quantity;
using Inkscape::DocumentUndo;
using Inkscape::UI::ToolboxFactory;
using Inkscape::UI::PrefPusher;
using Inkscape::Util::unit_table;
using Inkscape::UI::Tools::NodeTool;

//####################################
//# node editing callbacks
//####################################

/** Temporary hack: Returns the node tool in the active desktop.
 * Will go away during tool refactoring. */
static NodeTool *get_node_tool()
{
    NodeTool *tool = nullptr;
    if (SP_ACTIVE_DESKTOP ) {
        Inkscape::UI::Tools::ToolBase *ec = SP_ACTIVE_DESKTOP->event_context;
        if (INK_IS_NODE_TOOL(ec)) {
            tool = static_cast<NodeTool*>(ec);
        }
    }
    return tool;
}

static void sp_node_path_edit_add()
{
    NodeTool *nt = get_node_tool();
    if (nt) {
        nt->_multipath->insertNodes();
    }
}

static void sp_node_path_edit_add_min_x()
{
    NodeTool *nt = get_node_tool();
    if (nt) {
        nt->_multipath->insertNodesAtExtrema(Inkscape::UI::PointManipulator::EXTR_MIN_X);
    }
}
static void sp_node_path_edit_add_max_x()
{
    NodeTool *nt = get_node_tool();
    if (nt) {
        nt->_multipath->insertNodesAtExtrema(Inkscape::UI::PointManipulator::EXTR_MAX_X);
    }
}
static void sp_node_path_edit_add_min_y()
{
    NodeTool *nt = get_node_tool();
    if (nt) {
        nt->_multipath->insertNodesAtExtrema(Inkscape::UI::PointManipulator::EXTR_MIN_Y);
    }
}
static void sp_node_path_edit_add_max_y()
{
    NodeTool *nt = get_node_tool();
    if (nt) {
        nt->_multipath->insertNodesAtExtrema(Inkscape::UI::PointManipulator::EXTR_MAX_Y);
    }
}

static void sp_node_path_edit_delete()
{
    NodeTool *nt = get_node_tool();
    if (nt) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        nt->_multipath->deleteNodes(prefs->getBool("/tools/nodes/delete_preserves_shape", true));
    }
}

static void sp_node_path_edit_delete_segment()
{
    NodeTool *nt = get_node_tool();
    if (nt) {
        nt->_multipath->deleteSegments();
    }
}

static void sp_node_path_edit_break()
{
    NodeTool *nt = get_node_tool();
    if (nt) {
        nt->_multipath->breakNodes();
    }
}

static void sp_node_path_edit_join()
{
    NodeTool *nt = get_node_tool();
    if (nt) {
        nt->_multipath->joinNodes();
    }
}

static void sp_node_path_edit_join_segment()
{
    NodeTool *nt = get_node_tool();
    if (nt) {
        nt->_multipath->joinSegments();
    }
}

static void sp_node_path_edit_toline()
{
    NodeTool *nt = get_node_tool();
    if (nt) {
        nt->_multipath->setSegmentType(Inkscape::UI::SEGMENT_STRAIGHT);
    }
}

static void sp_node_path_edit_tocurve()
{
    NodeTool *nt = get_node_tool();
    if (nt) {
        nt->_multipath->setSegmentType(Inkscape::UI::SEGMENT_CUBIC_BEZIER);
    }
}

static void sp_node_path_edit_cusp()
{
    NodeTool *nt = get_node_tool();
    if (nt) {
        nt->_multipath->setNodeType(Inkscape::UI::NODE_CUSP);
    }
}

static void sp_node_path_edit_smooth()
{
    NodeTool *nt = get_node_tool();
    if (nt) {
        nt->_multipath->setNodeType(Inkscape::UI::NODE_SMOOTH);
    }
}

static void sp_node_path_edit_symmetrical()
{
    NodeTool *nt = get_node_tool();
    if (nt) {
        nt->_multipath->setNodeType(Inkscape::UI::NODE_SYMMETRIC);
    }
}

static void sp_node_path_edit_auto()
{
    NodeTool *nt = get_node_tool();
    if (nt) {
        nt->_multipath->setNodeType(Inkscape::UI::NODE_AUTO);
    }
}

static void sp_node_path_edit_nextLPEparam(GtkAction * /*act*/, gpointer data) {
    sp_selection_next_patheffect_param( reinterpret_cast<SPDesktop*>(data) );
}

namespace Inkscape {
namespace UI {
namespace Toolbar {

NodeToolbar::NodeToolbar(SPDesktop *desktop)
    : Toolbar(desktop),
    _tracker(new UnitTracker(Inkscape::Util::UNIT_TYPE_LINEAR))
{}

GtkWidget *
NodeToolbar::prep(SPDesktop *desktop, GtkActionGroup* mainActions)
{
    auto holder = new NodeToolbar(desktop);
    Unit doc_units = *desktop->getNamedView()->display_units;
    holder->_tracker->setActiveUnit(&doc_units);

    GtkIconSize secondarySize = ToolboxFactory::prefToSize("/toolbox/secondary", 1);

    {
        InkToolMenuAction* inky = ink_tool_menu_action_new( "NodeInsertAction",
                                                            _("Insert node"),
                                                            _("Insert new nodes into selected segments"),
                                                            INKSCAPE_ICON("node-add"),
                                                            secondarySize );
        gtk_action_set_short_label(GTK_ACTION(inky), _("Insert"));
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_node_path_edit_add), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
        GtkToolItem *menu_tool_button = gtk_menu_tool_button_new (nullptr, nullptr);
        gtk_activatable_set_related_action (GTK_ACTIVATABLE (menu_tool_button), GTK_ACTION(inky));
        // also create dummy menu action:
        gtk_action_group_add_action( mainActions, gtk_action_new("NodeInsertActionMenu", nullptr, nullptr, nullptr) );
    }

    {
        InkAction* inky = ink_action_new( "NodeInsertActionMinX",
                                          _("Insert node at min X"),
                                          _("Insert new nodes at min X into selected segments"),
                                          INKSCAPE_ICON("node_insert_min_x"),
                                          secondarySize );
        gtk_action_set_short_label( GTK_ACTION(inky), _("Insert min X"));
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_node_path_edit_add_min_x), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }
    {
        InkAction* inky = ink_action_new( "NodeInsertActionMaxX",
                                          _("Insert node at max X"),
                                          _("Insert new nodes at max X into selected segments"),
                                          INKSCAPE_ICON("node_insert_max_x"),
                                          secondarySize );
        g_object_set( inky, "short_label", _("Insert max X"), NULL );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_node_path_edit_add_max_x), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }
    {
        InkAction* inky = ink_action_new( "NodeInsertActionMinY",
                                          _("Insert node at min Y"),
                                          _("Insert new nodes at min Y into selected segments"),
                                          INKSCAPE_ICON("node_insert_min_y"),
                                          secondarySize );
        g_object_set( inky, "short_label", _("Insert min Y"), NULL );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_node_path_edit_add_min_y), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }
    {
        InkAction* inky = ink_action_new( "NodeInsertActionMaxY",
                                          _("Insert node at max Y"),
                                          _("Insert new nodes at max Y into selected segments"),
                                          INKSCAPE_ICON("node_insert_max_y"),
                                          secondarySize );
        g_object_set( inky, "short_label", _("Insert max Y"), NULL );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_node_path_edit_add_max_y), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }

    {
        InkAction* inky = ink_action_new( "NodeDeleteAction",
                                          _("Delete node"),
                                          _("Delete selected nodes"),
                                          INKSCAPE_ICON("node-delete"),
                                          secondarySize );
        g_object_set( inky, "short_label", _("Delete"), NULL );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_node_path_edit_delete), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }

    {
        InkAction* inky = ink_action_new( "NodeJoinAction",
                                          _("Join nodes"),
                                          _("Join selected nodes"),
                                          INKSCAPE_ICON("node-join"),
                                          secondarySize );
        g_object_set( inky, "short_label", _("Join"), NULL );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_node_path_edit_join), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }

    {
        InkAction* inky = ink_action_new( "NodeBreakAction",
                                          _("Break nodes"),
                                          _("Break path at selected nodes"),
                                          INKSCAPE_ICON("node-break"),
                                          secondarySize );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_node_path_edit_break), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }


    {
        InkAction* inky = ink_action_new( "NodeJoinSegmentAction",
                                          _("Join with segment"),
                                          _("Join selected endnodes with a new segment"),
                                          INKSCAPE_ICON("node-join-segment"),
                                          secondarySize );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_node_path_edit_join_segment), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }

    {
        InkAction* inky = ink_action_new( "NodeDeleteSegmentAction",
                                          _("Delete segment"),
                                          _("Delete segment between two non-endpoint nodes"),
                                          INKSCAPE_ICON("node-delete-segment"),
                                          secondarySize );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_node_path_edit_delete_segment), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }

    {
        InkAction* inky = ink_action_new( "NodeCuspAction",
                                          _("Node Cusp"),
                                          _("Make selected nodes corner"),
                                          INKSCAPE_ICON("node-type-cusp"),
                                          secondarySize );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_node_path_edit_cusp), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }

    {
        InkAction* inky = ink_action_new( "NodeSmoothAction",
                                          _("Node Smooth"),
                                          _("Make selected nodes smooth"),
                                          INKSCAPE_ICON("node-type-smooth"),
                                          secondarySize );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_node_path_edit_smooth), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }

    {
        InkAction* inky = ink_action_new( "NodeSymmetricAction",
                                          _("Node Symmetric"),
                                          _("Make selected nodes symmetric"),
                                          INKSCAPE_ICON("node-type-symmetric"),
                                          secondarySize );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_node_path_edit_symmetrical), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }

    {
        InkAction* inky = ink_action_new( "NodeAutoAction",
                                          _("Node Auto"),
                                          _("Make selected nodes auto-smooth"),
                                          INKSCAPE_ICON("node-type-auto-smooth"),
                                          secondarySize );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_node_path_edit_auto), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }

    {
        InkAction* inky = ink_action_new( "NodeLineAction",
                                          _("Node Line"),
                                          _("Make selected segments lines"),
                                          INKSCAPE_ICON("node-segment-line"),
                                          secondarySize );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_node_path_edit_toline), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }

    {
        InkAction* inky = ink_action_new( "NodeCurveAction",
                                          _("Node Curve"),
                                          _("Make selected segments curves"),
                                          INKSCAPE_ICON("node-segment-curve"),
                                          secondarySize );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_node_path_edit_tocurve), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }

    {
        InkToggleAction* act = ink_toggle_action_new( "NodesShowTransformHandlesAction",
                                                      _("Show Transform Handles"),
                                                      _("Show transformation handles for selected nodes"),
                                                      "node-transform",
                                                      secondarySize );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        holder->_pusher_show_transform_handles.reset(new PrefPusher(GTK_TOGGLE_ACTION(act), "/tools/nodes/show_transform_handles"));
    }

    {
        InkToggleAction* act = ink_toggle_action_new( "NodesShowHandlesAction",
                                                      _("Show Handles"),
                                                      _("Show Bezier handles of selected nodes"),
                                                      INKSCAPE_ICON("show-node-handles"),
                                                      secondarySize );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        holder->_pusher_show_handles.reset(new PrefPusher(GTK_TOGGLE_ACTION(act), "/tools/nodes/show_handles"));
    }

    {
        InkToggleAction* act = ink_toggle_action_new( "NodesShowHelperpath",
                                                      _("Show Outline"),
                                                      _("Show path outline (without path effects)"),
                                                      INKSCAPE_ICON("show-path-outline"),
                                                      secondarySize );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        holder->_pusher_show_outline.reset(new PrefPusher(GTK_TOGGLE_ACTION(act), "/tools/nodes/show_outline"));
    }

    {
        Inkscape::Verb* verb = Inkscape::Verb::get(SP_VERB_EDIT_NEXT_PATHEFFECT_PARAMETER);
        holder->_nodes_lpeedit = ink_action_new( verb->get_id(),
                                                 verb->get_name(),
                                                 verb->get_tip(),
                                                 INKSCAPE_ICON("path-effect-parameter-next"),
                                                 secondarySize );
        g_signal_connect_after( G_OBJECT(holder->_nodes_lpeedit), "activate", G_CALLBACK(sp_node_path_edit_nextLPEparam), desktop );
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_nodes_lpeedit) );
    }

    {
        InkToggleAction* inky = ink_toggle_action_new( "ObjectEditClipPathAction",
                                          _("Edit clipping paths"),
                                          _("Show clipping path(s) of selected object(s)"),
                                          INKSCAPE_ICON("path-clip-edit"),
                                          secondarySize );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
        holder->_pusher_edit_clipping_paths.reset(new PrefPusher(GTK_TOGGLE_ACTION(inky), "/tools/nodes/edit_clipping_paths"));
    }

    {
        InkToggleAction* inky = ink_toggle_action_new( "ObjectEditMaskPathAction",
                                          _("Edit masks"),
                                          _("Show mask(s) of selected object(s)"),
                                          INKSCAPE_ICON("path-mask-edit"),
                                          secondarySize );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
        holder->_pusher_edit_masks.reset(new PrefPusher(GTK_TOGGLE_ACTION(inky), "/tools/nodes/edit_masks"));
    }

    /* X coord of selected node(s) */
    {
        gchar const* labels[] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        gdouble values[] = {1, 2, 3, 5, 10, 20, 50, 100, 200, 500};
        holder->_nodes_x_action = create_adjustment_action( "NodeXAction",
                                                            _("X coordinate:"), _("X:"), _("X coordinate of selected node(s)"),
                                                            "/tools/nodes/Xcoord", 0,
                                                            TRUE, "altx-nodes",
                                                            -1e6, 1e6, SPIN_STEP, SPIN_PAGE_STEP,
                                                            labels, values, G_N_ELEMENTS(labels),
                                                            holder->_tracker.get() );
        ege_adjustment_action_set_focuswidget(holder->_nodes_x_action, GTK_WIDGET(desktop->canvas));
        holder->_nodes_x_adj = Glib::wrap(ege_adjustment_action_get_adjustment(holder->_nodes_x_action));
        holder->_nodes_x_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*holder, &NodeToolbar::value_changed), Geom::X));
        gtk_action_set_sensitive( GTK_ACTION(holder->_nodes_x_action), FALSE );
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_nodes_x_action) );
    }

    /* Y coord of selected node(s) */
    {
        gchar const* labels[] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        gdouble values[] = {1, 2, 3, 5, 10, 20, 50, 100, 200, 500};
        holder->_nodes_y_action = create_adjustment_action( "NodeYAction",
                                                            _("Y coordinate:"), _("Y:"), _("Y coordinate of selected node(s)"),
                                                            "/tools/nodes/Ycoord", 0,
                                                            FALSE, nullptr,
                                                            -1e6, 1e6, SPIN_STEP, SPIN_PAGE_STEP,
                                                            labels, values, G_N_ELEMENTS(labels),
                                                            holder->_tracker.get() );
        ege_adjustment_action_set_focuswidget(holder->_nodes_y_action, GTK_WIDGET(desktop->canvas));
        holder->_nodes_y_adj = Glib::wrap(ege_adjustment_action_get_adjustment(holder->_nodes_y_action));
        holder->_nodes_y_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*holder, &NodeToolbar::value_changed), Geom::Y));
        gtk_action_set_sensitive( GTK_ACTION(holder->_nodes_y_action), FALSE );
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_nodes_y_action) );
    }

    // add the units menu
    {
        InkSelectOneAction* act = holder->_tracker->createAction( "NodeUnitsAction", _("Units"), ("") );
        gtk_action_group_add_action( mainActions, act->gobj() );
    }

    holder->sel_changed(desktop->getSelection());
    desktop->connectEventContextChanged(sigc::mem_fun(*holder, &NodeToolbar::watch_ec));

    return GTK_WIDGET(holder->gobj());
} // NodeToolbar::prep()

void
NodeToolbar::value_changed(Geom::Dim2 d)
{
    auto adj = (d == Geom::X) ? _nodes_x_adj : _nodes_y_adj;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    if (!_tracker) {
        return;
    }

    Unit const *unit = _tracker->getActiveUnit();

    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        prefs->setDouble(Glib::ustring("/tools/nodes/") + (d == Geom::X ? "x" : "y"),
            Quantity::convert(adj->get_value(), unit, "px"));
    }

    // quit if run by the attr_changed listener
    if (_freeze || _tracker->isUpdating()) {
        return;
    }

    // in turn, prevent listener from responding
    _freeze = true;

    NodeTool *nt = get_node_tool();
    if (nt && !nt->_selected_nodes->empty()) {
        double val = Quantity::convert(adj->get_value(), unit, "px");
        double oldval = nt->_selected_nodes->pointwiseBounds()->midpoint()[d];
        Geom::Point delta(0,0);
        delta[d] = val - oldval;
        nt->_multipath->move(delta);
    }

    _freeze = false;
}

void
NodeToolbar::sel_changed(Inkscape::Selection *selection)
{
    SPItem *item = selection->singleItem();
    if (item && SP_IS_LPE_ITEM(item)) {
       if (SP_LPE_ITEM(item)->hasPathEffect()) {
           gtk_action_set_sensitive(GTK_ACTION(_nodes_lpeedit), TRUE);
       } else {
           gtk_action_set_sensitive(GTK_ACTION(_nodes_lpeedit), FALSE);
       }
    } else {
       gtk_action_set_sensitive(GTK_ACTION(_nodes_lpeedit), FALSE);
    }
}

void
NodeToolbar::watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec)
{
    if (INK_IS_NODE_TOOL(ec)) {
        // watch selection
        c_selection_changed = desktop->getSelection()->connectChanged(sigc::mem_fun(*this, &NodeToolbar::sel_changed));
        c_selection_modified = desktop->getSelection()->connectModified(sigc::mem_fun(*this, &NodeToolbar::sel_modified));
        c_subselection_changed = desktop->connectToolSubselectionChanged(sigc::mem_fun(*this, &NodeToolbar::coord_changed));

        sel_changed(desktop->getSelection());
    } else {
        if (c_selection_changed)
            c_selection_changed.disconnect();
        if (c_selection_modified)
            c_selection_modified.disconnect();
        if (c_subselection_changed)
            c_subselection_changed.disconnect();
    }
}

void
NodeToolbar::sel_modified(Inkscape::Selection *selection, guint /*flags*/)
{
    sel_changed(selection);
}

/* is called when the node selection is modified */
void
NodeToolbar::coord_changed(gpointer /*shape_editor*/)
{
    // quit if run by the attr_changed listener
    if (_freeze) {
        return;
    }

    // in turn, prevent listener from responding
    _freeze = true;

    if (!_tracker) {
        return;
    }
    Unit const *unit = _tracker->getActiveUnit();
    g_return_if_fail(unit != nullptr);

    NodeTool *nt = get_node_tool();
    if (!nt || !(nt->_selected_nodes) ||nt->_selected_nodes->empty()) {
        // no path selected
        gtk_action_set_sensitive(GTK_ACTION(_nodes_x_action), FALSE);
        gtk_action_set_sensitive(GTK_ACTION(_nodes_y_action), FALSE);
    } else {
        gtk_action_set_sensitive(GTK_ACTION(_nodes_x_action), TRUE);
        gtk_action_set_sensitive(GTK_ACTION(_nodes_y_action), TRUE);
        Geom::Coord oldx = Quantity::convert(_nodes_x_adj->get_value(), unit, "px");
        Geom::Coord oldy = Quantity::convert(_nodes_y_adj->get_value(), unit, "px");
        Geom::Point mid = nt->_selected_nodes->pointwiseBounds()->midpoint();

        if (oldx != mid[Geom::X]) {
            _nodes_x_adj->set_value(Quantity::convert(mid[Geom::X], "px", unit));
        }
        if (oldy != mid[Geom::Y]) {
            _nodes_y_adj->set_value(Quantity::convert(mid[Geom::Y], "px", unit));
        }
    }

    _freeze = false;
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

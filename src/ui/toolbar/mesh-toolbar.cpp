// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gradient aux toolbar
 *
 * Authors:
 *   bulia byak <bulia@dr.com>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Abhishek Sharma
 *   Tavmjong Bah <tavjong@free.fr>
 *
 * Copyright (C) 2012 Tavmjong Bah
 * Copyright (C) 2007 Johan Engelen
 * Copyright (C) 2005 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>
#include <gtkmm.h>

#include "mesh-toolbar.h"

#include "desktop-style.h"
#include "desktop.h"
#include "document-undo.h"
#include "gradient-chemistry.h"
#include "gradient-drag.h"
#include "widgets/ink-action.h"
#include "widgets/ink-toggle-action.h"
#include "inkscape.h"
#include "widgets/toolbox.h"
#include "verbs.h"

#include "object/sp-defs.h"
#include "object/sp-mesh-gradient.h"
#include "object/sp-stop.h"
#include "style.h"

#include "svg/css-ostringstream.h"

#include "ui/icon-names.h"
#include "ui/tools/gradient-tool.h"
#include "ui/tools/mesh-tool.h"
#include "ui/widget/color-preview.h"
#include "ui/widget/ink-select-one-action.h"

#include "widgets/ege-adjustment-action.h"
#include "widgets/gradient-image.h"
#include "widgets/spinbutton-events.h"

using Inkscape::DocumentUndo;
using Inkscape::UI::ToolboxFactory;
using Inkscape::UI::PrefPusher;
using Inkscape::UI::Tools::MeshTool;

static bool blocked = false;

//########################
//##        Mesh        ##
//########################


// Get a list of selected meshes taking into account fill/stroke toggles
std::vector<SPMeshGradient *>  ms_get_dt_selected_gradients(Inkscape::Selection *selection)
{
    std::vector<SPMeshGradient *> ms_selected;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool edit_fill   = prefs->getBool("/tools/mesh/edit_fill",   true);
    bool edit_stroke = prefs->getBool("/tools/mesh/edit_stroke", true);

    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;// get the items gradient, not the getVector() version
        SPStyle *style = item->style;

        if (style) {

            
            if (edit_fill   && style->fill.isPaintserver()) {
                SPPaintServer *server = item->style->getFillPaintServer();
                SPMeshGradient *mesh = dynamic_cast<SPMeshGradient *>(server);
                if (mesh) {
                    ms_selected.push_back(mesh);
                }
            }

            if (edit_stroke && style->stroke.isPaintserver()) {
                SPPaintServer *server = item->style->getStrokePaintServer();
                SPMeshGradient *mesh = dynamic_cast<SPMeshGradient *>(server);
                if (mesh) {
                    ms_selected.push_back(mesh);
                }
            }
        }

    }
    return ms_selected;
}


/*
 * Get the current selection status from the desktop
 */
void ms_read_selection( Inkscape::Selection *selection,
                        SPMeshGradient *&ms_selected,
                        bool &ms_selected_multi,
                        SPMeshType &ms_type,
                        bool &ms_type_multi )
{
    ms_selected = nullptr;
    ms_selected_multi = false;
    ms_type = SP_MESH_TYPE_COONS;
    ms_type_multi = false;

    bool first = true;

    // Read desktop selection, taking into account fill/stroke toggles
    std::vector<SPMeshGradient *> meshes = ms_get_dt_selected_gradients( selection );
    for (auto & meshe : meshes) {
        if (first) {
            ms_selected = meshe;
            ms_type = meshe->type;
            first = false;
        } else {
            if (ms_selected != meshe) {
                ms_selected_multi = true;
            }
            if (ms_type != meshe->type) {
                ms_type_multi = true;
            }
        }
    }
}


/*
 * Callback functions for user actions
 */


/** Temporary hack: Returns the mesh tool in the active desktop.
 * Will go away during tool refactoring. */
static MeshTool *get_mesh_tool()
{
    MeshTool *tool = nullptr;
    if (SP_ACTIVE_DESKTOP ) {
        Inkscape::UI::Tools::ToolBase *ec = SP_ACTIVE_DESKTOP->event_context;
        if (SP_IS_MESH_CONTEXT(ec)) {
            tool = static_cast<MeshTool*>(ec);
        }
    }
    return tool;
}

static void ms_toggle_sides()
{
    MeshTool *mt = get_mesh_tool();
    if (mt) {
        sp_mesh_context_corner_operation( mt, MG_CORNER_SIDE_TOGGLE );
    }
}

static void ms_make_elliptical()
{
    MeshTool *mt = get_mesh_tool();
    if (mt) {
        sp_mesh_context_corner_operation( mt, MG_CORNER_SIDE_ARC );
    }
}

static void ms_pick_colors()
{
    MeshTool *mt = get_mesh_tool();
    if (mt) {
        sp_mesh_context_corner_operation( mt, MG_CORNER_COLOR_PICK );
    }
}

static void ms_fit_mesh()
{
    MeshTool *mt = get_mesh_tool();
    if (mt) {
        sp_mesh_context_fit_mesh_in_bbox( mt );
    }
}


static void mesh_toolbox_watch_ec(SPDesktop* dt, Inkscape::UI::Tools::ToolBase* ec, GObject* holder);

namespace Inkscape {
namespace UI {
namespace Toolbar {
/**
 * Mesh auxiliary toolbar construction and setup.
 * Don't forget to add to XML in widgets/toolbox.cpp!
 *
 */
GtkWidget *
MeshToolbar::prep(SPDesktop * desktop, GtkActionGroup* mainActions)
{
    auto toolbar = new MeshToolbar(desktop);

    GtkIconSize secondarySize = ToolboxFactory::prefToSize("/toolbox/secondary", 1);

    EgeAdjustmentAction* eact = nullptr;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    /* New mesh: normal or conical */
    {
        InkSelectOneActionColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = _("normal");
        row[columns.col_tooltip  ] = _("Create mesh gradient");
        row[columns.col_icon     ] = INKSCAPE_ICON("paint-gradient-mesh");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("conical");
        row[columns.col_tooltip  ] = _("Create conical gradient");
        row[columns.col_icon     ] = INKSCAPE_ICON("paint-gradient-conical");
        row[columns.col_sensitive] = true;

        toolbar->_new_type_mode =
            InkSelectOneAction::create( "MeshNewTypeAction", // Name
                                        _("New:"),           // Label
                                        "",                  // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        toolbar->_new_type_mode->use_radio( true );
        toolbar->_new_type_mode->use_group_label( true );
        gint mode = prefs->getInt("/tools/mesh/mesh_geometry", SP_MESH_GEOMETRY_NORMAL);
        toolbar->_new_type_mode->set_active( mode );

        gtk_action_group_add_action( mainActions, GTK_ACTION( toolbar->_new_type_mode->gobj() ));

        toolbar->_new_type_mode->signal_changed().connect(sigc::mem_fun(*toolbar, &MeshToolbar::new_geometry_changed));
    }

    /* New gradient on fill or stroke*/
    {
        InkSelectOneActionColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = _("fill");
        row[columns.col_tooltip  ] = _("Create gradient in the fill");
        row[columns.col_icon     ] = INKSCAPE_ICON("object-fill");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("stroke");
        row[columns.col_tooltip  ] = _("Create gradient in the stroke");
        row[columns.col_icon     ] = INKSCAPE_ICON("object-stroke");
        row[columns.col_sensitive] = true;

        toolbar->_new_fillstroke_mode =
            InkSelectOneAction::create( "MeshNewFillStrokeAction", // Name
                                        "",                  // Label
                                        "",                  // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        toolbar->_new_fillstroke_mode->use_radio( true );
        toolbar->_new_fillstroke_mode->use_group_label( false );
        gint mode = prefs->getInt("/tools/mesh/newfillorstroke");
        toolbar->_new_fillstroke_mode->set_active( mode );

        gtk_action_group_add_action( mainActions, GTK_ACTION( toolbar->_new_fillstroke_mode->gobj() ));

        toolbar->_new_fillstroke_mode->signal_changed().connect(sigc::mem_fun(*toolbar, &MeshToolbar::new_fillstroke_changed));
    }

    /* Number of mesh rows */
    {
        gchar const** labels = nullptr;
        gdouble values[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        eact = create_adjustment_action( "MeshRowAction",
                                         _("Rows"), _("Rows:"), _("Number of rows in new mesh"),
                                         "/tools/mesh/mesh_rows", 1,
                                         FALSE, nullptr,
                                         1, 20, 1, 1,
                                         labels, values, 0,
                                         nullptr /*unit tracker*/,
                                         1.0, 0 );
        ege_adjustment_action_set_focuswidget(eact, GTK_WIDGET(desktop->canvas));
        toolbar->_row_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        toolbar->_row_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &MeshToolbar::row_changed));
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        gtk_action_set_sensitive( GTK_ACTION(eact), TRUE );
    }

    /* Number of mesh columns */
    {
        gchar const** labels = nullptr;
        gdouble values[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        eact = create_adjustment_action( "MeshColumnAction",
                                         _("Columns"), _("Columns:"), _("Number of columns in new mesh"),
                                         "/tools/mesh/mesh_cols", 1,
                                         FALSE, nullptr,
                                         1, 20, 1, 1,
                                         labels, values, 0,
                                         nullptr /*unit tracker*/,
                                         1.0, 0 );
        ege_adjustment_action_set_focuswidget(eact, GTK_WIDGET(desktop->canvas));
        toolbar->_col_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        toolbar->_col_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &MeshToolbar::col_changed));
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        gtk_action_set_sensitive( GTK_ACTION(eact), TRUE );
    }

    /* Edit fill mesh */
    {
        InkToggleAction* act = ink_toggle_action_new( "MeshEditFillAction",
                                                      _("Edit Fill"),
                                                      _("Edit fill mesh"),
                                                      INKSCAPE_ICON("object-fill"),
                                                      secondarySize );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        toolbar->_edit_fill_pusher.reset(new PrefPusher(GTK_TOGGLE_ACTION(act), "/tools/mesh/edit_fill"));
        g_signal_connect_after( G_OBJECT(act), "activate", G_CALLBACK(toggle_fill_stroke), (gpointer)toolbar);
    }

    /* Edit stroke mesh */
    {
        InkToggleAction* act = ink_toggle_action_new( "MeshEditStrokeAction",
                                                      _("Edit Stroke"),
                                                      _("Edit stroke mesh"),
                                                      INKSCAPE_ICON("object-stroke"),
                                                      secondarySize );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        toolbar->_edit_stroke_pusher.reset(new PrefPusher(GTK_TOGGLE_ACTION(act), "/tools/mesh/edit_stroke"));
        g_signal_connect_after( G_OBJECT(act), "activate", G_CALLBACK(toggle_fill_stroke), (gpointer)toolbar);
    }

    /* Show/hide side and tensor handles */
    {
        InkToggleAction* act = ink_toggle_action_new( "MeshShowHandlesAction",
                                                      _("Show Handles"),
                                                      _("Show handles"),
                                                      INKSCAPE_ICON("show-node-handles"),
                                                      secondarySize );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        toolbar->_show_handles_pusher.reset(new PrefPusher(GTK_TOGGLE_ACTION(act), "/tools/mesh/show_handles"));
        g_signal_connect_after( G_OBJECT(act), "activate", G_CALLBACK(toggle_handles), nullptr);
    }

    desktop->connectEventContextChanged(sigc::mem_fun(*toolbar, &MeshToolbar::watch_ec));

    /* Warning */
    {
        InkAction* act = ink_action_new( "MeshWarningAction",
                                         _("WARNING: Mesh SVG Syntax Subject to Change"),
                                         _("WARNING: Mesh SVG Syntax Subject to Change"),
                                         INKSCAPE_ICON("dialog-warning"),
                                         secondarySize );
        gtk_action_group_add_action( mainActions, GTK_ACTION(act) );
        g_signal_connect_after( G_OBJECT(act), "activate", G_CALLBACK(warning_popup), (gpointer)toolbar );
        gtk_action_set_sensitive( GTK_ACTION(act), TRUE );
    }

    /* Type */
    {
        InkSelectOneActionColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = C_("Type", "Coons");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Bicubic");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;

        // TRANSLATORS: Type of Smoothing. See https://en.wikipedia.org/wiki/Coons_patch
        toolbar->_select_type_action =
            InkSelectOneAction::create( "MeshSmoothAction",  // Name
                                        _("Smoothing"),      // Label
                                        _("Coons: no smothing. Bicubic: smothing across patch boundaries."),               // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        toolbar->_select_type_action->use_radio( false );
        toolbar->_select_type_action->use_label( true );
        toolbar->_select_type_action->use_icon( false );
        toolbar->_select_type_action->use_group_label( true );
        toolbar->_select_type_action->set_sensitive( false );
        toolbar->_select_type_action->set_active( 0 );

        gtk_action_group_add_action( mainActions, GTK_ACTION( toolbar->_select_type_action->gobj() ));

        toolbar->_select_type_action->signal_changed().connect(sigc::mem_fun(*toolbar, &MeshToolbar::type_changed));
    }

    {
        InkAction* act = ink_action_new( "MeshToggleSidesAction",
                                         _("Toggle Sides"),
                                         _("Toggle selected sides between Beziers and lines."),
                                         INKSCAPE_ICON("node-segment-line"),
                                         secondarySize );
        g_object_set( act, "short_label", _("Toggle side:"), NULL );
        g_signal_connect_after( G_OBJECT(act), "activate", G_CALLBACK(ms_toggle_sides), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(act) );
    }

    {
        InkAction* act = ink_action_new( "MeshMakeEllipticalAction",
                                         _("Make elliptical"),
                                         _("Make selected sides elliptical by changing length of handles. Works best if handles already approximate ellipse."),
                                         INKSCAPE_ICON("node-segment-curve"),
                                         secondarySize );
        g_object_set( act, "short_label", _("Make elliptical:"), NULL );
        g_signal_connect_after( G_OBJECT(act), "activate", G_CALLBACK(ms_make_elliptical), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(act) );
    }

    {
        InkAction* act = ink_action_new( "MeshPickColorsAction",
                                         _("Pick colors:"),
                                         _("Pick colors for selected corner nodes from underneath mesh."),
                                         INKSCAPE_ICON("color-picker"),
                                         secondarySize );
        g_object_set( act, "short_label", _("Pick Color"), NULL );
        g_signal_connect_after( G_OBJECT(act), "activate", G_CALLBACK(ms_pick_colors), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(act) );
    }


    {
        InkAction* act = ink_action_new( "MeshFitInBoundingBoxAction",
                                         _("Scale mesh to bounding box:"),
                                         _("Scale mesh to fit inside bounding box."),
                                         INKSCAPE_ICON("mesh-gradient-fit"),
                                         secondarySize );
        g_object_set( act, "short_label", _("Fit mesh"), NULL );
        g_signal_connect_after( G_OBJECT(act), "activate", G_CALLBACK(ms_fit_mesh), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(act) );
    }

    return GTK_WIDGET(toolbar->gobj());
}

void
MeshToolbar::new_geometry_changed(int mode)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt("/tools/mesh/mesh_geometry", mode);
}

void
MeshToolbar::new_fillstroke_changed(int mode)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt("/tools/mesh/newfillorstroke", mode);
}

void
MeshToolbar::row_changed()
{
    if (blocked) {
        return;
    }

    blocked = TRUE;

    int rows = _row_adj->get_value();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    prefs->setInt("/tools/mesh/mesh_rows", rows);

    blocked = FALSE;
}

void
MeshToolbar::col_changed()
{
    if (blocked) {
        return;
    }

    blocked = TRUE;

    int cols = _col_adj->get_value();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    prefs->setInt("/tools/mesh/mesh_cols", cols);

    blocked = FALSE;
}

void
MeshToolbar::toggle_fill_stroke(InkToggleAction * /*act*/, gpointer data)
{
    auto toolbar = reinterpret_cast<MeshToolbar *>(data);
    MeshTool *mt = get_mesh_tool();
    if (mt) {
        GrDrag *drag = mt->_grdrag;
        drag->updateDraggers();
        drag->updateLines();
        drag->updateLevels();
        toolbar->selection_changed(nullptr); // Need to update Type widget
    }
}

void
MeshToolbar::toggle_handles()
{
    MeshTool *mt = get_mesh_tool();
    if (mt) {
        GrDrag *drag = mt->_grdrag;
        drag->refreshDraggers();
    }
}

void
MeshToolbar::watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec)
{
    if (SP_IS_MESH_CONTEXT(ec)) {
        // connect to selection modified and changed signals
        Inkscape::Selection *selection = desktop->getSelection();
        SPDocument *document = desktop->getDocument();

        c_selection_changed = selection->connectChanged(sigc::mem_fun(*this, &MeshToolbar::selection_changed));
        c_selection_modified = selection->connectModified(sigc::mem_fun(*this, &MeshToolbar::selection_modified));
        c_subselection_changed = desktop->connectToolSubselectionChanged(sigc::mem_fun(*this, &MeshToolbar::drag_selection_changed));

        c_defs_release = document->getDefs()->connectRelease(sigc::mem_fun(*this, &MeshToolbar::defs_release));
        c_defs_modified = document->getDefs()->connectModified(sigc::mem_fun(*this, &MeshToolbar::defs_modified));
        selection_changed(selection);
    } else {
        if (c_selection_changed)
            c_selection_changed.disconnect();
        if (c_selection_modified)
            c_selection_modified.disconnect();
        if (c_subselection_changed)
            c_subselection_changed.disconnect();
        if (c_defs_release)
            c_defs_release.disconnect();
        if (c_defs_modified)
            c_defs_modified.disconnect();
    }
}

void
MeshToolbar::selection_modified(Inkscape::Selection *selection, guint /*flags*/)
{
    selection_changed(selection);
}

void
MeshToolbar::drag_selection_changed(gpointer /*dragger*/)
{
    selection_changed(nullptr);
}

void
MeshToolbar::defs_release(SPObject * /*defs*/)
{
    selection_changed(nullptr);
}

void
MeshToolbar::defs_modified(SPObject * /*defs*/, guint /*flags*/)
{
    selection_changed(nullptr);
}

/*
 * Core function, setup all the widgets whenever something changes on the desktop
 */
void
MeshToolbar::selection_changed(Inkscape::Selection * /* selection */)
{
    // std::cout << "ms_tb_selection_changed" << std::endl;

    if (blocked)
        return;

    if (!_desktop) {
        return;
    }

    Inkscape::Selection *selection = _desktop->getSelection(); // take from desktop, not from args
    if (selection) {
        // ToolBase *ev = sp_desktop_event_context(desktop);
        // GrDrag *drag = NULL;
        // if (ev) {
        //     drag = ev->get_drag();
        //     // Hide/show handles?
        // }

        SPMeshGradient *ms_selected = nullptr;
        SPMeshType ms_type = SP_MESH_TYPE_COONS;
        bool ms_selected_multi = false;
        bool ms_type_multi = false; 
        ms_read_selection( selection, ms_selected, ms_selected_multi, ms_type, ms_type_multi );
        // std::cout << "   type: " << ms_type << std::endl;
        
        if (_select_type_action) {
            _select_type_action->set_sensitive(!ms_type_multi);
            blocked = TRUE;
            _select_type_action->set_active(ms_type);
            blocked = FALSE;
        }
    }
}

void
MeshToolbar::warning_popup()
{
    char *msg = _("Mesh gradients are part of SVG 2:\n"
                  "* Syntax may change.\n"
                  "* Web browser implementation is not guaranteed.\n"
                  "\n"
                  "For web: convert to bitmap (Edit->Make bitmap copy).\n"
                  "For print: export to PDF.");
    Gtk::MessageDialog dialog(msg, false, Gtk::MESSAGE_WARNING,
                              Gtk::BUTTONS_OK, true);
    dialog.run();

}

/**
 * Sets mesh type: Coons, Bicubic
 */
void
MeshToolbar::type_changed(int mode)
{
    if (blocked) {
        return;
    }

    Inkscape::Selection *selection = _desktop->getSelection();
    std::vector<SPMeshGradient *> meshes = ms_get_dt_selected_gradients(selection);

    SPMeshType type = (SPMeshType) mode;
    for (auto & meshe : meshes) {
        meshe->type = type;
        meshe->type_set = true;
        meshe->updateRepr();
    }
    if (!meshes.empty() ) {
        DocumentUndo::done(_desktop->getDocument(), SP_VERB_CONTEXT_MESH,_("Set mesh type"));
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

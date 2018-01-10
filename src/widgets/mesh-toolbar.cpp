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
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtkmm.h>

#include "ui/widget/color-preview.h"
#include "toolbox.h"
#include "mesh-toolbar.h"

#include "verbs.h"

#include "widgets/spinbutton-events.h"
#include "widgets/gradient-vector.h"
#include "widgets/gradient-image.h"
#include "style.h"

#include "inkscape.h"
#include "document-private.h"
#include "document-undo.h"
#include "desktop.h"

#include <glibmm/i18n.h>

#include "ui/tools/gradient-tool.h"
#include "ui/tools/mesh-tool.h"
#include "ui/widget/ink-select-one-action.h"
#include "gradient-drag.h"
#include "sp-mesh-gradient.h"
#include "gradient-chemistry.h"
#include "ui/icon-names.h"

#include "widgets/ege-adjustment-action.h"
#include "ink-action.h"
#include "ink-radio-action.h"
#include "ink-toggle-action.h"

#include "sp-stop.h"
#include "svg/css-ostringstream.h"
#include "desktop-style.h"

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
    ms_selected = NULL;
    ms_selected_multi = false;
    ms_type = SP_MESH_TYPE_COONS;
    ms_type_multi = false;

    bool first = true;

    // Read desktop selection, taking into account fill/stroke toggles
    std::vector<SPMeshGradient *> meshes = ms_get_dt_selected_gradients( selection );
    for (auto i = meshes.begin(); i != meshes.end(); ++i) {
        if (first) {
            ms_selected = (*i);
            ms_type = (*i)->type;
            first = false;
        } else {
            if (ms_selected != (*i)) {
                ms_selected_multi = true;
            }
            if (ms_type != (*i)->type) {
                ms_type_multi = true;
            }
        }
    }
}

/*
 * Core function, setup all the widgets whenever something changes on the desktop
 */
static void ms_tb_selection_changed(Inkscape::Selection * /*selection*/, gpointer data)
{

    // std::cout << "ms_tb_selection_changed" << std::endl;

    if (blocked)
        return;

    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data(G_OBJECT(data), "desktop"));
    if (!desktop) {
        return;
    }

    Inkscape::Selection *selection = desktop->getSelection(); // take from desktop, not from args
    if (selection) {
        // ToolBase *ev = sp_desktop_event_context(desktop);
        // GrDrag *drag = NULL;
        // if (ev) {
        //     drag = ev->get_drag();
        //     // Hide/show handles?
        // }

        SPMeshGradient *ms_selected = 0;
        SPMeshType ms_type = SP_MESH_TYPE_COONS;
        bool ms_selected_multi = false;
        bool ms_type_multi = false; 
        ms_read_selection( selection, ms_selected, ms_selected_multi, ms_type, ms_type_multi );
        // std::cout << "   type: " << ms_type << std::endl;
        
        InkSelectOneAction* type = static_cast<InkSelectOneAction*> (g_object_get_data(G_OBJECT(data), "mesh_select_type_action"));
        if (type) {
            type->set_sensitive(!ms_type_multi);
            blocked = TRUE;
            type->set_active(ms_type);
            blocked = FALSE;
        }
    }
}


static void ms_tb_selection_modified(Inkscape::Selection *selection, guint /*flags*/, gpointer data)
{
    ms_tb_selection_changed(selection, data);
}

static void ms_drag_selection_changed(gpointer /*dragger*/, gpointer data)
{
    ms_tb_selection_changed(NULL, data);

}

static void ms_defs_release(SPObject * /*defs*/, GObject *widget)
{
    ms_tb_selection_changed(NULL, widget);
}

static void ms_defs_modified(SPObject * /*defs*/, guint /*flags*/, GObject *widget)
{
    ms_tb_selection_changed(NULL, widget);
}

/*
 * Callback functions for user actions
 */

static void ms_new_geometry_changed( GObject * /*tbl*/, int mode )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt("/tools/mesh/mesh_geometry", mode);
}

static void ms_new_fillstroke_changed( GObject * /*tbl*/, int mode )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt("/tools/mesh/newfillorstroke", mode);
}

static void ms_row_changed(GtkAdjustment *adj, GObject * /*tbl*/ )
{
    if (blocked) {
        return;
    }

    blocked = TRUE;

    int rows = gtk_adjustment_get_value(adj);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    prefs->setInt("/tools/mesh/mesh_rows", rows);

    blocked = FALSE;
}

static void ms_col_changed(GtkAdjustment *adj, GObject * /*tbl*/ )
{
    if (blocked) {
        return;
    }

    blocked = TRUE;

    int cols = gtk_adjustment_get_value(adj);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    prefs->setInt("/tools/mesh/mesh_cols", cols);

    blocked = FALSE;
}

/**
 * Sets mesh type: Coons, Bicubic
 */
static void ms_type_changed( GObject *tbl, int mode )
{
    if (blocked) {
        return;
    }

    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data(tbl, "desktop"));
    Inkscape::Selection *selection = desktop->getSelection();
    std::vector<SPMeshGradient *> meshes = ms_get_dt_selected_gradients(selection);

    SPMeshType type = (SPMeshType) mode;
    for (auto i = meshes.begin(); i != meshes.end(); ++i) {
        (*i)->type = type;
        (*i)->type_set = true;
        (*i)->updateRepr();
    }
    if (!meshes.empty() ) {
        DocumentUndo::done(desktop->getDocument(), SP_VERB_CONTEXT_MESH,_("Set mesh type"));
    }
}

/** Temporary hack: Returns the mesh tool in the active desktop.
 * Will go away during tool refactoring. */
static MeshTool *get_mesh_tool()
{
    MeshTool *tool = 0;
    if (SP_ACTIVE_DESKTOP ) {
        Inkscape::UI::Tools::ToolBase *ec = SP_ACTIVE_DESKTOP->event_context;
        if (SP_IS_MESH_CONTEXT(ec)) {
            tool = static_cast<MeshTool*>(ec);
        }
    }
    return tool;
}

static void ms_toggle_sides(void)
{
    MeshTool *mt = get_mesh_tool();
    if (mt) {
        sp_mesh_context_corner_operation( mt, MG_CORNER_SIDE_TOGGLE );
    }
}

static void ms_make_elliptical(void)
{
    MeshTool *mt = get_mesh_tool();
    if (mt) {
        sp_mesh_context_corner_operation( mt, MG_CORNER_SIDE_ARC );
    }
}

static void ms_pick_colors(void)
{
    MeshTool *mt = get_mesh_tool();
    if (mt) {
        sp_mesh_context_corner_operation( mt, MG_CORNER_COLOR_PICK );
    }
}

static void ms_fit_mesh(void)
{
    MeshTool *mt = get_mesh_tool();
    if (mt) {
        sp_mesh_context_fit_mesh_in_bbox( mt );
    }
}

static void ms_toggle_handles(void)
{
    MeshTool *mt = get_mesh_tool();
    if (mt) {
        GrDrag *drag = mt->_grdrag;
        drag->refreshDraggers();
    }
}

static void ms_toggle_fill_stroke(InkToggleAction * /*act*/, gpointer data)
{
    MeshTool *mt = get_mesh_tool();
    if (mt) {
        GrDrag *drag = mt->_grdrag;
        drag->updateDraggers();
        drag->updateLines();
        drag->updateLevels();
        ms_tb_selection_changed(NULL, data); // Need to update Type widget
    }
}

static void ms_warning_popup(void)
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

static void mesh_toolbox_watch_ec(SPDesktop* dt, Inkscape::UI::Tools::ToolBase* ec, GObject* holder);

/**
 * Mesh auxiliary toolbar construction and setup.
 * Don't forget to add to XML in widgets/toolbox.cpp!
 *
 */
void sp_mesh_toolbox_prep(SPDesktop * desktop, GtkActionGroup* mainActions, GObject* holder)
{
    GtkIconSize secondarySize = ToolboxFactory::prefToSize("/toolbox/secondary", 1);

    EgeAdjustmentAction* eact = 0;

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

        InkSelectOneAction* act =
            InkSelectOneAction::create( "MeshNewTypeAction", // Name
                                        _("New:"),           // Label
                                        _(""),               // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        act->use_radio( true );
        act->use_group_label( true );
        gint mode = prefs->getInt("/tools/mesh/mesh_geometry", SP_MESH_GEOMETRY_NORMAL);
        act->set_active( mode );

        gtk_action_group_add_action( mainActions, GTK_ACTION( act->gobj() ));
        g_object_set_data( holder, "mesh_new_type_mode", act );

        act->signal_changed().connect(sigc::bind<0>(sigc::ptr_fun(&ms_new_geometry_changed), holder));
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

        InkSelectOneAction* act =
            InkSelectOneAction::create( "MeshNewFillStrokeAction", // Name
                                        _(""),               // Label
                                        _(""),               // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        act->use_radio( true );
        act->use_group_label( false );
        gint mode = prefs->getInt("/tools/mesh/newfillorstroke");
        act->set_active( mode );

        gtk_action_group_add_action( mainActions, GTK_ACTION( act->gobj() ));
        g_object_set_data( holder, "mesh_new_type_mode", act );

        act->signal_changed().connect(sigc::bind<0>(sigc::ptr_fun(&ms_new_fillstroke_changed), holder));
    }

    /* Number of mesh rows */
    {
        gchar const** labels = NULL;
        gdouble values[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        eact = create_adjustment_action( "MeshRowAction",
                                         _("Rows"), _("Rows:"), _("Number of rows in new mesh"),
                                         "/tools/mesh/mesh_rows", 1,
                                         GTK_WIDGET(desktop->canvas), holder, FALSE, NULL,
                                         1, 20, 1, 1,
                                         labels, values, 0,
                                         ms_row_changed, NULL /*unit tracker*/,
                                         1.0, 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        gtk_action_set_sensitive( GTK_ACTION(eact), TRUE );
    }

    /* Number of mesh columns */
    {
        gchar const** labels = NULL;
        gdouble values[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        eact = create_adjustment_action( "MeshColumnAction",
                                         _("Columns"), _("Columns:"), _("Number of columns in new mesh"),
                                         "/tools/mesh/mesh_cols", 1,
                                         GTK_WIDGET(desktop->canvas), holder, FALSE, NULL,
                                         1, 20, 1, 1,
                                         labels, values, 0,
                                         ms_col_changed, NULL /*unit tracker*/,
                                         1.0, 0 );
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
        PrefPusher *pusher = new PrefPusher(GTK_TOGGLE_ACTION(act), "/tools/mesh/edit_fill");
        g_signal_connect( holder, "destroy", G_CALLBACK(delete_prefspusher), pusher);
        g_signal_connect_after( G_OBJECT(act), "activate", G_CALLBACK(ms_toggle_fill_stroke), holder);
    }

    /* Edit stroke mesh */
    {
        InkToggleAction* act = ink_toggle_action_new( "MeshEditStrokeAction",
                                                      _("Edit Stroke"),
                                                      _("Edit stroke mesh"),
                                                      INKSCAPE_ICON("object-stroke"),
                                                      secondarySize );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        PrefPusher *pusher = new PrefPusher(GTK_TOGGLE_ACTION(act), "/tools/mesh/edit_stroke");
        g_signal_connect( holder, "destroy", G_CALLBACK(delete_prefspusher), pusher);
        g_signal_connect_after( G_OBJECT(act), "activate", G_CALLBACK(ms_toggle_fill_stroke), holder);
    }

    /* Show/hide side and tensor handles */
    {
        InkToggleAction* act = ink_toggle_action_new( "MeshShowHandlesAction",
                                                      _("Show Handles"),
                                                      _("Show handles"),
                                                      INKSCAPE_ICON("show-node-handles"),
                                                      secondarySize );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        PrefPusher *pusher = new PrefPusher(GTK_TOGGLE_ACTION(act), "/tools/mesh/show_handles");
        g_signal_connect( holder, "destroy", G_CALLBACK(delete_prefspusher), pusher);
        g_signal_connect_after( G_OBJECT(act), "activate", G_CALLBACK(ms_toggle_handles), 0);
    }

    g_object_set_data(holder, "desktop", desktop);

    desktop->connectEventContextChanged(sigc::bind(sigc::ptr_fun(mesh_toolbox_watch_ec), holder));

    /* Warning */
    {
        InkAction* act = ink_action_new( "MeshWarningAction",
                                         _("WARNING: Mesh SVG Syntax Subject to Change"),
                                         _("WARNING: Mesh SVG Syntax Subject to Change"),
                                         INKSCAPE_ICON("dialog-warning"),
                                         secondarySize );
        gtk_action_group_add_action( mainActions, GTK_ACTION(act) );
        g_signal_connect_after( G_OBJECT(act), "activate", G_CALLBACK(ms_warning_popup), holder );
        gtk_action_set_sensitive( GTK_ACTION(act), TRUE );
    }

    /* Type */
    {
        InkSelectOneActionColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = C_("Type", "Coons");
        row[columns.col_tooltip  ] = _("");
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Bicubic");
        row[columns.col_tooltip  ] = _("");
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;

        // TRANSLATORS: Type of Smoothing. See https://en.wikipedia.org/wiki/Coons_patch
        InkSelectOneAction* act =
            InkSelectOneAction::create( "MeshSmoothAction",  // Name
                                        _("Smoothing"),      // Label
                                        _("Coons: no smothing. Bicubic: smothing across patch boundaries."),               // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        act->use_radio( false );
        act->use_label( true );
        act->use_icon( false );
        act->use_group_label( true );
        act->set_sensitive( false );
        act->set_active( 0 );

        gtk_action_group_add_action( mainActions, GTK_ACTION( act->gobj() ));
        g_object_set_data( holder, "mesh_select_type_action", act );

        act->signal_changed().connect(sigc::bind<0>(sigc::ptr_fun(&ms_type_changed), holder));
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

}

static void mesh_toolbox_watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec, GObject* holder)
{
    static sigc::connection c_selection_changed;
    static sigc::connection c_selection_modified;
    static sigc::connection c_subselection_changed;
    static sigc::connection c_defs_release;
    static sigc::connection c_defs_modified;

    if (SP_IS_MESH_CONTEXT(ec)) {
        // connect to selection modified and changed signals
        Inkscape::Selection *selection = desktop->getSelection();
        SPDocument *document = desktop->getDocument();

        c_selection_changed = selection->connectChanged(sigc::bind(sigc::ptr_fun(&ms_tb_selection_changed), holder));
        c_selection_modified = selection->connectModified(sigc::bind(sigc::ptr_fun(&ms_tb_selection_modified), holder));
        c_subselection_changed = desktop->connectToolSubselectionChanged(sigc::bind(sigc::ptr_fun(&ms_drag_selection_changed), holder));

        c_defs_release = document->getDefs()->connectRelease(sigc::bind<1>(sigc::ptr_fun(&ms_defs_release), holder));
        c_defs_modified = document->getDefs()->connectModified(sigc::bind<2>(sigc::ptr_fun(&ms_defs_modified), holder));
        ms_tb_selection_changed(selection, holder);
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

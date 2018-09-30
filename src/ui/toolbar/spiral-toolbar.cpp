/**
 * @file
 * Spiral aux toolbar
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

#include <gtk/gtk.h>
#include <glibmm/i18n.h>

#include "spiral-toolbar.h"

#include "desktop.h"
#include "document-undo.h"
#include "selection.h"
#include "widgets/toolbox.h"
#include "verbs.h"

#include "object/sp-spiral.h"

#include "ui/icon-names.h"
#include "ui/uxmanager.h"

#include "widgets/ege-adjustment-action.h"
#include "widgets/ege-output-action.h"
#include "widgets/ink-action.h"
#include "widgets/spinbutton-events.h"

#include "xml/node-event-vector.h"

using Inkscape::UI::UXManager;
using Inkscape::DocumentUndo;
using Inkscape::UI::ToolboxFactory;
using Inkscape::UI::PrefPusher;

//########################
//##       Spiral       ##
//########################

static void sp_spl_tb_value_changed(GtkAdjustment *adj, GObject *tbl, Glib::ustring const &value_name)
{
    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data( tbl, "desktop" ));

    if (DocumentUndo::getUndoSensitive(desktop->getDocument())) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setDouble("/tools/shapes/spiral/" + value_name,
            gtk_adjustment_get_value(adj));
    }

    // quit if run by the attr_changed listener
    if (g_object_get_data( tbl, "freeze" )) {
        return;
    }

    // in turn, prevent listener from responding
    g_object_set_data( tbl, "freeze", GINT_TO_POINTER(TRUE) );

    gchar* namespaced_name = g_strconcat("sodipodi:", value_name.data(), NULL);

    bool modmade = false;
    auto itemlist= desktop->getSelection()->items();
    for(auto i=itemlist.begin();i!=itemlist.end(); ++i){
        SPItem *item = *i;
        if (SP_IS_SPIRAL(item)) {
            Inkscape::XML::Node *repr = item->getRepr();
            sp_repr_set_svg_double( repr, namespaced_name,
                gtk_adjustment_get_value(adj) );
            item->updateRepr();
            modmade = true;
        }
    }

    g_free(namespaced_name);

    if (modmade) {
        DocumentUndo::done(desktop->getDocument(), SP_VERB_CONTEXT_SPIRAL,
                           _("Change spiral"));
    }

    g_object_set_data( tbl, "freeze", GINT_TO_POINTER(FALSE) );
}

static void sp_spl_tb_revolution_value_changed(GtkAdjustment *adj, GObject *tbl)
{
    sp_spl_tb_value_changed(adj, tbl, "revolution");
}

static void sp_spl_tb_expansion_value_changed(GtkAdjustment *adj, GObject *tbl)
{
    sp_spl_tb_value_changed(adj, tbl, "expansion");
}

static void sp_spl_tb_t0_value_changed(GtkAdjustment *adj, GObject *tbl)
{
    sp_spl_tb_value_changed(adj, tbl, "t0");
}

static void sp_spl_tb_defaults(GtkWidget * /*widget*/, GObject *obj)
{
    GtkWidget *tbl = GTK_WIDGET(obj);

    GtkAdjustment *adj;

    // fixme: make settable
    gdouble rev = 3;
    gdouble exp = 1.0;
    gdouble t0 = 0.0;

    adj = GTK_ADJUSTMENT(g_object_get_data(obj, "revolution"));
    gtk_adjustment_set_value(adj, rev);

#if !GTK_CHECK_VERSION(3,18,0)
    gtk_adjustment_value_changed(adj);
#endif

    adj = GTK_ADJUSTMENT(g_object_get_data(obj, "expansion"));
    gtk_adjustment_set_value(adj, exp);
    gtk_adjustment_value_changed(adj);

    adj = GTK_ADJUSTMENT(g_object_get_data(obj, "t0"));
    gtk_adjustment_set_value(adj, t0);

#if !GTK_CHECK_VERSION(3,18,0)
    gtk_adjustment_value_changed(adj);
#endif

    spinbutton_defocus(tbl);
}


static void spiral_tb_event_attr_changed(Inkscape::XML::Node *repr,
                                         gchar const * /*name*/,
                                         gchar const * /*old_value*/,
                                         gchar const * /*new_value*/,
                                         bool /*is_interactive*/,
                                         gpointer data)
{
    GtkWidget *tbl = GTK_WIDGET(data);

    // quit if run by the _changed callbacks
    if (g_object_get_data(G_OBJECT(tbl), "freeze")) {
        return;
    }

    // in turn, prevent callbacks from responding
    g_object_set_data(G_OBJECT(tbl), "freeze", GINT_TO_POINTER(TRUE));

    GtkAdjustment *adj;
    adj = GTK_ADJUSTMENT(g_object_get_data(G_OBJECT(tbl), "revolution"));
    double revolution = 3.0;
    sp_repr_get_double(repr, "sodipodi:revolution", &revolution);
    gtk_adjustment_set_value(adj, revolution);

    adj = GTK_ADJUSTMENT(g_object_get_data(G_OBJECT(tbl), "expansion"));
    double expansion = 1.0;
    sp_repr_get_double(repr, "sodipodi:expansion", &expansion);
    gtk_adjustment_set_value(adj, expansion);

    adj = GTK_ADJUSTMENT(g_object_get_data(G_OBJECT(tbl), "t0"));
    double t0 = 0.0;
    sp_repr_get_double(repr, "sodipodi:t0", &t0);
    gtk_adjustment_set_value(adj, t0);

    g_object_set_data(G_OBJECT(tbl), "freeze", GINT_TO_POINTER(FALSE));
}


static Inkscape::XML::NodeEventVector spiral_tb_repr_events = {
    nullptr, /* child_added */
    nullptr, /* child_removed */
    spiral_tb_event_attr_changed,
    nullptr, /* content_changed */
    nullptr  /* order_changed */
};

static void sp_spiral_toolbox_selection_changed(Inkscape::Selection *selection, GObject *tbl)
{
    int n_selected = 0;
    Inkscape::XML::Node *repr = nullptr;

    purge_repr_listener( tbl, tbl );

    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end(); ++i){
        SPItem *item = *i;
        if (SP_IS_SPIRAL(item)) {
            n_selected++;
            repr = item->getRepr();
        }
    }

    EgeOutputAction* act = EGE_OUTPUT_ACTION( g_object_get_data( tbl, "mode_action" ) );

    if (n_selected == 0) {
        g_object_set( G_OBJECT(act), "label", _("<b>New:</b>"), NULL );
    } else if (n_selected == 1) {
        g_object_set( G_OBJECT(act), "label", _("<b>Change:</b>"), NULL );

        if (repr) {
            g_object_set_data( tbl, "repr", repr );
            Inkscape::GC::anchor(repr);
            sp_repr_add_listener(repr, &spiral_tb_repr_events, tbl);
            sp_repr_synthesize_events(repr, &spiral_tb_repr_events, tbl);
        }
    } else {
        // FIXME: implement averaging of all parameters for multiple selected
        //gtk_label_set_markup(GTK_LABEL(l), _("<b>Average:</b>"));
        g_object_set( G_OBJECT(act), "label", _("<b>Change:</b>"), NULL );
    }
}


void sp_spiral_toolbox_prep(SPDesktop *desktop, GtkActionGroup* mainActions, GObject* holder)
{
    EgeAdjustmentAction* eact = nullptr;
    GtkIconSize secondarySize = ToolboxFactory::prefToSize("/toolbox/secondary", 1);

    {
        EgeOutputAction* act = ege_output_action_new( "SpiralStateAction", _("<b>New:</b>"), "", nullptr );
        ege_output_action_set_use_markup( act, TRUE );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        g_object_set_data( holder, "mode_action", act );
    }

    /* Revolution */
    {
        gchar const* labels[] = {_("just a curve"), nullptr, _("one full revolution"), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        gdouble values[] = {0.01, 0.5, 1, 2, 3, 5, 10, 20, 50, 100};
        eact = create_adjustment_action( "SpiralRevolutionAction",
                                         _("Number of turns"), _("Turns:"), _("Number of revolutions"),
                                         "/tools/shapes/spiral/revolution", 3.0,
                                         GTK_WIDGET(desktop->canvas), holder, TRUE, "altx-spiral",
                                         0.01, 1024.0, 0.1, 1.0,
                                         labels, values, G_N_ELEMENTS(labels),
                                         sp_spl_tb_revolution_value_changed, nullptr /*unit tracker*/, 1, 2);
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
    }

    /* Expansion */
    {
        gchar const* labels[] = {_("circle"), _("edge is much denser"), _("edge is denser"), _("even"), _("center is denser"), _("center is much denser"), nullptr};
        gdouble values[] = {0, 0.1, 0.5, 1, 1.5, 5, 20};
        eact = create_adjustment_action( "SpiralExpansionAction",
                                         _("Divergence"), _("Divergence:"), _("How much denser/sparser are outer revolutions; 1 = uniform"),
                                         "/tools/shapes/spiral/expansion", 1.0,
                                         GTK_WIDGET(desktop->canvas), holder, FALSE, nullptr,
                                         0.0, 1000.0, 0.01, 1.0,
                                         labels, values, G_N_ELEMENTS(labels),
                                         sp_spl_tb_expansion_value_changed);
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
    }

    /* T0 */
    {
        gchar const* labels[] = {_("starts from center"), _("starts mid-way"), _("starts near edge")};
        gdouble values[] = {0, 0.5, 0.9};
        eact = create_adjustment_action( "SpiralT0Action",
                                         _("Inner radius"), _("Inner radius:"), _("Radius of the innermost revolution (relative to the spiral size)"),
                                         "/tools/shapes/spiral/t0", 0.0,
                                         GTK_WIDGET(desktop->canvas), holder, FALSE, nullptr,
                                         0.0, 0.999, 0.01, 1.0,
                                         labels, values, G_N_ELEMENTS(labels),
                                         sp_spl_tb_t0_value_changed);
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
    }

    /* Reset */
    {
        InkAction* inky = ink_action_new( "SpiralResetAction",
                                          _("Defaults"),
                                          _("Reset shape parameters to defaults (use Inkscape Preferences > Tools to change defaults)"),
                                          INKSCAPE_ICON("edit-clear"),
                                          secondarySize );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_spl_tb_defaults), holder );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }


    sigc::connection *connection = new sigc::connection(
        desktop->getSelection()->connectChanged(sigc::bind(sigc::ptr_fun(sp_spiral_toolbox_selection_changed), holder))
        );
    g_signal_connect( holder, "destroy", G_CALLBACK(delete_connection), connection );
    g_signal_connect( holder, "destroy", G_CALLBACK(purge_repr_listener), holder );
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

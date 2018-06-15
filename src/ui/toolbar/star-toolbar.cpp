/**
 * @file
 * Star aux toolbar
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

#include <glibmm/i18n.h>

#include "star-toolbar.h"

#include "desktop.h"
#include "document-undo.h"
#include "widgets/ink-action.h"
#include "widgets/toolbox.h"
#include "selection.h"
#include "verbs.h"

#include "object/sp-star.h"

#include "ui/icon-names.h"
#include "ui/tools/star-tool.h"
#include "ui/uxmanager.h"
#include "ui/widget/ink-select-one-action.h"

#include "widgets/ege-adjustment-action.h"
#include "widgets/ege-output-action.h"

#include "xml/node-event-vector.h"

using Inkscape::UI::UXManager;
using Inkscape::DocumentUndo;
using Inkscape::UI::ToolboxFactory;
using Inkscape::UI::PrefPusher;


//########################
//##       Star         ##
//########################

static void sp_star_magnitude_value_changed( GtkAdjustment *adj, GObject *dataKludge )
{
    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data( dataKludge, "desktop" ));

    if (DocumentUndo::getUndoSensitive(desktop->getDocument())) {
        // do not remember prefs if this call is initiated by an undo change, because undoing object
        // creation sets bogus values to its attributes before it is deleted
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setInt("/tools/shapes/star/magnitude",
            (gint)gtk_adjustment_get_value(adj));
    }

    // quit if run by the attr_changed listener
    if (g_object_get_data( dataKludge, "freeze" )) {
        return;
    }

    // in turn, prevent listener from responding
    g_object_set_data( dataKludge, "freeze", GINT_TO_POINTER(TRUE) );

    bool modmade = false;

    Inkscape::Selection *selection = desktop->getSelection();
    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;
        if (SP_IS_STAR(item)) {
            Inkscape::XML::Node *repr = item->getRepr();
            sp_repr_set_int(repr,"sodipodi:sides",
                (gint)gtk_adjustment_get_value(adj));
            double arg1 = 0.5;
            sp_repr_get_double(repr, "sodipodi:arg1", &arg1);
            sp_repr_set_svg_double(repr, "sodipodi:arg2",
                                   (arg1 + M_PI / (gint)gtk_adjustment_get_value(adj)));
            item->updateRepr();
            modmade = true;
        }
    }
    if (modmade) {
        DocumentUndo::done(desktop->getDocument(), SP_VERB_CONTEXT_STAR,
                           _("Star: Change number of corners"));
    }

    g_object_set_data( dataKludge, "freeze", GINT_TO_POINTER(FALSE) );
}

static void sp_star_proportion_value_changed( GtkAdjustment *adj, GObject *dataKludge )
{
    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data( dataKludge, "desktop" ));

    if (DocumentUndo::getUndoSensitive(desktop->getDocument())) {
        if (!IS_NAN(gtk_adjustment_get_value(adj))) {
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            prefs->setDouble("/tools/shapes/star/proportion",
                gtk_adjustment_get_value(adj));
        }
    }

    // quit if run by the attr_changed listener
    if (g_object_get_data( dataKludge, "freeze" )) {
        return;
    }

    // in turn, prevent listener from responding
    g_object_set_data( dataKludge, "freeze", GINT_TO_POINTER(TRUE) );

    bool modmade = false;
    Inkscape::Selection *selection = desktop->getSelection();
    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;
        if (SP_IS_STAR(item)) {
            Inkscape::XML::Node *repr = item->getRepr();

            gdouble r1 = 1.0;
            gdouble r2 = 1.0;
            sp_repr_get_double(repr, "sodipodi:r1", &r1);
            sp_repr_get_double(repr, "sodipodi:r2", &r2);
            if (r2 < r1) {
                sp_repr_set_svg_double(repr, "sodipodi:r2",
                r1*gtk_adjustment_get_value(adj));
            } else {
                sp_repr_set_svg_double(repr, "sodipodi:r1",
                r2*gtk_adjustment_get_value(adj));
            }

            item->updateRepr();
            modmade = true;
        }
    }

    if (modmade) {
        DocumentUndo::done(desktop->getDocument(), SP_VERB_CONTEXT_STAR,
                           _("Star: Change spoke ratio"));
    }

    g_object_set_data( dataKludge, "freeze", GINT_TO_POINTER(FALSE) );
}

static void sp_star_side_mode_changed( GObject *dataKludge, int mode )
{
    bool flat = (mode == 0);

    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data( dataKludge, "desktop" ));

    if (DocumentUndo::getUndoSensitive(desktop->getDocument())) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setBool( "/tools/shapes/star/isflatsided", flat );
    }

    // quit if run by the attr_changed listener
    if (g_object_get_data( dataKludge, "freeze" )) {
        return;
    }

    // in turn, prevent listener from responding
    g_object_set_data( dataKludge, "freeze", GINT_TO_POINTER(TRUE) );

    Inkscape::Selection *selection = desktop->getSelection();
    GtkAction* prop_action = GTK_ACTION( g_object_get_data( dataKludge, "prop_action" ) );
    bool modmade = false;

    if ( prop_action ) {
        gtk_action_set_visible( prop_action, !flat );
    }

    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;
        if (SP_IS_STAR(item)) {
            Inkscape::XML::Node *repr = item->getRepr();
            repr->setAttribute("inkscape:flatsided", flat ? "true" : "false" );
            item->updateRepr();
            modmade = true;
        }
    }

    if (modmade) {
        DocumentUndo::done(desktop->getDocument(), SP_VERB_CONTEXT_STAR,
                           flat ? _("Make polygon") : _("Make star"));
    }

    g_object_set_data( dataKludge, "freeze", GINT_TO_POINTER(FALSE) );
}

static void sp_star_rounded_value_changed( GtkAdjustment *adj, GObject *dataKludge )
{
    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data( dataKludge, "desktop" ));

    if (DocumentUndo::getUndoSensitive(desktop->getDocument())) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setDouble("/tools/shapes/star/rounded", (gdouble) gtk_adjustment_get_value(adj));
    }

    // quit if run by the attr_changed listener
    if (g_object_get_data( dataKludge, "freeze" )) {
        return;
    }

    // in turn, prevent listener from responding
    g_object_set_data( dataKludge, "freeze", GINT_TO_POINTER(TRUE) );

    bool modmade = false;

    Inkscape::Selection *selection = desktop->getSelection();
    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;
        if (SP_IS_STAR(item)) {
            Inkscape::XML::Node *repr = item->getRepr();
            sp_repr_set_svg_double(repr, "inkscape:rounded",
                (gdouble) gtk_adjustment_get_value(adj));
            item->updateRepr();
            modmade = true;
        }
    }
    if (modmade) {
        DocumentUndo::done(desktop->getDocument(), SP_VERB_CONTEXT_STAR,
                           _("Star: Change rounding"));
    }

    g_object_set_data( dataKludge, "freeze", GINT_TO_POINTER(FALSE) );
}

static void sp_star_randomized_value_changed( GtkAdjustment *adj, GObject *dataKludge )
{
    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data( dataKludge, "desktop" ));

    if (DocumentUndo::getUndoSensitive(desktop->getDocument())) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setDouble("/tools/shapes/star/randomized",
            (gdouble) gtk_adjustment_get_value(adj));
    }

    // quit if run by the attr_changed listener
    if (g_object_get_data( dataKludge, "freeze" )) {
        return;
    }

    // in turn, prevent listener from responding
    g_object_set_data( dataKludge, "freeze", GINT_TO_POINTER(TRUE) );

    bool modmade = false;

    Inkscape::Selection *selection = desktop->getSelection();
    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;
        if (SP_IS_STAR(item)) {
            Inkscape::XML::Node *repr = item->getRepr();
            sp_repr_set_svg_double(repr, "inkscape:randomized",
                (gdouble) gtk_adjustment_get_value(adj));
            item->updateRepr();
            modmade = true;
        }
    }
    if (modmade) {
        DocumentUndo::done(desktop->getDocument(), SP_VERB_CONTEXT_STAR,
                           _("Star: Change randomization"));
    }

    g_object_set_data( dataKludge, "freeze", GINT_TO_POINTER(FALSE) );
}


static void star_tb_event_attr_changed(Inkscape::XML::Node *repr, gchar const *name,
                                       gchar const * /*old_value*/, gchar const * /*new_value*/,
                                       bool /*is_interactive*/, gpointer dataPointer)
{
    GObject *dataKludge = G_OBJECT( dataPointer );

    // quit if run by the _changed callbacks
    if (g_object_get_data(dataKludge, "freeze")) {
        return;
    }

    // in turn, prevent callbacks from responding
    g_object_set_data(dataKludge, "freeze", GINT_TO_POINTER(TRUE));

    GtkAdjustment *adj = nullptr;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool isFlatSided = prefs->getBool("/tools/shapes/star/isflatsided", true);

    if (!strcmp(name, "inkscape:randomized")) {

        adj = GTK_ADJUSTMENT( g_object_get_data(dataKludge, "randomized") );
        double randomized = 0.0;
        sp_repr_get_double(repr, "inkscape:randomized", &randomized);
        gtk_adjustment_set_value(adj, randomized);

    } else if (!strcmp(name, "inkscape:rounded")) {

        adj = GTK_ADJUSTMENT( g_object_get_data(dataKludge, "rounded") );
        double rounded = 0.0;
        sp_repr_get_double(repr, "inkscape:rounded", &rounded);
        gtk_adjustment_set_value(adj, rounded);

    } else if (!strcmp(name, "inkscape:flatsided")) {

        GtkAction* prop_action = GTK_ACTION( g_object_get_data(dataKludge, "prop_action") );
        char const *flatsides = repr->attribute("inkscape:flatsided");

        InkSelectOneAction* flat_action =
            static_cast<InkSelectOneAction *>(g_object_get_data( dataKludge, "flat_action" ) );
        if ( flatsides && !strcmp(flatsides,"false") ) {
            flat_action->set_active(1);
            gtk_action_set_visible( prop_action, TRUE );
        } else {
            flat_action->set_active(0);
            gtk_action_set_visible( prop_action, FALSE );
        }

    } else if ((!strcmp(name, "sodipodi:r1") || !strcmp(name, "sodipodi:r2")) && (!isFlatSided) ) {

        adj = GTK_ADJUSTMENT(g_object_get_data(dataKludge, "proportion"));
        gdouble r1 = 1.0;
        gdouble r2 = 1.0;
        sp_repr_get_double(repr, "sodipodi:r1", &r1);
        sp_repr_get_double(repr, "sodipodi:r2", &r2);
        if (r2 < r1) {
            gtk_adjustment_set_value(adj, r2/r1);
        } else {
            gtk_adjustment_set_value(adj, r1/r2);
        }

    } else if (!strcmp(name, "sodipodi:sides")) {

        adj = GTK_ADJUSTMENT(g_object_get_data(dataKludge, "magnitude"));
        int sides = 0;
        sp_repr_get_int(repr, "sodipodi:sides", &sides);
        gtk_adjustment_set_value(adj, sides);
    }

    g_object_set_data(dataKludge, "freeze", GINT_TO_POINTER(FALSE));
}


static Inkscape::XML::NodeEventVector star_tb_repr_events =
{
    nullptr, /* child_added */
    nullptr, /* child_removed */
    star_tb_event_attr_changed,
    nullptr, /* content_changed */
    nullptr  /* order_changed */
};


/**
 *  \param selection Should not be NULL.
 */
static void
sp_star_toolbox_selection_changed(Inkscape::Selection *selection, GObject *dataKludge)
{
    int n_selected = 0;
    Inkscape::XML::Node *repr = nullptr;

    purge_repr_listener( dataKludge, dataKludge );

    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;
        if (SP_IS_STAR(item)) {
            n_selected++;
            repr = item->getRepr();
        }
    }

    EgeOutputAction* act = EGE_OUTPUT_ACTION( g_object_get_data( dataKludge, "mode_action" ) );

    if (n_selected == 0) {
        g_object_set( G_OBJECT(act), "label", _("<b>New:</b>"), NULL );
    } else if (n_selected == 1) {
        g_object_set( G_OBJECT(act), "label", _("<b>Change:</b>"), NULL );

        if (repr) {
            g_object_set_data( dataKludge, "repr", repr );
            Inkscape::GC::anchor(repr);
            sp_repr_add_listener(repr, &star_tb_repr_events, dataKludge);
            sp_repr_synthesize_events(repr, &star_tb_repr_events, dataKludge);
        }
    } else {
        // FIXME: implement averaging of all parameters for multiple selected stars
        //gtk_label_set_markup(GTK_LABEL(l), _("<b>Average:</b>"));
        //gtk_label_set_markup(GTK_LABEL(l), _("<b>Change:</b>"));
    }
}


static void sp_star_defaults( GtkWidget * /*widget*/, GObject *dataKludge )
{
    // FIXME: in this and all other _default functions, set some flag telling the value_changed
    // callbacks to lump all the changes for all selected objects in one undo step

    GtkAdjustment *adj = nullptr;

    // fixme: make settable in prefs!
    gint mag = 5;
    gdouble prop = 0.5;
    gboolean flat = FALSE;
    gdouble randomized = 0;
    gdouble rounded = 0;

    InkSelectOneAction* flat_action =
        static_cast<InkSelectOneAction *>(g_object_get_data( dataKludge, "flat_action" ) );
    flat_action->set_active ( flat ? 0 : 1 );

    GtkAction* sb2 = GTK_ACTION( g_object_get_data( dataKludge, "prop_action" ) );
    gtk_action_set_visible( sb2, !flat );

    adj = GTK_ADJUSTMENT( g_object_get_data( dataKludge, "magnitude" ) );
    gtk_adjustment_set_value(adj, mag);

#if !GTK_CHECK_VERSION(3,18,0)
    gtk_adjustment_value_changed(adj);
#endif

    adj = GTK_ADJUSTMENT( g_object_get_data( dataKludge, "proportion" ) );
    gtk_adjustment_set_value(adj, prop);

#if !GTK_CHECK_VERSION(3,18,0)
    gtk_adjustment_value_changed(adj);
#endif

    adj = GTK_ADJUSTMENT( g_object_get_data( dataKludge, "rounded" ) );
    gtk_adjustment_set_value(adj, rounded);

#if !GTK_CHECK_VERSION(3,18,0)
    gtk_adjustment_value_changed(adj);
#endif

    adj = GTK_ADJUSTMENT( g_object_get_data( dataKludge, "randomized" ) );
    gtk_adjustment_set_value(adj, randomized);

#if !GTK_CHECK_VERSION(3,18,0)
    gtk_adjustment_value_changed(adj);
#endif
}

static void star_toolbox_watch_ec(SPDesktop* dt, Inkscape::UI::Tools::ToolBase* ec, GObject* dataKludge);

void sp_star_toolbox_prep(SPDesktop *desktop, GtkActionGroup* mainActions, GObject* dataKludge)
{
    GtkIconSize secondarySize = ToolboxFactory::prefToSize("/toolbox/secondary", 1);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool isFlatSided = prefs->getBool("/tools/shapes/star/isflatsided", true);

    EgeAdjustmentAction* eact = nullptr;

    {
        EgeOutputAction* act = ege_output_action_new( "StarStateAction", _("<b>New:</b>"), "", nullptr );
        ege_output_action_set_use_markup( act, TRUE );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
        g_object_set_data( dataKludge, "mode_action", act );
    }

    /* Flatsided checkbox */
    {
        InkSelectOneActionColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = _("Polygon");
        row[columns.col_tooltip  ] = _("Regular polygon (with one handle) instead of a star");
        row[columns.col_icon     ] = INKSCAPE_ICON("draw-polygon");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Star");
        row[columns.col_tooltip  ] = _("Star instead of a regular polygon (with one handle)");
        row[columns.col_icon     ] = INKSCAPE_ICON("draw-star");
        row[columns.col_sensitive] = true;

        InkSelectOneAction* act =
            InkSelectOneAction::create( "FlatAction",        // Name
                                        (""),                // Label
                                        (""),                // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store
        act->use_radio( true );
        act->use_label( false );
        act->set_active( isFlatSided ? 0 : 1 );

        gtk_action_group_add_action( mainActions, GTK_ACTION( act->gobj() ));
        g_object_set_data( dataKludge, "flat_action", act );

        act->signal_changed().connect(sigc::bind<0>(sigc::ptr_fun(&sp_star_side_mode_changed), dataKludge));
    }

    /* Magnitude */
    {
        gchar const* labels[] = {_("triangle/tri-star"), _("square/quad-star"), _("pentagon/five-pointed star"), _("hexagon/six-pointed star"), nullptr, nullptr, nullptr, nullptr, nullptr};
        gdouble values[] = {3, 4, 5, 6, 7, 8, 10, 12, 20};
        eact = create_adjustment_action( "MagnitudeAction",
                                         _("Corners"), _("Corners:"), _("Number of corners of a polygon or star"),
                                         "/tools/shapes/star/magnitude", 3,
                                         GTK_WIDGET(desktop->canvas), dataKludge, FALSE, nullptr,
                                         3, 1024, 1, 5,
                                         labels, values, G_N_ELEMENTS(labels),
                                         sp_star_magnitude_value_changed, nullptr /*unit tracker*/,
                                         1.0, 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        gtk_action_set_sensitive( GTK_ACTION(eact), TRUE );
    }

    /* Spoke ratio */
    {
        gchar const* labels[] = {_("thin-ray star"), nullptr, _("pentagram"), _("hexagram"), _("heptagram"), _("octagram"), _("regular polygon")};
        gdouble values[] = {0.01, 0.2, 0.382, 0.577, 0.692, 0.765, 1};
        eact = create_adjustment_action( "SpokeAction",
                                         _("Spoke ratio"), _("Spoke ratio:"),
                                         // TRANSLATORS: Tip radius of a star is the distance from the center to the farthest handle.
                                         // Base radius is the same for the closest handle.
                                         _("Base radius to tip radius ratio"),
                                         "/tools/shapes/star/proportion", 0.5,
                                         GTK_WIDGET(desktop->canvas), dataKludge, FALSE, nullptr,
                                         0.01, 1.0, 0.01, 0.1,
                                         labels, values, G_N_ELEMENTS(labels),
                                         sp_star_proportion_value_changed );
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        g_object_set_data( dataKludge, "prop_action", eact );

        if ( !isFlatSided ) {
            gtk_action_set_visible( GTK_ACTION(eact), TRUE );
        } else {
            gtk_action_set_visible( GTK_ACTION(eact), FALSE );
        }
    }

    /* Roundedness */
    {
        gchar const* labels[] = {_("stretched"), _("twisted"), _("slightly pinched"), _("NOT rounded"), _("slightly rounded"),
                                 _("visibly rounded"), _("well rounded"), _("amply rounded"), nullptr, _("stretched"), _("blown up")};
        gdouble values[] = {-1, -0.2, -0.03, 0, 0.05, 0.1, 0.2, 0.3, 0.5, 1, 10};
        eact = create_adjustment_action( "RoundednessAction",
                                         _("Rounded"), _("Rounded:"), _("How much rounded are the corners (0 for sharp)"),
                                         "/tools/shapes/star/rounded", 0.0,
                                         GTK_WIDGET(desktop->canvas), dataKludge, FALSE, nullptr,
                                         -10.0, 10.0, 0.01, 0.1,
                                         labels, values, G_N_ELEMENTS(labels),
                                         sp_star_rounded_value_changed );
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        gtk_action_set_sensitive( GTK_ACTION(eact), TRUE );
    }

    /* Randomization */
    {
        gchar const* labels[] = {_("NOT randomized"), _("slightly irregular"), _("visibly randomized"), _("strongly randomized"), _("blown up")};
        gdouble values[] = {0, 0.01, 0.1, 0.5, 10};
        eact = create_adjustment_action( "RandomizationAction",
                                         _("Randomized"), _("Randomized:"), _("Scatter randomly the corners and angles"),
                                         "/tools/shapes/star/randomized", 0.0,
                                         GTK_WIDGET(desktop->canvas), dataKludge, FALSE, nullptr,
                                         -10.0, 10.0, 0.001, 0.01,
                                         labels, values, G_N_ELEMENTS(labels),
                                         sp_star_randomized_value_changed, nullptr /*unit tracker*/, 0.1, 3 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        gtk_action_set_sensitive( GTK_ACTION(eact), TRUE );
    }

    /* Reset */
    {
        InkAction* inky = ink_action_new( "StarResetAction",
                                          _("Defaults"),
                                          _("Reset shape parameters to defaults (use Inkscape Preferences > Tools to change defaults)"),
                                          INKSCAPE_ICON("edit-clear"),
                                          GTK_ICON_SIZE_SMALL_TOOLBAR);
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_star_defaults), dataKludge );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
        gtk_action_set_sensitive( GTK_ACTION(inky), TRUE );
    }

    desktop->connectEventContextChanged(sigc::bind(sigc::ptr_fun(star_toolbox_watch_ec), dataKludge));
    g_signal_connect(dataKludge, "destroy", G_CALLBACK(purge_repr_listener), dataKludge);
}

static void star_toolbox_watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec, GObject* dataKludge)
{
    static sigc::connection changed;

    if (dynamic_cast<Inkscape::UI::Tools::StarTool const*>(ec) != nullptr) {
        changed = desktop->getSelection()->connectChanged(sigc::bind(sigc::ptr_fun(sp_star_toolbox_selection_changed), dataKludge));
        sp_star_toolbox_selection_changed(desktop->getSelection(), dataKludge);
    } else {
        if (changed)
            changed.disconnect();
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

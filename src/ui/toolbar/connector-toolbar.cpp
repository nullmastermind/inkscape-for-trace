// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Connector aux toolbar
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

#include "connector-toolbar.h"
#include "conn-avoid-ref.h"

#include "desktop.h"
#include "document-undo.h"
#include "enums.h"
#include "graphlayout.h"
#include "widgets/ink-action.h"
#include "widgets/ink-toggle-action.h"
#include "widgets/toolbox.h"
#include "inkscape.h"
#include "verbs.h"

#include "object/sp-namedview.h"
#include "object/sp-path.h"

#include "ui/icon-names.h"
#include "ui/tools/connector-tool.h"
#include "ui/uxmanager.h"

#include "widgets/ege-adjustment-action.h"
#include "widgets/spinbutton-events.h"

#include "xml/node-event-vector.h"

using Inkscape::UI::UXManager;
using Inkscape::DocumentUndo;
using Inkscape::UI::ToolboxFactory;

static Inkscape::XML::NodeEventVector connector_tb_repr_events = {
    nullptr, /* child_added */
    nullptr, /* child_removed */
    Inkscape::UI::Toolbar::ConnectorToolbar::event_attr_changed,
    nullptr, /* content_changed */
    nullptr  /* order_changed */
};

namespace Inkscape {
namespace UI {
namespace Toolbar {
GtkWidget *
ConnectorToolbar::prep( SPDesktop *desktop, GtkActionGroup* mainActions)
{
    auto toolbar = new ConnectorToolbar(desktop);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    GtkIconSize secondarySize = ToolboxFactory::prefToSize("/toolbox/secondary", 1);

    {
        InkAction* inky = ink_action_new( "ConnectorAvoidAction",
                                          _("Avoid"),
                                          _("Make connectors avoid selected objects"),
                                          INKSCAPE_ICON("connector-avoid"),
                                          secondarySize );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(path_set_avoid), (gpointer)toolbar);
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }

    {
        InkAction* inky = ink_action_new( "ConnectorIgnoreAction",
                                          _("Ignore"),
                                          _("Make connectors ignore selected objects"),
                                          INKSCAPE_ICON("connector-ignore"),
                                          secondarySize );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(path_set_ignore), (gpointer)toolbar);
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }

    // Orthogonal connectors toggle button
    {
        toolbar->_orthogonal = ink_toggle_action_new( "ConnectorOrthogonalAction",
                                                      _("Orthogonal"),
                                                      _("Make connector orthogonal or polyline"),
                                                      INKSCAPE_ICON("connector-orthogonal"),
                                                      GTK_ICON_SIZE_MENU );
        gtk_action_group_add_action( mainActions, GTK_ACTION( toolbar->_orthogonal ) );

        bool tbuttonstate = prefs->getBool("/tools/connector/orthogonal");
        gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(toolbar->_orthogonal), ( tbuttonstate ? TRUE : FALSE ));
        g_signal_connect_after( G_OBJECT(toolbar->_orthogonal), "toggled", G_CALLBACK(orthogonal_toggled), (gpointer)toolbar );
    }

    EgeAdjustmentAction* eact = nullptr;
    // Curvature spinbox
    eact = create_adjustment_action( "ConnectorCurvatureAction",
                                    _("Connector Curvature"), _("Curvature:"),
                                    _("The amount of connectors curvature"),
                                    "/tools/connector/curvature", defaultConnCurvature,
                                    TRUE, "inkscape:connector-curvature",
                                    0, 100, 1.0, 10.0,
                                    nullptr, nullptr, 0,
                                    nullptr /*unit tracker*/, 1, 0 );
    ege_adjustment_action_set_focuswidget(eact, GTK_WIDGET(desktop->canvas));
    toolbar->_curvature_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
    toolbar->_curvature_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &ConnectorToolbar::curvature_changed));
    gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );

    // Spacing spinbox
    eact = create_adjustment_action( "ConnectorSpacingAction",
                                    _("Connector Spacing"), _("Spacing:"),
                                    _("The amount of space left around objects by auto-routing connectors"),
                                    "/tools/connector/spacing", defaultConnSpacing,
                                    TRUE, "inkscape:connector-spacing",
                                    0, 100, 1.0, 10.0,
                                    nullptr, nullptr, 0,
                                    nullptr /*unit tracker*/, 1, 0 );
    ege_adjustment_action_set_focuswidget(eact, GTK_WIDGET(desktop->canvas));
    toolbar->_spacing_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
    toolbar->_spacing_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &ConnectorToolbar::spacing_changed));
    gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );

    // Graph (connector network) layout
    {
        InkAction* inky = ink_action_new( "ConnectorGraphAction",
                                          _("Graph"),
                                          _("Nicely arrange selected connector network"),
                                          INKSCAPE_ICON("distribute-graph"),
                                          secondarySize );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(graph_layout), (gpointer)toolbar);
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }

    // Default connector length spinbox
    eact = create_adjustment_action( "ConnectorLengthAction",
                                     _("Connector Length"), _("Length:"),
                                     _("Ideal length for connectors when layout is applied"),
                                     "/tools/connector/length", 100,
                                     TRUE, "inkscape:connector-length",
                                     10, 1000, 10.0, 100.0,
                                     nullptr, nullptr, 0,
                                     nullptr /*unit tracker*/, 1, 0 );
    ege_adjustment_action_set_focuswidget(eact, GTK_WIDGET(desktop->canvas));
    toolbar->_length_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
    toolbar->_length_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &ConnectorToolbar::length_changed));
    gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );

    // Directed edges toggle button
    {
        InkToggleAction* act = ink_toggle_action_new( "ConnectorDirectedAction",
                                                      _("Downwards"),
                                                      _("Make connectors with end-markers (arrows) point downwards"),
                                                      INKSCAPE_ICON("distribute-graph-directed"),
                                                      GTK_ICON_SIZE_MENU );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );

        bool tbuttonstate = prefs->getBool("/tools/connector/directedlayout");
        gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act), ( tbuttonstate ? TRUE : FALSE ));

        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(directed_graph_layout_toggled), (gpointer)toolbar);
        desktop->getSelection()->connectChanged(sigc::mem_fun(*toolbar, &ConnectorToolbar::selection_changed));
    }

    // Avoid overlaps toggle button
    {
        InkToggleAction* act = ink_toggle_action_new( "ConnectorOverlapAction",
                                                      _("Remove overlaps"),
                                                      _("Do not allow overlapping shapes"),
                                                      INKSCAPE_ICON("distribute-remove-overlaps"),
                                                      GTK_ICON_SIZE_MENU );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );

        bool tbuttonstate = prefs->getBool("/tools/connector/avoidoverlaplayout");
        gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act), (tbuttonstate ? TRUE : FALSE ));

        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(nooverlaps_graph_layout_toggled), (gpointer)toolbar);
    }

    // Code to watch for changes to the connector-spacing attribute in
    // the XML.
    Inkscape::XML::Node *repr = desktop->namedview->getRepr();
    g_assert(repr != nullptr);

    if(toolbar->_repr) {
        toolbar->_repr->removeListenerByData(toolbar);
        Inkscape::GC::release(toolbar->_repr);
        toolbar->_repr = nullptr;
    }

    if (repr) {
        toolbar->_repr = repr;
        Inkscape::GC::anchor(toolbar->_repr);
        toolbar->_repr->addListener(&connector_tb_repr_events, toolbar);
        toolbar->_repr->synthesizeEvents(&connector_tb_repr_events, toolbar);
    }

    return GTK_WIDGET(toolbar->gobj());
} // end of ConnectorToolbar::prep()

void
ConnectorToolbar::path_set_avoid()
{
    Inkscape::UI::Tools::cc_selection_set_avoid(true);
}

void
ConnectorToolbar::path_set_ignore()
{
    Inkscape::UI::Tools::cc_selection_set_avoid(false);
}

void
ConnectorToolbar::orthogonal_toggled( GtkToggleAction* act, gpointer data)
{
    auto toolbar = reinterpret_cast<ConnectorToolbar *>(data);
    SPDocument *doc = toolbar->_desktop->getDocument();

    if (!DocumentUndo::getUndoSensitive(doc)) {
        return;
    }

    // quit if run by the _changed callbacks
    if (toolbar->_freeze) {
        return;
    }

    // in turn, prevent callbacks from responding
    toolbar->_freeze = true;

    bool is_orthog = gtk_toggle_action_get_active( act );
    gchar orthog_str[] = "orthogonal";
    gchar polyline_str[] = "polyline";
    gchar *value = is_orthog ? orthog_str : polyline_str ;

    bool modmade = false;
    auto itemlist= toolbar->_desktop->getSelection()->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;

        if (Inkscape::UI::Tools::cc_item_is_connector(item)) {
            item->setAttribute( "inkscape:connector-type",
                    value, nullptr);
            item->avoidRef->handleSettingChange();
            modmade = true;
        }
    }

    if (!modmade) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setBool("/tools/connector/orthogonal", is_orthog);
    } else {

        DocumentUndo::done(doc, SP_VERB_CONTEXT_CONNECTOR,
                       is_orthog ? _("Set connector type: orthogonal"): _("Set connector type: polyline"));
    }

    toolbar->_freeze = false;
}

void
ConnectorToolbar::curvature_changed()
{
    SPDocument *doc = _desktop->getDocument();

    if (!DocumentUndo::getUndoSensitive(doc)) {
        return;
    }


    // quit if run by the _changed callbacks
    if (_freeze) {
        return;
    }

    // in turn, prevent callbacks from responding
    _freeze = true;

    auto newValue = _curvature_adj->get_value();
    gchar value[G_ASCII_DTOSTR_BUF_SIZE];
    g_ascii_dtostr(value, G_ASCII_DTOSTR_BUF_SIZE, newValue);

    bool modmade = false;
    auto itemlist= _desktop->getSelection()->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;

        if (Inkscape::UI::Tools::cc_item_is_connector(item)) {
            item->setAttribute( "inkscape:connector-curvature",
                    value, nullptr);
            item->avoidRef->handleSettingChange();
            modmade = true;
        }
    }

    if (!modmade) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setDouble(Glib::ustring("/tools/connector/curvature"), newValue);
    }
    else {
        DocumentUndo::done(doc, SP_VERB_CONTEXT_CONNECTOR,
                       _("Change connector curvature"));
    }

    _freeze = false;
}

void
ConnectorToolbar::spacing_changed()
{
    SPDocument *doc = _desktop->getDocument();

    if (!DocumentUndo::getUndoSensitive(doc)) {
        return;
    }

    Inkscape::XML::Node *repr = _desktop->namedview->getRepr();

    if ( !repr->attribute("inkscape:connector-spacing") &&
            ( _spacing_adj->get_value() == defaultConnSpacing )) {
        // Don't need to update the repr if the attribute doesn't
        // exist and it is being set to the default value -- as will
        // happen at startup.
        return;
    }

    // quit if run by the attr_changed listener
    if (_freeze) {
        return;
    }

    // in turn, prevent listener from responding
    _freeze = true;

    sp_repr_set_css_double(repr, "inkscape:connector-spacing", _spacing_adj->get_value());
    _desktop->namedview->updateRepr();
    bool modmade = false;

    std::vector<SPItem *> items;
    items = get_avoided_items(items, _desktop->currentRoot(), _desktop);
    for (std::vector<SPItem *>::const_iterator iter = items.begin(); iter != items.end(); ++iter ) {
        SPItem *item = *iter;
        Geom::Affine m = Geom::identity();
        avoid_item_move(&m, item);
        modmade = true;
    }

    if(modmade) {
        DocumentUndo::done(doc, SP_VERB_CONTEXT_CONNECTOR,
                       _("Change connector spacing"));
    }
    _freeze = false;
}

void
ConnectorToolbar::graph_layout()
{
    if (!SP_ACTIVE_DESKTOP) {
        return;
    }
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // hack for clones, see comment in align-and-distribute.cpp
    int saved_compensation = prefs->getInt("/options/clonecompensation/value", SP_CLONE_COMPENSATION_UNMOVED);
    prefs->setInt("/options/clonecompensation/value", SP_CLONE_COMPENSATION_UNMOVED);

    auto tmp = SP_ACTIVE_DESKTOP->getSelection()->items();
    std::vector<SPItem *> vec(tmp.begin(), tmp.end());
    graphlayout(vec);

    prefs->setInt("/options/clonecompensation/value", saved_compensation);

    DocumentUndo::done(SP_ACTIVE_DESKTOP->getDocument(), SP_VERB_DIALOG_ALIGN_DISTRIBUTE, _("Arrange connector network"));
}

void
ConnectorToolbar::length_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble("/tools/connector/length", _length_adj->get_value());
}

void
ConnectorToolbar::directed_graph_layout_toggled(GtkToggleAction *act,
                                                gpointer         data)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/connector/directedlayout",
                gtk_toggle_action_get_active( act ));
}

void
ConnectorToolbar::selection_changed(Inkscape::Selection *selection)
{
    SPItem *item = selection->singleItem();
    if (SP_IS_PATH(item))
    {
        gdouble curvature = SP_PATH(item)->connEndPair.getCurvature();
        bool is_orthog = SP_PATH(item)->connEndPair.isOrthogonal();
        gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(_orthogonal), is_orthog);
        _curvature_adj->set_value(curvature);
    }

}

void
ConnectorToolbar::nooverlaps_graph_layout_toggled( GtkToggleAction* act, gpointer /* data */)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/connector/avoidoverlaplayout",
                gtk_toggle_action_get_active( act ));
}

void
ConnectorToolbar::event_attr_changed(Inkscape::XML::Node *repr,
                                     gchar const         *name,
                                     gchar const         * /*old_value*/,
                                     gchar const         * /*new_value*/,
                                     bool                  /*is_interactive*/,
                                     gpointer             data)
{
    auto toolbar = reinterpret_cast<ConnectorToolbar *>(data);

    if ( !toolbar->_freeze
         && (strcmp(name, "inkscape:connector-spacing") == 0) ) {
        gdouble spacing = defaultConnSpacing;
        sp_repr_get_double(repr, "inkscape:connector-spacing", &spacing);

        toolbar->_spacing_adj->set_value(spacing);

#if !GTK_CHECK_VERSION(3,18,0)
        toolbar->_spacing_adj->value_changed();
#endif

        if(toolbar->_desktop->canvas) gtk_widget_grab_focus(GTK_WIDGET(toolbar->_desktop->canvas));
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

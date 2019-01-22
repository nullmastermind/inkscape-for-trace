// SPDX-License-Identifier: GPL-2.0-or-later
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
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

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


//########################
//##       Star         ##
//########################

static Inkscape::XML::NodeEventVector star_tb_repr_events =
{
    nullptr, /* child_added */
    nullptr, /* child_removed */
    Inkscape::UI::Toolbar::StarToolbar::event_attr_changed,
    nullptr, /* content_changed */
    nullptr  /* order_changed */
};

namespace Inkscape {
namespace UI {
namespace Toolbar {
StarToolbar::~StarToolbar()
{
    if (_repr) { // remove old listener
        _repr->removeListenerByData(this);
        Inkscape::GC::release(_repr);
        _repr = nullptr;
    }
}

GtkWidget *
StarToolbar::prep(SPDesktop *desktop, GtkActionGroup* mainActions)
{
    auto holder = new StarToolbar(desktop);

    GtkIconSize secondarySize = ToolboxFactory::prefToSize("/toolbox/secondary", 1);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool isFlatSided = prefs->getBool("/tools/shapes/star/isflatsided", true);

    EgeAdjustmentAction* eact = nullptr;

    {
        holder->_mode_action = ege_output_action_new( "StarStateAction", _("<b>New:</b>"), "", nullptr );
        ege_output_action_set_use_markup( holder->_mode_action, TRUE );
        gtk_action_group_add_action( mainActions, GTK_ACTION( holder->_mode_action ) );
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

        holder->_flat_action =
            InkSelectOneAction::create( "FlatAction",        // Name
                                        (""),                // Label
                                        (""),                // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store
        holder->_flat_action->use_radio( true );
        holder->_flat_action->use_label( false );
        holder->_flat_action->set_active( isFlatSided ? 0 : 1 );

        gtk_action_group_add_action( mainActions, GTK_ACTION( holder->_flat_action->gobj() ));

        holder->_flat_action->signal_changed().connect(sigc::mem_fun(*holder, &StarToolbar::side_mode_changed));
    }

    /* Magnitude */
    {
        gchar const* labels[] = {_("triangle/tri-star"), _("square/quad-star"), _("pentagon/five-pointed star"), _("hexagon/six-pointed star"), nullptr, nullptr, nullptr, nullptr, nullptr};
        gdouble values[] = {3, 4, 5, 6, 7, 8, 10, 12, 20};
        eact = create_adjustment_action( "MagnitudeAction",
                                         _("Corners"), _("Corners:"), _("Number of corners of a polygon or star"),
                                         "/tools/shapes/star/magnitude", 3,
                                         FALSE, nullptr,
                                         3, 1024, 1, 5,
                                         labels, values, G_N_ELEMENTS(labels),
                                         nullptr /*unit tracker*/,
                                         1.0, 0 );
        ege_adjustment_action_set_focuswidget(eact, GTK_WIDGET(desktop->canvas));

        holder->_magnitude_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        holder->_magnitude_adj->signal_value_changed().connect(sigc::mem_fun(*holder, &StarToolbar::magnitude_value_changed));
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        gtk_action_set_sensitive( GTK_ACTION(eact), TRUE );
    }

    /* Spoke ratio */
    {
        gchar const* labels[] = {_("thin-ray star"), nullptr, _("pentagram"), _("hexagram"), _("heptagram"), _("octagram"), _("regular polygon")};
        gdouble values[] = {0.01, 0.2, 0.382, 0.577, 0.692, 0.765, 1};
        holder->_prop_action = create_adjustment_action( "SpokeAction",
                                         _("Spoke ratio"), _("Spoke ratio:"),
                                         // TRANSLATORS: Tip radius of a star is the distance from the center to the farthest handle.
                                         // Base radius is the same for the closest handle.
                                         _("Base radius to tip radius ratio"),
                                         "/tools/shapes/star/proportion", 0.5,
                                         FALSE, nullptr,
                                         0.01, 1.0, 0.01, 0.1,
                                         labels, values, G_N_ELEMENTS(labels)
                                         );
        ege_adjustment_action_set_focuswidget(holder->_prop_action, GTK_WIDGET(desktop->canvas));
        holder->_spoke_adj = Glib::wrap(ege_adjustment_action_get_adjustment(holder->_prop_action));
        holder->_spoke_adj->signal_value_changed().connect(sigc::mem_fun(*holder, &StarToolbar::proportion_value_changed));
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_prop_action) );

        if ( !isFlatSided ) {
            gtk_action_set_visible( GTK_ACTION(holder->_prop_action), TRUE );
        } else {
            gtk_action_set_visible( GTK_ACTION(holder->_prop_action), FALSE );
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
                                         FALSE, nullptr,
                                         -10.0, 10.0, 0.01, 0.1,
                                         labels, values, G_N_ELEMENTS(labels)
                                         );
        ege_adjustment_action_set_focuswidget(eact, GTK_WIDGET(desktop->canvas));
        holder->_roundedness_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        holder->_roundedness_adj->signal_value_changed().connect(sigc::mem_fun(*holder, &StarToolbar::rounded_value_changed));
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
                                         FALSE, nullptr,
                                         -10.0, 10.0, 0.001, 0.01,
                                         labels, values, G_N_ELEMENTS(labels),
                                         nullptr /*unit tracker*/, 0.1, 3 );
        ege_adjustment_action_set_focuswidget(eact, GTK_WIDGET(desktop->canvas));
        holder->_randomization_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        holder->_randomization_adj->signal_value_changed().connect(sigc::mem_fun(*holder, &StarToolbar::randomized_value_changed));
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
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(StarToolbar::defaults), holder);
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
        gtk_action_set_sensitive( GTK_ACTION(inky), TRUE );
    }

    desktop->connectEventContextChanged(sigc::mem_fun(*holder, &StarToolbar::watch_ec));

    return GTK_WIDGET(holder->gobj());
}

void
StarToolbar::side_mode_changed(int mode)
{
    bool flat = (mode == 0);

    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setBool( "/tools/shapes/star/isflatsided", flat );
    }

    // quit if run by the attr_changed listener
    if (_freeze) {
        return;
    }

    // in turn, prevent listener from responding
    _freeze = true;

    Inkscape::Selection *selection = _desktop->getSelection();
    bool modmade = false;

    if ( _prop_action ) {
        gtk_action_set_visible( GTK_ACTION(_prop_action), !flat );
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
        DocumentUndo::done(_desktop->getDocument(), SP_VERB_CONTEXT_STAR,
                           flat ? _("Make polygon") : _("Make star"));
    }

    _freeze = false;
}

void
StarToolbar::magnitude_value_changed()
{
    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        // do not remember prefs if this call is initiated by an undo change, because undoing object
        // creation sets bogus values to its attributes before it is deleted
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setInt("/tools/shapes/star/magnitude",
            (gint)_magnitude_adj->get_value());
    }

    // quit if run by the attr_changed listener
    if (_freeze) {
        return;
    }

    // in turn, prevent listener from responding
    _freeze = true;

    bool modmade = false;

    Inkscape::Selection *selection = _desktop->getSelection();
    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;
        if (SP_IS_STAR(item)) {
            Inkscape::XML::Node *repr = item->getRepr();
            sp_repr_set_int(repr,"sodipodi:sides",
                (gint)_magnitude_adj->get_value());
            double arg1 = 0.5;
            sp_repr_get_double(repr, "sodipodi:arg1", &arg1);
            sp_repr_set_svg_double(repr, "sodipodi:arg2",
                                   (arg1 + M_PI / (gint)_magnitude_adj->get_value()));
            item->updateRepr();
            modmade = true;
        }
    }
    if (modmade) {
        DocumentUndo::done(_desktop->getDocument(), SP_VERB_CONTEXT_STAR,
                           _("Star: Change number of corners"));
    }

    _freeze = false;
}

void
StarToolbar::proportion_value_changed()
{
    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        if (!IS_NAN(_spoke_adj->get_value())) {
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            prefs->setDouble("/tools/shapes/star/proportion",
                _spoke_adj->get_value());
        }
    }

    // quit if run by the attr_changed listener
    if (_freeze) {
        return;
    }

    // in turn, prevent listener from responding
    _freeze = true;

    bool modmade = false;
    Inkscape::Selection *selection = _desktop->getSelection();
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
                r1*_spoke_adj->get_value());
            } else {
                sp_repr_set_svg_double(repr, "sodipodi:r1",
                r2*_spoke_adj->get_value());
            }

            item->updateRepr();
            modmade = true;
        }
    }

    if (modmade) {
        DocumentUndo::done(_desktop->getDocument(), SP_VERB_CONTEXT_STAR,
                           _("Star: Change spoke ratio"));
    }

    _freeze = false;
}

void
StarToolbar::rounded_value_changed()
{
    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setDouble("/tools/shapes/star/rounded", (gdouble) _roundedness_adj->get_value());
    }

    // quit if run by the attr_changed listener
    if (_freeze) {
        return;
    }

    // in turn, prevent listener from responding
    _freeze = true;

    bool modmade = false;

    Inkscape::Selection *selection = _desktop->getSelection();
    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;
        if (SP_IS_STAR(item)) {
            Inkscape::XML::Node *repr = item->getRepr();
            sp_repr_set_svg_double(repr, "inkscape:rounded",
                (gdouble) _roundedness_adj->get_value());
            item->updateRepr();
            modmade = true;
        }
    }
    if (modmade) {
        DocumentUndo::done(_desktop->getDocument(), SP_VERB_CONTEXT_STAR,
                           _("Star: Change rounding"));
    }

    _freeze = false;
}

void
StarToolbar::randomized_value_changed()
{
    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setDouble("/tools/shapes/star/randomized",
            (gdouble) _randomization_adj->get_value());
    }

    // quit if run by the attr_changed listener
    if (_freeze) {
        return;
    }

    // in turn, prevent listener from responding
    _freeze = true;

    bool modmade = false;

    Inkscape::Selection *selection = _desktop->getSelection();
    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;
        if (SP_IS_STAR(item)) {
            Inkscape::XML::Node *repr = item->getRepr();
            sp_repr_set_svg_double(repr, "inkscape:randomized",
                (gdouble) _randomization_adj->get_value());
            item->updateRepr();
            modmade = true;
        }
    }
    if (modmade) {
        DocumentUndo::done(_desktop->getDocument(), SP_VERB_CONTEXT_STAR,
                           _("Star: Change randomization"));
    }

    _freeze = false;
}

void
StarToolbar::defaults(GtkWidget * /*widget*/, gpointer user_data)
{
    auto toolbar = reinterpret_cast<StarToolbar *>(user_data);

    // FIXME: in this and all other _default functions, set some flag telling the value_changed
    // callbacks to lump all the changes for all selected objects in one undo step

    GtkAdjustment *adj = nullptr;

    // fixme: make settable in prefs!
    gint mag = 5;
    gdouble prop = 0.5;
    gboolean flat = FALSE;
    gdouble randomized = 0;
    gdouble rounded = 0;

    toolbar->_flat_action->set_active ( flat ? 0 : 1 );

    gtk_action_set_visible( GTK_ACTION(toolbar->_prop_action), !flat );

    toolbar->_magnitude_adj->set_value(mag);
    toolbar->_spoke_adj->set_value(prop);
    toolbar->_roundedness_adj->set_value(rounded);
    toolbar->_randomization_adj->set_value(randomized);

#if !GTK_CHECK_VERSION(3,18,0)
    toolbar->_magnitude_adj->value_changed();
    toolbar->_spoke_adj->value_changed();
    toolbar->_roundedness_adj->value_changed();
    toolbar->_randomization_adj->value_changed();
#endif
}

void
StarToolbar::watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec)
{
    if (dynamic_cast<Inkscape::UI::Tools::StarTool const*>(ec) != nullptr) {
        _changed = desktop->getSelection()->connectChanged(sigc::mem_fun(*this, &StarToolbar::selection_changed));
        selection_changed(desktop->getSelection());
    } else {
        if (_changed)
            _changed.disconnect();
    }
}

/**
 *  \param selection Should not be NULL.
 */
void
StarToolbar::selection_changed(Inkscape::Selection *selection)
{
    int n_selected = 0;
    Inkscape::XML::Node *repr = nullptr;

    if (_repr) { // remove old listener
        _repr->removeListenerByData(this);
        Inkscape::GC::release(_repr);
        _repr = nullptr;
    }

    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;
        if (SP_IS_STAR(item)) {
            n_selected++;
            repr = item->getRepr();
        }
    }

    if (n_selected == 0) {
        gtk_action_set_label(GTK_ACTION(_mode_action), _("<b>New:</b>"));
    } else if (n_selected == 1) {
        gtk_action_set_label(GTK_ACTION(_mode_action), _("<b>Change:</b>"));

        if (repr) {
            _repr = repr;
            Inkscape::GC::anchor(_repr);
            _repr->addListener(&star_tb_repr_events, this);
            _repr->synthesizeEvents(&star_tb_repr_events, this);
        }
    } else {
        // FIXME: implement averaging of all parameters for multiple selected stars
        //gtk_label_set_markup(GTK_LABEL(l), _("<b>Average:</b>"));
        //gtk_label_set_markup(GTK_LABEL(l), _("<b>Change:</b>"));
    }
}

void
StarToolbar::event_attr_changed(Inkscape::XML::Node *repr, gchar const *name,
                                gchar const * /*old_value*/, gchar const * /*new_value*/,
                                bool /*is_interactive*/, gpointer dataPointer)
{
    auto toolbar = reinterpret_cast<StarToolbar *>(dataPointer);

    // quit if run by the _changed callbacks
    if (toolbar->_freeze) {
        return;
    }

    // in turn, prevent callbacks from responding
    toolbar->_freeze = true;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool isFlatSided = prefs->getBool("/tools/shapes/star/isflatsided", true);

    if (!strcmp(name, "inkscape:randomized")) {
        double randomized = 0.0;
        sp_repr_get_double(repr, "inkscape:randomized", &randomized);
        toolbar->_randomization_adj->set_value(randomized);
    } else if (!strcmp(name, "inkscape:rounded")) {
        double rounded = 0.0;
        sp_repr_get_double(repr, "inkscape:rounded", &rounded);
        toolbar->_roundedness_adj->set_value(rounded);
    } else if (!strcmp(name, "inkscape:flatsided")) {
        char const *flatsides = repr->attribute("inkscape:flatsided");
        if ( flatsides && !strcmp(flatsides,"false") ) {
            toolbar->_flat_action->set_active(1);
            gtk_action_set_visible( GTK_ACTION(toolbar->_prop_action), TRUE );
        } else {
            toolbar->_flat_action->set_active(0);
            gtk_action_set_visible( GTK_ACTION(toolbar->_prop_action), FALSE );
        }
    } else if ((!strcmp(name, "sodipodi:r1") || !strcmp(name, "sodipodi:r2")) && (!isFlatSided) ) {
        gdouble r1 = 1.0;
        gdouble r2 = 1.0;
        sp_repr_get_double(repr, "sodipodi:r1", &r1);
        sp_repr_get_double(repr, "sodipodi:r2", &r2);
        if (r2 < r1) {
            toolbar->_spoke_adj->set_value(r2/r1);
        } else {
            toolbar->_spoke_adj->set_value(r1/r2);
        }
    } else if (!strcmp(name, "sodipodi:sides")) {
        int sides = 0;
        sp_repr_get_int(repr, "sodipodi:sides", &sides);
        toolbar->_magnitude_adj->set_value(sides);
    }

    toolbar->_freeze = false;
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

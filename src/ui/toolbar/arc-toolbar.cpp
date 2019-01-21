// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Arc aux toolbar
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

#include "arc-toolbar.h"

#include <glibmm/i18n.h>

#include "desktop.h"
#include "document-undo.h"
#include "widgets/ink-action.h"
#include "widgets/toolbox.h"
#include "mod360.h"
#include "selection.h"
#include "verbs.h"

#include "object/sp-ellipse.h"

#include "ui/icon-names.h"
#include "ui/tools/arc-tool.h"
#include "ui/uxmanager.h"
#include "ui/widget/ink-select-one-action.h"
#include "ui/widget/unit-tracker.h"

#include "widgets/ege-adjustment-action.h"
#include "widgets/ege-output-action.h"
#include "widgets/spinbutton-events.h"
#include "widgets/widget-sizes.h"

#include "xml/node-event-vector.h"

using Inkscape::UI::Widget::UnitTracker;
using Inkscape::UI::UXManager;
using Inkscape::DocumentUndo;
using Inkscape::UI::ToolboxFactory;
using Inkscape::UI::PrefPusher;
using Inkscape::Util::Quantity;
using Inkscape::Util::unit_table;


static Inkscape::XML::NodeEventVector arc_tb_repr_events = {
    nullptr, /* child_added */
    nullptr, /* child_removed */
    Inkscape::UI::Toolbar::ArcToolbar::event_attr_changed,
    nullptr, /* content_changed */
    nullptr  /* order_changed */
};

namespace Inkscape {
namespace UI {
namespace Toolbar {
ArcToolbar::ArcToolbar(SPDesktop *desktop) :
        Toolbar(desktop),
        _tracker(new UnitTracker(Inkscape::Util::UNIT_TYPE_LINEAR)),
        _freeze(false),
        _repr(nullptr)
{
    _tracker->setActiveUnit(unit_table.getUnit("px"));
}

ArcToolbar::~ArcToolbar()
{
    if(_repr) {
        _repr->removeListenerByData(this);
        GC::release(_repr);
        _repr = nullptr;
    }
}

GtkWidget *
ArcToolbar::prep(SPDesktop *desktop, GtkActionGroup* mainActions)
{
    auto toolbar = new ArcToolbar(desktop);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    GtkIconSize secondarySize = ToolboxFactory::prefToSize("/toolbox/secondary", 1);
    EgeAdjustmentAction* eact = nullptr;

    {
        toolbar->_mode_action = ege_output_action_new( "ArcStateAction", _("<b>New:</b>"), "", nullptr );
        ege_output_action_set_use_markup( toolbar->_mode_action, TRUE );
        gtk_action_group_add_action( mainActions, GTK_ACTION( toolbar->_mode_action ) );
    }

    /* Radius X */
    {
        gchar const* labels[] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        gdouble values[] = {1, 2, 3, 5, 10, 20, 50, 100, 200, 500};
        toolbar->_rx_action = create_adjustment_action( "ArcRadiusXAction",
                                                        _("Horizontal radius"), _("Rx:"), _("Horizontal radius of the circle, ellipse, or arc"),
                                                        "/tools/shapes/arc/rx", 0,
                                                        GTK_WIDGET(desktop->canvas),
                                                        TRUE, "altx-arc",
                                                        0, 1e6, SPIN_STEP, SPIN_PAGE_STEP,
                                                        labels, values, G_N_ELEMENTS(labels),
                                                        toolbar->_tracker);
        toolbar->_rx_adj = Glib::wrap(ege_adjustment_action_get_adjustment(toolbar->_rx_action));
        toolbar->_rx_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*toolbar, &ArcToolbar::value_changed),
                                                                    toolbar->_rx_adj, "rx"));
        gtk_action_set_sensitive( GTK_ACTION(toolbar->_rx_action), FALSE );
        gtk_action_group_add_action( mainActions, GTK_ACTION(toolbar->_rx_action) );
    }

    /* Radius Y */
    {
        gchar const* labels[] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        gdouble values[] = {1, 2, 3, 5, 10, 20, 50, 100, 200, 500};
        toolbar->_ry_action = create_adjustment_action( "ArcRadiusYAction",
                                                        _("Vertical radius"), _("Ry:"), _("Vertical radius of the circle, ellipse, or arc"),
                                                        "/tools/shapes/arc/ry", 0,
                                                        GTK_WIDGET(desktop->canvas),
                                                        FALSE, nullptr,
                                                        0, 1e6, SPIN_STEP, SPIN_PAGE_STEP,
                                                        labels, values, G_N_ELEMENTS(labels),
                                                        toolbar->_tracker);
        toolbar->_ry_adj = Glib::wrap(ege_adjustment_action_get_adjustment(toolbar->_ry_action));
        toolbar->_ry_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*toolbar, &ArcToolbar::value_changed),
                                                                    toolbar->_ry_adj, "ry"));
        gtk_action_set_sensitive( GTK_ACTION(toolbar->_ry_action), FALSE );
        gtk_action_group_add_action( mainActions, GTK_ACTION(toolbar->_ry_action) );
    }

    // add the units menu
    {
        Gtk::Action* act = toolbar->_tracker->createAction( "ArcUnitsAction", _("Units"), ("") );
        gtk_action_group_add_action( mainActions, act->gobj() );
    }

    /* Start */
    {
        eact = create_adjustment_action( "ArcStartAction",
                                         _("Start"), _("Start:"),
                                         _("The angle (in degrees) from the horizontal to the arc's start point"),
                                         "/tools/shapes/arc/start", 0.0,
                                         GTK_WIDGET(desktop->canvas),
                                         TRUE, "altx-arc",
                                         -360.0, 360.0, 1.0, 10.0,
                                         nullptr, nullptr, 0
                                         );
        toolbar->_start_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
    }

    /* End */
    {
        eact = create_adjustment_action( "ArcEndAction",
                                         _("End"), _("End:"),
                                         _("The angle (in degrees) from the horizontal to the arc's end point"),
                                         "/tools/shapes/arc/end", 0.0,
                                         GTK_WIDGET(desktop->canvas),
                                         FALSE, nullptr,
                                         -360.0, 360.0, 1.0, 10.0,
                                         nullptr, nullptr, 0
                                         );
        toolbar->_end_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
    }
    toolbar->_start_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*toolbar, &ArcToolbar::startend_value_changed),
                                                                   toolbar->_start_adj, "start", toolbar->_end_adj));
    toolbar->_end_adj->signal_value_changed().connect(  sigc::bind(sigc::mem_fun(*toolbar, &ArcToolbar::startend_value_changed),
                                                                   toolbar->_end_adj,   "end",   toolbar->_start_adj));

    /* Arc: Slice, Arc, Chord */
    {
        InkSelectOneActionColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = _("Slice");
        row[columns.col_tooltip  ] = _("Switch to slice (closed shape with two radii)"),
        row[columns.col_icon     ] = INKSCAPE_ICON("draw-ellipse-segment");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Arc (Open)");
        row[columns.col_tooltip  ] = _("Switch to arc (unclosed shape)");
        row[columns.col_icon     ] = INKSCAPE_ICON("draw-ellipse-arc");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Chord");
        row[columns.col_tooltip  ] = _("Switch to chord (closed shape)"),
        row[columns.col_icon     ] = INKSCAPE_ICON("draw-ellipse-chord");
        row[columns.col_sensitive] = true;

        toolbar->_type_action =
            InkSelectOneAction::create( "ArcTypeAction",   // Name
                                        "",                // Label
                                        "",                // Tooltip
                                        "Not Used",        // Icon
                                        store );           // Tree store

        toolbar->_type_action->use_radio( true );
        toolbar->_type_action->use_group_label( false );
        gint type = prefs->getInt("/tools/shapes/arc/arc_type", 0);
        toolbar->_type_action->set_active( type );

        gtk_action_group_add_action( mainActions, GTK_ACTION( toolbar->_type_action->gobj() ));

        toolbar->_type_action->signal_changed().connect(sigc::mem_fun(*toolbar, &ArcToolbar::type_changed));
    }

    /* Make Whole */
    {
        toolbar->_make_whole = ink_action_new( "ArcResetAction",
                                               _("Make whole"),
                                               _("Make the shape a whole ellipse, not arc or segment"),
                                               INKSCAPE_ICON("draw-ellipse-whole"),
                                               secondarySize );
        g_signal_connect_after( G_OBJECT(toolbar->_make_whole), "activate", G_CALLBACK(ArcToolbar::defaults), toolbar );
        gtk_action_group_add_action( mainActions, GTK_ACTION(toolbar->_make_whole) );
        gtk_action_set_sensitive( GTK_ACTION(toolbar->_make_whole), TRUE );
    }

    toolbar->_single = true;
    // sensitivize make whole and open checkbox
    {
        toolbar->sensitivize( toolbar->_start_adj->get_value(), toolbar->_end_adj->get_value() );
    }

    desktop->connectEventContextChanged(sigc::mem_fun(*toolbar, &ArcToolbar::check_ec));

    return GTK_WIDGET(toolbar->gobj());
}

void
ArcToolbar::value_changed(Glib::RefPtr<Gtk::Adjustment>&  adj,
                          gchar const                    *value_name)
{
    // Per SVG spec "a [radius] value of zero disables rendering of the element".
    // However our implementation does not allow a setting of zero in the UI (not even in the XML editor)
    // and ugly things happen if it's forced here, so better leave the properties untouched.
    if (!adj->get_value()) {
        return;
    }

    Unit const *unit = _tracker->getActiveUnit();
    g_return_if_fail(unit != nullptr);

    SPDocument* document = _desktop->getDocument();
    Geom::Scale scale = document->getDocumentScale();

    if (DocumentUndo::getUndoSensitive(document)) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setDouble(Glib::ustring("/tools/shapes/arc/") + value_name,
            Quantity::convert(adj->get_value(), unit, "px"));
    }

    // quit if run by the attr_changed listener
    if (_freeze || _tracker->isUpdating()) {
        return;
    }

    // in turn, prevent listener from responding
    _freeze = true;

    bool modmade = false;
    Inkscape::Selection *selection = _desktop->getSelection();
    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;
        if (SP_IS_GENERICELLIPSE(item)) {

            SPGenericEllipse *ge = SP_GENERICELLIPSE(item);

            if (!strcmp(value_name, "rx")) {
                ge->setVisibleRx(Quantity::convert(adj->get_value(), unit, "px"));
            } else {
                ge->setVisibleRy(Quantity::convert(adj->get_value(), unit, "px"));
            }

            ge->normalize();
            (SP_OBJECT(ge))->updateRepr();
            (SP_OBJECT(ge))->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);

            modmade = true;
        }
    }

    if (modmade) {
        DocumentUndo::done(_desktop->getDocument(), SP_VERB_CONTEXT_ARC,
                           _("Ellipse: Change radius"));
    }

    _freeze = false;
}

void
ArcToolbar::startend_value_changed(Glib::RefPtr<Gtk::Adjustment>&  adj,
                                   gchar const                    *value_name,
                                   Glib::RefPtr<Gtk::Adjustment>&  other_adj)
{
    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setDouble(Glib::ustring("/tools/shapes/arc/") + value_name, adj->get_value());
    }

    // quit if run by the attr_changed listener
    if (_freeze) {
        return;
    }

    // in turn, prevent listener from responding
    _freeze = true;

    gchar* namespaced_name = g_strconcat("sodipodi:", value_name, NULL);

    bool modmade = false;
    auto itemlist= _desktop->getSelection()->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;
        if (SP_IS_GENERICELLIPSE(item)) {

            SPGenericEllipse *ge = SP_GENERICELLIPSE(item);

            if (!strcmp(value_name, "start")) {
                ge->start = (adj->get_value() * M_PI)/ 180;
            } else {
                ge->end = (adj->get_value() * M_PI)/ 180;
            }

            ge->normalize();
            (SP_OBJECT(ge))->updateRepr();
            (SP_OBJECT(ge))->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);

            modmade = true;
        }
    }

    g_free(namespaced_name);

    sensitivize( adj->get_value(), other_adj->get_value() );

    if (modmade) {
        DocumentUndo::maybeDone(_desktop->getDocument(), value_name, SP_VERB_CONTEXT_ARC,
                                _("Arc: Change start/end"));
    }

    _freeze = false;
}

void
ArcToolbar::type_changed( int type )
{
    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setInt("/tools/shapes/arc/arc_type", type);
    }

    // quit if run by the attr_changed listener
    if (_freeze) {
        return;
    }

    // in turn, prevent listener from responding
    _freeze = true;

    Glib::ustring arc_type = "slice";
    bool open = false;
    switch (type) {
        case 0:
            arc_type = "slice";
            open = false;
            break;
        case 1:
            arc_type = "arc";
            open = true;
            break;
        case 2:
            arc_type = "chord";
            open = true; // For backward compat, not truly open but chord most like arc.
            break;
        default:
            std::cerr << "sp_arctb_type_changed: bad arc type: " << type << std::endl;
    }
               
    bool modmade = false;
    auto itemlist= _desktop->getSelection()->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;
        if (SP_IS_GENERICELLIPSE(item)) {
            Inkscape::XML::Node *repr = item->getRepr();
            repr->setAttribute("sodipodi:open", (open?"true":nullptr) );
            repr->setAttribute("sodipodi:arc-type", arc_type.c_str());
            item->updateRepr();
            modmade = true;
        }
    }

    if (modmade) {
        DocumentUndo::done(_desktop->getDocument(), SP_VERB_CONTEXT_ARC,
                           _("Arc: Changed arc type"));
    }

    _freeze = false;
}

void
ArcToolbar::defaults(GtkWidget *, GObject *obj)
{
    auto toolbar = reinterpret_cast<ArcToolbar *>(obj);
    GtkAdjustment *adj;

    toolbar->_start_adj->set_value(0.0);
    toolbar->_end_adj->set_value(0.0);

#if !GTK_CHECK_VERSION(3,18,0)
    toolbar->_start_adj->value_changed();
    toolbar->_end_adj->value_changed();
#endif

    if(toolbar->_desktop->canvas) gtk_widget_grab_focus(GTK_WIDGET(toolbar->_desktop->canvas));
}

void
ArcToolbar::sensitivize( double v1, double v2 )
{
    if (v1 == 0 && v2 == 0) {
        if (_single) { // only for a single selected ellipse (for now)
            _type_action->set_sensitive(false);
            gtk_action_set_sensitive( GTK_ACTION(_make_whole), FALSE );
        }
    } else {
        _type_action->set_sensitive(true);
        gtk_action_set_sensitive( GTK_ACTION(_make_whole), TRUE );
    }
}

void
ArcToolbar::check_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec)
{
    if (SP_IS_ARC_CONTEXT(ec)) {
        _changed = _desktop->getSelection()->connectChanged(sigc::mem_fun(*this, &ArcToolbar::selection_changed));
        selection_changed(desktop->getSelection());
    } else {
        if (_changed) {
            _changed.disconnect();
            if(_repr) {
                _repr->removeListenerByData(this);
                Inkscape::GC::release(_repr);
                _repr = nullptr;
            }
        }
    }
}

void
ArcToolbar::selection_changed(Inkscape::Selection *selection)
{
    int n_selected = 0;
    Inkscape::XML::Node *repr = nullptr;

    if ( _repr ) {
        _item = nullptr;
        _repr->removeListenerByData(this);
        GC::release(_repr);
        _repr = nullptr;
    }

    SPItem *item = nullptr;

    for(auto i : selection->items()){
        if (SP_IS_GENERICELLIPSE(i)) {
            n_selected++;
            item = i;
            repr = item->getRepr();
        }
    }

    _single = false;
    if (n_selected == 0) {
        gtk_action_set_label(GTK_ACTION(_mode_action), _("<b>New:</b>"));
    } else if (n_selected == 1) {
        _single = true;
        gtk_action_set_label(GTK_ACTION(_mode_action), _("<b>Change:</b>"));
        gtk_action_set_sensitive(GTK_ACTION(_rx_action), TRUE);
        gtk_action_set_sensitive(GTK_ACTION(_ry_action), TRUE);

        if (repr) {
            _repr = repr;
            _item = item;
            Inkscape::GC::anchor(_repr);
            _repr->addListener(&arc_tb_repr_events, this);
            _repr->synthesizeEvents(&arc_tb_repr_events, this);
        }
    } else {
        // FIXME: implement averaging of all parameters for multiple selected
        //gtk_label_set_markup(GTK_LABEL(l), _("<b>Average:</b>"));
        gtk_action_set_label(GTK_ACTION(_mode_action), _("<b>Change:</b>"));
        sensitivize( 1, 0 );
    }
}

void
ArcToolbar::event_attr_changed(Inkscape::XML::Node *repr, gchar const * /*name*/,
                               gchar const * /*old_value*/, gchar const * /*new_value*/,
                               bool /*is_interactive*/, gpointer data)
{
    auto toolbar = reinterpret_cast<ArcToolbar *>(data);

    // quit if run by the _changed callbacks
    if (toolbar->_freeze) {
        return;
    }

    // in turn, prevent callbacks from responding
    toolbar->_freeze = true;

    if (toolbar->_item && SP_IS_GENERICELLIPSE(toolbar->_item)) {
        SPGenericEllipse *ge = SP_GENERICELLIPSE(toolbar->_item);

        Unit const *unit = toolbar->_tracker->getActiveUnit();
        g_return_if_fail(unit != nullptr);

        gdouble rx = ge->getVisibleRx();
        gdouble ry = ge->getVisibleRy();
        toolbar->_rx_adj->set_value(Quantity::convert(rx, "px", unit));
        toolbar->_ry_adj->set_value(Quantity::convert(ry, "px", unit));

#if !GTK_CHECK_VERSION(3,18,0)
        toolbar->_rx_adj->value_changed();
        toolbar->_ry_adj->value_changed();
#endif
    }

    gdouble start = 0.;
    gdouble end = 0.;
    sp_repr_get_double(repr, "sodipodi:start", &start);
    sp_repr_get_double(repr, "sodipodi:end", &end);

    toolbar->_start_adj->set_value(mod360((start * 180)/M_PI));
    toolbar->_end_adj->set_value(mod360((end * 180)/M_PI));

    toolbar->sensitivize(toolbar->_start_adj->get_value(), toolbar->_end_adj->get_value());

    char const *arctypestr = nullptr;
    arctypestr = repr->attribute("sodipodi:arc-type");
    if (!arctypestr) { // For old files.
        char const *openstr = nullptr;
        openstr = repr->attribute("sodipodi:open");
        arctypestr = (openstr ? "arc" : "slice");
    }

    if (!strcmp(arctypestr,"slice")) {
        toolbar->_type_action->set_active( 0 );
    } else if (!strcmp(arctypestr,"arc")) {
        toolbar->_type_action->set_active( 1 );
    } else {
        toolbar->_type_action->set_active( 2 );
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

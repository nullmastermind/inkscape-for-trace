// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Rect aux toolbar
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

#include <gtk/gtk.h>
#include <glibmm/i18n.h>

#include "rect-toolbar.h"

#include "desktop.h"
#include "document-undo.h"
#include "inkscape.h"
#include "widgets/toolbox.h"
#include "verbs.h"

#include "object/sp-namedview.h"
#include "object/sp-rect.h"

#include "ui/icon-names.h"
#include "ui/tools/rect-tool.h"
#include "ui/uxmanager.h"
#include "ui/widget/ink-select-one-action.h"
#include "ui/widget/unit-tracker.h"

#include "widgets/ege-adjustment-action.h"
#include "widgets/ege-output-action.h"
#include "widgets/ink-action.h"
#include "widgets/widget-sizes.h"

#include "xml/node-event-vector.h"

using Inkscape::UI::Widget::UnitTracker;
using Inkscape::UI::UXManager;
using Inkscape::DocumentUndo;
using Inkscape::UI::ToolboxFactory;
using Inkscape::Util::Unit;
using Inkscape::Util::Quantity;
using Inkscape::Util::unit_table;

static Inkscape::XML::NodeEventVector rect_tb_repr_events = {
    nullptr, /* child_added */
    nullptr, /* child_removed */
    Inkscape::UI::Toolbar::RectToolbar::event_attr_changed,
    nullptr, /* content_changed */
    nullptr  /* order_changed */
};

namespace Inkscape {
namespace UI {
namespace Toolbar {

RectToolbar::RectToolbar(SPDesktop *desktop)
    : Toolbar(desktop),
      _tracker(new UnitTracker(Inkscape::Util::UNIT_TYPE_LINEAR)),
      _freeze(false),
      _single(true)
{
    // rx/ry units menu: create
    //tracker->addUnit( SP_UNIT_PERCENT, 0 );
    // fixme: add % meaning per cent of the width/height
    _tracker->setActiveUnit(unit_table.getUnit("px"));
}

RectToolbar::~RectToolbar()
{
    if (_repr) { // remove old listener
        _repr->removeListenerByData(this);
        Inkscape::GC::release(_repr);
        _repr = nullptr;
    }
}

GtkWidget *
RectToolbar::prep(SPDesktop *desktop, GtkActionGroup* mainActions)
{
    auto holder = new RectToolbar(desktop);

    EgeAdjustmentAction* eact = nullptr;
    GtkIconSize secondarySize = ToolboxFactory::prefToSize("/toolbox/secondary", 1);

    {
        holder->_mode_action = ege_output_action_new( "RectStateAction", _("<b>New:</b>"), "", nullptr );
        ege_output_action_set_use_markup( holder->_mode_action, TRUE );
        gtk_action_group_add_action( mainActions, GTK_ACTION( holder->_mode_action ) );
    }

    /* W */
    {
        gchar const* labels[] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        gdouble values[] = {1, 2, 3, 5, 10, 20, 50, 100, 200, 500};
        holder->_width_action = create_adjustment_action( "RectWidthAction",
                                                          _("Width"), _("W:"), _("Width of rectangle"),
                                                          "/tools/shapes/rect/width", 0,
                                                          TRUE, "altx-rect",
                                                          0, 1e6, SPIN_STEP, SPIN_PAGE_STEP,
                                                          labels, values, G_N_ELEMENTS(labels),
                                                          holder->_tracker);
        ege_adjustment_action_set_focuswidget(holder->_width_action, GTK_WIDGET(desktop->canvas));

        holder->_width_adj = Glib::wrap(ege_adjustment_action_get_adjustment(holder->_width_action));
        holder->_width_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*holder, &RectToolbar::value_changed),
                                                                      holder->_width_adj,
                                                                      "width",
                                                                      &SPRect::setVisibleWidth));
        gtk_action_set_sensitive( GTK_ACTION(holder->_width_action), FALSE );
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_width_action) );
    }

    /* H */
    {
        gchar const* labels[] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        gdouble values[] = {1, 2, 3, 5, 10, 20, 50, 100, 200, 500};
        holder->_height_action = create_adjustment_action( "RectHeightAction",
                                                           _("Height"), _("H:"), _("Height of rectangle"),
                                                           "/tools/shapes/rect/height", 0,
                                                           FALSE, nullptr,
                                                           0, 1e6, SPIN_STEP, SPIN_PAGE_STEP,
                                                           labels, values, G_N_ELEMENTS(labels),
                                                           holder->_tracker);
        ege_adjustment_action_set_focuswidget(holder->_height_action, GTK_WIDGET(desktop->canvas));
        holder->_height_adj = Glib::wrap(ege_adjustment_action_get_adjustment(holder->_height_action));
        holder->_height_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*holder, &RectToolbar::value_changed),
                                                                       holder->_height_adj,
                                                                       "height",
                                                                       &SPRect::setVisibleHeight));
        gtk_action_set_sensitive( GTK_ACTION(holder->_height_action), FALSE );
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_height_action) );
    }

    /* rx */
    {
        gchar const* labels[] = {_("not rounded"), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        gdouble values[] = {0.5, 1, 2, 3, 5, 10, 20, 50, 100};
        eact = create_adjustment_action( "RadiusXAction",
                                         _("Horizontal radius"), _("Rx:"), _("Horizontal radius of rounded corners"),
                                         "/tools/shapes/rect/rx", 0,
                                         FALSE, nullptr,
                                         0, 1e6, SPIN_STEP, SPIN_PAGE_STEP,
                                         labels, values, G_N_ELEMENTS(labels),
                                         holder->_tracker);
        ege_adjustment_action_set_focuswidget(eact, GTK_WIDGET(desktop->canvas));
        holder->_rx_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        holder->_rx_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*holder, &RectToolbar::value_changed),
                                                                   holder->_rx_adj,
                                                                   "rx",
                                                                   &SPRect::setVisibleRx));
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
    }

    /* ry */
    {
        gchar const* labels[] = {_("not rounded"), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        gdouble values[] = {0.5, 1, 2, 3, 5, 10, 20, 50, 100};
        eact = create_adjustment_action( "RadiusYAction",
                                         _("Vertical radius"), _("Ry:"), _("Vertical radius of rounded corners"),
                                         "/tools/shapes/rect/ry", 0,
                                         FALSE, nullptr,
                                         0, 1e6, SPIN_STEP, SPIN_PAGE_STEP,
                                         labels, values, G_N_ELEMENTS(labels),
                                         holder->_tracker);
        ege_adjustment_action_set_focuswidget(eact, GTK_WIDGET(desktop->canvas));
        holder->_ry_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        holder->_ry_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*holder, &RectToolbar::value_changed),
                                                                   holder->_ry_adj,
                                                                   "ry",
                                                                   &SPRect::setVisibleRy));
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
    }

    // add the units menu
    {
        InkSelectOneAction* act = holder->_tracker->createAction( "RectUnitsAction", _("Units"), ("") );
        gtk_action_group_add_action( mainActions, act->gobj() );
    }

    /* Reset */
    {
        holder->_not_rounded = ink_action_new( "RectResetAction",
                                               _("Not rounded"),
                                               _("Make corners sharp"),
                                               INKSCAPE_ICON("rectangle-make-corners-sharp"),
                                               secondarySize );
        g_signal_connect_after( G_OBJECT(holder->_not_rounded), "activate", G_CALLBACK(&RectToolbar::defaults), holder );
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_not_rounded) );
        gtk_action_set_sensitive( GTK_ACTION(holder->_not_rounded), TRUE );
    }

    holder->sensitivize();

    desktop->connectEventContextChanged(sigc::mem_fun(*holder, &RectToolbar::watch_ec));

    return GTK_WIDGET(holder->gobj());
}

void
RectToolbar::value_changed(Glib::RefPtr<Gtk::Adjustment>&  adj,
                           gchar const                    *value_name,
                           void (SPRect::*setter)(gdouble))
{
    Unit const *unit = _tracker->getActiveUnit();
    g_return_if_fail(unit != nullptr);

    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setDouble(Glib::ustring("/tools/shapes/rect/") + value_name,
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
        if (SP_IS_RECT(*i)) {
            if (adj->get_value() != 0) {
                (SP_RECT(*i)->*setter)(Quantity::convert(adj->get_value(), unit, "px"));
            } else {
                (*i)->getRepr()->setAttribute(value_name, nullptr);
            }
            modmade = true;
        }
    }

    sensitivize();

    if (modmade) {
        DocumentUndo::done(_desktop->getDocument(), SP_VERB_CONTEXT_RECT,
                           _("Change rectangle"));
    }

    _freeze = false;
}

void
RectToolbar::sensitivize()
{
    if (_rx_adj->get_value() == 0 && _ry_adj->get_value() == 0 && _single) { // only for a single selected rect (for now)
        gtk_action_set_sensitive( GTK_ACTION(_not_rounded), FALSE );
    } else {
        gtk_action_set_sensitive( GTK_ACTION(_not_rounded), TRUE );
    }
}

void
RectToolbar::defaults( GtkWidget * /*widget*/, GObject *obj)
{
    auto toolbar = reinterpret_cast<RectToolbar*>(obj);

    toolbar->_rx_adj->set_value(0.0);
    toolbar->_ry_adj->set_value(0.0);

#if !GTK_CHECK_VERSION(3,18,0)
    // this is necessary if the previous value was 0, but we still need to run the callback to change all selected objects
    toolbar->_rx_adj->value_changed();
    toolbar->_ry_adj->value_changed();
#endif

    toolbar->sensitivize();
}

void
RectToolbar::watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec)
{
    static sigc::connection changed;

    // use of dynamic_cast<> seems wrong here -- we just need to check the current tool

    if (dynamic_cast<Inkscape::UI::Tools::RectTool *>(ec)) {
        Inkscape::Selection *sel = desktop->getSelection();

        changed = sel->connectChanged(sigc::mem_fun(*this, &RectToolbar::selection_changed));

        // Synthesize an emission to trigger the update
        selection_changed(sel);
    } else {
        if (changed) {
            changed.disconnect();
        
            if (_repr) { // remove old listener
                _repr->removeListenerByData(this);
                Inkscape::GC::release(_repr);
                _repr = nullptr;
            }
        }
    }
}

/**
 *  \param selection should not be NULL.
 */
void
RectToolbar::selection_changed(Inkscape::Selection *selection)
{
    int n_selected = 0;
    Inkscape::XML::Node *repr = nullptr;
    SPItem *item = nullptr;

    if (_repr) { // remove old listener
        _item = nullptr;
        _repr->removeListenerByData(this);
        Inkscape::GC::release(_repr);
        _repr = nullptr;
    }

    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        if (SP_IS_RECT(*i)) {
            n_selected++;
            item = *i;
            repr = item->getRepr();
        }
    }

    _single = false;

    if (n_selected == 0) {
        gtk_action_set_label(GTK_ACTION(_mode_action), _("<b>New:</b>"));
        gtk_action_set_sensitive(GTK_ACTION(_width_action), FALSE);
        gtk_action_set_sensitive(GTK_ACTION(_height_action), FALSE);
    } else if (n_selected == 1) {
        gtk_action_set_label(GTK_ACTION(_mode_action), _("<b>Change:</b>"));
        _single = true;
        gtk_action_set_sensitive(GTK_ACTION(_width_action), TRUE);
        gtk_action_set_sensitive(GTK_ACTION(_height_action), TRUE);

        if (repr) {
            _repr = repr;
            _item = item;
            Inkscape::GC::anchor(_repr);
            _repr->addListener(&rect_tb_repr_events, this);
            _repr->synthesizeEvents(&rect_tb_repr_events, this);
        }
    } else {
        // FIXME: implement averaging of all parameters for multiple selected
        //gtk_label_set_markup(GTK_LABEL(l), _("<b>Average:</b>"));
        gtk_action_set_label(GTK_ACTION(_mode_action), _("<b>Change:</b>"));
        sensitivize();
    }
}

void RectToolbar::event_attr_changed(Inkscape::XML::Node * /*repr*/, gchar const * /*name*/,
                                     gchar const * /*old_value*/, gchar const * /*new_value*/,
                                     bool /*is_interactive*/, gpointer data)
{
    auto toolbar = reinterpret_cast<RectToolbar*>(data);

    // quit if run by the _changed callbacks
    if (toolbar->_freeze) {
        return;
    }

    // in turn, prevent callbacks from responding
    toolbar->_freeze = true;

    Unit const *unit = toolbar->_tracker->getActiveUnit();
    g_return_if_fail(unit != nullptr);

    if (toolbar->_item && SP_IS_RECT(toolbar->_item)) {
        {
            gdouble rx = SP_RECT(toolbar->_item)->getVisibleRx();
            toolbar->_rx_adj->set_value(Quantity::convert(rx, "px", unit));
        }

        {
            gdouble ry = SP_RECT(toolbar->_item)->getVisibleRy();
            toolbar->_ry_adj->set_value(Quantity::convert(ry, "px", unit));
        }

        {
            gdouble width = SP_RECT(toolbar->_item)->getVisibleWidth();
            toolbar->_width_adj->set_value(Quantity::convert(width, "px", unit));
        }

        {
            gdouble height = SP_RECT(toolbar->_item)->getVisibleHeight();
            toolbar->_height_adj->set_value(Quantity::convert(height, "px", unit));
        }
    }

    toolbar->sensitivize();
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

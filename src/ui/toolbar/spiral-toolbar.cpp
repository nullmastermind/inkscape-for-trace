// SPDX-License-Identifier: GPL-2.0-or-later
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
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
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

static Inkscape::XML::NodeEventVector spiral_tb_repr_events = {
    nullptr, /* child_added */
    nullptr, /* child_removed */
    Inkscape::UI::Toolbar::SpiralToolbar::event_attr_changed,
    nullptr, /* content_changed */
    nullptr  /* order_changed */
};

namespace Inkscape {
namespace UI {
namespace Toolbar {
SpiralToolbar::~SpiralToolbar()
{
    if(_repr) {
        _repr->removeListenerByData(this);
        GC::release(_repr);
        _repr = nullptr;
    }

    if(_connection) {
        _connection->disconnect();
        delete _connection;
    }
}

GtkWidget *
SpiralToolbar::prep(SPDesktop *desktop, GtkActionGroup* mainActions)
{
    auto toolbar = new SpiralToolbar(desktop);

    EgeAdjustmentAction* eact = nullptr;
    GtkIconSize secondarySize = ToolboxFactory::prefToSize("/toolbox/secondary", 1);

    {
        toolbar->_mode_action = ege_output_action_new( "SpiralStateAction", _("<b>New:</b>"), "", nullptr );
        ege_output_action_set_use_markup( toolbar->_mode_action, TRUE );
        gtk_action_group_add_action( mainActions, GTK_ACTION( toolbar->_mode_action ) );
    }

    /* Revolution */
    {
        gchar const* labels[] = {_("just a curve"), nullptr, _("one full revolution"), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        gdouble values[] = {0.01, 0.5, 1, 2, 3, 5, 10, 20, 50, 100};
        eact = create_adjustment_action( "SpiralRevolutionAction",
                                         _("Number of turns"), _("Turns:"), _("Number of revolutions"),
                                         "/tools/shapes/spiral/revolution", 3.0,
                                         GTK_WIDGET(desktop->canvas),
                                         nullptr, // dataKludge
                                         TRUE, "altx-spiral",
                                         0.01, 1024.0, 0.1, 1.0,
                                         labels, values, G_N_ELEMENTS(labels),
                                         nullptr, // callback
                                         nullptr /*unit tracker*/, 1, 2);
        toolbar->_revolution_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        toolbar->_revolution_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*toolbar, &SpiralToolbar::value_changed),
                                                                            toolbar->_revolution_adj, "revolution"));
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
    }

    /* Expansion */
    {
        gchar const* labels[] = {_("circle"), _("edge is much denser"), _("edge is denser"), _("even"), _("center is denser"), _("center is much denser"), nullptr};
        gdouble values[] = {0, 0.1, 0.5, 1, 1.5, 5, 20};
        eact = create_adjustment_action( "SpiralExpansionAction",
                                         _("Divergence"), _("Divergence:"), _("How much denser/sparser are outer revolutions; 1 = uniform"),
                                         "/tools/shapes/spiral/expansion", 1.0,
                                         GTK_WIDGET(desktop->canvas),
                                         nullptr, // dataKludge
                                         FALSE, nullptr,
                                         0.0, 1000.0, 0.01, 1.0,
                                         labels, values, G_N_ELEMENTS(labels),
                                         nullptr // dataKludge
                                         );
        toolbar->_expansion_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        toolbar->_expansion_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*toolbar, &SpiralToolbar::value_changed),
                                                                           toolbar->_expansion_adj, "expansion"));
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
    }

    /* T0 */
    {
        gchar const* labels[] = {_("starts from center"), _("starts mid-way"), _("starts near edge")};
        gdouble values[] = {0, 0.5, 0.9};
        eact = create_adjustment_action( "SpiralT0Action",
                                         _("Inner radius"), _("Inner radius:"), _("Radius of the innermost revolution (relative to the spiral size)"),
                                         "/tools/shapes/spiral/t0", 0.0,
                                         GTK_WIDGET(desktop->canvas),
                                         nullptr, // dataKludge
                                         FALSE, nullptr,
                                         0.0, 0.999, 0.01, 1.0,
                                         labels, values, G_N_ELEMENTS(labels),
                                         nullptr // callback
                                         );
        toolbar->_t0_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        toolbar->_t0_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*toolbar, &SpiralToolbar::value_changed),
                                                                    toolbar->_t0_adj, "t0"));
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
    }

    /* Reset */
    {
        InkAction* inky = ink_action_new( "SpiralResetAction",
                                          _("Defaults"),
                                          _("Reset shape parameters to defaults (use Inkscape Preferences > Tools to change defaults)"),
                                          INKSCAPE_ICON("edit-clear"),
                                          secondarySize );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(&SpiralToolbar::defaults), toolbar );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }


    toolbar->_connection = new sigc::connection(
        desktop->getSelection()->connectChanged(sigc::mem_fun(*toolbar, &SpiralToolbar::selection_changed)));

    return GTK_WIDGET(toolbar->gobj());
}

void
SpiralToolbar::value_changed(Glib::RefPtr<Gtk::Adjustment> &adj,
                             Glib::ustring const           &value_name)
{
    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setDouble("/tools/shapes/spiral/" + value_name,
            adj->get_value());
    }

    // quit if run by the attr_changed listener
    if (_freeze) {
        return;
    }

    // in turn, prevent listener from responding
    _freeze = true;

    gchar* namespaced_name = g_strconcat("sodipodi:", value_name.data(), NULL);

    bool modmade = false;
    auto itemlist= _desktop->getSelection()->items();
    for(auto i=itemlist.begin();i!=itemlist.end(); ++i){
        SPItem *item = *i;
        if (SP_IS_SPIRAL(item)) {
            Inkscape::XML::Node *repr = item->getRepr();
            sp_repr_set_svg_double( repr, namespaced_name,
                adj->get_value() );
            item->updateRepr();
            modmade = true;
        }
    }

    g_free(namespaced_name);

    if (modmade) {
        DocumentUndo::done(_desktop->getDocument(), SP_VERB_CONTEXT_SPIRAL,
                           _("Change spiral"));
    }

    _freeze = false;
}

void
SpiralToolbar::defaults(GtkWidget * /*widget*/, GObject *obj)
{
    auto toolbar = reinterpret_cast<SpiralToolbar *>(obj);

    // fixme: make settable
    gdouble rev = 3;
    gdouble exp = 1.0;
    gdouble t0 = 0.0;

    toolbar->_revolution_adj->set_value(rev);
    toolbar->_expansion_adj->set_value(exp);
    toolbar->_t0_adj->set_value(t0);

#if !GTK_CHECK_VERSION(3,18,0)
    toolbar->_revolution_adj->value_changed();
    toolbar->_expansion_adj->value_changed();
    toolbar->_t0_adj->value_changed();
#endif

    if(toolbar->_desktop->canvas) gtk_widget_grab_focus(GTK_WIDGET(toolbar->_desktop->canvas));
}

void
SpiralToolbar::selection_changed(Inkscape::Selection *selection)
{
    int n_selected = 0;
    Inkscape::XML::Node *repr = nullptr;

    if ( _repr ) {
        _repr->removeListenerByData(this);
        GC::release(_repr);
        _repr = nullptr;
    }

    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end(); ++i){
        SPItem *item = *i;
        if (SP_IS_SPIRAL(item)) {
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
            _repr->addListener(&spiral_tb_repr_events, this);
            _repr->synthesizeEvents(&spiral_tb_repr_events, this);
        }
    } else {
        // FIXME: implement averaging of all parameters for multiple selected
        //gtk_label_set_markup(GTK_LABEL(l), _("<b>Average:</b>"));
        gtk_action_set_label(GTK_ACTION(_mode_action), _("<b>Change:</b>"));
    }
}

void
SpiralToolbar::event_attr_changed(Inkscape::XML::Node *repr,
                                  gchar const * /*name*/,
                                  gchar const * /*old_value*/,
                                  gchar const * /*new_value*/,
                                  bool /*is_interactive*/,
                                  gpointer data)
{
    auto toolbar = reinterpret_cast<SpiralToolbar *>(data);

    // quit if run by the _changed callbacks
    if (toolbar->_freeze) {
        return;
    }

    // in turn, prevent callbacks from responding
    toolbar->_freeze = true;

    double revolution = 3.0;
    sp_repr_get_double(repr, "sodipodi:revolution", &revolution);
    toolbar->_revolution_adj->set_value(revolution);

    double expansion = 1.0;
    sp_repr_get_double(repr, "sodipodi:expansion", &expansion);
    toolbar->_expansion_adj->set_value(expansion);

    double t0 = 0.0;
    sp_repr_get_double(repr, "sodipodi:t0", &t0);
    toolbar->_t0_adj->set_value(t0);

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

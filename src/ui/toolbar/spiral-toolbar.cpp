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

#include "spiral-toolbar.h"

#include <glibmm/i18n.h>

#include <gtkmm/separatortoolitem.h>
#include <gtkmm/toolbutton.h>

#include "desktop.h"
#include "document-undo.h"
#include "selection.h"
#include "verbs.h"

#include "object/sp-spiral.h"

#include "ui/icon-names.h"
#include "ui/uxmanager.h"
#include "ui/widget/label-tool-item.h"
#include "ui/widget/spin-button-tool-item.h"

#include "widgets/spinbutton-events.h"

#include "xml/node-event-vector.h"

using Inkscape::UI::UXManager;
using Inkscape::DocumentUndo;

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
SpiralToolbar::SpiralToolbar(SPDesktop *desktop) :
        Toolbar(desktop),
        _freeze(false),
        _repr(nullptr)
{
    auto prefs = Inkscape::Preferences::get();

    {
        _mode_item = Gtk::manage(new UI::Widget::LabelToolItem(_("<b>New:</b>")));
        _mode_item->set_use_markup(true);
        add(*_mode_item);
    }

    /* Revolution */
    {
        std::vector<Glib::ustring> labels = {_("just a curve"),  "", _("one full revolution"), "", "", "", "", "", "",  ""};
        std::vector<double>        values = {             0.01, 0.5,                        1,  2,  3,  5, 10, 20, 50, 100};
        auto revolution_val = prefs->getDouble("/tools/shapes/spiral/revolution", 3.0);
        _revolution_adj = Gtk::Adjustment::create(revolution_val, 0.01, 1024.0, 0.1, 1.0);
        _revolution_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("spiral-revolutions", _("Turns:"), _revolution_adj, 1, 2));
        _revolution_item->set_tooltip_text(_("Number of revolutions"));
        _revolution_item->set_custom_numeric_menu_data(values, labels);
        _revolution_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _revolution_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &SpiralToolbar::value_changed),
                                                                   _revolution_adj, "revolution"));
        add(*_revolution_item);
    }

    /* Expansion */
    {
        std::vector<Glib::ustring> labels = {_("circle"), _("edge is much denser"), _("edge is denser"), _("even"), _("center is denser"), _("center is much denser"), ""};
        std::vector<double>        values = {          0,                      0.1,                 0.5,         1,                   1.5,                          5, 20};
        auto expansion_val = prefs->getDouble("/tools/shapes/spiral/expansion", 1.0);
        _expansion_adj = Gtk::Adjustment::create(expansion_val, 0.0, 1000.0, 0.01, 1.0);

        _expansion_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("spiral-expansion", _("Divergence:"), _expansion_adj));
        _expansion_item->set_tooltip_text(_("How much denser/sparser are outer revolutions; 1 = uniform"));
        _expansion_item->set_custom_numeric_menu_data(values, labels);
        _expansion_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _expansion_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &SpiralToolbar::value_changed),
                                                                  _expansion_adj, "expansion"));
        add(*_expansion_item);
    }

    /* T0 */
    {
        std::vector<Glib::ustring> labels = {_("starts from center"), _("starts mid-way"), _("starts near edge")};
        std::vector<double>        values = {                      0,                 0.5,                   0.9};
        auto t0_val = prefs->getDouble("/tools/shapes/spiral/t0", 0.0);
        _t0_adj = Gtk::Adjustment::create(t0_val, 0.0, 0.999, 0.01, 1.0);
        _t0_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("spiral-t0", _("Inner radius:"), _t0_adj));
        _t0_item->set_tooltip_text(_("Radius of the innermost revolution (relative to the spiral size)"));
        _t0_item->set_custom_numeric_menu_data(values, labels);
        _t0_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _t0_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &SpiralToolbar::value_changed),
                                                           _t0_adj, "t0"));
        add(*_t0_item);
    }

    add(*Gtk::manage(new Gtk::SeparatorToolItem()));

    /* Reset */
    {
        _reset_item = Gtk::manage(new Gtk::ToolButton(_("Defaults")));
        _reset_item->set_icon_name(INKSCAPE_ICON("edit-clear"));
        _reset_item->set_tooltip_text(_("Reset shape parameters to defaults (use Inkscape Preferences > Tools to change defaults)"));
        _reset_item->signal_clicked().connect(sigc::mem_fun(*this, &SpiralToolbar::defaults));
        add(*_reset_item);
    }

    _connection.reset(new sigc::connection(
        desktop->getSelection()->connectChanged(sigc::mem_fun(*this, &SpiralToolbar::selection_changed))));

    show_all();
}

SpiralToolbar::~SpiralToolbar()
{
    if(_repr) {
        _repr->removeListenerByData(this);
        GC::release(_repr);
        _repr = nullptr;
    }

    if(_connection) {
        _connection->disconnect();
    }
}

GtkWidget *
SpiralToolbar::create(SPDesktop *desktop)
{
    auto toolbar = new SpiralToolbar(desktop);
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
SpiralToolbar::defaults()
{
    // fixme: make settable
    gdouble rev = 3;
    gdouble exp = 1.0;
    gdouble t0 = 0.0;

    _revolution_adj->set_value(rev);
    _expansion_adj->set_value(exp);
    _t0_adj->set_value(t0);

    if(_desktop->canvas) gtk_widget_grab_focus(GTK_WIDGET(_desktop->canvas));
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
        _mode_item->set_markup(_("<b>New:</b>"));
    } else if (n_selected == 1) {
        _mode_item->set_markup(_("<b>Change:</b>"));

        if (repr) {
            _repr = repr;
            Inkscape::GC::anchor(_repr);
            _repr->addListener(&spiral_tb_repr_events, this);
            _repr->synthesizeEvents(&spiral_tb_repr_events, this);
        }
    } else {
        // FIXME: implement averaging of all parameters for multiple selected
        //gtk_label_set_markup(GTK_LABEL(l), _("<b>Average:</b>"));
        _mode_item->set_markup(_("<b>Change:</b>"));
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

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

#include "connector-toolbar.h"

#include <glibmm/i18n.h>

#include <gtkmm/separatortoolitem.h>

#include "conn-avoid-ref.h"

#include "desktop.h"
#include "document-undo.h"
#include "enums.h"
#include "graphlayout.h"
#include "selection.h"
#include "verbs.h"

#include "object/sp-namedview.h"
#include "object/sp-path.h"

#include "ui/icon-names.h"
#include "ui/tools/connector-tool.h"
#include "ui/uxmanager.h"
#include "ui/widget/canvas.h"
#include "ui/widget/spin-button-tool-item.h"

#include "xml/node-event-vector.h"

using Inkscape::UI::UXManager;
using Inkscape::DocumentUndo;

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
ConnectorToolbar::ConnectorToolbar(SPDesktop *desktop)
    : Toolbar(desktop),
    _freeze(false),
    _repr(nullptr)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    {
        auto avoid_item = Gtk::manage(new Gtk::ToolButton(_("Avoid")));
        avoid_item->set_tooltip_text(_("Make connectors avoid selected objects"));
        avoid_item->set_icon_name(INKSCAPE_ICON("connector-avoid"));
        avoid_item->signal_clicked().connect(sigc::mem_fun(*this, &ConnectorToolbar::path_set_avoid));
        add(*avoid_item);
    }

    {
        auto ignore_item = Gtk::manage(new Gtk::ToolButton(_("Ignore")));
        ignore_item->set_tooltip_text(_("Make connectors ignore selected objects"));
        ignore_item->set_icon_name(INKSCAPE_ICON("connector-ignore"));
        ignore_item->signal_clicked().connect(sigc::mem_fun(*this, &ConnectorToolbar::path_set_ignore));
        add(*ignore_item);
    }

    // Orthogonal connectors toggle button
    {
        _orthogonal = add_toggle_button(_("Orthogonal"),
                                        _("Make connector orthogonal or polyline"));
        _orthogonal->set_icon_name(INKSCAPE_ICON("connector-orthogonal"));

        bool tbuttonstate = prefs->getBool("/tools/connector/orthogonal");
        _orthogonal->set_active(( tbuttonstate ? TRUE : FALSE ));
        _orthogonal->signal_toggled().connect(sigc::mem_fun(*this, &ConnectorToolbar::orthogonal_toggled));
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    // Curvature spinbox
    auto curvature_val = prefs->getDouble("/tools/connector/curvature", defaultConnCurvature);
    _curvature_adj = Gtk::Adjustment::create(curvature_val, 0, 100, 1.0, 10.0);
    auto curvature_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("inkscape:connector-curvature", _("Curvature:"), _curvature_adj, 1, 0));
    curvature_item->set_tooltip_text(_("The amount of connectors curvature"));
    curvature_item->set_focus_widget(desktop->canvas);
    _curvature_adj->signal_value_changed().connect(sigc::mem_fun(*this, &ConnectorToolbar::curvature_changed));
    add(*curvature_item);

    // Spacing spinbox
    auto spacing_val = prefs->getDouble("/tools/connector/spacing", defaultConnSpacing);
    _spacing_adj = Gtk::Adjustment::create(spacing_val, 0, 100, 1.0, 10.0);
    auto spacing_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("inkscape:connector-spacing", _("Spacing:"), _spacing_adj, 1, 0));
    spacing_item->set_tooltip_text(_("The amount of space left around objects by auto-routing connectors"));
    spacing_item->set_focus_widget(desktop->canvas);
    _spacing_adj->signal_value_changed().connect(sigc::mem_fun(*this, &ConnectorToolbar::spacing_changed));
    add(*spacing_item);

    // Graph (connector network) layout
    {
        auto graph_item = Gtk::manage(new Gtk::ToolButton(_("Graph")));
        graph_item->set_tooltip_text(_("Nicely arrange selected connector network"));
        graph_item->set_icon_name(INKSCAPE_ICON("distribute-graph"));
        graph_item->signal_clicked().connect(sigc::mem_fun(*this, &ConnectorToolbar::graph_layout));
        add(*graph_item);
    }

    // Default connector length spinbox
    auto length_val = prefs->getDouble("/tools/connector/length", 100);
    _length_adj = Gtk::Adjustment::create(length_val, 10, 1000, 10.0, 100.0);
    auto length_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("inkscape:connector-length", _("Length:"), _length_adj, 1, 0));
    length_item->set_tooltip_text(_("Ideal length for connectors when layout is applied"));
    length_item->set_focus_widget(desktop->canvas);
    _length_adj->signal_value_changed().connect(sigc::mem_fun(*this, &ConnectorToolbar::length_changed));
    add(*length_item);

    // Directed edges toggle button
    {
        _directed_item = add_toggle_button(_("Downwards"),
                                           _("Make connectors with end-markers (arrows) point downwards"));
        _directed_item->set_icon_name(INKSCAPE_ICON("distribute-graph-directed"));

        bool tbuttonstate = prefs->getBool("/tools/connector/directedlayout");
        _directed_item->set_active(tbuttonstate ? TRUE : FALSE);

        _directed_item->signal_toggled().connect(sigc::mem_fun(*this, &ConnectorToolbar::directed_graph_layout_toggled));
        desktop->getSelection()->connectChanged(sigc::mem_fun(*this, &ConnectorToolbar::selection_changed));
    }

    // Avoid overlaps toggle button
    {
        _overlap_item = add_toggle_button(_("Remove overlaps"),
                                          _("Do not allow overlapping shapes"));
        _overlap_item->set_icon_name(INKSCAPE_ICON("distribute-remove-overlaps"));

        bool tbuttonstate = prefs->getBool("/tools/connector/avoidoverlaplayout");
        _overlap_item->set_active(tbuttonstate ? TRUE : FALSE);

        _overlap_item->signal_toggled().connect(sigc::mem_fun(*this, &ConnectorToolbar::nooverlaps_graph_layout_toggled));
    }

    // Code to watch for changes to the connector-spacing attribute in
    // the XML.
    Inkscape::XML::Node *repr = desktop->namedview->getRepr();
    g_assert(repr != nullptr);

    if(_repr) {
        _repr->removeListenerByData(this);
        Inkscape::GC::release(_repr);
        _repr = nullptr;
    }

    if (repr) {
        _repr = repr;
        Inkscape::GC::anchor(_repr);
        _repr->addListener(&connector_tb_repr_events, this);
        _repr->synthesizeEvents(&connector_tb_repr_events, this);
    }

    show_all();
}

GtkWidget *
ConnectorToolbar::create( SPDesktop *desktop)
{
    auto toolbar = new ConnectorToolbar(desktop);
    return GTK_WIDGET(toolbar->gobj());
} // end of ConnectorToolbar::prep()

void
ConnectorToolbar::path_set_avoid()
{
    Inkscape::UI::Tools::cc_selection_set_avoid(_desktop, true);
}

void
ConnectorToolbar::path_set_ignore()
{
    Inkscape::UI::Tools::cc_selection_set_avoid(_desktop, false);
}

void
ConnectorToolbar::orthogonal_toggled()
{
    auto doc = _desktop->getDocument();

    if (!DocumentUndo::getUndoSensitive(doc)) {
        return;
    }

    // quit if run by the _changed callbacks
    if (_freeze) {
        return;
    }

    // in turn, prevent callbacks from responding
    _freeze = true;

    bool is_orthog = _orthogonal->get_active();
    gchar orthog_str[] = "orthogonal";
    gchar polyline_str[] = "polyline";
    gchar *value = is_orthog ? orthog_str : polyline_str ;

    bool modmade = false;
    auto itemlist= _desktop->getSelection()->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;

        if (Inkscape::UI::Tools::cc_item_is_connector(item)) {
            item->setAttribute( "inkscape:connector-type",
                    value, nullptr);
            item->getAvoidRef().handleSettingChange();
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

    _freeze = false;
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
            item->getAvoidRef().handleSettingChange();
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
    for (auto item : items) {
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
    assert(_desktop);
    if (!_desktop) {
        return;
    }
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // hack for clones, see comment in align-and-distribute.cpp
    int saved_compensation = prefs->getInt("/options/clonecompensation/value", SP_CLONE_COMPENSATION_UNMOVED);
    prefs->setInt("/options/clonecompensation/value", SP_CLONE_COMPENSATION_UNMOVED);

    auto tmp = _desktop->getSelection()->items();
    std::vector<SPItem *> vec(tmp.begin(), tmp.end());
    graphlayout(vec);

    prefs->setInt("/options/clonecompensation/value", saved_compensation);

    DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_ALIGN_DISTRIBUTE, _("Arrange connector network"));
}

void
ConnectorToolbar::length_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble("/tools/connector/length", _length_adj->get_value());
}

void
ConnectorToolbar::directed_graph_layout_toggled()
{
    auto prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/connector/directedlayout", _directed_item->get_active());
}

void
ConnectorToolbar::selection_changed(Inkscape::Selection *selection)
{
    SPItem *item = selection->singleItem();
    if (SP_IS_PATH(item))
    {
        gdouble curvature = SP_PATH(item)->connEndPair.getCurvature();
        bool is_orthog = SP_PATH(item)->connEndPair.isOrthogonal();
        _orthogonal->set_active(is_orthog);
        _curvature_adj->set_value(curvature);
    }

}

void
ConnectorToolbar::nooverlaps_graph_layout_toggled()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/connector/avoidoverlaplayout",
                _overlap_item->get_active());
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

        if (toolbar->_desktop->canvas) {
            toolbar->_desktop->canvas->grab_focus();
        }
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

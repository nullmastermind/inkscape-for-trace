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

#include <gtkmm/radiotoolbutton.h>
#include <gtkmm/separatortoolitem.h>

#include "desktop.h"
#include "document-undo.h"
#include "mod360.h"
#include "selection.h"
#include "verbs.h"

#include "object/sp-ellipse.h"
#include "object/sp-namedview.h"

#include "ui/icon-names.h"
#include "ui/pref-pusher.h"
#include "ui/tools/arc-tool.h"
#include "ui/uxmanager.h"
#include "ui/widget/combo-tool-item.h"
#include "ui/widget/label-tool-item.h"
#include "ui/widget/spin-button-tool-item.h"
#include "ui/widget/unit-tracker.h"

#include "widgets/spinbutton-events.h"
#include "widgets/widget-sizes.h"

#include "xml/node-event-vector.h"

using Inkscape::UI::Widget::UnitTracker;
using Inkscape::UI::UXManager;
using Inkscape::DocumentUndo;
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
    _tracker->setActiveUnit(desktop->getNamedView()->display_units);
    auto prefs = Inkscape::Preferences::get();

    {
        _mode_item = Gtk::manage(new UI::Widget::LabelToolItem(_("<b>New:</b>")));
        _mode_item->set_use_markup(true);
        add(*_mode_item);
    }

    /* Radius X */
    {
        std::vector<double> values = {1, 2, 3, 5, 10, 20, 50, 100, 200, 500};
        auto rx_val = prefs->getDouble("/tools/shapes/arc/rx", 0);
        _rx_adj = Gtk::Adjustment::create(rx_val, 0, 1e6, SPIN_STEP, SPIN_PAGE_STEP);
        _rx_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("arc-rx", _("Rx:"), _rx_adj));
        _rx_item->set_tooltip_text(_("Horizontal radius of the circle, ellipse, or arc"));
        _rx_item->set_custom_numeric_menu_data(values);
        _tracker->addAdjustment(_rx_adj->gobj());
        _rx_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _rx_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &ArcToolbar::value_changed),
                                                           _rx_adj, "rx"));
        _rx_item->set_sensitive(false);
        add(*_rx_item);
    }

    /* Radius Y */
    {
        std::vector<double> values = {1, 2, 3, 5, 10, 20, 50, 100, 200, 500};
        auto ry_val = prefs->getDouble("/tools/shapes/arc/ry", 0);
        _ry_adj = Gtk::Adjustment::create(ry_val, 0, 1e6, SPIN_STEP, SPIN_PAGE_STEP);
        _ry_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("arc-ry", _("Ry:"), _ry_adj));
        _ry_item->set_tooltip_text(_("Vertical radius of the circle, ellipse, or arc"));
        _ry_item->set_custom_numeric_menu_data(values);
        _tracker->addAdjustment(_ry_adj->gobj());
        _ry_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _ry_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &ArcToolbar::value_changed),
                                                           _ry_adj, "ry"));
        _ry_item->set_sensitive(false);
        add(*_ry_item);
    }

    // add the units menu
    {
        auto unit_menu = _tracker->create_tool_item(_("Units"), ("") );
        add(*unit_menu);
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    /* Start */
    {
        auto start_val = prefs->getDouble("/tools/shapes/arc/start", 0.0);
        _start_adj = Gtk::Adjustment::create(start_val, -360.0, 360.0, 1.0, 10.0);
        auto eact = Gtk::manage(new UI::Widget::SpinButtonToolItem("arc-start", _("Start:"), _start_adj));
        eact->set_tooltip_text(_("The angle (in degrees) from the horizontal to the arc's start point"));
        eact->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        add(*eact);
    }

    /* End */
    {
        auto end_val = prefs->getDouble("/tools/shapes/arc/end", 0.0);
        _end_adj = Gtk::Adjustment::create(end_val, -360.0, 360.0, 1.0, 10.0);
        auto eact = Gtk::manage(new UI::Widget::SpinButtonToolItem("arc-end", _("End:"), _end_adj));
        eact->set_tooltip_text(_("The angle (in degrees) from the horizontal to the arc's end point"));
        eact->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        add(*eact);
    }
    _start_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &ArcToolbar::startend_value_changed),
                                                          _start_adj, "start", _end_adj));
    _end_adj->signal_value_changed().connect(  sigc::bind(sigc::mem_fun(*this, &ArcToolbar::startend_value_changed),
                                                          _end_adj,   "end",   _start_adj));

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    /* Arc: Slice, Arc, Chord */
    {
        Gtk::RadioToolButton::Group type_group;

        auto slice_btn = Gtk::manage(new Gtk::RadioToolButton(_("Slice")));
        slice_btn->set_tooltip_text(_("Switch to slice (closed shape with two radii)"));
        slice_btn->set_icon_name(INKSCAPE_ICON("draw-ellipse-segment"));
        _type_buttons.push_back(slice_btn);

        auto arc_btn = Gtk::manage(new Gtk::RadioToolButton(_("Arc (Open)")));
        arc_btn->set_tooltip_text(_("Switch to arc (unclosed shape)"));
        arc_btn->set_icon_name(INKSCAPE_ICON("draw-ellipse-arc"));
        _type_buttons.push_back(arc_btn);

        auto chord_btn = Gtk::manage(new Gtk::RadioToolButton(_("Chord")));
        chord_btn->set_tooltip_text(_("Switch to chord (closed shape)"));
        chord_btn->set_icon_name(INKSCAPE_ICON("draw-ellipse-chord"));
        _type_buttons.push_back(chord_btn);

        slice_btn->set_group(type_group);
        arc_btn->set_group(type_group);
        chord_btn->set_group(type_group);

        gint type = prefs->getInt("/tools/shapes/arc/arc_type", 0);
        _type_buttons[type]->set_active();

        int btn_index = 0;
        for (auto btn : _type_buttons)
        {
            btn->set_sensitive();
            btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &ArcToolbar::type_changed), btn_index++));
            add(*btn);
        }
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    /* Make Whole */
    {
        _make_whole = Gtk::manage(new Gtk::ToolButton(_("Make whole")));
        _make_whole->set_tooltip_text(_("Make the shape a whole ellipse, not arc or segment"));
        _make_whole->set_icon_name(INKSCAPE_ICON("draw-ellipse-whole"));
        _make_whole->signal_clicked().connect(sigc::mem_fun(*this, &ArcToolbar::defaults));
        add(*_make_whole);
        _make_whole->set_sensitive(true);
    }

    _single = true;
    // sensitivize make whole and open checkbox
    {
        sensitivize( _start_adj->get_value(), _end_adj->get_value() );
    }

    desktop->connectEventContextChanged(sigc::mem_fun(*this, &ArcToolbar::check_ec));

    show_all();
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
ArcToolbar::create(SPDesktop *desktop)
{
    auto toolbar = new ArcToolbar(desktop);
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
            repr->setAttribute("sodipodi:arc-type", arc_type);
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
ArcToolbar::defaults()
{
    _start_adj->set_value(0.0);
    _end_adj->set_value(0.0);

    if(_desktop->canvas) gtk_widget_grab_focus(GTK_WIDGET(_desktop->canvas));
}

void
ArcToolbar::sensitivize( double v1, double v2 )
{
    if (v1 == 0 && v2 == 0) {
        if (_single) { // only for a single selected ellipse (for now)
            for (auto btn : _type_buttons) btn->set_sensitive(false);
            _make_whole->set_sensitive(false);
        }
    } else {
        for (auto btn : _type_buttons) btn->set_sensitive(true);
        _make_whole->set_sensitive(true);
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
        _mode_item->set_markup(_("<b>New:</b>"));
    } else if (n_selected == 1) {
        _single = true;
        _mode_item->set_markup(_("<b>Change:</b>"));
        _rx_item->set_sensitive(true);
        _ry_item->set_sensitive(true);

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
        _mode_item->set_markup(_("<b>Change:</b>"));
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
        toolbar->_type_buttons[0]->set_active();
    } else if (!strcmp(arctypestr,"arc")) {
        toolbar->_type_buttons[1]->set_active();
    } else {
        toolbar->_type_buttons[2]->set_active();
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

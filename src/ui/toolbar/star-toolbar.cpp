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

#include "star-toolbar.h"

#include <glibmm/i18n.h>

#include <gtkmm/radiotoolbutton.h>
#include <gtkmm/separatortoolitem.h>

#include "desktop.h"
#include "document-undo.h"
#include "selection.h"
#include "verbs.h"

#include "object/sp-star.h"

#include "ui/icon-names.h"
#include "ui/tools/star-tool.h"
#include "ui/uxmanager.h"
#include "ui/widget/label-tool-item.h"
#include "ui/widget/spin-button-tool-item.h"

#include "xml/node-event-vector.h"

using Inkscape::UI::UXManager;
using Inkscape::DocumentUndo;

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
StarToolbar::StarToolbar(SPDesktop *desktop) :
    Toolbar(desktop),
    _mode_item(Gtk::manage(new UI::Widget::LabelToolItem(_("<b>New:</b>")))),
    _repr(nullptr),
    _freeze(false)
{
    _mode_item->set_use_markup(true);
    add(*_mode_item);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool isFlatSided = prefs->getBool("/tools/shapes/star/isflatsided", false);

    /* Flatsided checkbox */
    {
        Gtk::RadioToolButton::Group flat_item_group;

        auto flat_polygon_button = Gtk::manage(new Gtk::RadioToolButton(flat_item_group, _("Polygon")));
        flat_polygon_button->set_tooltip_text(_("Regular polygon (with one handle) instead of a star"));
        flat_polygon_button->set_icon_name(INKSCAPE_ICON("draw-polygon"));
        _flat_item_buttons.push_back(flat_polygon_button);

        auto flat_star_button = Gtk::manage(new Gtk::RadioToolButton(flat_item_group, _("Star")));
        flat_star_button->set_tooltip_text(_("Star instead of a regular polygon (with one handle)"));
        flat_star_button->set_icon_name(INKSCAPE_ICON("draw-star"));
        _flat_item_buttons.push_back(flat_star_button);

        _flat_item_buttons[ isFlatSided ? 0 : 1 ]->set_active();

        int btn_index = 0;

        for (auto btn : _flat_item_buttons)
        {
            add(*btn);
            btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &StarToolbar::side_mode_changed), btn_index++));
        }
    }

    add(*Gtk::manage(new Gtk::SeparatorToolItem()));

    /* Magnitude */
    {
        std::vector<Glib::ustring> labels = {_("triangle/tri-star"), _("square/quad-star"), _("pentagon/five-pointed star"), _("hexagon/six-pointed star"), "", "", "", "", ""};
        std::vector<double>        values = {                     3,                     4,                               5,                             6, 7,   8, 10, 12, 20};
        auto magnitude_val = prefs->getDouble("/tools/shapes/star/magnitude", 3);
        _magnitude_adj = Gtk::Adjustment::create(magnitude_val, 3, 1024, 1, 5);
        _magnitude_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("star-magnitude", _("Corners:"), _magnitude_adj, 1.0, 0));
        _magnitude_item->set_tooltip_text(_("Number of corners of a polygon or star"));
        _magnitude_item->set_custom_numeric_menu_data(values, labels);
        _magnitude_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _magnitude_adj->signal_value_changed().connect(sigc::mem_fun(*this, &StarToolbar::magnitude_value_changed));
        _magnitude_item->set_sensitive(true);
        add(*_magnitude_item);
    }

    /* Spoke ratio */
    {
        std::vector<Glib::ustring> labels = {_("thin-ray star"),  "", _("pentagram"), _("hexagram"), _("heptagram"), _("octagram"), _("regular polygon")};
        std::vector<double>        values = {              0.01, 0.2,          0.382,         0.577,          0.692,         0.765,                    1};
        auto prop_val = prefs->getDouble("/tools/shapes/star/proportion", 0.5);
        _spoke_adj = Gtk::Adjustment::create(prop_val, 0.01, 1.0, 0.01, 0.1);
        _spoke_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("star-spoke", _("Spoke ratio:"), _spoke_adj));
        // TRANSLATORS: Tip radius of a star is the distance from the center to the farthest handle.
        // Base radius is the same for the closest handle.
        _spoke_item->set_tooltip_text(_("Base radius to tip radius ratio"));
        _spoke_item->set_custom_numeric_menu_data(values, labels);
        _spoke_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _spoke_adj->signal_value_changed().connect(sigc::mem_fun(*this, &StarToolbar::proportion_value_changed));

        add(*_spoke_item);
    }

    /* Roundedness */
    {
        std::vector<Glib::ustring> labels = {_("stretched"), _("twisted"), _("slightly pinched"), _("NOT rounded"), _("slightly rounded"),
                                 _("visibly rounded"), _("well rounded"), _("amply rounded"), "", _("stretched"), _("blown up")};
        std::vector<double> values = {-1, -0.2, -0.03, 0, 0.05, 0.1, 0.2, 0.3, 0.5, 1, 10};
        auto roundedness_val = prefs->getDouble("/tools/shapes/star/rounded", 0.0);
        _roundedness_adj = Gtk::Adjustment::create(roundedness_val, -10.0, 10.0, 0.01, 0.1);
        _roundedness_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("star-roundedness", _("Rounded:"), _roundedness_adj));
        _roundedness_item->set_tooltip_text(_("How rounded are the corners (0 for sharp)"));
        _roundedness_item->set_custom_numeric_menu_data(values, labels);
        _roundedness_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _roundedness_adj->signal_value_changed().connect(sigc::mem_fun(*this, &StarToolbar::rounded_value_changed));
        _roundedness_item->set_sensitive(true);
        add(*_roundedness_item);
    }

    /* Randomization */
    {
        std::vector<Glib::ustring> labels = {_("NOT randomized"), _("slightly irregular"), _("visibly randomized"), _("strongly randomized"), _("blown up")};
        std::vector<double>        values = {                  0,                    0.01,                     0.1,                      0.5,            10};
        auto randomized_val = prefs->getDouble("/tools/shapes/star/randomized", 0.0);
        _randomization_adj = Gtk::Adjustment::create(randomized_val, -10.0, 10.0, 0.001, 0.01);
        _randomization_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("star-randomized", _("Randomized:"), _randomization_adj, 0.1, 3));
        _randomization_item->set_tooltip_text(_("Scatter randomly the corners and angles"));
        _randomization_item->set_custom_numeric_menu_data(values, labels);
        _randomization_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _randomization_adj->signal_value_changed().connect(sigc::mem_fun(*this, &StarToolbar::randomized_value_changed));
        _randomization_item->set_sensitive(true);
        add(*_randomization_item);
    }

    add(*Gtk::manage(new Gtk::SeparatorToolItem()));
    
    /* Reset */
    {
        _reset_item = Gtk::manage(new Gtk::ToolButton(_("Defaults")));
        _reset_item->set_icon_name(INKSCAPE_ICON("edit-clear"));
        _reset_item->set_tooltip_text(_("Reset shape parameters to defaults (use Inkscape Preferences > Tools to change defaults)"));
        _reset_item->signal_clicked().connect(sigc::mem_fun(*this, &StarToolbar::defaults));
        _reset_item->set_sensitive(true);
        add(*_reset_item);
    }

    desktop->connectEventContextChanged(sigc::mem_fun(*this, &StarToolbar::watch_ec));

    show_all();
    _spoke_item->set_visible(!isFlatSided);
}

StarToolbar::~StarToolbar()
{
    if (_repr) { // remove old listener
        _repr->removeListenerByData(this);
        Inkscape::GC::release(_repr);
        _repr = nullptr;
    }
}

GtkWidget *
StarToolbar::create(SPDesktop *desktop)
{
    auto toolbar = new StarToolbar(desktop);
    return GTK_WIDGET(toolbar->gobj());
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

    if (_spoke_item) {
        _spoke_item->set_visible(!flat);
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
        if (!std::isnan(_spoke_adj->get_value())) {
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
StarToolbar::defaults()
{

    // FIXME: in this and all other _default functions, set some flag telling the value_changed
    // callbacks to lump all the changes for all selected objects in one undo step

    // fixme: make settable in prefs!
    gint mag = 5;
    gdouble prop = 0.5;
    gboolean flat = FALSE;
    gdouble randomized = 0;
    gdouble rounded = 0;

    _flat_item_buttons[ flat ? 0 : 1 ]->set_active();

    _spoke_item->set_visible(!flat);

    _magnitude_adj->set_value(mag);
    _spoke_adj->set_value(prop);
    _roundedness_adj->set_value(rounded);
    _randomization_adj->set_value(randomized);
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
        _mode_item->set_markup(_("<b>New:</b>"));
    } else if (n_selected == 1) {
        _mode_item->set_markup(_("<b>Change:</b>"));

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
    bool isFlatSided = prefs->getBool("/tools/shapes/star/isflatsided", false);

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
            toolbar->_flat_item_buttons[1]->set_active();
            toolbar->_spoke_item->set_visible(true);
        } else {
            toolbar->_flat_item_buttons[0]->set_active();
            toolbar->_spoke_item->set_visible(false);
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

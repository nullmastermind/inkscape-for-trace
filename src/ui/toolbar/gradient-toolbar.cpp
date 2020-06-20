// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gradient aux toolbar
 *
 * Authors:
 *   bulia byak <bulia@dr.com>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Abhishek Sharma
 *
 * Copyright (C) 2007 Johan Engelen
 * Copyright (C) 2005 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>

#include <gtkmm/comboboxtext.h>
#include <gtkmm/radiotoolbutton.h>
#include <gtkmm/separatortoolitem.h>

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "gradient-chemistry.h"
#include "gradient-drag.h"
#include "gradient-toolbar.h"
#include "selection.h"
#include "verbs.h"

#include "object/sp-defs.h"
#include "object/sp-linear-gradient.h"
#include "object/sp-radial-gradient.h"
#include "object/sp-stop.h"
#include "style.h"

#include "ui/icon-names.h"
#include "ui/tools/gradient-tool.h"
#include "ui/util.h"
#include "ui/widget/canvas.h"
#include "ui/widget/color-preview.h"
#include "ui/widget/combo-tool-item.h"
#include "ui/widget/gradient-image.h"
#include "ui/widget/spin-button-tool-item.h"
#include "ui/widget/gradient-vector-selector.h"

using Inkscape::DocumentUndo;
using Inkscape::UI::Tools::ToolBase;

static bool blocked = false;

void gr_apply_gradient_to_item( SPItem *item, SPGradient *gr, SPGradientType initialType, Inkscape::PaintTarget initialMode, Inkscape::PaintTarget mode )
{
    SPStyle *style = item->style;
    bool isFill = (mode == Inkscape::FOR_FILL);
    if (style
        && (isFill ? style->fill.isPaintserver() : style->stroke.isPaintserver())
        //&& SP_IS_GRADIENT(isFill ? style->getFillPaintServer() : style->getStrokePaintServer()) ) {
        && (isFill ? SP_IS_GRADIENT(style->getFillPaintServer()) : SP_IS_GRADIENT(style->getStrokePaintServer())) ) {
        SPPaintServer *server = isFill ? style->getFillPaintServer() : style->getStrokePaintServer();
        if ( SP_IS_LINEARGRADIENT(server) ) {
            sp_item_set_gradient(item, gr, SP_GRADIENT_TYPE_LINEAR, mode);
        } else if ( SP_IS_RADIALGRADIENT(server) ) {
            sp_item_set_gradient(item, gr, SP_GRADIENT_TYPE_RADIAL, mode);
        }
    }
    else if (initialMode == mode)
    {
        sp_item_set_gradient(item, gr, initialType, mode);
    }
}

/**
Applies gradient vector gr to the gradients attached to the selected dragger of drag, or if none,
to all objects in selection. If there was no previous gradient on an item, uses gradient type and
fill/stroke setting from preferences to create new default (linear: left/right; radial: centered)
gradient.
*/
void gr_apply_gradient(Inkscape::Selection *selection, GrDrag *drag, SPGradient *gr)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    SPGradientType initialType = static_cast<SPGradientType>(prefs->getInt("/tools/gradient/newgradient", SP_GRADIENT_TYPE_LINEAR));
    Inkscape::PaintTarget initialMode = (prefs->getInt("/tools/gradient/newfillorstroke", 1) != 0) ? Inkscape::FOR_FILL : Inkscape::FOR_STROKE;

    // GRADIENTFIXME: make this work for multiple selected draggers.

    // First try selected dragger
    if (drag && !drag->selected.empty()) {
        GrDragger *dragger = *(drag->selected.begin());
        for(auto draggable : dragger->draggables) { //for all draggables of dragger
            gr_apply_gradient_to_item(draggable->item, gr, initialType, initialMode, draggable->fill_or_stroke);
        }
        return;
    }

   // If no drag or no dragger selected, act on selection
    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
       gr_apply_gradient_to_item(*i, gr, initialType, initialMode, initialMode);
   }
}

int gr_vector_list(Glib::RefPtr<Gtk::ListStore> store, SPDesktop *desktop,
                   bool selection_empty, SPGradient *gr_selected, bool gr_multi)
{
    int selected = -1;

    if (!blocked) {
        std::cerr << "gr_vector_list: should be blocked!" << std::endl;
    }

    // Get list of gradients in document.
    SPDocument *document = desktop->getDocument();
    std::vector<SPObject *> gl;
    std::vector<SPObject *> gradients = document->getResourceList( "gradient" );
    for (auto gradient : gradients) {
        SPGradient *grad = SP_GRADIENT(gradient);
        if ( grad->hasStops() && !grad->isSolid() ) {
            gl.push_back(gradient);
        }
    }

    store->clear();

    Inkscape::UI::Widget::ComboToolItemColumns columns;
    Gtk::TreeModel::Row row;

    if (gl.empty()) {
        // The document has no gradients

        row = *(store->append());
        row[columns.col_label    ] = _("No gradient");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_data     ] = nullptr;
        row[columns.col_sensitive] = true;

    } else if (selection_empty) {
        // Document has gradients, but nothing is currently selected.

        row = *(store->append());
        row[columns.col_label    ] = _("Nothing Selected");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_data     ] = nullptr;
        row[columns.col_sensitive] = true;

    } else {

        if (gr_selected == nullptr) {
            row = *(store->append());
            row[columns.col_label    ] = _("No gradient");
            row[columns.col_tooltip  ] = "";
            row[columns.col_icon     ] = "NotUsed";
            row[columns.col_data     ] = nullptr;
            row[columns.col_sensitive] = true;
        }

        if (gr_multi) {
            row = *(store->append());
            row[columns.col_label    ] = _("Multiple gradients");
            row[columns.col_tooltip  ] = "";
            row[columns.col_icon     ] = "NotUsed";
            row[columns.col_data     ] = nullptr;
            row[columns.col_sensitive] = true;
        }

        int idx = 0;
        for (auto it : gl) {
            SPGradient *gradient = SP_GRADIENT(it);

            Glib::ustring label = gr_prepare_label(gradient);
            Glib::RefPtr<Gdk::Pixbuf> pixbuf = sp_gradient_to_pixbuf_ref(gradient, 64, 16);

            row = *(store->append());
            row[columns.col_label    ] = label;
            row[columns.col_tooltip  ] = "";
            row[columns.col_icon     ] = "NotUsed";
            row[columns.col_pixbuf   ] = pixbuf;
            row[columns.col_data     ] = gradient;
            row[columns.col_sensitive] = true;

            if (gradient == gr_selected) {
                selected = idx;
            }
            idx ++;
        }

        if (gr_multi) {
            selected = 0; // This will show "Multiple Gradients"
        }
    }

    return selected;
}

/*
 * Get the gradient of the selected desktop item
 * This is gradient containing the repeat settings, not the underlying "getVector" href linked gradient.
 */
void gr_get_dt_selected_gradient(Inkscape::Selection *selection, SPGradient *&gr_selected)
{
    SPGradient *gradient = nullptr;

    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;// get the items gradient, not the getVector() version
         SPStyle *style = item->style;
         SPPaintServer *server = nullptr;

         if (style && (style->fill.isPaintserver())) {
             server = item->style->getFillPaintServer();
         }
         if (style && (style->stroke.isPaintserver())) {
             server = item->style->getStrokePaintServer();
         }

         if ( SP_IS_GRADIENT(server) ) {
             gradient = SP_GRADIENT(server);
         }
    }

    if (gradient && gradient->isSolid()) {
        gradient = nullptr;
    }

    if (gradient) {
        gr_selected = gradient;
    }
}

/*
 * Get the current selection and dragger status from the desktop
 */
void gr_read_selection( Inkscape::Selection *selection,
                        GrDrag *drag,
                        SPGradient *&gr_selected,
                        bool &gr_multi,
                        SPGradientSpread &spr_selected,
                        bool &spr_multi )
{
    if (drag && !drag->selected.empty()) {
        // GRADIENTFIXME: make this work for more than one selected dragger?
        GrDragger *dragger = *(drag->selected.begin());
        for(auto draggable : dragger->draggables) { //for all draggables of dragger
            SPGradient *gradient = sp_item_gradient_get_vector(draggable->item, draggable->fill_or_stroke);
            SPGradientSpread spread = sp_item_gradient_get_spread(draggable->item, draggable->fill_or_stroke);

            if (gradient && gradient->isSolid()) {
                gradient = nullptr;
            }

            if (gradient && (gradient != gr_selected)) {
                if (gr_selected) {
                    gr_multi = true;
                } else {
                    gr_selected = gradient;
                }
            }
            if (spread != spr_selected) {
                if (spr_selected != SP_GRADIENT_SPREAD_UNDEFINED) {
                    spr_multi = true;
                } else {
                    spr_selected = spread;
                }
            }
         }
        return;
    }

   // If no selected dragger, read desktop selection
    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;
        SPStyle *style = item->style;

        if (style && (style->fill.isPaintserver())) {
            SPPaintServer *server = item->style->getFillPaintServer();
            if ( SP_IS_GRADIENT(server) ) {
                SPGradient *gradient = SP_GRADIENT(server)->getVector();
                SPGradientSpread spread = SP_GRADIENT(server)->fetchSpread();

                if (gradient && gradient->isSolid()) {
                    gradient = nullptr;
                }

                if (gradient && (gradient != gr_selected)) {
                    if (gr_selected) {
                        gr_multi = true;
                    } else {
                        gr_selected = gradient;
                    }
                }
                if (spread != spr_selected) {
                    if (spr_selected != SP_GRADIENT_SPREAD_UNDEFINED) {
                        spr_multi = true;
                    } else {
                        spr_selected = spread;
                    }
                }
            }
        }
        if (style && (style->stroke.isPaintserver())) {
            SPPaintServer *server = item->style->getStrokePaintServer();
            if ( SP_IS_GRADIENT(server) ) {
                SPGradient *gradient = SP_GRADIENT(server)->getVector();
                SPGradientSpread spread = SP_GRADIENT(server)->fetchSpread();

                if (gradient && gradient->isSolid()) {
                    gradient = nullptr;
                }

                if (gradient && (gradient != gr_selected)) {
                    if (gr_selected) {
                        gr_multi = true;
                    } else {
                        gr_selected = gradient;
                    }
                }
                if (spread != spr_selected) {
                    if (spr_selected != SP_GRADIENT_SPREAD_UNDEFINED) {
                        spr_multi = true;
                    } else {
                        spr_selected = spread;
                    }
                }
            }
        }
    }
 }

namespace Inkscape {
namespace UI {
namespace Toolbar {
GradientToolbar::GradientToolbar(SPDesktop *desktop)
        : Toolbar(desktop)
{
    auto prefs = Inkscape::Preferences::get();

    /* New gradient linear or radial */
    {
        add_label(_("New:"));

        Gtk::RadioToolButton::Group new_type_group;

        auto linear_button = Gtk::manage(new Gtk::RadioToolButton(new_type_group, _("linear")));
        linear_button->set_tooltip_text(_("Create linear gradient"));
        linear_button->set_icon_name(INKSCAPE_ICON("paint-gradient-linear"));
        _new_type_buttons.push_back(linear_button);

        auto radial_button = Gtk::manage(new Gtk::RadioToolButton(new_type_group, _("radial")));
        radial_button->set_tooltip_text(_("Create radial (elliptic or circular) gradient"));
        radial_button->set_icon_name(INKSCAPE_ICON("paint-gradient-radial"));
        _new_type_buttons.push_back(radial_button);

        gint mode = prefs->getInt("/tools/gradient/newgradient", SP_GRADIENT_TYPE_LINEAR);
        _new_type_buttons[ mode == SP_GRADIENT_TYPE_LINEAR ? 0 : 1 ]->set_active(); // linear == 1, radial == 2

        int btn_index = 0;
        for (auto btn : _new_type_buttons)
        {
            btn->set_sensitive(true);
            btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &GradientToolbar::new_type_changed), btn_index++));
            add(*btn);
        }
    }

    /* New gradient on fill or stroke*/
    {
        Gtk::RadioToolButton::Group new_fillstroke_group;

        auto fill_btn = Gtk::manage(new Gtk::RadioToolButton(new_fillstroke_group, _("fill")));
        fill_btn->set_tooltip_text(_("Create gradient in the fill"));
        fill_btn->set_icon_name(INKSCAPE_ICON("object-fill"));
        _new_fillstroke_buttons.push_back(fill_btn);

        auto stroke_btn = Gtk::manage(new Gtk::RadioToolButton(new_fillstroke_group, _("stroke")));
        stroke_btn->set_tooltip_text(_("Create gradient in the stroke"));
        stroke_btn->set_icon_name(INKSCAPE_ICON("object-stroke"));
        _new_fillstroke_buttons.push_back(stroke_btn);

        auto fsmode = (prefs->getInt("/tools/gradient/newfillorstroke", 1) != 0) ? Inkscape::FOR_FILL : Inkscape::FOR_STROKE;
        _new_fillstroke_buttons[ fsmode == Inkscape::FOR_FILL ? 0 : 1 ]->set_active();

        auto btn_index = 0;
        for (auto btn : _new_fillstroke_buttons)
        {
            btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &GradientToolbar::new_fillstroke_changed), btn_index++));
            btn->set_sensitive();
            add(*btn);
        }
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    /* Gradient Select list*/
    {
        UI::Widget::ComboToolItemColumns columns;

        auto store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = _("No gradient");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;

        _select_cb = UI::Widget::ComboToolItem::create(_("Select"),         // Label
                                                       "",                  // Tooltip
                                                       "Not Used",          // Icon
                                                       store );             // Tree store

        _select_cb->use_icon( false );
        _select_cb->use_pixbuf( true );
        _select_cb->use_group_label( true );
        _select_cb->set_active( 0 );
        _select_cb->set_sensitive( false );

        add(*_select_cb);
        _select_cb->signal_changed().connect(sigc::mem_fun(*this, &GradientToolbar::gradient_changed));
    }

    // Gradients Linked toggle
    {
        _linked_item = add_toggle_button(_("Link gradients"),
                                         _("Link gradients to change all related gradients"));
        _linked_item->set_icon_name(INKSCAPE_ICON("object-unlocked"));
        _linked_item->signal_toggled().connect(sigc::mem_fun(*this, &GradientToolbar::linked_changed));

        bool linkedmode = prefs->getBool("/options/forkgradientvectors/value", true);
        _linked_item->set_active(!linkedmode);
    }

    /* Reverse */
    {
        _stops_reverse_item = Gtk::manage(new Gtk::ToolButton(_("Reverse")));
        _stops_reverse_item->set_tooltip_text(_("Reverse the direction of the gradient"));
        _stops_reverse_item->set_icon_name(INKSCAPE_ICON("object-flip-horizontal"));
        _stops_reverse_item->signal_clicked().connect(sigc::mem_fun(*this, &GradientToolbar::reverse));
        add(*_stops_reverse_item);
        _stops_reverse_item->set_sensitive(false);
    }

    // Gradient Spread type (how a gradient is drawn outside its nominal area)
    {
        UI::Widget::ComboToolItemColumns columns;
        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        std::vector<gchar*> spread_dropdown_items_list = {
            const_cast<gchar *>(C_("Gradient repeat type", "None")),
            _("Reflected"),
            _("Direct")
        };

        for (auto item: spread_dropdown_items_list) {
            Gtk::TreeModel::Row row = *(store->append());
            row[columns.col_label    ] = item;
            row[columns.col_sensitive] = true;
        }

        _spread_cb = Gtk::manage(UI::Widget::ComboToolItem::create(_("Repeat"),
                                        // TRANSLATORS: for info, see http://www.w3.org/TR/2000/CR-SVG-20000802/pservers.html#LinearGradientSpreadMethodAttribute
                                        _("Whether to fill with flat color beyond the ends of the gradient vector "
                                          "(spreadMethod=\"pad\"), or repeat the gradient in the same direction "
                                          "(spreadMethod=\"repeat\"), or repeat the gradient in alternating opposite "
                                          "directions (spreadMethod=\"reflect\")"),
                                          "Not Used", store));
        _spread_cb->use_group_label(true);

        _spread_cb->set_active(0);
        _spread_cb->set_sensitive(false);

        _spread_cb->signal_changed().connect(sigc::mem_fun(*this, &GradientToolbar::spread_changed));
        add(*_spread_cb);
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    /* Gradient Stop list */
    {
        UI::Widget::ComboToolItemColumns columns;

        auto store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = _("No stops");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;

        _stop_cb =
            UI::Widget::ComboToolItem::create(_("Stops" ),         // Label
                                              "",                  // Tooltip
                                              "Not Used",          // Icon
                                              store );             // Tree store

        _stop_cb->use_icon( false );
        _stop_cb->use_pixbuf( true );
        _stop_cb->use_group_label( true );
        _stop_cb->set_active( 0 );
        _stop_cb->set_sensitive( false );

        add(*_stop_cb);
        _stop_cb->signal_changed().connect(sigc::mem_fun(*this, &GradientToolbar::stop_changed));
    }

    /* Offset */
    {
        auto offset_val = prefs->getDouble("/tools/gradient/stopoffset", 0);
        _offset_adj = Gtk::Adjustment::create(offset_val, 0.0, 1.0, 0.01, 0.1);
        _offset_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("gradient-stopoffset", C_("Gradient", "Offset:"), _offset_adj, 0.01, 2));
        _offset_item->set_tooltip_text(_("Offset of selected stop"));
        _offset_item->set_focus_widget(desktop->canvas);
        _offset_adj->signal_value_changed().connect(sigc::mem_fun(*this, &GradientToolbar::stop_offset_adjustment_changed));
        add(*_offset_item);
        _offset_item->set_sensitive(false);
    }

    /* Add stop */
    {
        _stops_add_item = Gtk::manage(new Gtk::ToolButton(_("Insert new stop")));
        _stops_add_item->set_tooltip_text(_("Insert new stop"));
        _stops_add_item->set_icon_name(INKSCAPE_ICON("node-add"));
        _stops_add_item->signal_clicked().connect(sigc::mem_fun(*this, &GradientToolbar::add_stop));
        add(*_stops_add_item);
        _stops_add_item->set_sensitive(false);
    }

    /* Delete stop */
    {
        _stops_delete_item = Gtk::manage(new Gtk::ToolButton(_("Delete stop")));
        _stops_delete_item->set_tooltip_text(_("Delete stop"));
        _stops_delete_item->set_icon_name(INKSCAPE_ICON("node-delete"));
        _stops_delete_item->signal_clicked().connect(sigc::mem_fun(*this, &GradientToolbar::remove_stop));
        add(*_stops_delete_item);
        _stops_delete_item->set_sensitive(false);
    }

    desktop->connectEventContextChanged(sigc::mem_fun(*this, &GradientToolbar::check_ec));

    show_all();
}

/**
 * Gradient auxiliary toolbar construction and setup.
 *
 */
GtkWidget *
GradientToolbar::create(SPDesktop * desktop)
{
    auto toolbar = new GradientToolbar(desktop);
    return GTK_WIDGET(toolbar->gobj());
}

void
GradientToolbar::new_type_changed(int mode)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt("/tools/gradient/newgradient",
                  mode == 0 ? SP_GRADIENT_TYPE_LINEAR : SP_GRADIENT_TYPE_RADIAL);
}

void
GradientToolbar::new_fillstroke_changed(int mode)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Inkscape::PaintTarget fsmode = (mode == 0) ? Inkscape::FOR_FILL : Inkscape::FOR_STROKE;
    prefs->setInt("/tools/gradient/newfillorstroke", (fsmode == Inkscape::FOR_FILL) ? 1 : 0);
}

/*
 * User selected a gradient from the combobox
 */
void
GradientToolbar::gradient_changed(int active)
{
    if (blocked) {
        return;
    }

    if (active < 0) {
        return;
    }

    blocked = true;

    SPGradient *gr = get_selected_gradient();

    if (gr) {
        gr = sp_gradient_ensure_vector_normalized(gr);

        Inkscape::Selection *selection = _desktop->getSelection();
        ToolBase *ev = _desktop->getEventContext();

        gr_apply_gradient(selection, ev ? ev->get_drag() : nullptr, gr);

        DocumentUndo::done(_desktop->getDocument(), SP_VERB_CONTEXT_GRADIENT,
                   _("Assign gradient to object"));
    }

    blocked = false;
}

/**
 * \brief Return gradient selected in menu
 */
SPGradient *
GradientToolbar::get_selected_gradient()
{
    int active = _select_cb->get_active();

    auto store = _select_cb->get_store();
    auto row   = store->children()[active];
    UI::Widget::ComboToolItemColumns columns;

    void* pointer = row[columns.col_data];
    SPGradient *gr = static_cast<SPGradient *>(pointer);

    return gr;
}

/**
 * \brief User selected a spread method from the combobox
 */
void
GradientToolbar::spread_changed(int active)
{
    if (blocked) {
        return;
    }

    blocked = true;

    Inkscape::Selection *selection = _desktop->getSelection();
    SPGradient *gradient = nullptr;
    gr_get_dt_selected_gradient(selection, gradient);

    if (gradient) {
        SPGradientSpread spread = (SPGradientSpread) active;
        gradient->setSpread(spread);
        gradient->updateRepr();

        DocumentUndo::done(_desktop->getDocument(), SP_VERB_CONTEXT_GRADIENT,
                   _("Set gradient repeat"));
    }

    blocked = false;
}

/**
 * \brief User selected a stop from the combobox
 */
void
GradientToolbar::stop_changed(int active)
{
    if (blocked) {
        return;
    }

    blocked = true;

    ToolBase *ev = _desktop->getEventContext();
    SPGradient *gr = get_selected_gradient();

    select_dragger_by_stop(gr, ev);

    blocked = false;
}

void
GradientToolbar::select_dragger_by_stop(SPGradient *gradient,
                                        ToolBase   *ev)
{
    if (!blocked) {
        std::cerr << "select_dragger_by_stop: should be blocked!" << std::endl;
    }

    if (!ev || !gradient) {
        return;
    }

    GrDrag *drag = ev->get_drag();
    if (!drag) {
        return;
    }

    SPStop *stop = get_selected_stop();

    drag->selectByStop(stop, false, true);

    stop_set_offset();
}

/**
 * \brief Get stop selected by menu
 */
SPStop *
GradientToolbar::get_selected_stop()
{
    int active = _stop_cb->get_active();

    auto store = _stop_cb->get_store();
    auto row   = store->children()[active];
    UI::Widget::ComboToolItemColumns columns;
    void* pointer = row[columns.col_data];
    SPStop *stop = static_cast<SPStop *>(pointer);

    return stop;
}

/**
 *  Change desktop dragger selection to this stop
 *
 *  Set the offset widget value (based on which stop is selected)
 */
void
GradientToolbar::stop_set_offset()
{
    if (!blocked) {
        std::cerr << "gr_stop_set_offset: should be blocked!" << std::endl;
    }

    SPStop *stop = get_selected_stop();
    if (!stop) {
        // std::cerr << "gr_stop_set_offset: no stop!" << std::endl;
        return;
    }

    if (!_offset_item) {
        return;
    }
    bool isEndStop = false;

    SPStop *prev = nullptr;
    prev = stop->getPrevStop();
    if (prev != nullptr )  {
        _offset_adj->set_lower(prev->offset);
    } else {
        isEndStop = true;
        _offset_adj->set_lower(0);
    }

    SPStop *next = nullptr;
    next = stop->getNextStop();
    if (next != nullptr ) {
        _offset_adj->set_upper(next->offset);
    } else {
        isEndStop = true;
        _offset_adj->set_upper(1.0);
    }

    _offset_adj->set_value(stop->offset);
    _offset_item->set_sensitive( !isEndStop );
}

/**
 * \brief User changed the offset
 */
void
GradientToolbar::stop_offset_adjustment_changed()
{
    if (blocked) {
        return;
    }

    blocked = true;

    SPStop *stop = get_selected_stop();
    if (stop) {
        stop->offset = _offset_adj->get_value();
        sp_repr_set_css_double(stop->getRepr(), "offset", stop->offset);

        DocumentUndo::maybeDone(stop->document, "gradient:stop:offset", SP_VERB_CONTEXT_GRADIENT,
                                _("Change gradient stop offset"));

    }

    blocked = false;
}

/**
 * \brief Add stop to gradient
 */
void
GradientToolbar::add_stop()
{
    if (!_desktop) {
        return;
    }

    auto selection = _desktop->getSelection();
    if (!selection) {
        return;
    }

    auto ev = _desktop->getEventContext();
    auto rc = SP_GRADIENT_CONTEXT(ev);

    if (rc) {
        sp_gradient_context_add_stops_between_selected_stops(rc);
    }
}

/**
 * \brief Remove stop from vector
 */
void
GradientToolbar::remove_stop()
{
    if (!_desktop) {
        return;
    }

    auto selection = _desktop->getSelection(); // take from desktop, not from args
    if (!selection) {
        return;
    }

    auto ev = _desktop->getEventContext();
    GrDrag *drag = nullptr;
    if (ev) {
        drag = ev->get_drag();
    }

    if (drag) {
        drag->deleteSelected();
    }
}

/**
 * \brief Reverse vector
 */
void
GradientToolbar::reverse()
{
    sp_gradient_reverse_selected_gradients(_desktop);
}

/**
 * \brief Lock or unlock links
 */
void
GradientToolbar::linked_changed()
{
    bool active = _linked_item->get_active();
    if ( active ) {
        _linked_item->set_icon_name(INKSCAPE_ICON("object-locked"));
    } else {
        _linked_item->set_icon_name(INKSCAPE_ICON("object-unlocked"));
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/options/forkgradientvectors/value", !active);
}

// lp:1327267
/**
 * Checks the current tool and connects gradient aux toolbox signals if it happens to be the gradient tool.
 * Called every time the current tool changes by signal emission.
 */
void
GradientToolbar::check_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec)
{
    if (SP_IS_GRADIENT_CONTEXT(ec)) {
        Inkscape::Selection *selection = desktop->getSelection();
        SPDocument *document = desktop->getDocument();

        // connect to selection modified and changed signals
        _connection_changed  = selection->connectChanged(sigc::mem_fun(*this, &GradientToolbar::selection_changed));
        _connection_modified = selection->connectModified(sigc::mem_fun(*this, &GradientToolbar::selection_modified));
        _connection_subselection_changed = desktop->connectToolSubselectionChanged(sigc::mem_fun(*this, &GradientToolbar::drag_selection_changed));

        // Is this necessary? Couldn't hurt.
        selection_changed(selection);

        // connect to release and modified signals of the defs (i.e. when someone changes gradient)
        _connection_defs_release  = document->getDefs()->connectRelease(sigc::mem_fun(*this, &GradientToolbar::defs_release));
        _connection_defs_modified = document->getDefs()->connectModified(sigc::mem_fun(*this, &GradientToolbar::defs_modified));
    } else {
        if (_connection_changed)
            _connection_changed.disconnect();
        if (_connection_modified)
            _connection_modified.disconnect();
        if (_connection_subselection_changed)
            _connection_subselection_changed.disconnect();
        if (_connection_defs_release)
            _connection_defs_release.disconnect();
        if (_connection_defs_modified)
            _connection_defs_modified.disconnect();
    }
}

/**
 * Core function, setup all the widgets whenever something changes on the desktop
 */
void
GradientToolbar::selection_changed(Inkscape::Selection * /*selection*/)
{
    if (blocked)
        return;

    blocked = true;

    if (!_desktop) {
        return;
    }

    Inkscape::Selection *selection = _desktop->getSelection(); // take from desktop, not from args
    if (selection) {

        ToolBase *ev = _desktop->getEventContext();
        GrDrag *drag = nullptr;
        if (ev) {
            drag = ev->get_drag();
        }

        SPGradient *gr_selected = nullptr;
        SPGradientSpread spr_selected = SP_GRADIENT_SPREAD_UNDEFINED;
        bool gr_multi = false;
        bool spr_multi = false;

        gr_read_selection(selection, drag, gr_selected, gr_multi, spr_selected, spr_multi);

        // Gradient selection menu
        auto store = _select_cb->get_store();
        int gradient = gr_vector_list (store, _desktop, selection->isEmpty(), gr_selected, gr_multi);

        if (gradient < 0) {
            // No selection or no gradients
            _select_cb->set_active( 0 );
            _select_cb->set_sensitive (false);
        } else {
            // Single gradient or multiple gradients
            _select_cb->set_active( gradient );
            _select_cb->set_sensitive (true);
        }

        // Spread menu
        _spread_cb->set_sensitive( gr_selected && !gr_multi );
        _spread_cb->set_active( gr_selected ? (int)spr_selected : 0 );

        _stops_add_item->set_sensitive((gr_selected && !gr_multi && drag && !drag->selected.empty()));
        _stops_delete_item->set_sensitive((gr_selected && !gr_multi && drag && !drag->selected.empty()));
        _stops_reverse_item->set_sensitive((gr_selected!= nullptr));

        _stop_cb->set_sensitive( gr_selected && !gr_multi);

        update_stop_list (gr_selected, nullptr, gr_multi);
        select_stop_by_draggers(gr_selected, ev);
    }

    blocked = false;
}

/**
 * \brief Construct stop list
 */
int
GradientToolbar::update_stop_list( SPGradient *gradient, SPStop *new_stop, bool gr_multi)
{
    if (!blocked) {
        std::cerr << "update_stop_list should be blocked!" << std::endl;
    }

    int selected = -1;

    auto store = _stop_cb->get_store();

    if (!store) {
        return selected;
    }

    store->clear();

    UI::Widget::ComboToolItemColumns columns;
    Gtk::TreeModel::Row row;

    if (!SP_IS_GRADIENT(gradient)) {
        // No valid gradient

        row = *(store->append());
        row[columns.col_label    ] = _("No gradient");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_data     ] = nullptr;
        row[columns.col_sensitive] = true;

    } else if (!gradient->hasStops()) {
        // Has gradient but it has no stops

        row = *(store->append());
        row[columns.col_label    ] = _("No stops in gradient");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_data     ] = nullptr;
        row[columns.col_sensitive] = true;

    } else {
        // Gradient has stops

        // Get list of stops
        for (auto& ochild: gradient->children) {
            if (SP_IS_STOP(&ochild)) {

                SPStop *stop = SP_STOP(&ochild);
                Glib::RefPtr<Gdk::Pixbuf> pixbuf = sp_gradstop_to_pixbuf_ref (stop, 32, 16);

                Inkscape::XML::Node *repr = reinterpret_cast<SPItem *>(&ochild)->getRepr();
                Glib::ustring label = gr_ellipsize_text(repr->attribute("id"), 25);

                row = *(store->append());
                row[columns.col_label    ] = label;
                row[columns.col_tooltip  ] = "";
                row[columns.col_icon     ] = "NotUsed";
                row[columns.col_pixbuf   ] = pixbuf;
                row[columns.col_data     ] = stop;
                row[columns.col_sensitive] = true;
            }
        }
    }

    if (new_stop != nullptr) {
        selected = select_stop_in_list (gradient, new_stop);
    }

    return selected;
}

/**
 * \brief Find position of new_stop in menu.
 */
int
GradientToolbar::select_stop_in_list(SPGradient *gradient, SPStop *new_stop)
{
    int i = 0;
    for (auto& ochild: gradient->children) {
        if (SP_IS_STOP(&ochild)) {
            if (&ochild == new_stop) {
                return i;
            }
            i++;
        }
    }
    return -1;
}

/**
 * \brief Set stop in menu to match stops selected by draggers
 */
void
GradientToolbar::select_stop_by_draggers(SPGradient *gradient, ToolBase *ev)
{
    if (!blocked) {
        std::cerr << "select_stop_by_draggers should be blocked!" << std::endl;
    }

    if (!ev || !gradient)
        return;

    SPGradient *vector = gradient->getVector();
    if (!vector)
        return;

    GrDrag *drag = ev->get_drag();

    if (!drag || drag->selected.empty()) {
        _stop_cb->set_active(0);
        stop_set_offset();
        return;
    }

    gint n = 0;
    SPStop *stop = nullptr;
    int selected = -1;

    // For all selected draggers
    for(auto dragger : drag->selected) {

        // For all draggables of dragger
         for(auto draggable : dragger->draggables) {

            if (draggable->point_type != POINT_RG_FOCUS) {
                n++;
                if (n > 1) break;
            }

            stop = vector->getFirstStop();

            switch (draggable->point_type) {
                case POINT_LG_MID:
                case POINT_RG_MID1:
                case POINT_RG_MID2:
                    stop = sp_get_stop_i(vector, draggable->point_i);
                    break;
                case POINT_LG_END:
                case POINT_RG_R1:
                case POINT_RG_R2:
                    stop = sp_last_stop(vector);
                    break;
                default:
                    break;
            }
        }
        if (n > 1) break;
    }

    if (n > 1) {
        // Multiple stops selected
        if (_offset_item) {
            _offset_item->set_sensitive(false);
        }

        // Stop list always updated first... reinsert "Multiple stops" as first entry.
        UI::Widget::ComboToolItemColumns columns;
        auto store = _stop_cb->get_store();

        auto row = *(store->prepend());
        row[columns.col_label    ] = _("Multiple stops");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;
        selected = 0;

    } else {
        selected = select_stop_in_list(gradient, stop);
    }

    if (selected < 0) {
        _stop_cb->set_active (0);
        _stop_cb->set_sensitive (false);
    } else {
        _stop_cb->set_active (selected);
        _stop_cb->set_sensitive (true);
        stop_set_offset();
    }
}

void
GradientToolbar::selection_modified(Inkscape::Selection *selection, guint /*flags*/)
{
    selection_changed(selection);
}

void
GradientToolbar::drag_selection_changed(gpointer /*dragger*/)
{
   selection_changed(nullptr);
}

void
GradientToolbar::defs_release(SPObject * /*defs*/)
{
    selection_changed(nullptr);
}

void
GradientToolbar::defs_modified(SPObject * /*defs*/, guint /*flags*/)
{
    selection_changed(nullptr);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

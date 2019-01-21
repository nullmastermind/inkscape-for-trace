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


#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "gradient-chemistry.h"
#include "gradient-drag.h"
#include "gradient-toolbar.h"
#include "widgets/ink-action.h"
#include "widgets/ink-toggle-action.h"
#include "widgets/toolbox.h"
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
#include "ui/widget/color-preview.h"
#include "ui/widget/ink-select-one-action.h"

#include "widgets/ege-adjustment-action.h"
#include "widgets/gradient-image.h"
#include "widgets/gradient-vector.h"

using Inkscape::DocumentUndo;
using Inkscape::UI::ToolboxFactory;
using Inkscape::UI::Tools::ToolBase;

static bool blocked = false;

//########################
//##       Gradient     ##
//########################

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
        for(std::vector<GrDraggable *>::const_iterator i = dragger->draggables.begin(); i != dragger->draggables.end(); ++i) { //for all draggables of dragger
            GrDraggable *draggable =  *i;
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
    for (std::vector<SPObject *>::const_iterator it = gradients.begin(); it != gradients.end(); ++it) {
        SPGradient *grad = SP_GRADIENT(*it);
        if ( grad->hasStops() && !grad->isSolid() ) {
            gl.push_back(*it);
        }
    }

    store->clear();

    InkSelectOneActionColumns columns;
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
        for (std::vector<SPObject *>::const_iterator it = gl.begin(); it != gl.end(); ++it) {
            SPGradient *gradient = SP_GRADIENT(*it);

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
        for(std::vector<GrDraggable *>::const_iterator i = dragger->draggables.begin(); i != dragger->draggables.end(); ++i) { //for all draggables of dragger
            GrDraggable *draggable = *i;
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

/**
 * Gradient auxiliary toolbar construction and setup.
 *
 */
GtkWidget *
GradientToolbar::prep(SPDesktop * desktop, GtkActionGroup* mainActions)
{
    auto toolbar = new GradientToolbar(desktop);
    GtkIconSize secondarySize = ToolboxFactory::prefToSize("/toolbox/secondary", 1);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    /* New gradient linear or radial */
    {
        InkSelectOneActionColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = _("linear");
        row[columns.col_tooltip  ] = _("Create linear gradient");
        row[columns.col_icon     ] = INKSCAPE_ICON("paint-gradient-linear");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("radial");
        row[columns.col_tooltip  ] = _("Create radial (elliptic or circular) gradient");
        row[columns.col_icon     ] = INKSCAPE_ICON("paint-gradient-radial");
        row[columns.col_sensitive] = true;

        toolbar->_new_type_mode =
            InkSelectOneAction::create( "GradientNewTypeAction", // Name
                                        _("New"),            // Label
                                        "",                  // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        toolbar->_new_type_mode->use_radio( true );
        toolbar->_new_type_mode->use_group_label( true );
        gint mode = prefs->getInt("/tools/gradient/newgradient", SP_GRADIENT_TYPE_LINEAR);
        toolbar->_new_type_mode->set_active( mode == SP_GRADIENT_TYPE_LINEAR ? 0 : 1 ); // linear == 1, radial == 2

        gtk_action_group_add_action( mainActions, GTK_ACTION( toolbar->_new_type_mode->gobj() ));

        toolbar->_new_type_mode->signal_changed().connect(sigc::mem_fun(*toolbar, &GradientToolbar::new_type_changed));
    }

    /* New gradient on fill or stroke*/
    {
        InkSelectOneActionColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = _("fill");
        row[columns.col_tooltip  ] = _("Create gradient in the fill");
        row[columns.col_icon     ] = INKSCAPE_ICON("object-fill");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("stroke");
        row[columns.col_tooltip  ] = _("Create gradient in the stroke");
        row[columns.col_icon     ] = INKSCAPE_ICON("object-stroke");
        row[columns.col_sensitive] = true;

        toolbar->_new_fillstroke_action =
            InkSelectOneAction::create( "GradientNewFillStrokeAction", // Name
                                        "",                  // Label
                                        "",                  // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        toolbar->_new_fillstroke_action->use_radio( true );
        toolbar->_new_fillstroke_action->use_group_label( false );
        Inkscape::PaintTarget fsmode = (prefs->getInt("/tools/gradient/newfillorstroke", 1) != 0) ? Inkscape::FOR_FILL : Inkscape::FOR_STROKE;
        toolbar->_new_fillstroke_action->set_active( fsmode == Inkscape::FOR_FILL ? 0 : 1 );

        gtk_action_group_add_action( mainActions, GTK_ACTION( toolbar->_new_fillstroke_action->gobj() ));

        toolbar->_new_fillstroke_action->signal_changed().connect(sigc::mem_fun(*toolbar, &GradientToolbar::new_fillstroke_changed));
    }

    /* Gradient Select list*/
    {
        InkSelectOneActionColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = _("No gradient");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;

        toolbar->_select_action =
            InkSelectOneAction::create( "GradientSelectGradientAction", // Name
                                        _("Select"),         // Label
                                        "",                  // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        toolbar->_select_action->use_radio( false );
        toolbar->_select_action->use_icon( false );
        toolbar->_select_action->use_pixbuf( true );
        toolbar->_select_action->use_group_label( true );
        toolbar->_select_action->set_active( 0 );
        toolbar->_select_action->set_sensitive( false );

        gtk_action_group_add_action( mainActions, GTK_ACTION( toolbar->_select_action->gobj() ));

        toolbar->_select_action->signal_changed().connect(sigc::mem_fun(*toolbar, &GradientToolbar::gradient_changed));
    }

    // Gradient Spread type (how a gradient is drawn outside it's nominal area)
    {
        InkSelectOneActionColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = C_("Gradient repeat type", "None");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Reflected");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Direct");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;

        toolbar->_spread_action =
            InkSelectOneAction::create( "GradientSelectSpreadAction", // Name
                                        _("Repeat"),            // Label
                                        // TRANSLATORS: for info, see http://www.w3.org/TR/2000/CR-SVG-20000802/pservers.html#LinearGradientSpreadMethodAttribute
                                        _("Whether to fill with flat color beyond the ends of the gradient vector "
                                          "(spreadMethod=\"pad\"), or repeat the gradient in the same direction "
                                          "(spreadMethod=\"repeat\"), or repeat the gradient in alternating opposite "
                                          "directions (spreadMethod=\"reflect\")"),    // Tooltip
                                        "Not Used",             // Icon
                                        store );                // Tree store

        toolbar->_spread_action->use_radio( false );
        toolbar->_spread_action->use_icon( false );
        toolbar->_spread_action->use_group_label( true );
        toolbar->_spread_action->set_sensitive( false );

        gtk_action_group_add_action( mainActions, GTK_ACTION( toolbar->_spread_action->gobj() ));

        toolbar->_spread_action->signal_changed().connect(sigc::mem_fun(*toolbar, &GradientToolbar::spread_changed));
    }

    /* Gradidnt Stop list */
    {
        InkSelectOneActionColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = _("No stops");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;

        toolbar->_stop_action =
            InkSelectOneAction::create( "GradientStopAction", // Name
                                        _("Stops" ),         // Label
                                        "",                  // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        toolbar->_stop_action->use_radio( false );
        toolbar->_stop_action->use_icon( false );
        toolbar->_stop_action->use_pixbuf( true );
        toolbar->_stop_action->use_group_label( true );
        toolbar->_stop_action->set_active( 0 );
        toolbar->_stop_action->set_sensitive( false );

        gtk_action_group_add_action( mainActions, GTK_ACTION( toolbar->_stop_action->gobj() ));

        toolbar->_stop_action->signal_changed().connect(sigc::mem_fun(*toolbar, &GradientToolbar::stop_changed));
    }

    /* Offset */
    {
        toolbar->_offset_action = create_adjustment_action( "GradientEditOffsetAction",
                                                            _("Offset"), C_("Gradient", "Offset:"), _("Offset of selected stop"),
                                                            "/tools/gradient/stopoffset", 0,
                                                            GTK_WIDGET(desktop->canvas),
                                                            FALSE, nullptr,
                                                            0.0, 1.0, 0.01, 0.1,
                                                            nullptr, nullptr, 0,
                                                            nullptr /*unit tracker*/,
                                                            0.01, 2, 1.0);

        toolbar->_offset_adj = Glib::wrap(ege_adjustment_action_get_adjustment(toolbar->_offset_action));
        toolbar->_offset_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &GradientToolbar::stop_offset_adjustment_changed));
        gtk_action_group_add_action( mainActions, GTK_ACTION(toolbar->_offset_action) );
        gtk_action_set_sensitive( GTK_ACTION(toolbar->_offset_action), FALSE );
    }

    /* Add stop */
    {
        toolbar->_stops_add_action = ink_action_new( "GradientEditAddAction",
                                                     _("Insert new stop"),
                                                     _("Insert new stop"),
                                                     INKSCAPE_ICON("node-add"),
                                                     secondarySize );
        gtk_action_set_short_label(GTK_ACTION(toolbar->_stops_add_action), _("Delete"));
        g_signal_connect_after( G_OBJECT(toolbar->_stops_add_action), "activate", G_CALLBACK(add_stop), (gpointer)toolbar);
        gtk_action_group_add_action( mainActions, GTK_ACTION(toolbar->_stops_add_action) );
        gtk_action_set_sensitive( GTK_ACTION(toolbar->_stops_add_action), FALSE );
    }

    /* Delete stop */
    {
        toolbar->_stops_delete_action = ink_action_new( "GradientEditDeleteAction",
                                                        _("Delete stop"),
                                                        _("Delete stop"),
                                                        INKSCAPE_ICON("node-delete"),
                                                        secondarySize );
        gtk_action_set_short_label(GTK_ACTION(toolbar->_stops_delete_action), _("Delete"));
        g_signal_connect_after( G_OBJECT(toolbar->_stops_delete_action), "activate", G_CALLBACK(remove_stop), (gpointer)toolbar );
        gtk_action_group_add_action( mainActions, GTK_ACTION(toolbar->_stops_delete_action) );
        gtk_action_set_sensitive( GTK_ACTION(toolbar->_stops_delete_action), FALSE );
    }

    /* Reverse */
    {
        toolbar->_stops_reverse_action = ink_action_new( "GradientEditReverseAction",
                                                         _("Reverse"),
                                                         _("Reverse the direction of the gradient"),
                                                         INKSCAPE_ICON("object-flip-horizontal"),
                                                         secondarySize );
        // TODO: Is this label correct?
        gtk_action_set_short_label(GTK_ACTION(toolbar->_stops_reverse_action), _("Delete"));
        g_signal_connect_after( G_OBJECT(toolbar->_stops_reverse_action), "activate", G_CALLBACK(reverse), desktop );
        gtk_action_group_add_action( mainActions, GTK_ACTION(toolbar->_stops_reverse_action) );
        gtk_action_set_sensitive( GTK_ACTION(toolbar->_stops_reverse_action), FALSE );
    }

    // Gradients Linked toggle
    {
        InkToggleAction* itact = ink_toggle_action_new( "GradientEditLinkAction",
                                                        _("Link gradients"),
                                                        _("Link gradients to change all related gradients"),
                                                        INKSCAPE_ICON("object-unlocked"),
                                                        GTK_ICON_SIZE_MENU );
        // TODO: Shouldn't this be translatable?
        gtk_action_set_short_label( GTK_ACTION(itact), "Lock");
        g_signal_connect_after( G_OBJECT(itact), "toggled", G_CALLBACK(linked_changed), desktop) ;
        gtk_action_group_add_action( mainActions, GTK_ACTION(itact) );

        bool linkedmode = prefs->getBool("/options/forkgradientvectors/value", true);
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(itact), !linkedmode );
    }

    desktop->connectEventContextChanged(sigc::mem_fun(*toolbar, &GradientToolbar::check_ec));

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
    int active = _select_action->get_active();

    Glib::RefPtr<Gtk::ListStore> store = _select_action->get_store();
    Gtk::TreeModel::Row row = store->children()[active];
    InkSelectOneActionColumns columns;

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
    int active = _stop_action->get_active();

    Glib::RefPtr<Gtk::ListStore> store = _stop_action->get_store();
    Gtk::TreeModel::Row row = store->children()[active];
    InkSelectOneActionColumns columns;
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

    if (!_offset_action) {
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
    gtk_action_set_sensitive( GTK_ACTION(_offset_action), !isEndStop );
    _offset_adj->changed();
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
GradientToolbar::add_stop(GtkWidget * /*button*/, gpointer data)
{
    auto toolbar = reinterpret_cast<GradientToolbar *>(data);
    if (!toolbar->_desktop) {
        return;
    }

    Inkscape::Selection *selection = toolbar->_desktop->getSelection();
    if (!selection) {
        return;
    }

    ToolBase *ev = toolbar->_desktop->getEventContext();
    Inkscape::UI::Tools::GradientTool *rc = SP_GRADIENT_CONTEXT(ev);

    if (rc) {
        sp_gradient_context_add_stops_between_selected_stops(rc);
    }
}

/**
 * \brief Remove stop from vector
 */
void
GradientToolbar::remove_stop(GtkWidget * /*button*/, gpointer data)
{
    auto toolbar = reinterpret_cast<GradientToolbar *>(data);

    if (!toolbar->_desktop) {
        return;
    }

    Inkscape::Selection *selection = toolbar->_desktop->getSelection(); // take from desktop, not from args
    if (!selection) {
        return;
    }

    ToolBase *ev = toolbar->_desktop->getEventContext();
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
GradientToolbar::reverse(GtkWidget * /*button*/, gpointer data)
{
    SPDesktop *desktop = static_cast<SPDesktop *>(data);
    sp_gradient_reverse_selected_gradients(desktop);
}

/**
 * \brief Lock or unlock links
 */
void
GradientToolbar::linked_changed(GtkToggleAction *act, gpointer /*data*/)
{
    gboolean active = gtk_toggle_action_get_active( act );
    if ( active ) {
        g_object_set( G_OBJECT(act), "iconId", INKSCAPE_ICON("object-locked"), NULL );
    } else {
        g_object_set( G_OBJECT(act), "iconId", INKSCAPE_ICON("object-unlocked"), NULL );
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
        auto store = _select_action->get_store();
        int gradient = gr_vector_list (store, _desktop, selection->isEmpty(), gr_selected, gr_multi);

        if (gradient < 0) {
            // No selection or no gradients
            _select_action->set_active( 0 );
            _select_action->set_sensitive (false);
        } else {
            // Single gradient or multiple gradients
            _select_action->set_active( gradient );
            _select_action->set_sensitive (true);
        }

        // Spread menu
        _spread_action->set_sensitive( gr_selected && !gr_multi );
        _spread_action->set_active( gr_selected ? (int)spr_selected : 0 );

        gtk_action_set_sensitive(GTK_ACTION(_stops_add_action),     (gr_selected && !gr_multi && drag && !drag->selected.empty()));
        gtk_action_set_sensitive(GTK_ACTION(_stops_delete_action),  (gr_selected && !gr_multi && drag && !drag->selected.empty()));
        gtk_action_set_sensitive(GTK_ACTION(_stops_reverse_action), (gr_selected!= nullptr));

        _stop_action->set_sensitive( gr_selected && !gr_multi);

        int stop = update_stop_list (gr_selected, nullptr, gr_multi);
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

    auto store = _stop_action->get_store();

    if (!store) {
        return selected;
    }

    store->clear();

    InkSelectOneActionColumns columns;
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
        _stop_action->set_active(0);
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
        if (_offset_action) {
            gtk_action_set_sensitive( GTK_ACTION(_offset_action), FALSE);
        }

        // Stop list always updated first... reinsert "Multiple stops" as first entry.
        InkSelectOneActionColumns columns;
        Glib::RefPtr<Gtk::ListStore> store = _stop_action->get_store();

        Gtk::TreeModel::Row row = *(store->prepend());
        row[columns.col_label    ] = _("Multiple stops");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;
        selected = 0;

    } else {
        selected = select_stop_in_list(gradient, stop);
    }

    if (selected < 0) {
        _stop_action->set_active (0);
        _stop_action->set_sensitive (false);
    } else {
        _stop_action->set_active (selected);
        _stop_action->set_sensitive (true);
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

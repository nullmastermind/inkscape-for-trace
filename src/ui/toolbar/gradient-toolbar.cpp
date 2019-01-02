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

/* Get stop selected by menu */
static SPStop *get_selected_stop( GObject *data)
{
    InkSelectOneAction *act =
        static_cast<InkSelectOneAction *>(g_object_get_data(data, "gradient_stop_action"));

    int active = act->get_active();

    Glib::RefPtr<Gtk::ListStore> store = act->get_store();
    Gtk::TreeModel::Row row = store->children()[active];
    InkSelectOneActionColumns columns;
    void* pointer = row[columns.col_data];
    SPStop *stop = static_cast<SPStop *>(pointer);

    return stop;
}

/* Add stop to gradient */
static void gr_add_stop(GtkWidget * /*button*/, GObject *data)
{
    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data(data, "desktop"));
    if (!desktop) {
        return;
    }

    Inkscape::Selection *selection = desktop->getSelection();
    if (!selection) {
        return;
    }

    ToolBase *ev = desktop->getEventContext();
    Inkscape::UI::Tools::GradientTool *rc = SP_GRADIENT_CONTEXT(ev);

    if (rc) {
        sp_gradient_context_add_stops_between_selected_stops(rc);
    }

}

/* Remove stop from vector */
static void gr_remove_stop(GtkWidget * /*button*/, GObject *data)
{

    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data(data, "desktop"));
    if (!desktop) {
        return;
    }

    Inkscape::Selection *selection = desktop->getSelection(); // take from desktop, not from args
    if (!selection) {
        return;
    }

    ToolBase *ev = desktop->getEventContext();
    GrDrag *drag = nullptr;
    if (ev) {
        drag = ev->get_drag();
    }

    if (drag) {
        drag->deleteSelected();
    }

}

/* Lock or unlock links */
static void gr_linked_changed(GtkToggleAction *act, gpointer /*data*/)
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

/* Reverse vector */
static void gr_reverse(GtkWidget * /*button*/, gpointer data)
{
    SPDesktop *desktop = static_cast<SPDesktop *>(data);
    sp_gradient_reverse_selected_gradients(desktop);
}

/*
 *  Change desktop dragger selection to this stop
 */
/* Set the offset widget value (based on which stop is selected). */
static void gr_stop_set_offset (GObject *data)
{
    if (!blocked) {
        std::cerr << "gr_stop_set_offset: should be blocked!" << std::endl;
    }

    SPStop *stop = get_selected_stop(data);
    if (!stop) {
        // std::cerr << "gr_stop_set_offset: no stop!" << std::endl;
        return;
    }

    EgeAdjustmentAction* act = (EgeAdjustmentAction *)g_object_get_data( data, "offset_action");
    if (!act) {
        return;
    }
    GtkAdjustment *adj = ege_adjustment_action_get_adjustment(act);

    bool isEndStop = false;

    SPStop *prev = nullptr;
    prev = stop->getPrevStop();
    if (prev != nullptr )  {
        gtk_adjustment_set_lower(adj, prev->offset);
    } else {
        isEndStop = true;
        gtk_adjustment_set_lower(adj, 0);
    }

    SPStop *next = nullptr;
    next = stop->getNextStop();
    if (next != nullptr ) {
        gtk_adjustment_set_upper(adj, next->offset);
    } else {
        isEndStop = true;
        gtk_adjustment_set_upper(adj, 1.0);
    }

    gtk_adjustment_set_value(adj, stop->offset);
    gtk_action_set_sensitive( GTK_ACTION(act), !isEndStop );
    gtk_adjustment_changed(adj);
}

static void select_dragger_by_stop( GObject *data, SPGradient *gradient, ToolBase *ev)
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

    SPStop *stop = get_selected_stop(data);

    drag->selectByStop(stop, false, true);

    gr_stop_set_offset(data);
}

/* Find position of new_stop in menu. */
static int select_stop_in_list (SPGradient *gradient, SPStop *new_stop, GObject *data)
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

/* Construct stop list */
static int update_stop_list( SPGradient *gradient, SPStop *new_stop, GObject *data, bool gr_multi)
{
    if (!blocked) {
        std::cerr << "update_stop_list should be blocked!" << std::endl;
    }

    int selected = -1;

    auto act = static_cast<InkSelectOneAction*>(g_object_get_data(data, "gradient_stop_action"));
    Glib::RefPtr<Gtk::ListStore> store = act->get_store();

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
        selected = select_stop_in_list (gradient, new_stop, data);
    }

    return selected;
}

/* Set stop in menu to match stops selected by draggers. */
static void select_stop_by_draggers(SPGradient *gradient, ToolBase *ev, GObject *data)
{
    if (!blocked) {
        std::cerr << "select_stop_by_draggers should be blocked!" << std::endl;
    }

    if (!ev || !gradient)
        return;

    SPGradient *vector = gradient->getVector();
    if (!vector)
        return;

    InkSelectOneAction *act =
        static_cast<InkSelectOneAction *>(g_object_get_data(data, "gradient_stop_action"));

    GrDrag *drag = ev->get_drag();

    if (!drag || drag->selected.empty()) {
        act->set_active(0);
        gr_stop_set_offset(data);
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

        EgeAdjustmentAction* offset = (EgeAdjustmentAction *)g_object_get_data( G_OBJECT(data), "offset_action");
        if (offset) {
            gtk_action_set_sensitive( GTK_ACTION(offset), FALSE);
        }

        // Stop list always updated first... reinsert "Multiple stops" as first entry.
        InkSelectOneActionColumns columns;
        Glib::RefPtr<Gtk::ListStore> store = act->get_store();

        Gtk::TreeModel::Row row = *(store->prepend());
        row[columns.col_label    ] = _("Multiple stops");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;
        selected = 0;

    } else {
        selected = select_stop_in_list( gradient, stop, data);
    }

    if (selected < 0) {
        act->set_active (0);
        act->set_sensitive (false);
    } else {
        act->set_active (selected);
        act->set_sensitive (true);
        gr_stop_set_offset(data);
    }
}

/* Return gradient selected in menu. */
static SPGradient *gr_get_selected_gradient(GObject *data)
{
    InkSelectOneAction *act =
        static_cast<InkSelectOneAction *>(g_object_get_data(data, "gradient_select_action"));

    int active = act->get_active();

    Glib::RefPtr<Gtk::ListStore> store = act->get_store();
    Gtk::TreeModel::Row row = store->children()[active];
    InkSelectOneActionColumns columns;

    void* pointer = row[columns.col_data];
    SPGradient *gr = static_cast<SPGradient *>(pointer);

    return gr;
}

/*
 * Callback functions for user actions
 */

static void gr_new_type_changed( GObject * /*data*/, int mode )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt("/tools/gradient/newgradient",
                  mode == 0 ? SP_GRADIENT_TYPE_LINEAR : SP_GRADIENT_TYPE_RADIAL);
}

static void gr_new_fillstroke_changed( GObject * /*data*/, int mode )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Inkscape::PaintTarget fsmode = (mode == 0) ? Inkscape::FOR_FILL : Inkscape::FOR_STROKE;
    prefs->setInt("/tools/gradient/newfillorstroke", (fsmode == Inkscape::FOR_FILL) ? 1 : 0);
}

/*
 * User selected a gradient from the combobox
 */
static void gr_gradient_changed( GObject *data, int active )
{
    if (blocked) {
        return;
    }

    if (active < 0) {
        return;
    }

    blocked = true;

    SPGradient *gr = gr_get_selected_gradient (data);

    if (gr) {
        gr = sp_gradient_ensure_vector_normalized(gr);

        SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data(data, "desktop"));
        Inkscape::Selection *selection = desktop->getSelection();
        ToolBase *ev = desktop->getEventContext();

        gr_apply_gradient(selection, ev ? ev->get_drag() : nullptr, gr);

        DocumentUndo::done(desktop->getDocument(), SP_VERB_CONTEXT_GRADIENT,
                   _("Assign gradient to object"));
    }

    blocked = false;
}

/*
 * User selected a spread method from the combobox
 */
static void gr_spread_changed( GObject *data, int active )
{
    if (blocked) {
        return;
    }

    blocked = true;

    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data(data, "desktop"));
    Inkscape::Selection *selection = desktop->getSelection();
    SPGradient *gradient = nullptr;
    gr_get_dt_selected_gradient(selection, gradient);

    if (gradient) {
        SPGradientSpread spread = (SPGradientSpread) active;
        gradient->setSpread(spread);
        gradient->updateRepr();

        DocumentUndo::done(desktop->getDocument(), SP_VERB_CONTEXT_GRADIENT,
                   _("Set gradient repeat"));
    }

    blocked = false;
}

/*
 * User selected a stop from the combobox
 */
static void gr_stop_changed( GObject *data, int active )
{
    if (blocked) {
        return;
    }

    blocked = true;

    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data(G_OBJECT(data), "desktop"));
    ToolBase *ev = desktop->getEventContext();
    SPGradient *gr = gr_get_selected_gradient(data);

    select_dragger_by_stop(data, gr, ev);

    blocked = false;
}

/*
 * User selected a stop from the combobox
 */
static void gr_stop_combo_changed(GtkComboBox * /*widget*/, GObject *data)
{
    if (blocked) {
        return;
    }

    blocked = true;

    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data(G_OBJECT(data), "desktop"));
    ToolBase *ev = desktop->getEventContext();
    SPGradient *gr = gr_get_selected_gradient(data);

    select_dragger_by_stop(data, gr, ev);

    blocked = false;
}

/*
 * User changed the offset
 */
static void gr_stop_offset_adjustment_changed(GtkAdjustment *adj, GObject *data)
{
    if (blocked) {
        return;
    }

    blocked = true;

    SPStop *stop = get_selected_stop(data);
    if (stop) {
        stop->offset = gtk_adjustment_get_value(adj);
        sp_repr_set_css_double(stop->getRepr(), "offset", stop->offset);

        DocumentUndo::maybeDone(stop->document, "gradient:stop:offset", SP_VERB_CONTEXT_GRADIENT,
                                _("Change gradient stop offset"));

    }

    blocked = false;
}


static void gradient_toolbox_check_ec(SPDesktop* dt, Inkscape::UI::Tools::ToolBase* ec, GObject* data);

/**
 * Gradient auxiliary toolbar construction and setup.
 *
 */
void sp_gradient_toolbox_prep(SPDesktop * desktop, GtkActionGroup* mainActions, GObject* data)
{
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

        InkSelectOneAction* act =
            InkSelectOneAction::create( "GradientNewTypeAction", // Name
                                        _("New"),            // Label
                                        "",                  // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        act->use_radio( true );
        act->use_group_label( true );
        gint mode = prefs->getInt("/tools/gradient/newgradient", SP_GRADIENT_TYPE_LINEAR);
        act->set_active( mode == SP_GRADIENT_TYPE_LINEAR ? 0 : 1 ); // linear == 1, radial == 2

        gtk_action_group_add_action( mainActions, GTK_ACTION( act->gobj() ));
        g_object_set_data( data, "gradient_new_type_mode", act );

        act->signal_changed().connect(sigc::bind<0>(sigc::ptr_fun(&gr_new_type_changed), data));
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

        InkSelectOneAction* act =
            InkSelectOneAction::create( "GradientNewFillStrokeAction", // Name
                                        "",                  // Label
                                        "",                  // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        act->use_radio( true );
        act->use_group_label( false );
        Inkscape::PaintTarget fsmode = (prefs->getInt("/tools/gradient/newfillorstroke", 1) != 0) ? Inkscape::FOR_FILL : Inkscape::FOR_STROKE;
        act->set_active( fsmode == Inkscape::FOR_FILL ? 0 : 1 );

        gtk_action_group_add_action( mainActions, GTK_ACTION( act->gobj() ));
        g_object_set_data( data, "gradient_new_fillstroke_action", act );

        act->signal_changed().connect(sigc::bind<0>(sigc::ptr_fun(&gr_new_fillstroke_changed), data));
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

        InkSelectOneAction* act =
            InkSelectOneAction::create( "GradientSelectGradientAction", // Name
                                        _("Select"),         // Label
                                        "",                  // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        act->use_radio( false );
        act->use_icon( false );
        act->use_pixbuf( true );
        act->use_group_label( true );
        act->set_active( 0 );
        act->set_sensitive( false );

        gtk_action_group_add_action( mainActions, GTK_ACTION( act->gobj() ));
        g_object_set_data( data, "gradient_select_action", act );

        act->signal_changed().connect(sigc::bind<0>(sigc::ptr_fun(&gr_gradient_changed), data));
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

        InkSelectOneAction* act =
            InkSelectOneAction::create( "GradientSelectSpreadAction", // Name
                                        _("Repeat"),            // Label
                                        // TRANSLATORS: for info, see http://www.w3.org/TR/2000/CR-SVG-20000802/pservers.html#LinearGradientSpreadMethodAttribute
                                        _("Whether to fill with flat color beyond the ends of the gradient vector "
                                          "(spreadMethod=\"pad\"), or repeat the gradient in the same direction "
                                          "(spreadMethod=\"repeat\"), or repeat the gradient in alternating opposite "
                                          "directions (spreadMethod=\"reflect\")"),    // Tooltip
                                        "Not Used",             // Icon
                                        store );                // Tree store

        act->use_radio( false );
        act->use_icon( false );
        act->use_group_label( true );
        act->set_sensitive( false );

        gtk_action_group_add_action( mainActions, GTK_ACTION( act->gobj() ));
        g_object_set_data( data, "gradient_spread_action", act );

        act->signal_changed().connect(sigc::bind<0>(sigc::ptr_fun(&gr_spread_changed), data));
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

        InkSelectOneAction* act =
            InkSelectOneAction::create( "GradientStopAction", // Name
                                        _("Stops" ),         // Label
                                        "",                  // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        act->use_radio( false );
        act->use_icon( false );
        act->use_pixbuf( true );
        act->use_group_label( true );
        act->set_active( 0 );
        act->set_sensitive( false );

        gtk_action_group_add_action( mainActions, GTK_ACTION( act->gobj() ));
        g_object_set_data( data, "gradient_stop_action", act );

        act->signal_changed().connect(sigc::bind<0>(sigc::ptr_fun(&gr_stop_changed), data));
    }

    /* Offset */
    {
        EgeAdjustmentAction* eact = nullptr;
        eact = create_adjustment_action( "GradientEditOffsetAction",
                                         _("Offset"), C_("Gradient", "Offset:"), _("Offset of selected stop"),
                                         "/tools/gradient/stopoffset", 0,
                                         GTK_WIDGET(desktop->canvas), data, FALSE, nullptr,
                                         0.0, 1.0, 0.01, 0.1,
                                         nullptr, nullptr, 0,
                                         gr_stop_offset_adjustment_changed,
                                         nullptr /*unit tracker*/,
                                         0.01, 2, 1.0);

        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        g_object_set_data( data, "offset_action", eact );
        gtk_action_set_sensitive( GTK_ACTION(eact), FALSE );

    }

    /* Add stop */
    {
        InkAction* inky = ink_action_new( "GradientEditAddAction",
                                          _("Insert new stop"),
                                          _("Insert new stop"),
                                          INKSCAPE_ICON("node-add"),
                                          secondarySize );
        g_object_set( inky, "short_label", _("Delete"), NULL );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(gr_add_stop), data );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
        gtk_action_set_sensitive( GTK_ACTION(inky), FALSE );
        g_object_set_data( data, "gradient_stops_add_action", inky );
    }

    /* Delete stop */
    {
        InkAction* inky = ink_action_new( "GradientEditDeleteAction",
                                          _("Delete stop"),
                                          _("Delete stop"),
                                          INKSCAPE_ICON("node-delete"),
                                          secondarySize );
        g_object_set( inky, "short_label", _("Delete"), NULL );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(gr_remove_stop), data );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
        gtk_action_set_sensitive( GTK_ACTION(inky), FALSE );
        g_object_set_data( data, "gradient_stops_delete_action", inky );
    }

    /* Reverse */
    {
        InkAction* inky = ink_action_new( "GradientEditReverseAction",
                                          _("Reverse"),
                                          _("Reverse the direction of the gradient"),
                                          INKSCAPE_ICON("object-flip-horizontal"),
                                          secondarySize );
        g_object_set( inky, "short_label", _("Delete"), NULL );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(gr_reverse), desktop );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
        gtk_action_set_sensitive( GTK_ACTION(inky), FALSE );
        g_object_set_data( data, "gradient_stops_reverse_action", inky );

    }

    // Gradients Linked toggle
    {
        InkToggleAction* itact = ink_toggle_action_new( "GradientEditLinkAction",
                                                        _("Link gradients"),
                                                        _("Link gradients to change all related gradients"),
                                                        INKSCAPE_ICON("object-unlocked"),
                                                        GTK_ICON_SIZE_MENU );
        g_object_set( itact, "short_label", "Lock", NULL );
        g_signal_connect_after( G_OBJECT(itact), "toggled", G_CALLBACK(gr_linked_changed), desktop) ;
        gtk_action_group_add_action( mainActions, GTK_ACTION(itact) );

        bool linkedmode = prefs->getBool("/options/forkgradientvectors/value", true);
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(itact), !linkedmode );
    }

    g_object_set_data(data, "desktop", desktop);

    desktop->connectEventContextChanged(sigc::bind(sigc::ptr_fun(&gradient_toolbox_check_ec), data));
}


// =================== Selection Changed Callbacks ======================= //

/*
 * Core function, setup all the widgets whenever something changes on the desktop
 */
static void gr_tb_selection_changed(Inkscape::Selection * /*selection*/, GObject* data)
{
    if (blocked)
        return;

    blocked = true;

    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data(data, "desktop"));
    if (!desktop) {
        return;
    }

    Inkscape::Selection *selection = desktop->getSelection(); // take from desktop, not from args
    if (selection) {

        ToolBase *ev = desktop->getEventContext();
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
        auto sel = static_cast<InkSelectOneAction*>(g_object_get_data(data, "gradient_select_action"));
        Glib::RefPtr<Gtk::ListStore> store = sel->get_store();
        int gradient = gr_vector_list (store, desktop, selection->isEmpty(), gr_selected, gr_multi);

        if (gradient < 0) {
            // No selection or no gradients
            sel->set_active( 0 );
            sel->set_sensitive (false);
        } else {
            // Single gradient or multiple gradients
            sel->set_active( gradient );
            sel->set_sensitive (true);
        }

        // Spread menu
        auto spread = static_cast<InkSelectOneAction*>(g_object_get_data(data, "gradient_spread_action"));
        spread->set_sensitive( gr_selected && !gr_multi );
        spread->set_active( gr_selected ? (int)spr_selected : 0 );

        InkAction *add = (InkAction *) g_object_get_data(data, "gradient_stops_add_action");
        gtk_action_set_sensitive(GTK_ACTION(add), (gr_selected && !gr_multi && drag && !drag->selected.empty()));

        InkAction *del = (InkAction *) g_object_get_data(data, "gradient_stops_delete_action");
        gtk_action_set_sensitive(GTK_ACTION(del), (gr_selected && !gr_multi && drag && !drag->selected.empty()));

        InkAction *reverse = (InkAction *) g_object_get_data(data, "gradient_stops_reverse_action");
        gtk_action_set_sensitive(GTK_ACTION(reverse), (gr_selected!= nullptr));

        InkSelectOneAction *stops_action = (InkSelectOneAction *) g_object_get_data(data, "gradient_stop_action");
        stops_action->set_sensitive( gr_selected && !gr_multi);

        int stop = update_stop_list (gr_selected, nullptr, data, gr_multi);
        select_stop_by_draggers(gr_selected, ev, data);
    }

    blocked = false;
}

static void gr_tb_selection_modified(Inkscape::Selection *selection, guint /*flags*/, GObject* data)
{
    gr_tb_selection_changed(selection, data);
}

static void gr_drag_selection_changed(gpointer /*dragger*/, GObject* data)
{
    gr_tb_selection_changed(nullptr, data);
}

static void gr_defs_release(SPObject * /*defs*/, GObject* data)
{
    gr_tb_selection_changed(nullptr, data);
}

static void gr_defs_modified(SPObject * /*defs*/, guint /*flags*/, GObject* data)
{
    gr_tb_selection_changed(nullptr, data);
}

// lp:1327267
/**
 * Checks the current tool and connects gradient aux toolbox signals if it happens to be the gradient tool.
 * Called every time the current tool changes by signal emission.
 */
static void gradient_toolbox_check_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec, GObject* data)
{
    static sigc::connection connChanged;
    static sigc::connection connModified;
    static sigc::connection connSubselectionChanged;
    static sigc::connection connDefsRelease;
    static sigc::connection connDefsModified;

    if (SP_IS_GRADIENT_CONTEXT(ec)) {
        Inkscape::Selection *selection = desktop->getSelection();
        SPDocument *document = desktop->getDocument();

        // connect to selection modified and changed signals
        connChanged  = selection->connectChanged(sigc::bind(sigc::ptr_fun(&gr_tb_selection_changed), data));
        connModified = selection->connectModified(sigc::bind(sigc::ptr_fun(&gr_tb_selection_modified), data));
        connSubselectionChanged = desktop->connectToolSubselectionChanged(sigc::bind(sigc::ptr_fun(&gr_drag_selection_changed), data));

        // Is this necessary? Couldn't hurt.
        gr_tb_selection_changed(selection, data);

        // connect to release and modified signals of the defs (i.e. when someone changes gradient)
        connDefsRelease  = document->getDefs()->connectRelease( sigc::bind<1>(sigc::ptr_fun(&gr_defs_release),  data));
        connDefsModified = document->getDefs()->connectModified(sigc::bind<2>(sigc::ptr_fun(&gr_defs_modified), data));
    } else {
        if (connChanged)
            connChanged.disconnect();
        if (connModified)
            connModified.disconnect();
        if (connSubselectionChanged)
            connSubselectionChanged.disconnect();
        if (connDefsRelease)
            connDefsRelease.disconnect();
        if (connDefsModified)
            connDefsModified.disconnect();
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

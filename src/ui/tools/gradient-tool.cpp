// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gradient drawing and editing tool
 *
 * Authors:
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Abhishek Sharma
 *
 * Copyright (C) 2007 Johan Engelen
 * Copyright (C) 2005 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>
#include <gdk/gdkkeysyms.h>

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "gradient-chemistry.h"
#include "gradient-drag.h"
#include "include/macros.h"
#include "message-context.h"
#include "message-stack.h"
#include "rubberband.h"
#include "selection-chemistry.h"
#include "selection.h"
#include "snap.h"
#include "verbs.h"

#include "object/sp-namedview.h"
#include "object/sp-stop.h"

#include "display/control/canvas-item-curve.h"

#include "svg/css-ostringstream.h"

#include "ui/tools/gradient-tool.h"

using Inkscape::DocumentUndo;

namespace Inkscape {
namespace UI {
namespace Tools {

static void sp_gradient_drag(GradientTool &rc, Geom::Point const pt, guint state, guint32 etime);

const std::string& GradientTool::getPrefsPath() {
	return GradientTool::prefsPath;
}

const std::string GradientTool::prefsPath = "/tools/gradient";


GradientTool::GradientTool()
    : ToolBase("gradient.svg")
    , cursor_addnode(false)
    , node_added(false)
// TODO: Why are these connections stored as pointers?
    , selcon(nullptr)
    , subselcon(nullptr)
{
    // TODO: This value is overwritten in the root handler
    this->tolerance = 6;
}

GradientTool::~GradientTool() {
    this->enableGrDrag(false);

    this->selcon->disconnect();
    delete this->selcon;

    this->subselcon->disconnect();
    delete this->subselcon;
}

// This must match GrPointType enum sp-gradient.h
// We should move this to a shared header (can't simply move to gradient.h since that would require
// including <glibmm/i18n.h> which messes up "N_" in extensions... argh!).
const gchar *gr_handle_descr [] = {
    N_("Linear gradient <b>start</b>"), //POINT_LG_BEGIN
    N_("Linear gradient <b>end</b>"),
    N_("Linear gradient <b>mid stop</b>"),
    N_("Radial gradient <b>center</b>"),
    N_("Radial gradient <b>radius</b>"),
    N_("Radial gradient <b>radius</b>"),
    N_("Radial gradient <b>focus</b>"), // POINT_RG_FOCUS
    N_("Radial gradient <b>mid stop</b>"),
    N_("Radial gradient <b>mid stop</b>"),
    N_("Mesh gradient <b>corner</b>"),
    N_("Mesh gradient <b>handle</b>"),
    N_("Mesh gradient <b>tensor</b>")
};

void GradientTool::selection_changed(Inkscape::Selection*) {

    GrDrag *drag = _grdrag;
    Inkscape::Selection *selection = this->desktop->getSelection();
    if (selection == nullptr) {
        return;
    }
    guint n_obj = (guint) boost::distance(selection->items());

    if (!drag->isNonEmpty() || selection->isEmpty())
        return;
    guint n_tot = drag->numDraggers();
    guint n_sel = drag->numSelected();

    //The use of ngettext in the following code is intentional even if the English singular form would never be used
    if (n_sel == 1) {
        if (drag->singleSelectedDraggerNumDraggables() == 1) {
            gchar * message = g_strconcat(
                //TRANSLATORS: %s will be substituted with the point name (see previous messages); This is part of a compound message
                _("%s selected"),
                //TRANSLATORS: Mind the space in front. This is part of a compound message
                ngettext(" out of %d gradient handle"," out of %d gradient handles",n_tot),
                ngettext(" on %d selected object"," on %d selected objects",n_obj),NULL);
            message_context->setF(Inkscape::NORMAL_MESSAGE,
                                  message,_(gr_handle_descr[drag->singleSelectedDraggerSingleDraggableType()]), n_tot, n_obj);
        } else {
            gchar * message = g_strconcat(
                //TRANSLATORS: This is a part of a compound message (out of two more indicating: grandint handle count & object count)
                ngettext("One handle merging %d stop (drag with <b>Shift</b> to separate) selected",
                         "One handle merging %d stops (drag with <b>Shift</b> to separate) selected",drag->singleSelectedDraggerNumDraggables()),
                ngettext(" out of %d gradient handle"," out of %d gradient handles",n_tot),
                ngettext(" on %d selected object"," on %d selected objects",n_obj),NULL);
            message_context->setF(Inkscape::NORMAL_MESSAGE,message,drag->singleSelectedDraggerNumDraggables(), n_tot, n_obj);
        }
    } else if (n_sel > 1) {
        //TRANSLATORS: The plural refers to number of selected gradient handles. This is part of a compound message (part two indicates selected object count)
        gchar * message = g_strconcat(ngettext("<b>%d</b> gradient handle selected out of %d","<b>%d</b> gradient handles selected out of %d",n_sel),
                                      //TRANSLATORS: Mind the space in front. (Refers to gradient handles selected). This is part of a compound message
                                      ngettext(" on %d selected object"," on %d selected objects",n_obj),NULL);
        message_context->setF(Inkscape::NORMAL_MESSAGE,message, n_sel, n_tot, n_obj);
    } else if (n_sel == 0) {
        message_context->setF(Inkscape::NORMAL_MESSAGE,
                              //TRANSLATORS: The plural refers to number of selected objects
                              ngettext("<b>No</b> gradient handles selected out of %d on %d selected object",
                                       "<b>No</b> gradient handles selected out of %d on %d selected objects",n_obj), n_tot, n_obj);
    }
}

void GradientTool::setup() {
    ToolBase::setup();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    
    if (prefs->getBool("/tools/gradient/selcue", true)) {
        this->enableSelectionCue();
    }

    this->enableGrDrag();
    Inkscape::Selection *selection = this->desktop->getSelection();

    this->selcon = new sigc::connection(selection->connectChanged(
    	sigc::mem_fun(this, &GradientTool::selection_changed)
    ));

    this->subselcon = new sigc::connection(this->desktop->connectToolSubselectionChanged(
    	sigc::hide(sigc::bind(
    		sigc::mem_fun(this, &GradientTool::selection_changed),
    		(Inkscape::Selection*)nullptr
    	))
    ));

    this->selection_changed(selection);
}

void
sp_gradient_context_select_next (ToolBase *event_context)
{
    GrDrag *drag = event_context->_grdrag;
    g_assert (drag);

    GrDragger *d = drag->select_next();

    event_context->getDesktop()->scroll_to_point(d->point, 1.0);
}

void
sp_gradient_context_select_prev (ToolBase *event_context)
{
    GrDrag *drag = event_context->_grdrag;
    g_assert (drag);

    GrDragger *d = drag->select_prev();

    event_context->getDesktop()->scroll_to_point(d->point, 1.0);
}

static SPItem*
sp_gradient_context_is_over_curve (GradientTool *rc, Geom::Point event_p)
{
    const SPDesktop *desktop = rc->getDesktop();

    //Translate mouse point into proper coord system: needed later.
    rc->mousepoint_doc = desktop->w2d(event_p);

    GrDrag *drag = rc->_grdrag;
    for (auto curve : drag->item_curves) {
        if (curve->contains(event_p, rc->tolerance)) {
            return curve->get_item();
        }
    }
    return nullptr;
}

static std::vector<Geom::Point>
sp_gradient_context_get_stop_intervals (GrDrag *drag, std::vector<SPStop *> &these_stops, std::vector<SPStop *> &next_stops)
{
    std::vector<Geom::Point> coords;

    // for all selected draggers
    for (std::set<GrDragger *>::const_iterator i = drag->selected.begin(); i != drag->selected.end() ; ++i ) {
        GrDragger *dragger = *i;
        // remember the coord of the dragger to reselect it later
        coords.push_back(dragger->point);
        // for all draggables of dragger
        for (std::vector<GrDraggable *>::const_iterator j = dragger->draggables.begin(); j != dragger->draggables.end(); ++j) {
            GrDraggable *d = *j;

            // find the gradient
            SPGradient *gradient = getGradient(d->item, d->fill_or_stroke);
            SPGradient *vector = sp_gradient_get_forked_vector_if_necessary (gradient, false);

            // these draggable types cannot have a next draggabe to insert a stop between them
            if (d->point_type == POINT_LG_END ||
                d->point_type == POINT_RG_FOCUS ||
                d->point_type == POINT_RG_R1 ||
                d->point_type == POINT_RG_R2) {
                continue;
            }

            // from draggables to stops
            SPStop *this_stop = sp_get_stop_i (vector, d->point_i);
            SPStop *next_stop = this_stop->getNextStop();
            SPStop *last_stop = sp_last_stop (vector);

            Inkscape::PaintTarget fs = d->fill_or_stroke;
            SPItem *item = d->item;
            gint type = d->point_type;
            gint p_i = d->point_i;

            // if there's a next stop,
            if (next_stop) {
                GrDragger *dnext = nullptr;
                // find its dragger
                // (complex because it may have different types, and because in radial,
                // more than one dragger may correspond to a stop, so we must distinguish)
                if (type == POINT_LG_BEGIN || type == POINT_LG_MID) {
                    if (next_stop == last_stop) {
                        dnext = drag->getDraggerFor(item, POINT_LG_END, p_i+1, fs);
                    } else {
                        dnext = drag->getDraggerFor(item, POINT_LG_MID, p_i+1, fs);
                    }
                } else { // radial
                    if (type == POINT_RG_CENTER || type == POINT_RG_MID1) {
                        if (next_stop == last_stop) {
                            dnext = drag->getDraggerFor(item, POINT_RG_R1, p_i+1, fs);
                        } else {
                            dnext = drag->getDraggerFor(item, POINT_RG_MID1, p_i+1, fs);
                        }
                    }
                    if ((type == POINT_RG_MID2) ||
                        (type == POINT_RG_CENTER && dnext && !dnext->isSelected())) {
                        if (next_stop == last_stop) {
                            dnext = drag->getDraggerFor(item, POINT_RG_R2, p_i+1, fs);
                        } else {
                            dnext = drag->getDraggerFor(item, POINT_RG_MID2, p_i+1, fs);
                        }
                    }
                }

                // if both adjacent draggers selected,
                if ((std::find(these_stops.begin(),these_stops.end(),this_stop)==these_stops.end()) && dnext && dnext->isSelected()) {

                    // remember the coords of the future dragger to select it
                    coords.push_back(0.5*(dragger->point + dnext->point));

                    // do not insert a stop now, it will confuse the loop;
                    // just remember the stops
                    these_stops.push_back(this_stop);
                    next_stops.push_back(next_stop);
                }
            }
        }
    }
    return coords;
}

void
sp_gradient_context_add_stops_between_selected_stops (GradientTool *rc)
{
    SPDocument *doc = nullptr;
    GrDrag *drag = rc->_grdrag;

    std::vector<SPStop *> these_stops;
    std::vector<SPStop *> next_stops;

    std::vector<Geom::Point> coords = sp_gradient_context_get_stop_intervals (drag, these_stops, next_stops);

    if (these_stops.empty() && drag->numSelected() == 1) {
        // if a single stop is selected, add between that stop and the next one
        GrDragger *dragger = *(drag->selected.begin());
        for (auto d : dragger->draggables) {
            if (d->point_type == POINT_RG_FOCUS) {
                /*
                 *  There are 2 draggables at the center (start) of a radial gradient
                 *  To avoid creating 2 separate stops, ignore this draggable point type
                 */
                continue;
            }
            SPGradient *gradient = getGradient(d->item, d->fill_or_stroke);
            SPGradient *vector = sp_gradient_get_forked_vector_if_necessary (gradient, false);
            SPStop *this_stop = sp_get_stop_i (vector, d->point_i);
            if (this_stop) {
                SPStop *next_stop = this_stop->getNextStop();
                if (next_stop) {
                    these_stops.push_back(this_stop);
                    next_stops.push_back(next_stop);
                }
            }
        }
    }

    // now actually create the new stops
    auto i = these_stops.rbegin();
    auto j = next_stops.rbegin();
    std::vector<SPStop *> new_stops;

    for (;i != these_stops.rend() && j != next_stops.rend(); ++i, ++j ) {
        SPStop *this_stop = *i;
        SPStop *next_stop = *j;
        gfloat offset = 0.5*(this_stop->offset + next_stop->offset);
        SPObject *parent = this_stop->parent;
        if (SP_IS_GRADIENT (parent)) {
            doc = parent->document;
            SPStop *new_stop = sp_vector_add_stop (SP_GRADIENT (parent), this_stop, next_stop, offset);
            new_stops.push_back(new_stop);
            SP_GRADIENT(parent)->ensureVector();
        }
    }

    if (!these_stops.empty() && doc) {
        DocumentUndo::done(doc, SP_VERB_CONTEXT_GRADIENT, _("Add gradient stop"));
        drag->updateDraggers();
        // so that it does not automatically update draggers in idle loop, as this would deselect
        drag->local_change = true;

        // select the newly created stops
        for (auto i:new_stops) {
            drag->selectByStop(i);
        }
    }
}

static double sqr(double x) {return x*x;}

/**
 * Remove unnecessary stops in the adjacent currently selected stops
 *
 * For selected stops that are adjacent to each other, remove
 * stops that don't change the gradient visually, within a range of tolerance.
 *
 * @param rc GradientTool used to extract selected stops
 * @param tolerance maximum difference between stop and expected color at that position
 */
static void
sp_gradient_simplify(GradientTool *rc, double tolerance)
{
    SPDocument *doc = nullptr;
    GrDrag *drag = rc->_grdrag;

    std::vector<SPStop *> these_stops;
    std::vector<SPStop *> next_stops;

    std::vector<Geom::Point> coords = sp_gradient_context_get_stop_intervals (drag, these_stops, next_stops);

    std::set<SPStop *> todel;

    auto i = these_stops.begin();
    auto j = next_stops.begin();
    for (; i != these_stops.end() && j != next_stops.end(); ++i, ++j) {
        SPStop *stop0 = *i;
        SPStop *stop1 = *j;

        // find the next adjacent stop if it exists and is in selection
        auto i1 = std::find(these_stops.begin(), these_stops.end(), stop1);
        if (i1 != these_stops.end()) {
            if (next_stops.size()>(i1-these_stops.begin())) {
                SPStop *stop2 = *(next_stops.begin() + (i1-these_stops.begin()));

                if (todel.find(stop0)!=todel.end() || todel.find(stop2) != todel.end())
                    continue;

                // compare color of stop1 to the average color of stop0 and stop2
                guint32 const c0 = stop0->get_rgba32();
                guint32 const c2 = stop2->get_rgba32();
                guint32 const c1r = stop1->get_rgba32();
                guint32 c1 = average_color (c0, c2,
                       (stop1->offset - stop0->offset) / (stop2->offset - stop0->offset));

                double diff =
                    sqr(SP_RGBA32_R_F(c1) - SP_RGBA32_R_F(c1r)) +
                    sqr(SP_RGBA32_G_F(c1) - SP_RGBA32_G_F(c1r)) +
                    sqr(SP_RGBA32_B_F(c1) - SP_RGBA32_B_F(c1r)) +
                    sqr(SP_RGBA32_A_F(c1) - SP_RGBA32_A_F(c1r));

                if (diff < tolerance)
                    todel.insert(stop1);
            }
        }
    }

    for (auto stop : todel) {
        doc = stop->document;
        Inkscape::XML::Node * parent = stop->getRepr()->parent();
        parent->removeChild( stop->getRepr() );
    }

    if (!todel.empty()) {
        DocumentUndo::done(doc, SP_VERB_CONTEXT_GRADIENT, _("Simplify gradient"));
        drag->local_change = true;
        drag->updateDraggers();
        drag->selectByCoords(coords);
    }
}


static void
sp_gradient_context_add_stop_near_point (GradientTool *rc, SPItem *item,  Geom::Point mouse_p, guint32 /*etime*/)
{
    // item is the selected item. mouse_p the location in doc coordinates of where to add the stop

    const SPDesktop *desktop = rc->getDesktop();

    double tolerance = (double) rc->tolerance;

    SPStop *newstop = rc->get_drag()->addStopNearPoint (item, mouse_p, tolerance/desktop->current_zoom());

    DocumentUndo::done(desktop->getDocument(), SP_VERB_CONTEXT_GRADIENT,
                       _("Add gradient stop"));

    rc->get_drag()->updateDraggers();
    rc->get_drag()->local_change = true;
    rc->get_drag()->selectByStop(newstop);
}

bool GradientTool::root_handler(GdkEvent* event) {
    static bool dragging;

    Inkscape::Selection *selection = desktop->getSelection();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    tolerance = prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100);

    GrDrag *drag = this->_grdrag;
    g_assert (drag);

    gint ret = FALSE;

    switch (event->type) {
    case GDK_2BUTTON_PRESS:
        if ( event->button.button == 1 ) {

            SPItem* item = sp_gradient_context_is_over_curve(this, Geom::Point(event->motion.x, event->motion.y));
            if (item) {
                // we take the first item in selection, because with doubleclick, the first click
                // always resets selection to the single object under cursor
                sp_gradient_context_add_stop_near_point(this, SP_ITEM(selection->items().front()), mousepoint_doc, event->button.time);
            } else {
            	auto items= selection->items();
                for (auto i = items.begin();i!=items.end();++i) {
                    SPItem *item = *i;
                    SPGradientType new_type = (SPGradientType) prefs->getInt("/tools/gradient/newgradient", SP_GRADIENT_TYPE_LINEAR);
                    Inkscape::PaintTarget fsmode = (prefs->getInt("/tools/gradient/newfillorstroke", 1) != 0) ? Inkscape::FOR_FILL : Inkscape::FOR_STROKE;

                    SPGradient *vector = sp_gradient_vector_for_object(desktop->getDocument(), desktop, item, fsmode);

                    SPGradient *priv = sp_item_set_gradient(item, vector, new_type, fsmode);
                    sp_gradient_reset_to_userspace(priv, item);
                }
                desktop->redrawDesktop();;
                DocumentUndo::done(desktop->getDocument(), SP_VERB_CONTEXT_GRADIENT,
                                   _("Create default gradient"));
            }
            ret = TRUE;
        }
        break;

    case GDK_BUTTON_PRESS:
        if ( event->button.button == 1 && !this->space_panning ) {
            Geom::Point button_w(event->button.x, event->button.y);

            // save drag origin
            this->xp = (gint) button_w[Geom::X];
            this->yp = (gint) button_w[Geom::Y];
            this->within_tolerance = true;

            dragging = true;

            Geom::Point button_dt = desktop->w2d(button_w);
            if (event->button.state & GDK_SHIFT_MASK) {
                Inkscape::Rubberband::get(desktop)->start(desktop, button_dt);
            } else {
                // remember clicked item, disregarding groups, honoring Alt; do nothing with Crtl to
                // enable Ctrl+doubleclick of exactly the selected item(s)
                if (!(event->button.state & GDK_CONTROL_MASK)) {
                    this->item_to_select = sp_event_context_find_item (desktop, button_w, event->button.state & GDK_MOD1_MASK, TRUE);
                }

                if (!selection->isEmpty()) {
                    SnapManager &m = desktop->namedview->snap_manager;
                    m.setup(desktop);
                    m.freeSnapReturnByRef(button_dt, Inkscape::SNAPSOURCE_NODE_HANDLE);
                    m.unSetup();
                }

                this->origin = button_dt;
            }

            ret = TRUE;
        }
        break;

    case GDK_MOTION_NOTIFY:
        if (dragging && ( event->motion.state & GDK_BUTTON1_MASK ) && !this->space_panning) {
            if ( this->within_tolerance
                 && ( abs( (gint) event->motion.x - this->xp ) < this->tolerance )
                 && ( abs( (gint) event->motion.y - this->yp ) < this->tolerance ) ) {
                break; // do not drag if we're within tolerance from origin
            }
            // Once the user has moved farther than tolerance from the original location
            // (indicating they intend to draw, not click), then always process the
            // motion notify coordinates as given (no snapping back to origin)
            this->within_tolerance = false;

            Geom::Point const motion_w(event->motion.x,
                                     event->motion.y);
            Geom::Point const motion_dt = this->desktop->w2d(motion_w);

            if (Inkscape::Rubberband::get(desktop)->is_started()) {
                Inkscape::Rubberband::get(desktop)->move(motion_dt);
                this->defaultMessageContext()->set(Inkscape::NORMAL_MESSAGE, _("<b>Draw around</b> handles to select them"));
            } else {
                sp_gradient_drag(*this, motion_dt, event->motion.state, event->motion.time);
            }

            gobble_motion_events(GDK_BUTTON1_MASK);

            ret = TRUE;
        } else {
            if (!drag->mouseOver() && !selection->isEmpty()) {
                SnapManager &m = desktop->namedview->snap_manager;
                m.setup(desktop);

                Geom::Point const motion_w(event->motion.x, event->motion.y);
                Geom::Point const motion_dt = this->desktop->w2d(motion_w);

                m.preSnap(Inkscape::SnapCandidatePoint(motion_dt, Inkscape::SNAPSOURCE_OTHER_HANDLE));
                m.unSetup();
            }

            SPItem *item = sp_gradient_context_is_over_curve(this, Geom::Point(event->motion.x, event->motion.y));

            if (this->cursor_addnode && !item) {
                cursor_filename = "gradient.svg";
                this->sp_event_context_update_cursor();
                this->cursor_addnode = false;
            } else if (!this->cursor_addnode && item) {
                cursor_filename = "gradient-add.svg";
                this->sp_event_context_update_cursor();
                this->cursor_addnode = true;
            }
        }
        break;

    case GDK_BUTTON_RELEASE:
        this->xp = this->yp = 0;

        if ( event->button.button == 1 && !this->space_panning ) {

            SPItem *item = sp_gradient_context_is_over_curve(this, Geom::Point(event->motion.x, event->motion.y));

            if ( (event->button.state & GDK_CONTROL_MASK) && (event->button.state & GDK_MOD1_MASK ) ) {
                if (item) {
                    sp_gradient_context_add_stop_near_point(this, item, this->mousepoint_doc, 0);
                    ret = TRUE;
                }
            } else {
                dragging = false;

                // unless clicked with Ctrl (to enable Ctrl+doubleclick).
                if (event->button.state & GDK_CONTROL_MASK) {
                    ret = TRUE;
                    break;
                }

                if (!this->within_tolerance) {
                    // we've been dragging, either do nothing (grdrag handles that),
                    // or rubberband-select if we have rubberband
                    Inkscape::Rubberband *r = Inkscape::Rubberband::get(desktop);

                    if (r->is_started() && !this->within_tolerance) {
                        // this was a rubberband drag
                        if (r->getMode() == RUBBERBAND_MODE_RECT) {
                            Geom::OptRect const b = r->getRectangle();
                            drag->selectRect(*b);
                        }
                    }
                } else if (this->item_to_select) {
                    if (item) {
                        // Clicked on an existing gradient line, don't change selection. This stops
                        // possible change in selection during a double click with overlapping objects
                    } else {
                        // no dragging, select clicked item if any
                        if (event->button.state & GDK_SHIFT_MASK) {
                            selection->toggle(this->item_to_select);
                        } else {
                            drag->deselectAll();
                            selection->set(this->item_to_select);
                        }
                    }
                } else {
                    // click in an empty space; do the same as Esc
                    if (!drag->selected.empty()) {
                        drag->deselectAll();
                    } else {
                        selection->clear();
                    }
                }

                this->item_to_select = nullptr;
                ret = TRUE;
            }

            Inkscape::Rubberband::get(desktop)->stop();
        }
        break;

    case GDK_KEY_PRESS:
        switch (get_latin_keyval (&event->key)) {
        case GDK_KEY_Alt_L:
        case GDK_KEY_Alt_R:
        case GDK_KEY_Control_L:
        case GDK_KEY_Control_R:
        case GDK_KEY_Shift_L:
        case GDK_KEY_Shift_R:
        case GDK_KEY_Meta_L:  // Meta is when you press Shift+Alt (at least on my machine)
        case GDK_KEY_Meta_R:
            sp_event_show_modifier_tip (this->defaultMessageContext(), event,
                                        _("<b>Ctrl</b>: snap gradient angle"),
                                        _("<b>Shift</b>: draw gradient around the starting point"),
                                        nullptr);
            break;

        case GDK_KEY_x:
        case GDK_KEY_X:
            if (MOD__ALT_ONLY(event)) {
                desktop->setToolboxFocusTo ("altx-grad");
                ret = TRUE;
            }
            break;

        case GDK_KEY_A:
        case GDK_KEY_a:
            if (MOD__CTRL_ONLY(event) && drag->isNonEmpty()) {
                drag->selectAll();
                ret = TRUE;
            }
            break;

        case GDK_KEY_L:
        case GDK_KEY_l:
            if (MOD__CTRL_ONLY(event) && drag->isNonEmpty() && drag->hasSelection()) {
                sp_gradient_simplify(this, 1e-4);
                ret = TRUE;
            }
            break;

        case GDK_KEY_Escape:
            if (!drag->selected.empty()) {
                drag->deselectAll();
            } else {
                Inkscape::SelectionHelper::selectNone(desktop);
            }
            ret = TRUE;
            //TODO: make dragging escapable by Esc
            break;

        case GDK_KEY_r:
        case GDK_KEY_R:
            if (MOD__SHIFT_ONLY(event)) {
                sp_gradient_reverse_selected_gradients(desktop);
                ret = TRUE;
            }
            break;

        case GDK_KEY_Insert:
        case GDK_KEY_KP_Insert:
            // with any modifiers:
            sp_gradient_context_add_stops_between_selected_stops (this);
            ret = TRUE;
            break;

        case GDK_KEY_i:
        case GDK_KEY_I:
            if (MOD__SHIFT_ONLY(event)) {
                // Shift+I - insert stops (alternate keybinding for keyboards
                //           that don't have the Insert key)
                sp_gradient_context_add_stops_between_selected_stops (this);
                ret = TRUE;
            }
            break;

        case GDK_KEY_Delete:
        case GDK_KEY_KP_Delete:
        case GDK_KEY_BackSpace:
            ret = this->deleteSelectedDrag(MOD__CTRL_ONLY(event));
            break;

        default:
            ret = drag->key_press_handler(event);
            break;
        }
        break;

    case GDK_KEY_RELEASE:
        switch (get_latin_keyval (&event->key)) {
        case GDK_KEY_Alt_L:
        case GDK_KEY_Alt_R:
        case GDK_KEY_Control_L:
        case GDK_KEY_Control_R:
        case GDK_KEY_Shift_L:
        case GDK_KEY_Shift_R:
        case GDK_KEY_Meta_L:  // Meta is when you press Shift+Alt
        case GDK_KEY_Meta_R:
            this->defaultMessageContext()->clear();
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }

    if (!ret) {
    	ret = ToolBase::root_handler(event);
    }

    return ret;
}

// Creates a new linear or radial gradient.
static void sp_gradient_drag(GradientTool &rc, Geom::Point const pt, guint /*state*/, guint32 etime)
{
    SPDesktop *desktop = rc.getDesktop();
    Inkscape::Selection *selection = desktop->getSelection();
    SPDocument *document = desktop->getDocument();

    if (!selection->isEmpty()) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        int type = prefs->getInt("/tools/gradient/newgradient", 1);
        Inkscape::PaintTarget fill_or_stroke = (prefs->getInt("/tools/gradient/newfillorstroke", 1) != 0) ? Inkscape::FOR_FILL : Inkscape::FOR_STROKE;

        SPGradient *vector;
        if (rc.item_to_select) {
            // pick color from the object where drag started
            vector = sp_gradient_vector_for_object(document, desktop, rc.item_to_select, fill_or_stroke);
        } else {
            // Starting from empty space:
            // Sort items so that the topmost comes last
        	std::vector<SPItem*> items(selection->items().begin(), selection->items().end());
            sort(items.begin(),items.end(),sp_item_repr_compare_position_bool);
            // take topmost
            vector = sp_gradient_vector_for_object(document, desktop, SP_ITEM(items.back()), fill_or_stroke);
        }

        // HACK: reset fill-opacity - that 0.75 is annoying; BUT remove this when we have an opacity slider for all tabs
        SPCSSAttr *css = sp_repr_css_attr_new();
        sp_repr_css_set_property(css, "fill-opacity", "1.0");

        auto itemlist = selection->items();
        for (auto i = itemlist.begin();i!=itemlist.end();++i) {

            //FIXME: see above
            sp_repr_css_change_recursive((*i)->getRepr(), css, "style");

            sp_item_set_gradient(*i, vector, (SPGradientType) type, fill_or_stroke);

            if (type == SP_GRADIENT_TYPE_LINEAR) {
                sp_item_gradient_set_coords (*i, POINT_LG_BEGIN, 0, rc.origin, fill_or_stroke, true, false);
                sp_item_gradient_set_coords (*i, POINT_LG_END, 0, pt, fill_or_stroke, true, false);
            } else if (type == SP_GRADIENT_TYPE_RADIAL) {
                sp_item_gradient_set_coords (*i, POINT_RG_CENTER, 0, rc.origin, fill_or_stroke, true, false);
                sp_item_gradient_set_coords (*i, POINT_RG_R1, 0, pt, fill_or_stroke, true, false);
            }
            (*i)->requestModified(SP_OBJECT_MODIFIED_FLAG);
        }
        if (rc._grdrag) {
            rc._grdrag->updateDraggers();
            // prevent regenerating draggers by selection modified signal, which sometimes
            // comes too late and thus destroys the knot which we will now grab:
            rc._grdrag->local_change = true;
            // give the grab out-of-bounds values of xp/yp because we're already dragging
            // and therefore are already out of tolerance
            rc._grdrag->grabKnot (selection->items().front(),
                                   type == SP_GRADIENT_TYPE_LINEAR? POINT_LG_END : POINT_RG_R1,
                                   -1, // ignore number (though it is always 1)
                                   fill_or_stroke, 99999, 99999, etime);
        }
        // We did an undoable action, but SPDocumentUndo::done will be called by the knot when released

        // status text; we do not track coords because this branch is run once, not all the time
        // during drag
        int n_objects = (int) boost::distance(selection->items());
        rc.message_context->setF(Inkscape::NORMAL_MESSAGE,
                                  ngettext("<b>Gradient</b> for %d object; with <b>Ctrl</b> to snap angle",
                                           "<b>Gradient</b> for %d objects; with <b>Ctrl</b> to snap angle", n_objects),
                                  n_objects);
    } else {
        desktop->getMessageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>objects</b> on which to create gradient."));
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

/** \file
 * Pen event context implementation.
 */

/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 * Copyright (C) 2004 Monash University
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gdk/gdkkeysyms.h>
#include <cstring>
#include <string>

#include "pen-context.h"
#include "sp-namedview.h"
#include "sp-metrics.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "selection.h"
#include "selection-chemistry.h"
#include "draw-anchor.h"
#include "message-stack.h"
#include "message-context.h"
#include "preferences.h"
#include "sp-path.h"
#include "display/sp-canvas.h"
#include "display/curve.h"
#include "pixmaps/cursor-pen.xpm"
#include "display/canvas-bpath.h"
#include "display/sp-ctrlline.h"
#include "display/sodipodi-ctrl.h"
#include <glibmm/i18n.h>
#include "helper/units.h"
#include "macros.h"
#include "context-fns.h"
#include "tools-switch.h"
#include "ui/control-manager.h"
//BSpline
//Incluimos
#define INKSCAPE_LPE_SPIRO_C
#include "live_effects/lpe-spiro.h"

#include "display/curve.h"
#include <typeinfo>
#include <2geom/pathvector.h>
#include <2geom/affine.h>
#include <2geom/bezier-curve.h>
#include <2geom/hvlinesegment.h>
#include "helper/geom-nodetype.h"
#include "helper/geom-curves.h"

// For handling un-continuous paths:
#include "message-stack.h"
#include "inkscape.h"
#include "desktop.h"

#include "live_effects/spiro.h"


#define INKSCAPE_LPE_BSPLINE_C
#include "live_effects/lpe-bspline.h"
#include <2geom/nearest-point.h>

//BSpline End
using Inkscape::ControlManager;

static void sp_pen_context_dispose(GObject *object);

static void sp_pen_context_setup(SPEventContext *ec);
static void sp_pen_context_finish(SPEventContext *ec);
static void sp_pen_context_set(SPEventContext *ec, Inkscape::Preferences::Entry *val);
static gint sp_pen_context_root_handler(SPEventContext *ec, GdkEvent *event);
static gint sp_pen_context_item_handler(SPEventContext *event_context, SPItem *item, GdkEvent *event);

static void spdc_pen_set_initial_point(SPPenContext *pc, Geom::Point const p);
/*
 *BSpline
 *Added functions
*/

/*
 *Sobrecarga la función "sp_pen_context_set_polyline_mode"
 *Le da valor a la nueva propiedad "pc->spiro" que como se auto define indica si estamos en modo spiro
 *En el futuro se la dará a "pc->bspline"
*/
static void sp_pen_context_set_mode(SPPenContext *const pc, guint mode);
//Esta función cambia los colores rojo,verde y azul haciendolos transparentes o no en función de si se usa spiro
static void spiro_color(SPPenContext *const pc);
//Guarda el valor si se ha pulsado la tecla SHIFT al continuar una curva
static void spiro(SPPenContext *const pc,bool shift);
static void spiroOn(SPPenContext *const pc);
static void spiroOff(SPPenContext *const pc);
static void spiroStartAnchor(SPPenContext *const pc,bool shift);
static void spiroStartAnchorOn(SPPenContext *const pc);
static void spiroStartAnchorOff(SPPenContext *const pc);
static void spiroMotion(SPPenContext *const pc,bool shift);
static void spiroEndAnchorOn(SPPenContext *const pc);
static void spiroEndAnchorOff(SPPenContext *const pc);
//Unimos todas las curvas en juego y llamamos a la función doEffect.
static void spiro_build(SPPenContext *const pc);
//function spiro cloned from lpe-spiro.cpp
static void spiro_doEffect(SPCurve * curve);
//Preparamos la curva roja para que se muestre según esté pulsada la tecla SHIFT
static void bspline(SPPenContext *const pc,bool shift);
static void bsplineOn(SPPenContext *const pc);
static void bsplineOff(SPPenContext *const pc);
static void bsplineStartAnchor(SPPenContext *const pc,bool shift);
static void bsplineStartAnchorOn(SPPenContext *const pc);
static void bsplineStartAnchorOff(SPPenContext *const pc);
static void bsplineMotion(SPPenContext *const pc,bool shift);
static void bsplineEndAnchorOn(SPPenContext *const pc);
static void bsplineEndAnchorOff(SPPenContext *const pc);
//Unimos todas las curvas en juego y llamamos a la función doEffect.
static void bspline_build(SPPenContext *const pc);
//function bspline cloned from lpe-bspline.cpp
static void bspline_doEffect(SPCurve * curve);
//BSpline end


static void spdc_pen_set_subsequent_point(SPPenContext *const pc, Geom::Point const p, bool statusbar, guint status = 0);
static void spdc_pen_set_ctrl(SPPenContext *pc, Geom::Point const p, guint state);
static void spdc_pen_finish_segment(SPPenContext *pc, Geom::Point p, guint state);

static void spdc_pen_finish(SPPenContext *pc, gboolean closed);

static gint pen_handle_button_press(SPPenContext *const pc, GdkEventButton const &bevent);
static gint pen_handle_motion_notify(SPPenContext *const pc, GdkEventMotion const &mevent);
static gint pen_handle_button_release(SPPenContext *const pc, GdkEventButton const &revent);
static gint pen_handle_2button_press(SPPenContext *const pc, GdkEventButton const &bevent);
static gint pen_handle_key_press(SPPenContext *const pc, GdkEvent *event);
static void spdc_reset_colors(SPPenContext *pc);

static void pen_disable_events(SPPenContext *const pc);
static void pen_enable_events(SPPenContext *const pc);

static Geom::Point pen_drag_origin_w(0, 0);
static bool pen_within_tolerance = false;

static int pen_next_paraxial_direction(const SPPenContext *const pc, Geom::Point const &pt, Geom::Point const &origin, guint state);
static void pen_set_to_nearest_horiz_vert(const SPPenContext *const pc, Geom::Point &pt, guint const state, bool snap);

static int pen_last_paraxial_dir = 0; // last used direction in horizontal/vertical mode; 0 = horizontal, 1 = vertical

G_DEFINE_TYPE(SPPenContext, sp_pen_context, SP_TYPE_DRAW_CONTEXT);

/**
 * Initialize the SPPenContext vtable.
 */
static void sp_pen_context_class_init(SPPenContextClass *klass)
{
    GObjectClass *object_class;
    SPEventContextClass *event_context_class;

    object_class = (GObjectClass *) klass;
    event_context_class = (SPEventContextClass *) klass;

    object_class->dispose = sp_pen_context_dispose;

    event_context_class->setup = sp_pen_context_setup;
    event_context_class->finish = sp_pen_context_finish;
    event_context_class->set = sp_pen_context_set;
    event_context_class->root_handler = sp_pen_context_root_handler;
    event_context_class->item_handler = sp_pen_context_item_handler;
}

/**
 * Callback to initialize SPPenContext object.
 */
static void sp_pen_context_init(SPPenContext *pc)
{
    SPEventContext *event_context = SP_EVENT_CONTEXT(pc);

    event_context->cursor_shape = cursor_pen_xpm;
    event_context->hot_x = 4;
    event_context->hot_y = 4;

    pc->npoints = 0;
    pc->mode = SP_PEN_CONTEXT_MODE_CLICK;
    pc->state = SP_PEN_CONTEXT_POINT;

    pc->c0 = NULL;
    pc->c1 = NULL;
    pc->cl0 = NULL;
    pc->cl1 = NULL;

    pc->events_disabled = 0;

    pc->num_clicks = 0;
    pc->waiting_LPE = NULL;
    pc->waiting_item = NULL;
}

/**
 * Callback to destroy the SPPenContext object's members and itself.
 */
static void sp_pen_context_dispose(GObject *object)
{
    SPPenContext *pc = SP_PEN_CONTEXT(object);

    if (pc->c0) {
        sp_canvas_item_destroy(pc->c0);
        pc->c0 = NULL;
    }
    if (pc->c1) {
        sp_canvas_item_destroy(pc->c1);
        pc->c1 = NULL;
    }
    if (pc->cl0) {
        sp_canvas_item_destroy(pc->cl0);
        pc->cl0 = NULL;
    }
    if (pc->cl1) {
        sp_canvas_item_destroy(pc->cl1);
        pc->cl1 = NULL;
    }

    G_OBJECT_CLASS(sp_pen_context_parent_class)->dispose(object);

    if (pc->expecting_clicks_for_LPE > 0) {
        // we received too few clicks to sanely set the parameter path so we remove the LPE from the item
        sp_lpe_item_remove_current_path_effect(pc->waiting_item, false);
    }
}

void sp_pen_context_set_polyline_mode(SPPenContext *const pc) {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    guint mode = prefs->getInt("/tools/freehand/pen/freehand-mode", 0);
    pc->polylines_only = (mode == 3 || mode == 4);
    pc->polylines_paraxial = (mode == 4);
    //BSpline
    //we call the function which defines the Spiro modes and the B-spline in the future
    //todo: merge to one function only
    sp_pen_context_set_mode(pc, mode);
    //BSpline End
}

//BSpline
/*
*.Set the mode of draw now spiro, and later b-splines
*/
void sp_pen_context_set_mode(SPPenContext *const pc, guint mode) {
    pc->spiro = (mode == 1);
    pc->bspline = (mode == 2);
}
//BSpline End

/**
 * Callback to initialize SPPenContext object.
 */
static void sp_pen_context_setup(SPEventContext *ec)
{
    SPPenContext *pc = SP_PEN_CONTEXT(ec);

    if (((SPEventContextClass *) sp_pen_context_parent_class)->setup) {
        ((SPEventContextClass *) sp_pen_context_parent_class)->setup(ec);
    }

    ControlManager &mgr = ControlManager::getManager();

    // Pen indicators
    pc->c0 = mgr.createControl(sp_desktop_controls(SP_EVENT_CONTEXT_DESKTOP(ec)), Inkscape::CTRL_TYPE_ADJ_HANDLE);
    mgr.track(pc->c0);

    pc->c1 = mgr.createControl(sp_desktop_controls(SP_EVENT_CONTEXT_DESKTOP(ec)), Inkscape::CTRL_TYPE_ADJ_HANDLE);
    mgr.track(pc->c1);

    pc->cl0 = mgr.createControlLine(sp_desktop_controls(SP_EVENT_CONTEXT_DESKTOP(ec)));
    pc->cl1 = mgr.createControlLine(sp_desktop_controls(SP_EVENT_CONTEXT_DESKTOP(ec)));


    sp_canvas_item_hide(pc->c0);
    sp_canvas_item_hide(pc->c1);
    sp_canvas_item_hide(pc->cl0);
    sp_canvas_item_hide(pc->cl1);

    sp_event_context_read(ec, "mode");

    pc->anchor_statusbar = false;

    sp_pen_context_set_polyline_mode(pc);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/tools/freehand/pen/selcue")) {
        ec->enableSelectionCue();
    }
}

static void pen_cancel (SPPenContext *const pc)
{
    pc->num_clicks = 0;
    pc->state = SP_PEN_CONTEXT_STOP;
    spdc_reset_colors(pc);
    sp_canvas_item_hide(pc->c0);
    sp_canvas_item_hide(pc->c1);
    sp_canvas_item_hide(pc->cl0);
    sp_canvas_item_hide(pc->cl1);
    pc->_message_context->clear();
    pc->_message_context->flash(Inkscape::NORMAL_MESSAGE, _("Drawing cancelled"));

    pc->desktop->canvas->endForcedFullRedraws();
}

/**
 * Finalization callback.
 */
static void sp_pen_context_finish(SPEventContext *ec)
{
    SPPenContext *pc = SP_PEN_CONTEXT(ec);

    sp_event_context_discard_delayed_snap_event(ec);

    if (pc->npoints != 0) {
        pen_cancel (pc);
    }

    if (((SPEventContextClass *) sp_pen_context_parent_class)->finish) {
        ((SPEventContextClass *) sp_pen_context_parent_class)->finish(ec);
    }
}

/**
 * Callback that sets key to value in pen context.
 */
static void sp_pen_context_set(SPEventContext *ec, Inkscape::Preferences::Entry *val)
{
    SPPenContext *pc = SP_PEN_CONTEXT(ec);
    Glib::ustring name = val->getEntryName();

    if (name == "mode") {
        if ( val->getString() == "drag" ) {
            pc->mode = SP_PEN_CONTEXT_MODE_DRAG;
        } else {
            pc->mode = SP_PEN_CONTEXT_MODE_CLICK;
        }
    }
}

/**
 * Snaps new node relative to the previous node.
 */
static void spdc_endpoint_snap(SPPenContext const *const pc, Geom::Point &p, guint const state)
{
    if ((state & GDK_CONTROL_MASK) && !pc->polylines_paraxial) { //CTRL enables angular snapping
        if (pc->npoints > 0) {
            spdc_endpoint_snap_rotation(pc, p, pc->p[0], state);
        }
    } else {
        // We cannot use shift here to disable snapping because the shift-key is already used
        // to toggle the paraxial direction; if the user wants to disable snapping (s)he will
        // have to use the %-key, the menu, or the snap toolbar
        if ((pc->npoints > 0) && pc->polylines_paraxial) {
            // snap constrained
            pen_set_to_nearest_horiz_vert(pc, p, state, true);
        } else {
            // snap freely
            boost::optional<Geom::Point> origin = pc->npoints > 0 ? pc->p[0] : boost::optional<Geom::Point>();
            spdc_endpoint_snap_free(pc, p, origin, state); // pass the origin, to allow for perpendicular / tangential snapping
        }
    }
}

/**
 * Snaps new node's handle relative to the new node.
 */
static void spdc_endpoint_snap_handle(SPPenContext const *const pc, Geom::Point &p, guint const state)
{
    g_return_if_fail(( pc->npoints == 2 ||
                       pc->npoints == 5   ));

    if ((state & GDK_CONTROL_MASK)) { //CTRL enables angular snapping
        spdc_endpoint_snap_rotation(pc, p, pc->p[pc->npoints - 2], state);
    } else {
        if (!(state & GDK_SHIFT_MASK)) { //SHIFT disables all snapping, except the angular snapping above
            boost::optional<Geom::Point> origin = pc->p[pc->npoints - 2];
            spdc_endpoint_snap_free(pc, p, origin, state);
        }
    }
}

static gint sp_pen_context_item_handler(SPEventContext *ec, SPItem *item, GdkEvent *event)
{
    SPPenContext *const pc = SP_PEN_CONTEXT(ec);

    gint ret = FALSE;

    switch (event->type) {
        case GDK_BUTTON_PRESS:
            ret = pen_handle_button_press(pc, event->button);
            break;
        case GDK_BUTTON_RELEASE:
            ret = pen_handle_button_release(pc, event->button);
            break;
        default:
            break;
    }

    if (!ret) {
        if (((SPEventContextClass *) sp_pen_context_parent_class)->item_handler)
            ret = ((SPEventContextClass *) sp_pen_context_parent_class)->item_handler(ec, item, event);
    }

    return ret;
}

/**
 * Callback to handle all pen events.
 */
static gint sp_pen_context_root_handler(SPEventContext *ec, GdkEvent *event)
{
    SPPenContext *const pc = SP_PEN_CONTEXT(ec);

    gint ret = FALSE;

    switch (event->type) {
        case GDK_BUTTON_PRESS:
            ret = pen_handle_button_press(pc, event->button);
            break;

        case GDK_MOTION_NOTIFY:
            ret = pen_handle_motion_notify(pc, event->motion);
            break;

        case GDK_BUTTON_RELEASE:
            ret = pen_handle_button_release(pc, event->button);
            break;

        case GDK_2BUTTON_PRESS:
            ret = pen_handle_2button_press(pc, event->button);
            break;

        case GDK_KEY_PRESS:
            ret = pen_handle_key_press(pc, event);
            break;

        default:
            break;
    }

    if (!ret) {
        gint (*const parent_root_handler)(SPEventContext *, GdkEvent *)
            = ((SPEventContextClass *) sp_pen_context_parent_class)->root_handler;
        if (parent_root_handler) {
            ret = parent_root_handler(ec, event);
        }
    }

    return ret;
}

/**
 * Handle mouse button press event.
 */
static gint pen_handle_button_press(SPPenContext *const pc, GdkEventButton const &bevent)
{
    if (pc->events_disabled) {
        // skip event processing if events are disabled
        return FALSE;
    }

    SPDrawContext * const dc = SP_DRAW_CONTEXT(pc);
    SPDesktop * const desktop = SP_EVENT_CONTEXT_DESKTOP(dc);
    Geom::Point const event_w(bevent.x, bevent.y);
    Geom::Point event_dt(desktop->w2d(event_w));
    SPEventContext *event_context = SP_EVENT_CONTEXT(pc);
    //Test whether we hit any anchor.
    //Anchor changed from Const because need to update pc->ea for BSpline.
    SPDrawAnchor * anchor = spdc_test_inside(pc, event_w);
    //BSpline
    //with this we avoid creating a new point over the existing one
    if((pc->spiro || pc->bspline) && pc->npoints > 0 && pc->p[0] == pc->p[3]){
        return FALSE;
    } 
    //BSpline end
    gint ret = FALSE;
    if (bevent.button == 1 && !event_context->space_panning
        // make sure this is not the last click for a waiting LPE (otherwise we want to finish the path)
        && (pc->expecting_clicks_for_LPE != 1)) {

        if (Inkscape::have_viable_layer(desktop, dc->_message_context) == false) {
            return TRUE;
        }

        if (!pc->grab ) {
            // Grab mouse, so release will not pass unnoticed
            pc->grab = SP_CANVAS_ITEM(desktop->acetate);
            sp_canvas_item_grab(pc->grab, ( GDK_KEY_PRESS_MASK | GDK_BUTTON_PRESS_MASK   |
                                            GDK_BUTTON_RELEASE_MASK |
                                            GDK_POINTER_MOTION_MASK  ),
                                NULL, bevent.time);
        }

        pen_drag_origin_w = event_w;
        pen_within_tolerance = true;

        switch (pc->mode) {
            case SP_PEN_CONTEXT_MODE_CLICK:
                // In click mode we add point on release
                switch (pc->state) {
                    case SP_PEN_CONTEXT_POINT:
                    case SP_PEN_CONTEXT_CONTROL:
                    case SP_PEN_CONTEXT_CLOSE:
                        break;
                    case SP_PEN_CONTEXT_STOP:
                        // This is allowed, if we just canceled curve
                        pc->state = SP_PEN_CONTEXT_POINT;
                        break;
                    default:
                        break;
                }
                break;
            case SP_PEN_CONTEXT_MODE_DRAG:
                switch (pc->state) {
                    case SP_PEN_CONTEXT_STOP:
                        // This is allowed, if we just canceled curve
                    case SP_PEN_CONTEXT_POINT:
                        if (pc->npoints == 0 ) {

                            Geom::Point p;
                            if ((bevent.state & GDK_CONTROL_MASK) && (pc->polylines_only || pc->polylines_paraxial)) {
                                p = event_dt;
                                if (!(bevent.state & GDK_SHIFT_MASK)) {
                                    SnapManager &m = desktop->namedview->snap_manager;
                                    m.setup(desktop);
                                    m.freeSnapReturnByRef(p, Inkscape::SNAPSOURCE_NODE_HANDLE);
                                    m.unSetup();
                                }
                              spdc_create_single_dot(event_context, p, "/tools/freehand/pen", bevent.state);
                              ret = TRUE;
                              break;
                            }

                            // TODO: Perhaps it would be nicer to rearrange the following case
                            // distinction so that the case of a waiting LPE is treated separately

                            // Set start anchor
                            pc->sa = anchor;
                            //BSpline
                            if(anchor){
                                if(pc->spiro){
                                    spiroStartAnchor(pc,(bevent.state & GDK_SHIFT_MASK));
                                }
                                if(pc->bspline){
                                    bsplineStartAnchor(pc,(bevent.state & GDK_SHIFT_MASK));
                                }
                            }
                            //BSpline End
                            if (anchor && !sp_pen_context_has_waiting_LPE(pc)) {
                                // Adjust point to anchor if needed; if we have a waiting LPE, we need
                                // a fresh path to be created so don't continue an existing one
                                p = anchor->dp;
                                desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Continuing selected path"));
                            } else {
                                // This is the first click of a new curve; deselect item so that
                                // this curve is not combined with it (unless it is drawn from its
                                // anchor, which is handled by the sibling branch above)
                                Inkscape::Selection * const selection = sp_desktop_selection(desktop);
                                if (!(bevent.state & GDK_SHIFT_MASK) || sp_pen_context_has_waiting_LPE(pc)) {
                                    // if we have a waiting LPE, we need a fresh path to be created
                                    // so don't append to an existing one
                                    selection->clear();
                                    desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Creating new path"));
                                } else if (selection->singleItem() && SP_IS_PATH(selection->singleItem())) {
                                    desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Appending to selected path"));
                                }

                                // Create green anchor
                                p = event_dt;
                                spdc_endpoint_snap(pc, p, bevent.state);
                                pc->green_anchor = sp_draw_anchor_new(pc, pc->green_curve, TRUE, p);
                            }
                            spdc_pen_set_initial_point(pc, p);
                        } else {

                            // Set end anchor
                            pc->ea = anchor;
                            Geom::Point p;
                            if (anchor) {
                                p = anchor->dp;
                                // we hit an anchor, will finish the curve (either with or without closing)
                                // in release handler
                                pc->state = SP_PEN_CONTEXT_CLOSE;

                                if (pc->green_anchor && pc->green_anchor->active) {
                                    // we clicked on the current curve start, so close it even if
                                    // we drag a handle away from it
                                    dc->green_closed = TRUE;
                                }
                                ret = TRUE;
                                break;

                            } else {
                                p = event_dt;
                                spdc_endpoint_snap(pc, p, bevent.state); // Snap node only if not hitting anchor.
                                spdc_pen_set_subsequent_point(pc, p, true);
                            }
                        }
                        //BSpline
                        //Esto evita arrastrar los manejadores ya que el punto se crea
                        //al soltar el botón del ratón.
                        pc->state = (pc->spiro || pc->bspline || pc->polylines_only) ? SP_PEN_CONTEXT_POINT : SP_PEN_CONTEXT_CONTROL;
                        //BSpline End
                        ret = TRUE;
                        break;
                    case SP_PEN_CONTEXT_CONTROL:
                        g_warning("Button down in CONTROL state");
                        break;
                    case SP_PEN_CONTEXT_CLOSE:
                        g_warning("Button down in CLOSE state");
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    } else if (pc->expecting_clicks_for_LPE == 1 && pc->npoints != 0) {
        // when the last click for a waiting LPE occurs we want to finish the path
        spdc_pen_finish_segment(pc, event_dt, bevent.state);
        if (pc->green_closed) {
            // finishing at the start anchor, close curve
            spdc_pen_finish(pc, TRUE);
        } else {
            // finishing at some other anchor, finish curve but not close
            spdc_pen_finish(pc, FALSE);
        }

        ret = TRUE;
    } else if (bevent.button == 3 && pc->npoints != 0) {
        // right click - finish path
        spdc_pen_finish(pc, FALSE);
        ret = TRUE;
    }

    if (pc->expecting_clicks_for_LPE > 0) {
        --pc->expecting_clicks_for_LPE;
    }
    return ret;
}

/**
 * Handle motion_notify event.
 */
static gint pen_handle_motion_notify(SPPenContext *const pc, GdkEventMotion const &mevent)
{
    gint ret = FALSE;

    SPEventContext *event_context = SP_EVENT_CONTEXT(pc);
    SPDesktop * const dt = SP_EVENT_CONTEXT_DESKTOP(event_context);

    if (event_context->space_panning || mevent.state & GDK_BUTTON2_MASK || mevent.state & GDK_BUTTON3_MASK) {
        // allow scrolling
        return FALSE;
    }

    if (pc->events_disabled) {
        // skip motion events if pen events are disabled
        return FALSE;
    }

    Geom::Point const event_w(mevent.x,
                              mevent.y);
    //BSpline
    //we take out the function the const "tolerance" because we need it later
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gint const tolerance = prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100);
    //"spiro_color" lo ejecutamos siempre sea o no spiro
    spiro_color(pc);
    if (pen_within_tolerance) {
        if ( Geom::LInfty( event_w - pen_drag_origin_w ) < tolerance) {
            return FALSE;   // Do not drag if we're within tolerance from origin.
        }
    }
    //BSpline END
    // Once the user has moved farther than tolerance from the original location
    // (indicating they intend to move the object, not click), then always process the
    // motion notify coordinates as given (no snapping back to origin)
    pen_within_tolerance = false;

    // Find desktop coordinates
    Geom::Point p = dt->w2d(event_w);
    // Test, whether we hit any anchor
    SPDrawAnchor *anchor = spdc_test_inside(pc, event_w);
    switch (pc->mode) {
        case SP_PEN_CONTEXT_MODE_CLICK:
            switch (pc->state) {
                case SP_PEN_CONTEXT_POINT:
                    if ( pc->npoints != 0 ) {
                        // Only set point, if we are already appending
                        spdc_endpoint_snap(pc, p, mevent.state);
                        spdc_pen_set_subsequent_point(pc, p, true);
                        ret = TRUE;
                    } else if (!sp_event_context_knot_mouseover(pc)) {
                        SnapManager &m = dt->namedview->snap_manager;
                        m.setup(dt);
                        m.preSnap(Inkscape::SnapCandidatePoint(p, Inkscape::SNAPSOURCE_NODE_HANDLE));
                        m.unSetup();
                    }
                    break;
                case SP_PEN_CONTEXT_CONTROL:
                case SP_PEN_CONTEXT_CLOSE:
                    // Placing controls is last operation in CLOSE state
                    spdc_endpoint_snap(pc, p, mevent.state);
                    spdc_pen_set_ctrl(pc, p, mevent.state);
                    ret = TRUE;
                    break;
                case SP_PEN_CONTEXT_STOP:
                    // This is perfectly valid
                    break;
                default:
                    break;
            }
            break;
        case SP_PEN_CONTEXT_MODE_DRAG:
            switch (pc->state) {
                case SP_PEN_CONTEXT_POINT:
                    if ( pc->npoints > 0 ) {
                        // Only set point, if we are already appending

                        if (!anchor) {   // Snap node only if not hitting anchor
                            spdc_endpoint_snap(pc, p, mevent.state);
                            spdc_pen_set_subsequent_point(pc, p, true, mevent.state);
                        } else {
                            spdc_pen_set_subsequent_point(pc, anchor->dp, false, mevent.state);
                        }

                        if (anchor && !pc->anchor_statusbar) {
                            if(!pc->spiro && !pc->bspline){
                                pc->_message_context->set(Inkscape::NORMAL_MESSAGE, _("<b>Click</b> or <b>click and drag</b> to close and finish the path."));
                            }else{
                                pc->_message_context->set(Inkscape::NORMAL_MESSAGE, _("<b>Click</b> or <b>click and drag</b> to close and finish the path. Shift to cusp node"));      
                            }
                            pc->anchor_statusbar = true;
                        } else if (!anchor && pc->anchor_statusbar) {
                            pc->_message_context->clear();
                            pc->anchor_statusbar = false;
                        }

                        ret = TRUE;
                    } else {
                        if (anchor && !pc->anchor_statusbar) {
                            if(!pc->spiro && !pc->bspline){
                                pc->_message_context->set(Inkscape::NORMAL_MESSAGE, _("<b>Click</b> or <b>click and drag</b> to continue the path from this point."));
                            }else{
                                pc->_message_context->set(Inkscape::NORMAL_MESSAGE, _("<b>Click</b> or <b>click and drag</b> to continue the path from this point. Shift to cusp node"));      
                            }
                            pc->anchor_statusbar = true;
                        } else if (!anchor && pc->anchor_statusbar) {
                            pc->_message_context->clear();
                            pc->anchor_statusbar = false;
                        }
                        if (!sp_event_context_knot_mouseover(pc)) {
                            SnapManager &m = dt->namedview->snap_manager;
                            m.setup(dt);
                            m.preSnap(Inkscape::SnapCandidatePoint(p, Inkscape::SNAPSOURCE_NODE_HANDLE));
                            m.unSetup();
                        }
                    }
                    break;
                case SP_PEN_CONTEXT_CONTROL:
                case SP_PEN_CONTEXT_CLOSE:
                    // Placing controls is last operation in CLOSE state
                    // snap the handle
                    spdc_endpoint_snap_handle(pc, p, mevent.state);
                    if (!pc->polylines_only) {
                        spdc_pen_set_ctrl(pc, p, mevent.state);
                    } else {
                        spdc_pen_set_ctrl(pc, pc->p[1], mevent.state);
                    }

                    gobble_motion_events(GDK_BUTTON1_MASK);
                    ret = TRUE;
                    break;
                case SP_PEN_CONTEXT_STOP:
                    // This is perfectly valid
                    break;
                default:
                    if (!sp_event_context_knot_mouseover(pc)) {
                        SnapManager &m = dt->namedview->snap_manager;
                        m.setup(dt);
                        m.preSnap(Inkscape::SnapCandidatePoint(p, Inkscape::SNAPSOURCE_NODE_HANDLE));
                        m.unSetup();
                    }
                    break;
            }
            break;
        default:
            break;
    }
    //BSpline
    if ( Geom::LInfty( event_w - pen_drag_origin_w ) > tolerance || mevent.time == 0) {
        if(pc->spiro){
            spiroMotion(pc,(mevent.state & GDK_SHIFT_MASK));
        }
        if(pc->bspline){
            bsplineMotion(pc,(mevent.state & GDK_SHIFT_MASK));
        }
        pen_drag_origin_w = event_w;
    }
    //BSpline End
    return ret;
}

/**
 * Handle mouse button release event.
 */
static gint pen_handle_button_release(SPPenContext *const pc, GdkEventButton const &revent)
{
    if (pc->events_disabled) {
        // skip event processing if events are disabled
        return FALSE;
    }
    
    gint ret = FALSE;
    SPEventContext *event_context = SP_EVENT_CONTEXT(pc);
    if ( revent.button == 1  && !event_context->space_panning ) {
        SPDrawContext *dc = SP_DRAW_CONTEXT (pc);

        Geom::Point const event_w(revent.x,
                                revent.y);
        // Find desktop coordinates
        Geom::Point p = pc->desktop->w2d(event_w);

        // Test whether we hit any anchor.
        SPDrawAnchor *anchor = spdc_test_inside(pc, event_w);
        //BSpline
        if(pc->spiro || pc->bspline){
            //Si intentamos crear un nodo en el mismo sitio que el origen, paramos.
            if(pc->npoints > 0 && pc->p[0] == pc->p[3]){
                return FALSE;
            }
        }
        //BSpline End
        switch (pc->mode) {
            case SP_PEN_CONTEXT_MODE_CLICK:
                switch (pc->state) {
                    case SP_PEN_CONTEXT_POINT:
                        if ( pc->npoints == 0 ) {
                            // Start new thread only with button release
                            if (anchor) {
                                p = anchor->dp;
                            }
                            pc->sa = anchor;
                            //BSpline
                            if (anchor) {
                                if(pc->spiro){
                                    spiroStartAnchor(pc,(revent.state & GDK_SHIFT_MASK));
                                }
                                if(pc->bspline){
                                    bsplineStartAnchor(pc,(revent.state & GDK_SHIFT_MASK));
                                }
                            }
                            //BSpline End
                            spdc_pen_set_initial_point(pc, p);
                        } else {
                            // Set end anchor here
                            pc->ea = anchor;
                            if (anchor) {
                                p = anchor->dp;
                            }
                        }
                        pc->state = SP_PEN_CONTEXT_CONTROL;
                        ret = TRUE;
                        break;
                    case SP_PEN_CONTEXT_CONTROL:
                        // End current segment
                        spdc_endpoint_snap(pc, p, revent.state);
                        spdc_pen_finish_segment(pc, p, revent.state);
                        pc->state = SP_PEN_CONTEXT_POINT;
                        ret = TRUE;
                        break;
                    case SP_PEN_CONTEXT_CLOSE:
                        // End current segment
                        if (!anchor) {   // Snap node only if not hitting anchor
                            spdc_endpoint_snap(pc, p, revent.state);
                        }
                        spdc_pen_finish_segment(pc, p, revent.state);
                        //BSpline
                        //Ocultamos la guia del penultimo nodo al cerrar la curva
                        if(pc->spiro){
                            sp_canvas_item_hide(pc->c1);
                        }
                        //BSpline End
                        spdc_pen_finish(pc, TRUE);
                        pc->state = SP_PEN_CONTEXT_POINT;
                        ret = TRUE;
                        break;
                    case SP_PEN_CONTEXT_STOP:
                        // This is allowed, if we just canceled curve
                        pc->state = SP_PEN_CONTEXT_POINT;
                        ret = TRUE;
                        break;
                    default:
                        break;
                }
                break;
            case SP_PEN_CONTEXT_MODE_DRAG:
                switch (pc->state) {
                    case SP_PEN_CONTEXT_POINT:
                    case SP_PEN_CONTEXT_CONTROL:
                        spdc_endpoint_snap(pc, p, revent.state);
                        spdc_pen_finish_segment(pc, p, revent.state);
                        break;
                    case SP_PEN_CONTEXT_CLOSE:
                        // End current segment
                        if (!anchor) {   // Snap node only if not hitting anchor
                            spdc_endpoint_snap(pc, p, revent.state);
                        }
                        spdc_pen_finish_segment(pc, p, revent.state);
                        //BSpline
                        //Ocultamos la guia del penultimo nodo al cerrar la curva
                        if(pc->spiro){
                            sp_canvas_item_hide(pc->c1);
                        }
                        //BSpline End
                        if (pc->green_closed) {
                            // finishing at the start anchor, close curve
                            spdc_pen_finish(pc, TRUE);
                        } else {
                            // finishing at some other anchor, finish curve but not close
                            spdc_pen_finish(pc, FALSE);
                        }
                        break;
                    case SP_PEN_CONTEXT_STOP:
                        // This is allowed, if we just cancelled curve
                        break;
                    default:    
                        break;
                }
                pc->state = SP_PEN_CONTEXT_POINT;
                ret = TRUE;
                break;
            default:
                break;
        }
        if (pc->grab) {
            // Release grab now
            sp_canvas_item_ungrab(pc->grab, revent.time);
            pc->grab = NULL;
        }

        ret = TRUE;

        dc->green_closed = FALSE;
    }

    // TODO: can we be sure that the path was created correctly?
    // TODO: should we offer an option to collect the clicks in a list?
    if (pc->expecting_clicks_for_LPE == 0 && sp_pen_context_has_waiting_LPE(pc)) {
        sp_pen_context_set_polyline_mode(pc);

        SPEventContext *ec = SP_EVENT_CONTEXT(pc);
        Inkscape::Selection *selection = sp_desktop_selection (ec->desktop);

        if (pc->waiting_LPE) {
            // we have an already created LPE waiting for a path
            pc->waiting_LPE->acceptParamPath(SP_PATH(selection->singleItem()));
            selection->add(SP_OBJECT(pc->waiting_item));
            pc->waiting_LPE = NULL;
        } else {
            // the case that we need to create a new LPE and apply it to the just-drawn path is
            // handled in spdc_check_for_and_apply_waiting_LPE() in draw-context.cpp
        }
    }
    return ret;
}

static gint pen_handle_2button_press(SPPenContext *const pc, GdkEventButton const &bevent)
{
    gint ret = FALSE;
    // only end on LMB double click. Otherwise horizontal scrolling causes ending of the path
    if (pc->npoints != 0 && bevent.button == 1) {
        spdc_pen_finish(pc, FALSE);
        ret = TRUE;
    }
    return ret;
}

static void pen_redraw_all (SPPenContext *const pc)
{
    // green
    if (pc->green_bpaths) {
        // remove old piecewise green canvasitems
        while (pc->green_bpaths) {
            sp_canvas_item_destroy(SP_CANVAS_ITEM(pc->green_bpaths->data));
            pc->green_bpaths = g_slist_remove(pc->green_bpaths, pc->green_bpaths->data);
        }
        // one canvas bpath for all of green_curve
        SPCanvasItem *cshape = sp_canvas_bpath_new(sp_desktop_sketch(pc->desktop), pc->green_curve);
        sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(cshape), pc->green_color, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
        sp_canvas_bpath_set_fill(SP_CANVAS_BPATH(cshape), 0, SP_WIND_RULE_NONZERO);
        pc->green_bpaths = g_slist_prepend(pc->green_bpaths, cshape);
    }
    if (pc->green_anchor)
        SP_CTRL(pc->green_anchor->ctrl)->moveto(pc->green_anchor->dp);
    pc->red_curve->reset();
    pc->red_curve->moveto(pc->p[0]);
    pc->red_curve->curveto(pc->p[1], pc->p[2], pc->p[3]);
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->red_bpath), pc->red_curve);

    // handles
    //BSpline
    if (pc->p[0] != pc->p[1] && !pc->spiro && !pc->bspline) {
    //BSpline End
        SP_CTRL(pc->c1)->moveto(pc->p[1]);
        pc->cl1->setCoords(pc->p[0], pc->p[1]);
        sp_canvas_item_show(pc->c1);
        sp_canvas_item_show(pc->cl1);
    } else {
        sp_canvas_item_hide(pc->c1);
        sp_canvas_item_hide(pc->cl1);
    }

    Geom::Curve const * last_seg = pc->green_curve->last_segment();
    if (last_seg) {
        Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const *>( last_seg );
        //BSpline
        if ( cubic &&
             (*cubic)[2] != pc->p[0] && !pc->spiro && !pc->bspline )
        {
        //BSpline End
            Geom::Point p2 = (*cubic)[2];
            SP_CTRL(pc->c0)->moveto(p2);
            pc->cl0->setCoords(p2, pc->p[0]);
            sp_canvas_item_show(pc->c0);
            sp_canvas_item_show(pc->cl0);
        } else {
            sp_canvas_item_hide(pc->c0);
            sp_canvas_item_hide(pc->cl0);
        }
    }
    //BSpline
    //Simplemente redibujamos la spiro teniendo en cuenta si el nodo es cusp o symm.
    //como es un redibujo simplemente no llamamos a la función global
    //sino al final de esta
    if(pc->spiro){
        //we redraw spiro
        spiro_build(pc);
    }
    if(pc->bspline){
        //we redraw bspline
        bspline_build(pc);
    }
    //BSpline End
}

static void pen_lastpoint_move (SPPenContext *const pc, gdouble x, gdouble y)
{
    if (pc->npoints != 5)
        return;

    // green
    if (!pc->green_curve->is_empty()) {
        pc->green_curve->last_point_additive_move( Geom::Point(x,y) );
    } else {
        // start anchor too
        if (pc->green_anchor) {
            pc->green_anchor->dp += Geom::Point(x, y);
        }
    }

    // red
    pc->p[0] += Geom::Point(x, y);
    pc->p[1] += Geom::Point(x, y);
    pen_redraw_all(pc);
}

static void pen_lastpoint_move_screen (SPPenContext *const pc, gdouble x, gdouble y)
{
    pen_lastpoint_move (pc, x / pc->desktop->current_zoom(), y / pc->desktop->current_zoom());
}

static void pen_lastpoint_tocurve (SPPenContext *const pc)
{
    //BSpline
    //Evitamos que si la "red_curve" tiene solo dos puntos -recta- no se pare aqui.
    if (pc->npoints != 5 && !pc->spiro && !pc->bspline)
        return;
    //BSpline
    Geom::CubicBezier const * cubic;
    if(!pc->bspline)
        pc->p[1] = pc->p[0];
    //Para formar una curva necesitamos un nodo symm en el "lastpoint"
    //de esta manera modificamos el final de la "curva_verde" para que sea symm con el pricipio de la "red_curve"
    if(pc->spiro){
        Geom::Point A(0,0);
        Geom::Point B(0,0);
        Geom::Point C(0,0);
        Geom::Point D(0,0);
        SPCurve * previous = new SPCurve();
        cubic = dynamic_cast<Geom::CubicBezier const *>( pc->green_curve->last_segment() );
        //We obtain the last segment 4 points in the previous curve 
        if ( cubic ){
            A = (*cubic)[0];
            B = (*cubic)[1];
            C = pc->p[0] + (Geom::Point)(pc->p[0] - pc->p[1]);
            D = (*cubic)[3];
        }else{
            A = pc->green_curve->last_segment()->initialPoint();
            B = pc->green_curve->last_segment()->initialPoint();
            C = pc->p[0] + (Geom::Point)(pc->p[0] - pc->p[1]);
            D = pc->green_curve->last_segment()->finalPoint();
        }
        previous->moveto(A);
        previous->curveto(B, C, D);
        if( pc->green_curve->get_segment_count() == 1){
            pc->green_curve = previous;
        }else{
            //we eliminate the last segment
            pc->green_curve->backspace();
            //and we add it again with the recreation
            pc->green_curve->append_continuous(previous, 0.0625);
        }
    }

//BSpline
    //Para formar una recta necesitamos un nodo con manejador en el "lastpoint"
    //de esta manera modificamos el final de la "curva_verde" para que tenga manejador
    if(pc->bspline && !pc->green_curve->is_empty()){
        Geom::Point A(0,0);
        Geom::Point B(0,0);
        Geom::Point C(0,0);
        Geom::Point D(0,0);
        using Geom::X;
        using Geom::Y;
        SPCurve * previous = new SPCurve();
        //We obtain the last segment 4 points in the previous curve 
        A = pc->green_curve->last_segment()->initialPoint();
        B = pc->green_curve->last_segment()->initialPoint();
        C = pc->green_curve->last_segment()->finalPoint() + (1./3)* (Geom::Point)(pc->green_curve->last_segment()->initialPoint() - pc->green_curve->last_segment()->finalPoint());
        pc->p[1] = pc->p[0] + (1./3)*(C - pc->p[0]);
        D = pc->green_curve->last_segment()->finalPoint();
        previous->moveto(A);
        previous->curveto(B, C, D);
        if( pc->green_curve->get_segment_count() == 1){
            pc->green_curve = previous;
        }else{
            //we eliminate the last segment
            pc->green_curve->backspace();
            //and we add it again with the recreation
            pc->green_curve->append_continuous(previous, 0.0625);
        }
    }
    //Spiro Live
    pen_redraw_all(pc);
}

static void pen_lastpoint_toline (SPPenContext *const pc)
{
    //BSpline
    //Si no es bspline
    if (pc->npoints != 5 && !pc->bspline)
        return;
    Geom::CubicBezier const * cubic;
    if(pc->spiro){
        Geom::Point A(0,0);
        Geom::Point B(0,0);
        Geom::Point C(0,0);
        Geom::Point D(0,0);
        SPCurve * previous = new SPCurve();
        Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const *>( pc->green_curve->last_segment() );
        if ( cubic ) {
            A = pc->green_curve->last_segment()->initialPoint();
            B = (*cubic)[1];
            C = pc->green_curve->last_segment()->finalPoint();
            D = C;
            pc->p[1] = pc->p[0] + (Geom::Point)( (*cubic)[3] - (*cubic)[2] );
        } else {
            //We obtain the last segment 4 points in the previous curve 
            A = pc->green_curve->last_segment()->initialPoint();
            B = A;
            C = pc->green_curve->last_segment()->finalPoint();
            D = C;
        }
        previous->moveto(A);
        previous->curveto(B, C, D);
        if( pc->green_curve->get_segment_count() == 1){
            pc->green_curve = previous;
        }else{
            //we eliminate the last segment
            pc->green_curve->backspace();
            //and we add it again with the recreation
            pc->green_curve->append_continuous(previous, 0.0625);
        }
    }

    if(!pc->bspline){
        cubic = dynamic_cast<Geom::CubicBezier const *>( pc->green_curve->last_segment() );
        if ( cubic ) {
            pc->p[1] = pc->p[0] + (Geom::Point)( pc->p[0] - (*cubic)[2] );
        } else {
            pc->p[1] = pc->p[0] + (1./3)*(pc->p[3] - pc->p[0]);
        }
    }

    //Para formar una curva bspline necesitamos un nodo cusp en el "lastpoint"
    //de esta manera modificamos el final de la "curva_verde" para que sea cusp con el pricipio de la "red_curve"
    //Que se quedará recta
    if(pc->bspline){
        Geom::Point A(0,0);
        Geom::Point B(0,0);
        SPCurve * previous = new SPCurve();
        //We obtain the last segment 2 points in the previous curve 
        A = pc->green_curve->last_segment()->initialPoint();
        B = pc->green_curve->last_segment()->finalPoint();
        previous->moveto(A);
        previous->lineto(B);
        if( pc->green_curve->get_segment_count() == 1){
            pc->green_curve = previous;
        }else{
            //we eliminate the last segment
            pc->green_curve->backspace();
            //and we add it again with the recreation
            pc->green_curve->append_continuous(previous, 0.0625);
        }
        
    }
    pen_redraw_all(pc);
}


static gint pen_handle_key_press(SPPenContext *const pc, GdkEvent *event)
{

    gint ret = FALSE;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gdouble const nudge = prefs->getDoubleLimited("/options/nudgedistance/value", 2, 0, 1000, "px"); // in px

    switch (get_group0_keyval (&event->key)) {

        case GDK_KEY_Left: // move last point left
        case GDK_KEY_KP_Left:
            if (!MOD__CTRL) { // not ctrl
                if (MOD__ALT) { // alt
                    if (MOD__SHIFT) pen_lastpoint_move_screen(pc, -10, 0); // shift
                    else pen_lastpoint_move_screen(pc, -1, 0); // no shift
                }
                else { // no alt
                    if (MOD__SHIFT) pen_lastpoint_move(pc, -10*nudge, 0); // shift
                    else pen_lastpoint_move(pc, -nudge, 0); // no shift
                }
                ret = TRUE;
            }
            break;
        case GDK_KEY_Up: // move last point up
        case GDK_KEY_KP_Up:
            if (!MOD__CTRL) { // not ctrl
                if (MOD__ALT) { // alt
                    if (MOD__SHIFT) pen_lastpoint_move_screen(pc, 0, 10); // shift
                    else pen_lastpoint_move_screen(pc, 0, 1); // no shift
                }
                else { // no alt
                    if (MOD__SHIFT) pen_lastpoint_move(pc, 0, 10*nudge); // shift
                    else pen_lastpoint_move(pc, 0, nudge); // no shift
                }
                ret = TRUE;
            }
            break;
        case GDK_KEY_Right: // move last point right
        case GDK_KEY_KP_Right:
            if (!MOD__CTRL) { // not ctrl
                if (MOD__ALT) { // alt
                    if (MOD__SHIFT) pen_lastpoint_move_screen(pc, 10, 0); // shift
                    else pen_lastpoint_move_screen(pc, 1, 0); // no shift
                }
                else { // no alt
                    if (MOD__SHIFT) pen_lastpoint_move(pc, 10*nudge, 0); // shift
                    else pen_lastpoint_move(pc, nudge, 0); // no shift
                }
                ret = TRUE;
            }
            break;
        case GDK_KEY_Down: // move last point down
        case GDK_KEY_KP_Down:
            if (!MOD__CTRL) { // not ctrl
                if (MOD__ALT) { // alt
                    if (MOD__SHIFT) pen_lastpoint_move_screen(pc, 0, -10); // shift
                    else pen_lastpoint_move_screen(pc, 0, -1); // no shift
                }
                else { // no alt
                    if (MOD__SHIFT) pen_lastpoint_move(pc, 0, -10*nudge); // shift
                    else pen_lastpoint_move(pc, 0, -nudge); // no shift
                }
                ret = TRUE;
            }
            break;

/*TODO: this is not yet enabled?? looks like some traces of the Geometry tool
        case GDK_KEY_P:
        case GDK_KEY_p:
            if (MOD__SHIFT_ONLY) {
                sp_pen_context_wait_for_LPE_mouse_clicks(pc, Inkscape::LivePathEffect::PARALLEL, 2);
                ret = TRUE;
            }
            break;

        case GDK_KEY_C:
        case GDK_KEY_c:
            if (MOD__SHIFT_ONLY) {
                sp_pen_context_wait_for_LPE_mouse_clicks(pc, Inkscape::LivePathEffect::CIRCLE_3PTS, 3);
                ret = TRUE;
            }
            break;

        case GDK_KEY_B:
        case GDK_KEY_b:
            if (MOD__SHIFT_ONLY) {
                sp_pen_context_wait_for_LPE_mouse_clicks(pc, Inkscape::LivePathEffect::PERP_BISECTOR, 2);
                ret = TRUE;
            }
            break;

        case GDK_KEY_A:
        case GDK_KEY_a:
            if (MOD__SHIFT_ONLY) {
                sp_pen_context_wait_for_LPE_mouse_clicks(pc, Inkscape::LivePathEffect::ANGLE_BISECTOR, 3);
                ret = TRUE;
            }
            break;
*/

        case GDK_KEY_U:
        case GDK_KEY_u:
            if (MOD__SHIFT_ONLY) {
                pen_lastpoint_tocurve(pc);
                ret = TRUE;
            }
            break;
        case GDK_KEY_L:
        case GDK_KEY_l:
            if (MOD__SHIFT_ONLY) {
                pen_lastpoint_toline(pc);
                ret = TRUE;
            }
            break;

        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
            if (pc->npoints != 0) {
                spdc_pen_finish(pc, FALSE);
                ret = TRUE;
            }
            break;
        case GDK_KEY_Escape:
            if (pc->npoints != 0) {
                // if drawing, cancel, otherwise pass it up for deselecting
                pen_cancel (pc);
                ret = TRUE;
            }
            break;
        case GDK_KEY_z:
        case GDK_KEY_Z:
            if (MOD__CTRL_ONLY && pc->npoints != 0) {
                // if drawing, cancel, otherwise pass it up for undo
                pen_cancel (pc);
                ret = TRUE;
            }
            break;
        case GDK_KEY_g:
        case GDK_KEY_G:
            if (MOD__SHIFT_ONLY) {
                sp_selection_to_guides(SP_EVENT_CONTEXT(pc)->desktop);
                ret = true;
            }
            break;
        case GDK_KEY_BackSpace:
        case GDK_KEY_Delete:
        case GDK_KEY_KP_Delete:
            if ( pc->green_curve->is_empty() || (pc->green_curve->last_segment() == NULL) ) {
                if (!pc->red_curve->is_empty()) {
                    pen_cancel (pc);
                    ret = TRUE;
                } else {
                    // do nothing; this event should be handled upstream
                }
            } else {
                // Reset red curve
                pc->red_curve->reset();
                // Destroy topmost green bpath
                if (pc->green_bpaths) {
                    if (pc->green_bpaths->data)
                        sp_canvas_item_destroy(SP_CANVAS_ITEM(pc->green_bpaths->data));
                    pc->green_bpaths = g_slist_remove(pc->green_bpaths, pc->green_bpaths->data);
                }
                // Get last segment
                if ( pc->green_curve->is_empty() ) {
                    g_warning("pen_handle_key_press, case GDK_KP_Delete: Green curve is empty");
                    break;
                }
                // The code below assumes that pc->green_curve has only ONE path !
                Geom::Curve const * crv = pc->green_curve->last_segment();
                pc->p[0] = crv->initialPoint();
                if ( Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const *>(crv)) {
                    pc->p[1] = (*cubic)[1];
                    //BSpline
                    if(pc->spiro || pc->bspline)pc->p[1] = pc->p[0] + (1./3)*(pc->p[3] - pc->p[0]);
                    //BSpline End
                } else {
                    pc->p[1] = pc->p[0];
                }

                Geom::Point const pt((pc->npoints < 4
                                     ? (Geom::Point)(crv->finalPoint())
                                     : pc->p[3]));
                
                pc->npoints = 2;
                //BSpline
                if( pc->green_curve->get_segment_count() == 1){
                    pc->npoints = 5;
                    if (pc->green_bpaths) {
                        if (pc->green_bpaths->data)
                            sp_canvas_item_destroy(SP_CANVAS_ITEM(pc->green_bpaths->data));
                        pc->green_bpaths = g_slist_remove(pc->green_bpaths, pc->green_bpaths->data);
                    }
                    pc->green_curve->reset();
                }else{
                    pc->green_curve->backspace();
                }
                //BSpline End
                sp_canvas_item_hide(pc->cl0);
                sp_canvas_item_hide(pc->cl1);
                pc->state = SP_PEN_CONTEXT_POINT;
                spdc_pen_set_subsequent_point(pc, pt, true);
                pen_last_paraxial_dir = !pen_last_paraxial_dir;
                //BSpline
                if(pc->spiro || pc->bspline) bspline_build(pc);
                //BSpline End
                ret = TRUE;
            }
            break;
        default:
            break;
    }
    return ret;
}

static void spdc_reset_colors(SPPenContext *pc)
{
    // Red
    pc->red_curve->reset();
    
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->red_bpath), NULL);
    // Blue
    pc->blue_curve->reset();
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->blue_bpath), NULL);
    // Green
    while (pc->green_bpaths) {
        sp_canvas_item_destroy(SP_CANVAS_ITEM(pc->green_bpaths->data));
        pc->green_bpaths = g_slist_remove(pc->green_bpaths, pc->green_bpaths->data);
    }
    pc->green_curve->reset();
    if (pc->green_anchor) {
        pc->green_anchor = sp_draw_anchor_destroy(pc->green_anchor);
    }
    pc->sa = NULL;
    pc->ea = NULL;
    pc->npoints = 0;
    pc->red_curve_is_valid = false;
}


static void spdc_pen_set_initial_point(SPPenContext *const pc, Geom::Point const p)
{
    g_assert( pc->npoints == 0 );

    pc->p[0] = p;
    pc->p[1] = p;
    pc->npoints = 2;
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->red_bpath), NULL);

    pc->desktop->canvas->forceFullRedrawAfterInterruptions(5);
}

/**
 * Show the status message for the current line/curve segment.
 * This type of message always shows angle/distance as the last
 * two parameters ("angle %3.2f&#176;, distance %s").
 */
static void spdc_pen_set_angle_distance_status_message(SPPenContext *const pc, Geom::Point const p, int pc_point_to_compare, gchar const *message)
{
    g_assert(pc != NULL);
    g_assert((pc_point_to_compare == 0) || (pc_point_to_compare == 3)); // exclude control handles
    g_assert(message != NULL);

    SPDesktop *desktop = SP_EVENT_CONTEXT(pc)->desktop;
    Geom::Point rel = p - pc->p[pc_point_to_compare];
    GString *dist = SP_PX_TO_METRIC_STRING(Geom::L2(rel), desktop->namedview->getDefaultMetric());
    double angle = atan2(rel[Geom::Y], rel[Geom::X]) * 180 / M_PI;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/options/compassangledisplay/value", 0) != 0)
        angle = angle_to_compass (angle);

    pc->_message_context->setF(Inkscape::IMMEDIATE_MESSAGE, message, angle, dist->str);
    g_string_free(dist, FALSE);
}



//Esta función cambia los colores rojo,verde y azul haciendolos transparentes o no en función de si se usa spiro
static void spiro_color(SPPenContext *const pc)
{
    bool remake_green_bpaths = false;
    if(pc->spiro){
        //If the colour is not defined as trasparent, por example when changing
        //from drawing to spiro mode or when selecting the pen tool
        if(pc->green_color != 0x00ff000){
            //We change the green and red colours to transparent, so this lines are not necessary
            //to the drawing with spiro
            pc->red_color = 0xff00000;
            pc->green_color = 0x00ff000;
            remake_green_bpaths = true;
        }
    }else{
        //If we come from working with the spiro curve and change the mode the "green_curve" colour is transparent
        if(pc->green_color != 0x00ff007f){
            //since we are not im spiro mode, we assign the original colours
            //to the red and the green curve, removing their transparency 
            pc->red_color = 0xff00007f;
            pc->green_color = 0x00ff007f;
            remake_green_bpaths = true;
        }
        //we hide the spiro/bspline rests
        if(!pc->bspline){
            sp_canvas_item_hide(pc->blue_bpath);
        }
    }
    //We erase all the "green_bpaths" to recreate them after with the colour
    //transparency recently modified
    if (pc->green_bpaths && remake_green_bpaths) {
        // remove old piecewise green canvasitems
        while (pc->green_bpaths) {
            sp_canvas_item_destroy(SP_CANVAS_ITEM(pc->green_bpaths->data));
            pc->green_bpaths = g_slist_remove(pc->green_bpaths, pc->green_bpaths->data);
        }
        // one canvas bpath for all of green_curve
        SPCanvasItem *cshape = sp_canvas_bpath_new(sp_desktop_sketch(pc->desktop), pc->green_curve);
        sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(cshape), pc->green_color, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
        sp_canvas_bpath_set_fill(SP_CANVAS_BPATH(cshape), 0, SP_WIND_RULE_NONZERO);
        pc->green_bpaths = g_slist_prepend(pc->green_bpaths, cshape);
    }
    sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(pc->red_bpath), pc->red_color, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
}


//Unimos todas las curvas en juego y llamamos a la función doEffect.

static void spiro(SPPenContext *const pc, bool shift)
{
    if(!pc->anchor_statusbar)
        shift?spiroOff(pc):spiroOn(pc);

    spiro_build(pc);
}

static void spiroOn(SPPenContext *const pc){
    if(!pc->red_curve->is_empty()){
        using Geom::X;
        using Geom::Y;
        pc->npoints = 5;
        pc->p[0] = pc->red_curve->first_segment()->initialPoint();
        pc->p[3] = pc->red_curve->first_segment()->finalPoint();
        pc->p[2] = pc->p[3] + (1./3)*(pc->p[0] - pc->p[3]);
        pc->p[2] = Geom::Point(pc->p[2][X] + 0.0625,pc->p[2][Y] + 0.0625);
    }
}

static void spiroOff(SPPenContext *const pc)
{
    if(!pc->red_curve->is_empty()){
        pc->npoints = 5;
        pc->p[0] = pc->red_curve->first_segment()->initialPoint();
        pc->p[3] = pc->red_curve->first_segment()->finalPoint();
        pc->p[2] = pc->p[3];
    }
}

static void spiroStartAnchor(SPPenContext *const pc, bool shift)
{
    if(pc->sa->curve->is_empty())
        return;

    if(shift)
        bsplineStartAnchorOff(pc);
    else
        bsplineStartAnchorOn(pc);
}

static void spiroStartAnchorOn(SPPenContext *const pc)
{
    using Geom::X;
    using Geom::Y;
    SPCurve *tmpCurve = new SPCurve();
    tmpCurve = pc->sa->curve->copy();
    if(pc->sa->start)
        tmpCurve = tmpCurve->create_reverse();
    Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const*>(&*tmpCurve->last_segment());
    SPCurve *lastSeg = new SPCurve();
    Geom::Point A = tmpCurve->last_segment()->initialPoint();
    Geom::Point D = tmpCurve->last_segment()->finalPoint();
    Geom::Point C = D + (1./3)*(A - D);
    C = Geom::Point(C[X] + 0.0625,C[Y] + 0.0625);
    if(cubic){
        lastSeg->moveto(A);
        lastSeg->curveto((*cubic)[1],C,D);
    }else{
        lastSeg->moveto(A);
        lastSeg->curveto(A,C,D);
    }
    if( tmpCurve->get_segment_count() == 1){
        tmpCurve = lastSeg;
    }else{
        //we eliminate the last segment
        tmpCurve->backspace();
        //and we add it again with the recreation
        tmpCurve->append_continuous(lastSeg, 0.0625);
    }
    if (pc->sa->start) {
        tmpCurve = tmpCurve->create_reverse();
    }
    pc->sa->curve->reset();
    pc->sa->curve = tmpCurve;
}

static void spiroStartAnchorOff(SPPenContext *const pc)
{
    SPCurve *tmpCurve = new SPCurve();
    tmpCurve = pc->sa->curve->copy();
    if(pc->sa->start)
        tmpCurve = tmpCurve->create_reverse();
    Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const*>(&*tmpCurve->last_segment());
    if(cubic){
        SPCurve *lastSeg = new SPCurve();
        lastSeg->moveto((*cubic)[0]);
        lastSeg->curveto((*cubic)[1],(*cubic)[3],(*cubic)[3]);
        if( tmpCurve->get_segment_count() == 1){
            tmpCurve = lastSeg;
        }else{
            //we eliminate the last segment
            tmpCurve->backspace();
            //and we add it again with the recreation
            tmpCurve->append_continuous(lastSeg, 0.0625);
        }
        if (pc->sa->start) {
            tmpCurve = tmpCurve->create_reverse();
        }
        pc->sa->curve->reset();
        pc->sa->curve = tmpCurve;
    }

}

static void spiroMotion(SPPenContext *const pc, bool shift){
    using Geom::X;
    using Geom::Y;
    SPCurve *tmpCurve = new SPCurve();
    if(shift)
        pc->p[2] = pc->p[3];
    else
        pc->p[2] = pc->p[3] + (1./3)*(pc->p[0] - pc->p[3]);
        pc->p[2] = Geom::Point(pc->p[2][X] + 0.0625,pc->p[2][Y] + 0.0625);

    if(pc->green_curve->is_empty() && !pc->sa){
        pc->p[1] = pc->p[0] + (1./3)*(pc->p[3] - pc->p[0]);
    }else if(!pc->green_curve->is_empty()){
        tmpCurve = pc->green_curve->copy();
    }else{
        tmpCurve = pc->sa->curve->copy();
        if(pc->sa->start)
            tmpCurve = tmpCurve->create_reverse();
    }
    if(!tmpCurve->is_empty() && !pc->red_curve->is_empty()){
        Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const*>(&*tmpCurve->last_segment());
        if(cubic){
            pc->p[1] =  (*cubic)[3] + (Geom::Point)((*cubic)[3] - (*cubic)[2] );
        }else{
            pc->p[1] = pc->p[0];
        }
    }

    if(pc->anchor_statusbar && !pc->red_curve->is_empty()){
        if(shift)
            bsplineEndAnchorOff(pc);
        else
            bsplineEndAnchorOn(pc);
    }

    spiro_build(pc);
}

static void spiroEndAnchorOn(SPPenContext *const pc)
{
    using Geom::X;
    using Geom::Y;
    pc->p[2] = pc->p[3] + (1./3)*(pc->p[0] - pc->p[3]);
    pc->p[2] = Geom::Point(pc->p[2][X] + 0.0625,pc->p[2][Y] + 0.0625);
    SPCurve *tmpCurve = new SPCurve();
    SPCurve *lastSeg = new SPCurve();
    Geom::Point C(0,0);
    if(!pc->sa || pc->sa->curve->is_empty()){
        tmpCurve = pc->green_curve->create_reverse();
        Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const*>(&*tmpCurve->last_segment());
        C = tmpCurve->last_segment()->finalPoint() + + (Geom::Point)(tmpCurve->last_segment()->finalPoint() - pc->p[2]);
        if(cubic){
            lastSeg->moveto((*cubic)[0]);
            lastSeg->curveto((*cubic)[1],C,(*cubic)[3]);
        }else{
            lastSeg->moveto(tmpCurve->last_segment()->initialPoint());
            lastSeg->curveto(tmpCurve->last_segment()->initialPoint(),C,tmpCurve->last_segment()->finalPoint());
        }
        if( tmpCurve->get_segment_count() == 1){
            tmpCurve = lastSeg;
        }else{
            //we eliminate the last segment
            tmpCurve->backspace();
            //and we add it again with the recreation
            tmpCurve->append_continuous(lastSeg, 0.0625);
        }
        tmpCurve = tmpCurve->create_reverse();
        pc->green_curve->reset();
        pc->green_curve = tmpCurve;
    }else{
        tmpCurve = pc->sa->curve->copy();
        if(!pc->sa->start)
            tmpCurve = tmpCurve->create_reverse();
        Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const*>(&*tmpCurve->last_segment());
        C = tmpCurve->last_segment()->finalPoint() + + (Geom::Point)(tmpCurve->last_segment()->finalPoint() - pc->p[2]);
        if(cubic){
            lastSeg->moveto((*cubic)[0]);
            lastSeg->curveto((*cubic)[1],C,(*cubic)[3]);
        }else{
            lastSeg->moveto(tmpCurve->last_segment()->initialPoint());
            lastSeg->curveto(tmpCurve->last_segment()->initialPoint(),C,tmpCurve->last_segment()->finalPoint());
        }
        if( tmpCurve->get_segment_count() == 1){
            tmpCurve = lastSeg;
        }else{
            //we eliminate the last segment
            tmpCurve->backspace();
            //and we add it again with the recreation
            tmpCurve->append_continuous(lastSeg, 0.0625);
        }
        if (!pc->sa->start) {
            tmpCurve = tmpCurve->create_reverse();
        }
        pc->sa->curve->reset();
        pc->sa->curve = tmpCurve;
    }
}

static void spiroEndAnchorOff(SPPenContext *const pc)
{
    pc->p[2] = pc->p[3];
    SPCurve *tmpCurve = new SPCurve();
    SPCurve *lastSeg = new SPCurve();
    if(!pc->sa || pc->sa->curve->is_empty()){
        tmpCurve = pc->green_curve->create_reverse();
        Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const*>(&*tmpCurve->last_segment());
        if(cubic){
            lastSeg->moveto((*cubic)[0]);
            lastSeg->curveto((*cubic)[1],(*cubic)[3],(*cubic)[3]);
            if( tmpCurve->get_segment_count() == 1){
                tmpCurve = lastSeg;
            }else{
                //we eliminate the last segment
                tmpCurve->backspace();
                //and we add it again with the recreation
                tmpCurve->append_continuous(lastSeg, 0.0625);
            }
            tmpCurve = tmpCurve->create_reverse();
            pc->green_curve->reset();
            pc->green_curve = tmpCurve;
        }
    }else{
        tmpCurve = pc->sa->curve->copy();
        if(!pc->sa->start)
            tmpCurve = tmpCurve->create_reverse();
        Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const*>(&*tmpCurve->last_segment());
        if(cubic){
            lastSeg->moveto((*cubic)[0]);
            lastSeg->curveto((*cubic)[1],(*cubic)[3],(*cubic)[3]);
            if( tmpCurve->get_segment_count() == 1){
                tmpCurve = lastSeg;
            }else{
                //we eliminate the last segment
                tmpCurve->backspace();
                //and we add it again with the recreation
                tmpCurve->append_continuous(lastSeg, 0.0625);
            }
            if (!pc->sa->start) {
                tmpCurve = tmpCurve->create_reverse();
            }
            pc->sa->curve->reset();
            pc->sa->curve = tmpCurve;
        }
    }
}

//Unimos todas las curvas en juego y llamamos a la función doEffect.
static void spiro_build(SPPenContext *const pc)
{
    //We create the base curve
    SPCurve *curve = new SPCurve();
    //If we continuate the existing curve we add it at the start
    if(pc->sa && !pc->sa->curve->is_empty()){
        curve = pc->sa->curve->copy();
        if (pc->sa->start) {
            curve = curve->create_reverse();
        }
    }
    //We add also the green curve
    if (!pc->green_curve->is_empty())
        curve->append_continuous(pc->green_curve, 0.0625);

    //and the red one
    if (!pc->red_curve->is_empty()){
        pc->red_curve->reset();
        pc->red_curve->moveto(pc->p[0]);
        pc->red_curve->curveto(pc->p[1],pc->p[2],pc->p[3]);
        sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->red_bpath), pc->red_curve);
        curve->append_continuous(pc->red_curve, 0.0625);
    }

    if(!curve->is_empty()){
        //cerramos la curva si estan cerca los puntos finales de la curva spiro
        if(Geom::are_near(curve->first_path()->initialPoint(), curve->last_path()->finalPoint())){
            curve->closepath_current();
        }
        //TODO: CALL TO CLONED FUNCTION SPIRO::doEffect IN lpe-spiro.cpp
        //For example
        //using namespace Inkscape::LivePathEffect;
        //LivePathEffectObject *lpeobj = static_cast<LivePathEffectObject*> (curve);
        //Effect *spr = static_cast<Effect*> ( new LPEspiro(lpeobj) );
        //spr->doEffect(curve);
        spiro_doEffect(curve);
        sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->blue_bpath), curve);   
        sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(pc->blue_bpath), pc->blue_color, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
        sp_canvas_item_show(pc->blue_bpath); 
        curve->unref();
        sp_canvas_item_show(pc->c1);
        pc->blue_curve->reset();
        SP_CTRL(pc->c1)->moveto(pc->p[0]);
        //We hide the holders that doesn't contribute anything
        sp_canvas_item_hide(pc->cl1);
        sp_canvas_item_hide(pc->c0);
        sp_canvas_item_hide(pc->cl0);
    }else{
        //if the curve is empty
        sp_canvas_item_hide(pc->blue_bpath);
    }
}
//Spiro function cloned from lpe-spiro.cpp
static void spiro_doEffect(SPCurve * curve)
{
    using Geom::X;
    using Geom::Y;

    // Make copy of old path as it is changed during processing
    Geom::PathVector const original_pathv = curve->get_pathvector();
    guint len = curve->get_segment_count() + 2;

    curve->reset();
    Spiro::spiro_cp *path = g_new (Spiro::spiro_cp, len);
    int ip = 0;

    for(Geom::PathVector::const_iterator path_it = original_pathv.begin(); path_it != original_pathv.end(); ++path_it) {
        if (path_it->empty())
            continue;

        // start of path
        {
            Geom::Point p = path_it->front().pointAt(0);
            path[ip].x = p[X];
            path[ip].y = p[Y];
            path[ip].ty = '{' ;  // for closed paths, this is overwritten
            ip++;
        }

        // midpoints
        Geom::Path::const_iterator curve_it1 = path_it->begin();      // incoming curve
        Geom::Path::const_iterator curve_it2 = ++(path_it->begin());         // outgoing curve

        Geom::Path::const_iterator curve_endit = path_it->end_default(); // this determines when the loop has to stop
        if (path_it->closed()) {
            // if the path is closed, maybe we have to stop a bit earlier because the closing line segment has zerolength.
            const Geom::Curve &closingline = path_it->back_closed(); // the closing line segment is always of type Geom::LineSegment.
            if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
                // closingline.isDegenerate() did not work, because it only checks for *exact* zero length, which goes wrong for relative coordinates and rounding errors...
                // the closing line segment has zero-length. So stop before that one!
                curve_endit = path_it->end_open();
            }
        }

        while ( curve_it2 != curve_endit )
        {
            /* This deals with the node between curve_it1 and curve_it2.
             * Loop to end_default (so without last segment), loop ends when curve_it2 hits the end
             * and then curve_it1 points to end or closing segment */
            Geom::Point p = curve_it1->finalPoint();
            path[ip].x = p[X];
            path[ip].y = p[Y];

            // Determine type of spiro node this is, determined by the tangents (angles) of the curves
            // TODO: see if this can be simplified by using /helpers/geom-nodetype.cpp:get_nodetype
            bool this_is_line = is_straight_curve(*curve_it1);
            bool next_is_line = is_straight_curve(*curve_it2);

            Geom::NodeType nodetype = Geom::get_nodetype(*curve_it1, *curve_it2);

            if ( nodetype == Geom::NODE_SMOOTH || nodetype == Geom::NODE_SYMM )
            {
                if (this_is_line && !next_is_line) {
                    path[ip].ty = ']';
                } else if (next_is_line && !this_is_line) {
                    path[ip].ty = '[';
                } else {
                    path[ip].ty = 'c';
                }
            } else {
                path[ip].ty = 'v';
            }

            ++curve_it1;
            ++curve_it2;
            ip++;
        }

        // add last point to the spiropath
        Geom::Point p = curve_it1->finalPoint();
        path[ip].x = p[X];
        path[ip].y = p[Y];
        if (path_it->closed()) {
            // curve_it1 points to the (visually) closing segment. determine the match between first and this last segment (the closing node)
            Geom::NodeType nodetype = Geom::get_nodetype(*curve_it1, path_it->front());
            switch (nodetype) {
                case Geom::NODE_NONE: // can't happen! but if it does, it means the path isn't closed :-)
                    path[ip].ty = '}';
                    ip++;
                    break;
                case Geom::NODE_CUSP:
                    path[0].ty = path[ip].ty = 'v';
                    break;
                case Geom::NODE_SMOOTH:
                case Geom::NODE_SYMM:
                    path[0].ty = path[ip].ty = 'c';
                    break;
            }
        } else {
            // set type to path closer
            path[ip].ty = '}';
            ip++;
        }

        // run subpath through spiro
        int sp_len = ip;
        Spiro::spiro_run(path, sp_len, *curve);
        ip = 0;
    }

    g_free (path);
}

//Unimos todas las curvas en juego y llamamos a la función doEffect.

static void bspline(SPPenContext *const pc, bool shift)
{
    if(!pc->anchor_statusbar)
        shift?bsplineOff(pc):bsplineOn(pc);

    bspline_build(pc);
}

static void bsplineOn(SPPenContext *const pc){
    if(!pc->red_curve->is_empty()){
        using Geom::X;
        using Geom::Y;
        pc->npoints = 5;
        pc->p[0] = pc->red_curve->first_segment()->initialPoint();
        pc->p[3] = pc->red_curve->first_segment()->finalPoint();
        pc->p[2] = pc->p[3] + (1./3)*(pc->p[0] - pc->p[3]);
        pc->p[2] = Geom::Point(pc->p[2][X] + 0.0625,pc->p[2][Y] + 0.0625);
    }
}

static void bsplineOff(SPPenContext *const pc)
{
    if(!pc->red_curve->is_empty()){
        pc->npoints = 5;
        pc->p[0] = pc->red_curve->first_segment()->initialPoint();
        pc->p[3] = pc->red_curve->first_segment()->finalPoint();
        pc->p[2] = pc->p[3];
    }
}

static void bsplineStartAnchor(SPPenContext *const pc, bool shift)
{
    if(pc->sa->curve->is_empty())
        return;

    if(shift)
        bsplineStartAnchorOff(pc);
    else
        bsplineStartAnchorOn(pc);
}

static void bsplineStartAnchorOn(SPPenContext *const pc)
{
    using Geom::X;
    using Geom::Y;
    SPCurve *tmpCurve = new SPCurve();
    tmpCurve = pc->sa->curve->copy();
    if(pc->sa->start)
        tmpCurve = tmpCurve->create_reverse();
    Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const*>(&*tmpCurve->last_segment());
    SPCurve *lastSeg = new SPCurve();
    Geom::Point A = tmpCurve->last_segment()->initialPoint();
    Geom::Point D = tmpCurve->last_segment()->finalPoint();
    Geom::Point C = D + (1./3)*(A - D);
    C = Geom::Point(C[X] + 0.0625,C[Y] + 0.0625);
    if(cubic){
        lastSeg->moveto(A);
        lastSeg->curveto((*cubic)[1],C,D);
    }else{
        lastSeg->moveto(A);
        lastSeg->curveto(A,C,D);
    }
    if( tmpCurve->get_segment_count() == 1){
        tmpCurve = lastSeg;
    }else{
        //we eliminate the last segment
        tmpCurve->backspace();
        //and we add it again with the recreation
        tmpCurve->append_continuous(lastSeg, 0.0625);
    }
    if (pc->sa->start) {
        tmpCurve = tmpCurve->create_reverse();
    }
    pc->sa->curve->reset();
    pc->sa->curve = tmpCurve;
}

static void bsplineStartAnchorOff(SPPenContext *const pc)
{
    SPCurve *tmpCurve = new SPCurve();
    tmpCurve = pc->sa->curve->copy();
    if(pc->sa->start)
        tmpCurve = tmpCurve->create_reverse();
    Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const*>(&*tmpCurve->last_segment());
    if(cubic){
        SPCurve *lastSeg = new SPCurve();
        lastSeg->moveto((*cubic)[0]);
        lastSeg->curveto((*cubic)[1],(*cubic)[3],(*cubic)[3]);
        if( tmpCurve->get_segment_count() == 1){
            tmpCurve = lastSeg;
        }else{
            //we eliminate the last segment
            tmpCurve->backspace();
            //and we add it again with the recreation
            tmpCurve->append_continuous(lastSeg, 0.0625);
        }
        if (pc->sa->start) {
            tmpCurve = tmpCurve->create_reverse();
        }
        pc->sa->curve->reset();
        pc->sa->curve = tmpCurve;
    }

}

static void bsplineMotion(SPPenContext *const pc, bool shift){
    using Geom::X;
    using Geom::Y;
    SPCurve *tmpCurve = new SPCurve();
    if(shift)
        pc->p[2] = pc->p[3];
    else
        pc->p[2] = pc->p[3] + (1./3)*(pc->p[0] - pc->p[3]);
        pc->p[2] = Geom::Point(pc->p[2][X] + 0.0625,pc->p[2][Y] + 0.0625);

    if(pc->green_curve->is_empty() && !pc->sa){
        pc->p[1] = pc->p[0] + (1./3)*(pc->p[3] - pc->p[0]);
    }else if(!pc->green_curve->is_empty()){
        tmpCurve = pc->green_curve->copy();
    }else{
        tmpCurve = pc->sa->curve->copy();
        if(pc->sa->start)
            tmpCurve = tmpCurve->create_reverse();
    }
    if(!tmpCurve->is_empty() && !pc->red_curve->is_empty()){
        Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const*>(&*tmpCurve->last_segment());
        if(cubic){
            SPCurve * WPower = new SPCurve();
            Geom::D2< Geom::SBasis > SBasisWPower;
            WPower->moveto(tmpCurve->last_segment()->finalPoint());
            WPower->lineto(tmpCurve->last_segment()->initialPoint());
            float WP = Geom::nearest_point((*cubic)[2],*WPower->first_segment());
            WPower->reset();
            WPower->moveto(pc->red_curve->last_segment()->initialPoint());
            WPower->lineto(pc->red_curve->last_segment()->finalPoint());
            SBasisWPower = WPower->first_segment()->toSBasis();
            WPower->reset();
            pc->p[1] = SBasisWPower.valueAt(WP);
            pc->p[1] = Geom::Point(pc->p[1][X] + 0.0625,pc->p[1][Y] + 0.0625);
        }else{
            pc->p[1] = pc->p[0];
        }
    }

    if(pc->anchor_statusbar && !pc->red_curve->is_empty()){
        if(shift)
            bsplineEndAnchorOff(pc);
        else
            bsplineEndAnchorOn(pc);
    }

    bspline_build(pc);
}

static void bsplineEndAnchorOn(SPPenContext *const pc)
{
    using Geom::X;
    using Geom::Y;
    pc->p[2] = pc->p[3] + (1./3)*(pc->p[0] - pc->p[3]);
    pc->p[2] = Geom::Point(pc->p[2][X] + 0.0625,pc->p[2][Y] + 0.0625);
    SPCurve *tmpCurve = new SPCurve();
    SPCurve *lastSeg = new SPCurve();
    Geom::Point C(0,0);
    if(!pc->sa || pc->sa->curve->is_empty()){
        tmpCurve = pc->green_curve->create_reverse();
        Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const*>(&*tmpCurve->last_segment());
        C = tmpCurve->last_segment()->finalPoint() + (1./3)*(tmpCurve->last_segment()->initialPoint() - tmpCurve->last_segment()->finalPoint());
        C = Geom::Point(C[X] + 0.0625,C[Y] + 0.0625);
        if(cubic){
            lastSeg->moveto((*cubic)[0]);
            lastSeg->curveto((*cubic)[1],C,(*cubic)[3]);
        }else{
            lastSeg->moveto(tmpCurve->last_segment()->initialPoint());
            lastSeg->curveto(tmpCurve->last_segment()->initialPoint(),C,tmpCurve->last_segment()->finalPoint());
        }
        if( tmpCurve->get_segment_count() == 1){
            tmpCurve = lastSeg;
        }else{
            //we eliminate the last segment
            tmpCurve->backspace();
            //and we add it again with the recreation
            tmpCurve->append_continuous(lastSeg, 0.0625);
        }
        tmpCurve = tmpCurve->create_reverse();
        pc->green_curve->reset();
        pc->green_curve = tmpCurve;
    }else{
        tmpCurve = pc->sa->curve->copy();
        if(!pc->sa->start)
            tmpCurve = tmpCurve->create_reverse();
        Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const*>(&*tmpCurve->last_segment());
        C = tmpCurve->last_segment()->finalPoint() + (1./3)*(tmpCurve->last_segment()->initialPoint() - tmpCurve->last_segment()->finalPoint());
        C = Geom::Point(C[X] + 0.0625,C[Y] + 0.0625);
        if(cubic){
            lastSeg->moveto((*cubic)[0]);
            lastSeg->curveto((*cubic)[1],C,(*cubic)[3]);
        }else{
            lastSeg->moveto(tmpCurve->last_segment()->initialPoint());
            lastSeg->curveto(tmpCurve->last_segment()->initialPoint(),C,tmpCurve->last_segment()->finalPoint());
        }
        if( tmpCurve->get_segment_count() == 1){
            tmpCurve = lastSeg;
        }else{
            //we eliminate the last segment
            tmpCurve->backspace();
            //and we add it again with the recreation
            tmpCurve->append_continuous(lastSeg, 0.0625);
        }
        if (!pc->sa->start) {
            tmpCurve = tmpCurve->create_reverse();
        }
        pc->sa->curve->reset();
        pc->sa->curve = tmpCurve;
    }
}

static void bsplineEndAnchorOff(SPPenContext *const pc)
{
    pc->p[2] = pc->p[3];
    SPCurve *tmpCurve = new SPCurve();
    SPCurve *lastSeg = new SPCurve();
    if(!pc->sa || pc->sa->curve->is_empty()){
        tmpCurve = pc->green_curve->create_reverse();
        Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const*>(&*tmpCurve->last_segment());
        if(cubic){
            lastSeg->moveto((*cubic)[0]);
            lastSeg->curveto((*cubic)[1],(*cubic)[3],(*cubic)[3]);
            if( tmpCurve->get_segment_count() == 1){
                tmpCurve = lastSeg;
            }else{
                //we eliminate the last segment
                tmpCurve->backspace();
                //and we add it again with the recreation
                tmpCurve->append_continuous(lastSeg, 0.0625);
            }
            tmpCurve = tmpCurve->create_reverse();
            pc->green_curve->reset();
            pc->green_curve = tmpCurve;
        }
    }else{
        tmpCurve = pc->sa->curve->copy();
        if(!pc->sa->start)
            tmpCurve = tmpCurve->create_reverse();
        Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const*>(&*tmpCurve->last_segment());
        if(cubic){
            lastSeg->moveto((*cubic)[0]);
            lastSeg->curveto((*cubic)[1],(*cubic)[3],(*cubic)[3]);
            if( tmpCurve->get_segment_count() == 1){
                tmpCurve = lastSeg;
            }else{
                //we eliminate the last segment
                tmpCurve->backspace();
                //and we add it again with the recreation
                tmpCurve->append_continuous(lastSeg, 0.0625);
            }
            if (!pc->sa->start) {
                tmpCurve = tmpCurve->create_reverse();
            }
            pc->sa->curve->reset();
            pc->sa->curve = tmpCurve;
        }
    }
}


//preparates the curves for its trasformation into BSline curves.
static void bspline_build(SPPenContext *const pc)
{
    //We create the base curve
    SPCurve *curve = new SPCurve();
    //If we continuate the existing curve we add it at the start
    if(pc->sa && !pc->sa->curve->is_empty()){
        curve = pc->sa->curve->copy();
        if (pc->sa->start) {
            curve = curve->create_reverse();
        }
    }

    if (!pc->green_curve->is_empty())
        curve->append_continuous(pc->green_curve, 0.0625);

    //and the red one
    if (!pc->red_curve->is_empty()){
        pc->red_curve->reset();
        pc->red_curve->moveto(pc->p[0]);
        pc->red_curve->curveto(pc->p[1],pc->p[2],pc->p[3]);
        sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->red_bpath), pc->red_curve);
        curve->append_continuous(pc->red_curve, 0.0625);
    }

    if(!curve->is_empty()){
        //cerramos la curva si estan cerca los puntos finales de la curva spiro
        if(Geom::are_near(curve->first_path()->initialPoint(), curve->last_path()->finalPoint())){
            curve->closepath_current();
        }
        //TODO: CALL TO CLONED FUNCTION SPIRO::doEffect IN lpe-spiro.cpp
        //For example
        //using namespace Inkscape::LivePathEffect;
        //LivePathEffectObject *lpeobj = static_cast<LivePathEffectObject*> (curve);
        //Effect *spr = static_cast<Effect*> ( new LPEbspline(lpeobj) );
        //spr->doEffect(curve);
        bspline_doEffect(curve);
        sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->blue_bpath), curve);   
        sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(pc->blue_bpath), pc->blue_color, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
        sp_canvas_item_show(pc->blue_bpath); 
        curve->unref();
        pc->blue_curve->reset();
        //We hide the holders that doesn't contribute anything
        sp_canvas_item_hide(pc->c1);
        sp_canvas_item_hide(pc->cl1);
        sp_canvas_item_hide(pc->c0);
        sp_canvas_item_hide(pc->cl0);
    }else{
        //if the curve is empty
        sp_canvas_item_hide(pc->blue_bpath);
    }
}

static void bspline_doEffect(SPCurve * curve)
{
    if(curve->get_segment_count() < 2)
        return;
    // Make copy of old path as it is changed during processing
    Geom::PathVector const original_pathv = curve->get_pathvector();
    curve->reset();

    //Recorremos todos los paths a los que queremos aplicar el efecto, hasta el penúltimo
    for(Geom::PathVector::const_iterator path_it = original_pathv.begin(); path_it != original_pathv.end(); ++path_it) {
        //Si está vacío... 
        if (path_it->empty())
            continue;
        //Itreadores
        
        Geom::Path::const_iterator curve_it1 = path_it->begin();      // incoming curve
        Geom::Path::const_iterator curve_it2 = ++(path_it->begin());         // outgoing curve
        Geom::Path::const_iterator curve_endit = path_it->end_default(); // this determines when the loop has to stop
        //Creamos las lineas rectas que unen todos los puntos del trazado y donde se calcularán
        //los puntos clave para los manejadores. 
        //Esto hace que la curva BSpline no pierda su condición aunque se trasladen
        //dichos manejadores
        SPCurve *nCurve = new SPCurve();
        Geom::Point startNode(0,0);
        Geom::Point previousNode(0,0);
        Geom::Point node(0,0);
        Geom::Point pointAt1(0,0);
        Geom::Point pointAt2(0,0);
        Geom::Point nextPointAt1(0,0);
        Geom::Point nextPointAt2(0,0);
        Geom::Point nextPointAt3(0,0);
        Geom::D2< Geom::SBasis > SBasisIn;
        Geom::D2< Geom::SBasis > SBasisOut;
        Geom::D2< Geom::SBasis > SBasisHelper;
        Geom::CubicBezier const *cubic = NULL;
        //Si la curva está cerrada calculamos el punto donde
        //deveria estar el nodo BSpline de cierre/inicio de la curva
        //en posible caso de que se cierre con una linea recta creando un nodo BSPline

        if (path_it->closed()) {
            //Calculamos el nodo de inicio BSpline
            const Geom::Curve &closingline = path_it->back_closed();
            if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
                curve_endit = path_it->end_open();
            }

            SPCurve * in = new SPCurve();
            in->moveto(curve_it1->initialPoint());
            in->lineto(curve_it1->finalPoint());
            SBasisIn = in->first_segment()->toSBasis();

            SPCurve *lineHelper = new SPCurve();
            cubic = dynamic_cast<Geom::CubicBezier const*>(&*curve_it1);
            if(cubic){
                lineHelper->moveto(SBasisIn.valueAt(Geom::nearest_point((*cubic)[1],*in->first_segment())));
            }else{
                lineHelper->moveto(in->first_segment()->initialPoint());
            }
            in->reset();
            delete in;

            SPCurve * end = new SPCurve();
            end->moveto(curve_endit->initialPoint());
            end->lineto(curve_endit->finalPoint());
            Geom::D2< Geom::SBasis > SBasisEnd = end->first_segment()->toSBasis();
            //Geom::BezierCurve const *bezier = dynamic_cast<Geom::BezierCurve const*>(&*curve_endit);
            cubic = dynamic_cast<Geom::CubicBezier const*>(&*curve_endit);
            if(cubic){
                lineHelper->lineto(SBasisEnd.valueAt(Geom::nearest_point((*cubic)[2],*end->first_segment())));
            }else{
                lineHelper->lineto(end->first_segment()->finalPoint());
            }
            end->reset();
            delete end;
            SBasisHelper = lineHelper->first_segment()->toSBasis();
            lineHelper->reset();
            delete lineHelper;
            //Guardamos el principio de la curva
            startNode = SBasisHelper.valueAt(0.5);
            //Definimos el punto de inicio original de la curva resultante
            node = startNode;
        }else{
            //Guardamos el principio de la curva
            SPCurve * in = new SPCurve();
            in->moveto(curve_it1->initialPoint());
            in->lineto(curve_it1->finalPoint());
            startNode = in->first_segment()->initialPoint();
            in->reset();
            delete in;
            //Definimos el punto de inicio original de la curva resultante
            node = startNode;
        }
        //Recorremos todos los segmentos menos el último
        while ( curve_it2 != curve_endit )
        {
            //previousPointAt3 = pointAt3;
            //Calculamos los puntos que dividirían en tres segmentos iguales el path recto de entrada y de salida
            SPCurve * in = new SPCurve();
            in->moveto(curve_it1->initialPoint());
            in->lineto(curve_it1->finalPoint());
            cubic = dynamic_cast<Geom::CubicBezier const*>(&*curve_it1);
            if(cubic){
                SBasisIn = in->first_segment()->toSBasis();
                pointAt1 = SBasisIn.valueAt(Geom::nearest_point((*cubic)[1],*in->first_segment()));
                pointAt2 = SBasisIn.valueAt(Geom::nearest_point((*cubic)[2],*in->first_segment()));
            }else{
                pointAt1 = in->first_segment()->initialPoint();
                pointAt2 = in->first_segment()->finalPoint();
            }
            in->reset();
            delete in;
            //Y hacemos lo propio con el path de salida
            //nextPointAt0 = curveOut.valueAt(0);
            SPCurve * out = new SPCurve();
            out->moveto(curve_it2->initialPoint());
            out->lineto(curve_it2->finalPoint());
            cubic = dynamic_cast<Geom::CubicBezier const*>(&*curve_it2);
            if(cubic){
                SBasisOut = out->first_segment()->toSBasis();
                nextPointAt1 = SBasisOut.valueAt(Geom::nearest_point((*cubic)[1],*out->first_segment()));
                nextPointAt2 = SBasisOut.valueAt(Geom::nearest_point((*cubic)[2],*out->first_segment()));;
                nextPointAt3 = (*cubic)[3];
            }else{
                nextPointAt1 = out->first_segment()->initialPoint();
                nextPointAt2 = out->first_segment()->finalPoint();
                nextPointAt3 = out->first_segment()->finalPoint();
            }
            out->reset();
            delete out;
            //La curva BSpline se forma calculando el centro del segmanto de unión
            //de el punto situado en las 2/3 partes de el segmento de entrada
            //con el punto situado en la posición 1/3 del segmento de salida
            //Estos dos puntos ademas estan posicionados en el lugas correspondiente de
            //los manejadores de la curva
            SPCurve *lineHelper = new SPCurve();
            lineHelper->moveto(pointAt2);
            lineHelper->lineto(nextPointAt1);
            SBasisHelper  = lineHelper->first_segment()->toSBasis();
            lineHelper->reset();
            delete lineHelper;
            //almacenamos el punto del anterior bucle -o el de cierre- que nos hara de principio de curva
            previousNode = node;
            //Y este hará de final de curva
            node = SBasisHelper.valueAt(0.5);
            SPCurve *curveHelper = new SPCurve();
            curveHelper->moveto(previousNode);
            curveHelper->curveto(pointAt1, pointAt2, node);
            //añadimos la curva generada a la curva pricipal
            nCurve->append_continuous(curveHelper, 0.0625);
            curveHelper->reset();
            delete curveHelper;
            //aumentamos los valores para el siguiente paso en el bucle
            ++curve_it1;
            ++curve_it2;
        }
        //Aberiguamos la ultima parte de la curva correspondiente al último segmento
        SPCurve *curveHelper = new SPCurve();
        curveHelper->moveto(node);
        //Si está cerrada la curva, la cerramos sobre  el valor guardado previamente
        //Si no finalizamos en el punto final
        if (path_it->closed()) {
            curveHelper->curveto(nextPointAt1, nextPointAt2, startNode);
        }else{
            curveHelper->curveto(nextPointAt1, nextPointAt2, nextPointAt3);
        }
        //añadimos este último segmento
        nCurve->append_continuous(curveHelper, 0.0625);
        curveHelper->reset();
        delete curveHelper;
        //y cerramos la curva
        if (path_it->closed()) {
            nCurve->closepath_current();
        }
        curve->append(nCurve,false);
        nCurve->reset();
        delete nCurve;
    }
}
//BSpline end

static void spdc_pen_set_subsequent_point(SPPenContext *const pc, Geom::Point const p, bool statusbar, guint status)
{
    g_assert( pc->npoints != 0 );
    // todo: Check callers to see whether 2 <= npoints is guaranteed.

    pc->p[2] = p;
    pc->p[3] = p;
    pc->p[4] = p;
    pc->npoints = 5;
    pc->red_curve->reset();
    bool is_curve;
    pc->red_curve->moveto(pc->p[0]);
    if (pc->polylines_paraxial && !statusbar) {
        // we are drawing horizontal/vertical lines and hit an anchor;
        Geom::Point const origin = pc->p[0];
        // if the previous point and the anchor are not aligned either horizontally or vertically...
        if ((abs(p[Geom::X] - origin[Geom::X]) > 1e-9) && (abs(p[Geom::Y] - origin[Geom::Y]) > 1e-9)) {
            // ...then we should draw an L-shaped path, consisting of two paraxial segments
            Geom::Point intermed = p;
            pen_set_to_nearest_horiz_vert(pc, intermed, status, false);
            pc->red_curve->lineto(intermed);
        }
        pc->red_curve->lineto(p);
        is_curve = false;
    } else {
        // one of the 'regular' modes
        //SpiroLive
        if (pc->p[1] != pc->p[0] || pc->spiro) {
        //SpiroLive End
            pc->red_curve->curveto(pc->p[1], p, p);
            is_curve = true;
        } else {
            pc->red_curve->lineto(p);
            is_curve = false;
        }
    }
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->red_bpath), pc->red_curve);
    if (statusbar) {
        gchar *message = is_curve ?
            _("<b>Curve segment</b>: angle %3.2f&#176;, distance %s; with <b>Ctrl</b> to snap angle, <b>Enter</b> to finish the path" ):
            _("<b>Line segment</b>: angle %3.2f&#176;, distance %s; with <b>Ctrl</b> to snap angle, <b>Enter</b> to finish the path");
        //BSpline
        if(pc->spiro || pc->bspline){
            message = is_curve ?
            _("<b>Curve segment</b>: angle %3.2f&#176;, distance %s; with <b>Shift</b> to cusp node, <b>Enter</b> to finish the path" ):
            _("<b>Line segment</b>: angle %3.2f&#176;, distance %s; with <b>Shift</b> to cusp node, <b>Enter</b> to finish the path");        
        }
        //BSpline End
        spdc_pen_set_angle_distance_status_message(pc, p, 0, message);
    }
}

static void spdc_pen_set_ctrl(SPPenContext *const pc, Geom::Point const p, guint const state)
{
    sp_canvas_item_show(pc->c1);
    sp_canvas_item_show(pc->cl1);

    if ( pc->npoints == 2 ) {
        pc->p[1] = p;
        sp_canvas_item_hide(pc->c0);
        sp_canvas_item_hide(pc->cl0);
        SP_CTRL(pc->c1)->moveto(pc->p[1]);
        pc->cl1->setCoords(pc->p[0], pc->p[1]);
        spdc_pen_set_angle_distance_status_message(pc, p, 0, _("<b>Curve handle</b>: angle %3.2f&#176;, length %s; with <b>Ctrl</b> to snap angle"));
    } else if ( pc->npoints == 5 ) {
        pc->p[4] = p;
        sp_canvas_item_show(pc->c0);
        sp_canvas_item_show(pc->cl0);
        bool is_symm = false;
        if ( ( ( pc->mode == SP_PEN_CONTEXT_MODE_CLICK ) && ( state & GDK_CONTROL_MASK ) ) ||
             ( ( pc->mode == SP_PEN_CONTEXT_MODE_DRAG ) &&  !( state & GDK_SHIFT_MASK  ) ) ) {
            Geom::Point delta = p - pc->p[3];
            pc->p[2] = pc->p[3] - delta;
            is_symm = true;
            pc->red_curve->reset();
            pc->red_curve->moveto(pc->p[0]);
            pc->red_curve->curveto(pc->p[1], pc->p[2], pc->p[3]);
            sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->red_bpath), pc->red_curve);
        }
        SP_CTRL(pc->c0)->moveto(pc->p[2]);
        pc->cl0 ->setCoords(pc->p[3], pc->p[2]);
        SP_CTRL(pc->c1)->moveto(pc->p[4]);
        pc->cl1->setCoords(pc->p[3], pc->p[4]);
        gchar *message = is_symm ?
            _("<b>Curve handle, symmetric</b>: angle %3.2f&#176;, length %s; with <b>Ctrl</b> to snap angle, with <b>Shift</b> to move this handle only") :
            _("<b>Curve handle</b>: angle %3.2f&#176;, length %s; with <b>Ctrl</b> to snap angle, with <b>Shift</b> to move this handle only");
        spdc_pen_set_angle_distance_status_message(pc, p, 3, message);
    } else {
        g_warning("Something bad happened - npoints is %d", pc->npoints);
    }
}

static void spdc_pen_finish_segment(SPPenContext *const pc, Geom::Point const p, guint const state)
{
    if (pc->polylines_paraxial) {
        pen_last_paraxial_dir = pen_next_paraxial_direction(pc, p, pc->p[0], state);
    }

    ++pc->num_clicks;

    if (!pc->red_curve->is_empty()) {
        if(pc->spiro){
            spiro(pc,(state & GDK_SHIFT_MASK));
        }
        if(pc->bspline){
            bspline(pc,(state & GDK_SHIFT_MASK));
        }
        pc->green_curve->append_continuous(pc->red_curve, 0.0625);
        SPCurve *curve = pc->red_curve->copy();
        /// \todo fixme:
        SPCanvasItem *cshape = sp_canvas_bpath_new(sp_desktop_sketch(pc->desktop), curve);
        curve->unref();
        sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(cshape), pc->green_color, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);

        pc->green_bpaths = g_slist_prepend(pc->green_bpaths, cshape);

        pc->p[0] = pc->p[3];
        pc->p[1] = pc->p[4];
        pc->npoints = 2;

        pc->red_curve->reset();
    }
}

static void spdc_pen_finish(SPPenContext *const pc, gboolean const closed)
{
    if (pc->expecting_clicks_for_LPE > 1) {
        // don't let the path be finished before we have collected the required number of mouse clicks
        return;
    }

    pc->num_clicks = 0;
    pen_disable_events(pc);

    SPDesktop *const desktop = pc->desktop;
    pc->_message_context->clear();
    desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Drawing finished"));

    pc->red_curve->reset();
    spdc_concat_colors_and_flush(pc, closed);
    pc->sa = NULL;
    pc->ea = NULL;

    pc->npoints = 0;
    pc->state = SP_PEN_CONTEXT_POINT;

    sp_canvas_item_hide(pc->c0);
    sp_canvas_item_hide(pc->c1);
    sp_canvas_item_hide(pc->cl0);
    sp_canvas_item_hide(pc->cl1);

    if (pc->green_anchor) {
        pc->green_anchor = sp_draw_anchor_destroy(pc->green_anchor);
    }


    pc->desktop->canvas->endForcedFullRedraws();

    pen_enable_events(pc);
}

static void pen_disable_events(SPPenContext *const pc) {
  pc->events_disabled++;
}

static void pen_enable_events(SPPenContext *const pc) {
  g_return_if_fail(pc->events_disabled != 0);

  pc->events_disabled--;
}

void sp_pen_context_wait_for_LPE_mouse_clicks(SPPenContext *pc, Inkscape::LivePathEffect::EffectType effect_type,
                                         unsigned int num_clicks, bool use_polylines)
{
    if (effect_type == Inkscape::LivePathEffect::INVALID_LPE)
        return;

    pc->waiting_LPE_type = effect_type;
    pc->expecting_clicks_for_LPE = num_clicks;
    pc->polylines_only = use_polylines;
    pc->polylines_paraxial = false; // TODO: think if this is correct for all cases
}

void sp_pen_context_cancel_waiting_for_LPE(SPPenContext *pc)
{
    pc->waiting_LPE_type = Inkscape::LivePathEffect::INVALID_LPE;
    pc->expecting_clicks_for_LPE = 0;
    sp_pen_context_set_polyline_mode(pc);
}

static int pen_next_paraxial_direction(const SPPenContext *const pc,
                                       Geom::Point const &pt, Geom::Point const &origin, guint state) {
    //
    // after the first mouse click we determine whether the mouse pointer is closest to a
    // horizontal or vertical segment; for all subsequent mouse clicks, we use the direction
    // orthogonal to the last one; pressing Shift toggles the direction
    //
    // num_clicks is not reliable because spdc_pen_finish_segment is sometimes called too early
    // (on first mouse release), in which case num_clicks immediately becomes 1.
    // if (pc->num_clicks == 0) {

    if (pc->green_curve->is_empty()) {
        // first mouse click
        double dist_h = fabs(pt[Geom::X] - origin[Geom::X]);
        double dist_v = fabs(pt[Geom::Y] - origin[Geom::Y]);
        int ret = (dist_h < dist_v) ? 1 : 0; // 0 = horizontal, 1 = vertical
        pen_last_paraxial_dir = (state & GDK_SHIFT_MASK) ? 1 - ret : ret;
        return pen_last_paraxial_dir;
    } else {
        // subsequent mouse click
        return (state & GDK_SHIFT_MASK) ? pen_last_paraxial_dir : 1 - pen_last_paraxial_dir;
    }
}

void pen_set_to_nearest_horiz_vert(const SPPenContext *const pc, Geom::Point &pt, guint const state, bool snap)
{
    Geom::Point const origin = pc->p[0];

    int next_dir = pen_next_paraxial_direction(pc, pt, origin, state);

    if (!snap) {
        if (next_dir == 0) {
            // line is forced to be horizontal
            pt[Geom::Y] = origin[Geom::Y];
        } else {
            // line is forced to be vertical
            pt[Geom::X] = origin[Geom::X];
        }
    } else {
        // Create a horizontal or vertical constraint line
        Inkscape::Snapper::SnapConstraint cl(origin, next_dir ? Geom::Point(0, 1) : Geom::Point(1, 0));

        // Snap along the constraint line; if we didn't snap then still the constraint will be applied
        SnapManager &m = pc->desktop->namedview->snap_manager;

        Inkscape::Selection *selection = sp_desktop_selection (pc->desktop);
        // selection->singleItem() is the item that is currently being drawn. This item will not be snapped to (to avoid self-snapping)
        // TODO: Allow snapping to the stationary parts of the item, and only ignore the last segment

        m.setup(pc->desktop, true, selection->singleItem());
        m.constrainedSnapReturnByRef(pt, Inkscape::SNAPSOURCE_NODE_HANDLE, cl);
        m.unSetup();
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


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

#include "ui/tools/pen-tool.h"
#include "sp-namedview.h"
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
#include "macros.h"
#include "context-fns.h"
#include "tools-switch.h"
#include "ui/control-manager.h"
//spanish: incluimos los archivos necesarios para las BSpline y Spiro
#include "live_effects/effect.h"
#include "live_effects/lpeobject.h"
#include "live_effects/lpeobject-reference.h"
#include "live_effects/parameter/path.h"
#define INKSCAPE_LPE_SPIRO_C
#include "live_effects/lpe-spiro.h"


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

#include "tool-factory.h"

#include "live_effects/effect.h"


using Inkscape::ControlManager;

namespace Inkscape {
namespace UI {
namespace Tools {

static void spdc_pen_set_initial_point(PenTool *pc, Geom::Point const p);
//spanish: añade los modos spiro y bspline
static void sp_pen_context_set_mode(PenTool *const pc, guint mode);
//spanish: esta función cambia los colores rojo,verde y azul haciendolos transparentes o no en función de si se usa spiro
static void bspline_spiro_color(PenTool *const pc);
//spanish: crea un nodo en modo bspline o spiro
static void bspline_spiro(PenTool *const pc,bool shift);
//spanish: crea un nodo de modo spiro o bspline
static void bspline_spiro_on(PenTool *const pc);
//spanish: crea un nodo de tipo CUSP
static void bspline_spiro_off(PenTool *const pc);
//spanish: continua una curva existente en modo bspline o spiro
static void bspline_spiro_start_anchor(PenTool *const pc,bool shift);
//spanish: continua una curva exsitente con el nodo de union en modo bspline o spiro
static void bspline_spiro_start_anchor_on(PenTool *const pc);
//spanish: continua una curva existente con el nodo de union en modo CUSP
static void bspline_spiro_start_anchor_off(PenTool *const pc);
//spanish: modifica la "red_curve" cuando se detecta movimiento
static void bspline_spiro_motion(PenTool *const pc,bool shift);
//spanish: cierra la curva con el último nodo en modo bspline o spiro
static void bspline_spiro_end_anchor_on(PenTool *const pc);
//spanish: cierra la curva con el último nodo en modo CUSP
static void bspline_spiro_end_anchor_off(PenTool *const pc);
//spanish: unimos todas las curvas en juego y llamamos a la función doEffect.
static void bspline_spiro_build(PenTool *const pc);
//function bspline cloned from lpe-bspline.cpp
static void bspline_doEffect(SPCurve * curve);
//function spiro cloned from lpe-spiro.cpp
static void spiro_doEffect(SPCurve * curve);

static void spdc_pen_set_subsequent_point(PenTool *const pc, Geom::Point const p, bool statusbar, guint status = 0);
static void spdc_pen_set_ctrl(PenTool *pc, Geom::Point const p, guint state);
static void spdc_pen_finish_segment(PenTool *pc, Geom::Point p, guint state);

static void spdc_pen_finish(PenTool *pc, gboolean closed);

static gint pen_handle_button_press(PenTool *const pc, GdkEventButton const &bevent);
static gint pen_handle_motion_notify(PenTool *const pc, GdkEventMotion const &mevent);
static gint pen_handle_button_release(PenTool *const pc, GdkEventButton const &revent);
static gint pen_handle_2button_press(PenTool *const pc, GdkEventButton const &bevent);
static gint pen_handle_key_press(PenTool *const pc, GdkEvent *event);
static void spdc_reset_colors(PenTool *pc);

static void pen_disable_events(PenTool *const pc);
static void pen_enable_events(PenTool *const pc);

static Geom::Point pen_drag_origin_w(0, 0);
static bool pen_within_tolerance = false;

static int pen_next_paraxial_direction(const PenTool *const pc, Geom::Point const &pt, Geom::Point const &origin, guint state);
static void pen_set_to_nearest_horiz_vert(const PenTool *const pc, Geom::Point &pt, guint const state, bool snap);

static int pen_last_paraxial_dir = 0; // last used direction in horizontal/vertical mode; 0 = horizontal, 1 = vertical

namespace {
	ToolBase* createPenContext() {
		return new PenTool();
	}

	bool penContextRegistered = ToolFactory::instance().registerObject("/tools/freehand/pen", createPenContext);
}

const std::string& PenTool::getPrefsPath() {
	return PenTool::prefsPath;
}

const std::string PenTool::prefsPath = "/tools/freehand/pen";

PenTool::PenTool() : FreehandBase() {
	this->polylines_only = false;
	this->polylines_paraxial = false;
	this->expecting_clicks_for_LPE = 0;

    this->cursor_shape = cursor_pen_xpm;
    this->hot_x = 4;
    this->hot_y = 4;

    this->npoints = 0;
    this->mode = MODE_CLICK;
    this->state = POINT;

    this->c0 = NULL;
    this->c1 = NULL;
    this->cl0 = NULL;
    this->cl1 = NULL;

    this->events_disabled = 0;

    this->num_clicks = 0;
    this->waiting_LPE = NULL;
    this->waiting_item = NULL;
}

PenTool::~PenTool() {
    if (this->c0) {
        sp_canvas_item_destroy(this->c0);
        this->c0 = NULL;
    }
    if (this->c1) {
        sp_canvas_item_destroy(this->c1);
        this->c1 = NULL;
    }
    if (this->cl0) {
        sp_canvas_item_destroy(this->cl0);
        this->cl0 = NULL;
    }
    if (this->cl1) {
        sp_canvas_item_destroy(this->cl1);
        this->cl1 = NULL;
    }

    if (this->expecting_clicks_for_LPE > 0) {
        // we received too few clicks to sanely set the parameter path so we remove the LPE from the item
        this->waiting_item->removeCurrentPathEffect(false);
    }
}

void sp_pen_context_set_polyline_mode(PenTool *const pc) {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    guint mode = prefs->getInt("/tools/freehand/pen/freehand-mode", 0);
    //spanish: cambiamos los modos para dar cabida al modo bspline
    pc->polylines_only = (mode == 3 || mode == 4);
    pc->polylines_paraxial = (mode == 4);
    //we call the function which defines the Spiro modes and the BSpline
    //todo: merge to one function only
    sp_pen_context_set_mode(pc, mode);
}

/*
*.Set the mode of draw spiro, and bsplines
*/
void sp_pen_context_set_mode(PenTool *const pc, guint mode) {
    //spanish: definimos los modos
    pc->spiro = (mode == 1);
    pc->bspline = (mode == 2);
}

/**
 * Callback to initialize PenTool object.
 */
void PenTool::setup() {
    FreehandBase::setup();

    ControlManager &mgr = ControlManager::getManager();

    // Pen indicators
    this->c0 = mgr.createControl(sp_desktop_controls(this->desktop), Inkscape::CTRL_TYPE_ADJ_HANDLE);
    mgr.track(this->c0);

    this->c1 = mgr.createControl(sp_desktop_controls(this->desktop), Inkscape::CTRL_TYPE_ADJ_HANDLE);
    mgr.track(this->c1);

    this->cl0 = mgr.createControlLine(sp_desktop_controls(this->desktop));
    this->cl1 = mgr.createControlLine(sp_desktop_controls(this->desktop));

    sp_canvas_item_hide(this->c0);
    sp_canvas_item_hide(this->c1);
    sp_canvas_item_hide(this->cl0);
    sp_canvas_item_hide(this->cl1);

    sp_event_context_read(this, "mode");

    this->anchor_statusbar = false;

    sp_pen_context_set_polyline_mode(this);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/tools/freehand/pen/selcue")) {
        this->enableSelectionCue();
    }
}

static void pen_cancel (PenTool *const pc)
{
    pc->num_clicks = 0;
    pc->state = PenTool::STOP;
    spdc_reset_colors(pc);
    sp_canvas_item_hide(pc->c0);
    sp_canvas_item_hide(pc->c1);
    sp_canvas_item_hide(pc->cl0);
    sp_canvas_item_hide(pc->cl1);
    pc->message_context->clear();
    pc->message_context->flash(Inkscape::NORMAL_MESSAGE, _("Drawing cancelled"));

    pc->desktop->canvas->endForcedFullRedraws();
}

/**
 * Finalization callback.
 */
void PenTool::finish() {
    sp_event_context_discard_delayed_snap_event(this);

    if (this->npoints != 0) {
        pen_cancel(this);
    }

    FreehandBase::finish();
}

/**
 * Callback that sets key to value in pen context.
 */
void PenTool::set(const Inkscape::Preferences::Entry& val) {
    Glib::ustring name = val.getEntryName();

    if (name == "mode") {
        if ( val.getString() == "drag" ) {
            this->mode = MODE_DRAG;
        } else {
            this->mode = MODE_CLICK;
        }
    }
}

/**
 * Snaps new node relative to the previous node.
 */
static void spdc_endpoint_snap(PenTool const *const pc, Geom::Point &p, guint const state)
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
static void spdc_endpoint_snap_handle(PenTool const *const pc, Geom::Point &p, guint const state)
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

bool PenTool::item_handler(SPItem* item, GdkEvent* event) {
    gint ret = FALSE;

    switch (event->type) {
        case GDK_BUTTON_PRESS:
            ret = pen_handle_button_press(this, event->button);
            break;
        case GDK_BUTTON_RELEASE:
            ret = pen_handle_button_release(this, event->button);
            break;
        default:
            break;
    }

    if (!ret) {
    	ret = FreehandBase::item_handler(item, event);
    }

    return ret;
}

/**
 * Callback to handle all pen events.
 */
bool PenTool::root_handler(GdkEvent* event) {
    gint ret = FALSE;

    switch (event->type) {
        case GDK_BUTTON_PRESS:
            ret = pen_handle_button_press(this, event->button);
            break;

        case GDK_MOTION_NOTIFY:
            ret = pen_handle_motion_notify(this, event->motion);
            break;

        case GDK_BUTTON_RELEASE:
            ret = pen_handle_button_release(this, event->button);
            break;

        case GDK_2BUTTON_PRESS:
            ret = pen_handle_2button_press(this, event->button);
            break;

        case GDK_KEY_PRESS:
            ret = pen_handle_key_press(this, event);
            break;

        default:
            break;
    }

    if (!ret) {
    	ret = FreehandBase::root_handler(event);
    }

    return ret;
}

/**
 * Handle mouse button press event.
 */
static gint pen_handle_button_press(PenTool *const pc, GdkEventButton const &bevent)
{
    if (pc->events_disabled) {
        // skip event processing if events are disabled
        return FALSE;
    }

    FreehandBase * const dc = SP_DRAW_CONTEXT(pc);
    SPDesktop * const desktop = dc->desktop;
    Geom::Point const event_w(bevent.x, bevent.y);
    Geom::Point event_dt(desktop->w2d(event_w));
    ToolBase *event_context = SP_EVENT_CONTEXT(pc);
    //Test whether we hit any anchor.
    SPDrawAnchor * const anchor = spdc_test_inside(pc, event_w);

    //with this we avoid creating a new point over the existing one
    if(bevent.button != 3 && (pc->spiro || pc->bspline) && pc->npoints > 0 && pc->p[0] == pc->p[3]){
        pc->state = PenTool::STOP;
        if( anchor && anchor == pc->sa && pc->green_curve->is_empty()){
            //spanish Borrar siguiente linea para evitar un nodo encima de otro
            spdc_pen_finish_segment(pc, event_dt, bevent.state);
            spdc_pen_finish(pc, FALSE);
            return TRUE;
        }
        return FALSE;
    } 
    gint ret = FALSE;
    if (bevent.button == 1 && !event_context->space_panning
        // make sure this is not the last click for a waiting LPE (otherwise we want to finish the path)
        && pc->expecting_clicks_for_LPE != 1) {

        if (Inkscape::have_viable_layer(desktop, dc->message_context) == false) {
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
            case PenTool::MODE_CLICK:
                // In click mode we add point on release
                switch (pc->state) {
                    case PenTool::POINT:
                    case PenTool::CONTROL:
                    case PenTool::CLOSE:
                        break;
                    case PenTool::STOP:
                        // This is allowed, if we just canceled curve
                        pc->state = PenTool::POINT;
                        break;
                    default:
                        break;
                }
                break;
            case PenTool::MODE_DRAG:
                switch (pc->state) {
                    case PenTool::STOP:
                        // This is allowed, if we just canceled curve
                    case PenTool::POINT:
                        if (pc->npoints == 0) {

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
                            //spanish: continuamos una curva existente
                            if(anchor){
                                bspline_spiro_start_anchor(pc,(bevent.state & GDK_SHIFT_MASK));
                            }
                            if (anchor && (!sp_pen_context_has_waiting_LPE(pc) || pc->bspline || pc->spiro)) {
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
                                pc->state = PenTool::CLOSE;

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

                        //spanish: evitamos la creación de un punto de control para que se cree el nodo en el evento de soltar
                        pc->state = (pc->spiro || pc->bspline || pc->polylines_only) ? PenTool::POINT : PenTool::CONTROL;

                        ret = TRUE;
                        break;
                    case PenTool::CONTROL:
                        g_warning("Button down in CONTROL state");
                        break;
                    case PenTool::CLOSE:
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
static gint pen_handle_motion_notify(PenTool *const pc, GdkEventMotion const &mevent)
{
    gint ret = FALSE;

    ToolBase *event_context = SP_EVENT_CONTEXT(pc);
    SPDesktop * const dt = event_context->desktop;

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
    //we take out the function the const "tolerance" because we need it later
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gint const tolerance = prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100);
    if (pen_within_tolerance) {
        if ( Geom::LInfty( event_w - pen_drag_origin_w ) < tolerance ) {
            return FALSE;   // Do not drag if we're within tolerance from origin.
        }
    }
    // Once the user has moved farther than tolerance from the original location
    // (indicating they intend to move the object, not click), then always process the
    // motion notify coordinates as given (no snapping back to origin)
    pen_within_tolerance = false;

    // Find desktop coordinates
    Geom::Point p = dt->w2d(event_w);

    // Test, whether we hit any anchor
    SPDrawAnchor *anchor = spdc_test_inside(pc, event_w);

    switch (pc->mode) {
        case PenTool::MODE_CLICK:
            switch (pc->state) {
                case PenTool::POINT:
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
                case PenTool::CONTROL:
                case PenTool::CLOSE:
                    // Placing controls is last operation in CLOSE state
                    spdc_endpoint_snap(pc, p, mevent.state);
                    spdc_pen_set_ctrl(pc, p, mevent.state);
                    ret = TRUE;
                    break;
                case PenTool::STOP:
                    // This is perfectly valid
                    break;
                default:
                    break;
            }
            break;
        case PenTool::MODE_DRAG:
            switch (pc->state) {
                case PenTool::POINT:
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
                                pc->message_context->set(Inkscape::NORMAL_MESSAGE, _("<b>Click</b> or <b>click and drag</b> to close and finish the path."));
                            }else{
                                pc->message_context->set(Inkscape::NORMAL_MESSAGE, _("<b>Click</b> or <b>click and drag</b> to close and finish the path. Shift to cusp node"));
                            }
                            pc->anchor_statusbar = true;
                        } else if (!anchor && pc->anchor_statusbar) {
                            pc->message_context->clear();
                            pc->anchor_statusbar = false;
                        }

                        ret = TRUE;
                    } else {
                        if (anchor && !pc->anchor_statusbar) {
                            if(!pc->spiro && !pc->bspline){
                                pc->message_context->set(Inkscape::NORMAL_MESSAGE, _("<b>Click</b> or <b>click and drag</b> to continue the path from this point."));
                            }else{
                                pc->message_context->set(Inkscape::NORMAL_MESSAGE, _("<b>Click</b> or <b>click and drag</b> to continue the path from this point. Shift to cusp node"));      
                            }
                            pc->anchor_statusbar = true;
                        } else if (!anchor && pc->anchor_statusbar) {
                            pc->message_context->clear();
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
                case PenTool::CONTROL:
                case PenTool::CLOSE:
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
                case PenTool::STOP:
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
    //spanish: lanzamos la función "bspline_spiro_motion" al moverse el ratón o cuando se para.
    if(pc->bspline){
        bspline_spiro_color(pc);
        bspline_spiro_motion(pc,(mevent.state & GDK_SHIFT_MASK));
    }else{
        if ( Geom::LInfty( event_w - pen_drag_origin_w ) > tolerance || mevent.time == 0) {
            bspline_spiro_color(pc);
            bspline_spiro_motion(pc,(mevent.state & GDK_SHIFT_MASK));
            pen_drag_origin_w = event_w;
        }
    }

    return ret;
}

/**
 * Handle mouse button release event.
 */
static gint pen_handle_button_release(PenTool *const pc, GdkEventButton const &revent)
{
    if (pc->events_disabled) {
        // skip event processing if events are disabled
        return FALSE;
    }

    gint ret = FALSE;
    ToolBase *event_context = SP_EVENT_CONTEXT(pc);
    if ( revent.button == 1  && !event_context->space_panning) {

        FreehandBase *dc = SP_DRAW_CONTEXT (pc);

        Geom::Point const event_w(revent.x,
                                revent.y);
        // Find desktop coordinates
        Geom::Point p = pc->desktop->w2d(event_w);

        // Test whether we hit any anchor.
        SPDrawAnchor *anchor = spdc_test_inside(pc, event_w);
        //with this we avoid creating a new point over the existing one
        //spanish: si intentamos crear un nodo en el mismo sitio que el origen, paramos.

        if((!anchor || anchor == pc->sa) && (pc->spiro || pc->bspline) && pc->npoints > 0 && pc->p[0] == pc->p[3]){
            return TRUE;
        }

        switch (pc->mode) {
            case PenTool::MODE_CLICK:
                switch (pc->state) {
                    case PenTool::POINT:
                        if ( pc->npoints == 0 ) {
                            // Start new thread only with button release
                            if (anchor) {
                                p = anchor->dp;
                            }
                            pc->sa = anchor;
                            //spanish: continuamos una curva existente
                            if (anchor) {
                                if(pc->bspline || pc->spiro){
                                    bspline_spiro_start_anchor(pc,(revent.state & GDK_SHIFT_MASK));
                                }
                            }
                            spdc_pen_set_initial_point(pc, p);
                        } else {
                            // Set end anchor here
                            pc->ea = anchor;
                            if (anchor) {
                                p = anchor->dp;
                            }
                        }
                        pc->state = PenTool::CONTROL;
                        ret = TRUE;
                        break;
                    case PenTool::CONTROL:
                        // End current segment
                        spdc_endpoint_snap(pc, p, revent.state);
                        spdc_pen_finish_segment(pc, p, revent.state);
                        pc->state = PenTool::POINT;
                        ret = TRUE;
                        break;
                    case PenTool::CLOSE:
                        // End current segment
                        if (!anchor) {   // Snap node only if not hitting anchor
                            spdc_endpoint_snap(pc, p, revent.state);
                        }
                        spdc_pen_finish_segment(pc, p, revent.state);
                        //spanish: ocultamos la guia del penultimo nodo al cerrar la curva
                        if(pc->spiro){
                            sp_canvas_item_hide(pc->c1);
                        }
                        spdc_pen_finish(pc, TRUE);
                        pc->state = PenTool::POINT;
                        ret = TRUE;
                        break;
                    case PenTool::STOP:
                        // This is allowed, if we just canceled curve
                        pc->state = PenTool::POINT;
                        ret = TRUE;
                        break;
                    default:
                        break;
                }
                break;
            case PenTool::MODE_DRAG:
                switch (pc->state) {
                    case PenTool::POINT:
                    case PenTool::CONTROL:
                        spdc_endpoint_snap(pc, p, revent.state);
                        spdc_pen_finish_segment(pc, p, revent.state);
                        break;
                    case PenTool::CLOSE:
                        spdc_endpoint_snap(pc, p, revent.state);
                        spdc_pen_finish_segment(pc, p, revent.state);
                        //spanish: ocultamos la guia del penultimo nodo al cerrar la curva
                        if(pc->spiro){
                            sp_canvas_item_hide(pc->c1);
                        }
                        if (pc->green_closed) {
                            // finishing at the start anchor, close curve
                            spdc_pen_finish(pc, TRUE);
                        } else {
                            // finishing at some other anchor, finish curve but not close
                            spdc_pen_finish(pc, FALSE);
                        }
                        break;
                    case PenTool::STOP:
                        // This is allowed, if we just cancelled curve
                        break;
                    default:
                        break;
                }
                pc->state = PenTool::POINT;
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

        ToolBase *ec = SP_EVENT_CONTEXT(pc);
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

static gint pen_handle_2button_press(PenTool *const pc, GdkEventButton const &bevent)
{
    gint ret = FALSE;
    // only end on LMB double click. Otherwise horizontal scrolling causes ending of the path
    if (pc->npoints != 0 && bevent.button == 1) {
        spdc_pen_finish(pc, FALSE);
        ret = TRUE;
    }
    return ret;
}

static void pen_redraw_all (PenTool *const pc)
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
    //spanish: ocultamos los tiradores en modo bspline y spiro
    if (pc->p[0] != pc->p[1] && !pc->spiro && !pc->bspline) {
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
        //spanish: ocultamos los tiradores en modo bspline y spiro
        if ( cubic &&
             (*cubic)[2] != pc->p[0] && !pc->spiro && !pc->bspline )
        {
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

    //spanish: simplemente redibujamos la spiro.
    //como es un redibujo simplemente no llamamos a la función global sino al final de esta
    //Lanzamos solamente el redibujado
     bspline_spiro_build(pc);
}

static void pen_lastpoint_move (PenTool *const pc, gdouble x, gdouble y)
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

static void pen_lastpoint_move_screen (PenTool *const pc, gdouble x, gdouble y)
{
    pen_lastpoint_move (pc, x / pc->desktop->current_zoom(), y / pc->desktop->current_zoom());
}

static void pen_lastpoint_tocurve (PenTool *const pc)
{
    //spanish: evitamos que si la "red_curve" tiene solo dos puntos -recta- no se pare aqui.
    if (pc->npoints != 5 && !pc->spiro && !pc->bspline)
        return;
    Geom::CubicBezier const * cubic;
    pc->p[1] = pc->red_curve->last_segment()->initialPoint() + (1./3)* (Geom::Point)(pc->red_curve->last_segment()->finalPoint() - pc->red_curve->last_segment()->initialPoint());
    //spanish: modificamos el último segmento de la curva verde para que forme el tipo de nodo que deseamos
    if(pc->spiro||pc->bspline){
        if(!pc->green_curve->is_empty()){
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
                if(pc->spiro){
                    C = pc->p[0] + (Geom::Point)(pc->p[0] - pc->p[1]);
                }else
                    C = pc->green_curve->last_segment()->finalPoint() + (1./3)* (Geom::Point)(pc->green_curve->last_segment()->initialPoint() - pc->green_curve->last_segment()->finalPoint());
                D = (*cubic)[3];
            }else{
                A = pc->green_curve->last_segment()->initialPoint();
                B = pc->green_curve->last_segment()->initialPoint();
                if(pc->spiro)
                    C = pc->p[0] + (Geom::Point)(pc->p[0] - pc->p[1]);
                else
                    C = pc->green_curve->last_segment()->finalPoint() + (1./3)* (Geom::Point)(pc->green_curve->last_segment()->initialPoint() - pc->green_curve->last_segment()->finalPoint());
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
        //spanish: si el último nodo es una union con otra curva
        if(pc->green_curve->is_empty() && pc->sa && !pc->sa->curve->is_empty()){
            bspline_spiro_start_anchor(pc, false); 
        }
    }

    pen_redraw_all(pc);
}

static void pen_lastpoint_toline (PenTool *const pc)
{
    //spanish: evitamos que si la "red_curve" tiene solo dos puntos -recta- no se pare aqui.
    if (pc->npoints != 5 && !pc->bspline)
        return;

    //spanish: modificamos el último segmento de la curva verde para que forme el tipo de nodo que deseamos
    if(pc->spiro || pc->bspline){
        if(!pc->green_curve->is_empty()){
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
        //spanish: si el último nodo es una union con otra curva
        if(pc->green_curve->is_empty() && pc->sa && !pc->sa->curve->is_empty()){
            bspline_spiro_start_anchor(pc, true);
        }
    }
    
    pc->p[1] = pc->p[0];
    pen_redraw_all(pc);
}


static gint pen_handle_key_press(PenTool *const pc, GdkEvent *event)
{

    gint ret = FALSE;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gdouble const nudge = prefs->getDoubleLimited("/options/nudgedistance/value", 2, 0, 1000, "px"); // in px

    switch (get_group0_keyval (&event->key)) {

        case GDK_KEY_Left: // move last point left
        case GDK_KEY_KP_Left:
            if (!MOD__CTRL(event)) { // not ctrl
                if (MOD__ALT(event)) { // alt
                    if (MOD__SHIFT(event)) pen_lastpoint_move_screen(pc, -10, 0); // shift
                    else pen_lastpoint_move_screen(pc, -1, 0); // no shift
                }
                else { // no alt
                    if (MOD__SHIFT(event)) pen_lastpoint_move(pc, -10*nudge, 0); // shift
                    else pen_lastpoint_move(pc, -nudge, 0); // no shift
                }
                ret = TRUE;
            }
            break;
        case GDK_KEY_Up: // move last point up
        case GDK_KEY_KP_Up:
            if (!MOD__CTRL(event)) { // not ctrl
                if (MOD__ALT(event)) { // alt
                    if (MOD__SHIFT(event)) pen_lastpoint_move_screen(pc, 0, 10); // shift
                    else pen_lastpoint_move_screen(pc, 0, 1); // no shift
                }
                else { // no alt
                    if (MOD__SHIFT(event)) pen_lastpoint_move(pc, 0, 10*nudge); // shift
                    else pen_lastpoint_move(pc, 0, nudge); // no shift
                }
                ret = TRUE;
            }
            break;
        case GDK_KEY_Right: // move last point right
        case GDK_KEY_KP_Right:
            if (!MOD__CTRL(event)) { // not ctrl
                if (MOD__ALT(event)) { // alt
                    if (MOD__SHIFT(event)) pen_lastpoint_move_screen(pc, 10, 0); // shift
                    else pen_lastpoint_move_screen(pc, 1, 0); // no shift
                }
                else { // no alt
                    if (MOD__SHIFT(event)) pen_lastpoint_move(pc, 10*nudge, 0); // shift
                    else pen_lastpoint_move(pc, nudge, 0); // no shift
                }
                ret = TRUE;
            }
            break;
        case GDK_KEY_Down: // move last point down
        case GDK_KEY_KP_Down:
            if (!MOD__CTRL(event)) { // not ctrl
                if (MOD__ALT(event)) { // alt
                    if (MOD__SHIFT(event)) pen_lastpoint_move_screen(pc, 0, -10); // shift
                    else pen_lastpoint_move_screen(pc, 0, -1); // no shift
                }
                else { // no alt
                    if (MOD__SHIFT(event)) pen_lastpoint_move(pc, 0, -10*nudge); // shift
                    else pen_lastpoint_move(pc, 0, -nudge); // no shift
                }
                ret = TRUE;
            }
            break;

/*TODO: this is not yet enabled?? looks like some traces of the Geometry tool
        case GDK_KEY_P:
        case GDK_KEY_p:
            if (MOD__SHIFT_ONLY(event)) {
                sp_pen_context_wait_for_LPE_mouse_clicks(pc, Inkscape::LivePathEffect::PARALLEL, 2);
                ret = TRUE;
            }
            break;

        case GDK_KEY_C:
        case GDK_KEY_c:
            if (MOD__SHIFT_ONLY(event)) {
                sp_pen_context_wait_for_LPE_mouse_clicks(pc, Inkscape::LivePathEffect::CIRCLE_3PTS, 3);
                ret = TRUE;
            }
            break;

        case GDK_KEY_B:
        case GDK_KEY_b:
            if (MOD__SHIFT_ONLY(event)) {
                sp_pen_context_wait_for_LPE_mouse_clicks(pc, Inkscape::LivePathEffect::PERP_BISECTOR, 2);
                ret = TRUE;
            }
            break;

        case GDK_KEY_A:
        case GDK_KEY_a:
            if (MOD__SHIFT_ONLY(event)) {
                sp_pen_context_wait_for_LPE_mouse_clicks(pc, Inkscape::LivePathEffect::ANGLE_BISECTOR, 3);
                ret = TRUE;
            }
            break;
*/

        case GDK_KEY_U:
        case GDK_KEY_u:
            if (MOD__SHIFT_ONLY(event)) {
                pen_lastpoint_tocurve(pc);
                ret = TRUE;
            }
            break;
        case GDK_KEY_L:
        case GDK_KEY_l:
            if (MOD__SHIFT_ONLY(event)) {
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
            if (MOD__CTRL_ONLY(event) && pc->npoints != 0) {
                // if drawing, cancel, otherwise pass it up for undo
                pen_cancel (pc);
                ret = TRUE;
            }
            break;
        case GDK_KEY_g:
        case GDK_KEY_G:
            if (MOD__SHIFT_ONLY(event)) {
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
                Geom::CubicBezier const * cubic = NULL;
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
                cubic = dynamic_cast<Geom::CubicBezier const *>(crv);
                if ( cubic ) {
                    pc->p[1] = (*cubic)[1];
                } else {
                    pc->p[1] = pc->p[0];
                }
                //spanish: asignamos el valor a un tercio de distancia del último segmento.
                if(pc->bspline){
                    pc->p[1] = pc->p[0] + (1./3)*(pc->p[3] - pc->p[0]);
                }

                Geom::Point const pt((pc->npoints < 4
                                     ? (Geom::Point)(crv->finalPoint())
                                     : pc->p[3]));
                
                pc->npoints = 2;
                //spanish: eliminamos el último segmento de la curva verde
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
                //spanish: asignamos el valor de pc->p[1] a el opuesto de el ultimo segmento de la línea verde
                if(pc->spiro){
                    cubic = dynamic_cast<Geom::CubicBezier const *>(pc->green_curve->last_segment());
                    if ( cubic ) {
                        pc->p[1] = (*cubic)[3] + (Geom::Point)((*cubic)[3] - (*cubic)[2]);
                        SP_CTRL(pc->c1)->moveto(pc->p[0]);
                    } else {
                        pc->p[1] = pc->p[0];
                    }
                }
                sp_canvas_item_hide(pc->cl0);
                sp_canvas_item_hide(pc->cl1);
                pc->state = PenTool::POINT;
                spdc_pen_set_subsequent_point(pc, pt, true);
                pen_last_paraxial_dir = !pen_last_paraxial_dir;
                //spanish: redibujamos
                bspline_spiro_build(pc);
                ret = TRUE;
            }
            break;
        default:
            break;
    }
    return ret;
}

static void spdc_reset_colors(PenTool *pc)
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


static void spdc_pen_set_initial_point(PenTool *const pc, Geom::Point const p)
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
static void spdc_pen_set_angle_distance_status_message(PenTool *const pc, Geom::Point const p, int pc_point_to_compare, gchar const *message)
{
    g_assert(pc != NULL);
    g_assert((pc_point_to_compare == 0) || (pc_point_to_compare == 3)); // exclude control handles
    g_assert(message != NULL);

    SPDesktop *desktop = SP_EVENT_CONTEXT(pc)->desktop;
    Geom::Point rel = p - pc->p[pc_point_to_compare];
    Inkscape::Util::Quantity q = Inkscape::Util::Quantity(Geom::L2(rel), "px");
    GString *dist = g_string_new(q.string(desktop->namedview->doc_units).c_str());
    double angle = atan2(rel[Geom::Y], rel[Geom::X]) * 180 / M_PI;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/options/compassangledisplay/value", 0) != 0) {
        angle = 90 - angle;
        if (angle < 0) {
            angle += 360;
        }
    }

    pc->message_context->setF(Inkscape::IMMEDIATE_MESSAGE, message, angle, dist->str);
    g_string_free(dist, FALSE);
}


//spanish: esta función cambia los colores rojo,verde y azul haciendolos transparentes o no en función de si se usa spiro
static void bspline_spiro_color(PenTool *const pc)
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


static void bspline_spiro(PenTool *const pc, bool shift)
{
    if(!pc->spiro && !pc->bspline)
        return;

    shift?bspline_spiro_off(pc):bspline_spiro_on(pc);
    bspline_spiro_build(pc);
}

static void bspline_spiro_on(PenTool *const pc)
{
    if(!pc->red_curve->is_empty()){
        using Geom::X;
        using Geom::Y;
        pc->npoints = 5;
        pc->p[0] = pc->red_curve->first_segment()->initialPoint();
        pc->p[3] = pc->red_curve->first_segment()->finalPoint();
        pc->p[2] = pc->p[3] + (1./3)*(pc->p[0] - pc->p[3]);
        pc->p[2] = Geom::Point(pc->p[2][X] + 0.005,pc->p[2][Y] + 0.005);
    }
}

static void bspline_spiro_off(PenTool *const pc)
{
    if(!pc->red_curve->is_empty()){
        pc->npoints = 5;
        pc->p[0] = pc->red_curve->first_segment()->initialPoint();
        pc->p[3] = pc->red_curve->first_segment()->finalPoint();
        pc->p[2] = pc->p[3];
    }
}

static void bspline_spiro_start_anchor(PenTool *const pc, bool shift)
{
    LivePathEffect::LPEBSpline *lpe_bsp = NULL;

    if (SP_IS_LPE_ITEM(pc->white_item) && SP_LPE_ITEM(pc->white_item)->hasPathEffect()){
        Inkscape::LivePathEffect::Effect* thisEffect = SP_LPE_ITEM(pc->white_item)->getPathEffectOfType(Inkscape::LivePathEffect::BSPLINE);
        if(thisEffect){
            lpe_bsp = dynamic_cast<LivePathEffect::LPEBSpline*>(thisEffect->getLPEObj()->get_lpe());
        }
    }
    if(lpe_bsp){
        pc->bspline = true;
    }else{
        pc->bspline = false;
    }
    LivePathEffect::LPESpiro *lpe_spi = NULL;

    if (SP_IS_LPE_ITEM(pc->white_item) && SP_LPE_ITEM(pc->white_item)->hasPathEffect()){
        Inkscape::LivePathEffect::Effect* thisEffect = SP_LPE_ITEM(pc->white_item)->getPathEffectOfType(Inkscape::LivePathEffect::SPIRO);
        if(thisEffect){
            lpe_spi = dynamic_cast<LivePathEffect::LPESpiro*>(thisEffect->getLPEObj()->get_lpe());
        }
    }
    if(lpe_spi){
        pc->spiro = true;
    }else{
        pc->spiro = false;
    }
    if(!pc->spiro && !pc->bspline)
        return;

    if(pc->sa->curve->is_empty())
        return;

    if(shift)
        bspline_spiro_start_anchor_off(pc);
    else
        bspline_spiro_start_anchor_on(pc);
}

static void bspline_spiro_start_anchor_on(PenTool *const pc)
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
    C = Geom::Point(C[X] + 0.005,C[Y] + 0.005);
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

static void bspline_spiro_start_anchor_off(PenTool *const pc)
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

static void bspline_spiro_motion(PenTool *const pc, bool shift){
    if(!pc->spiro && !pc->bspline)
        return;

    using Geom::X;
    using Geom::Y;
    if(pc->red_curve->is_empty()) return;
    pc->npoints = 5;
    SPCurve *tmpCurve = new SPCurve();
    pc->p[2] = pc->p[3] + (1./3)*(pc->p[0] - pc->p[3]);
    pc->p[2] = Geom::Point(pc->p[2][X] + 0.005,pc->p[2][Y] + 0.005);
    if(pc->green_curve->is_empty() && !pc->sa){
        pc->p[1] = pc->p[0] + (1./3)*(pc->p[3] - pc->p[0]);
        pc->p[1] = Geom::Point(pc->p[1][X] + 0.005,pc->p[1][Y] + 0.005);
    }else if(!pc->green_curve->is_empty()){
        tmpCurve = pc->green_curve->copy();
    }else{
        tmpCurve = pc->sa->curve->copy();
        if(pc->sa->start)
            tmpCurve = tmpCurve->create_reverse();
    }

    if(!tmpCurve->is_empty()){
        Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const*>(&*tmpCurve->last_segment());
        if(cubic){
            if(pc->bspline){
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
                if(!Geom::are_near(pc->p[1],pc->p[0]))
                    pc->p[1] = Geom::Point(pc->p[1][X] + 0.005,pc->p[1][Y] + 0.005);
                if(shift)
                    pc->p[2] = pc->p[3];
            }else{
                pc->p[1] =  (*cubic)[3] + (Geom::Point)((*cubic)[3] - (*cubic)[2] );
                pc->p[1] = Geom::Point(pc->p[1][X] + 0.005,pc->p[1][Y] + 0.005);
            }
        }else{
            pc->p[1] = pc->p[0];
            if(shift)
                pc->p[2] = pc->p[3];
        }
    }

    if(pc->anchor_statusbar && !pc->red_curve->is_empty()){
        if(shift){
            bspline_spiro_end_anchor_off(pc);
        }else{
            bspline_spiro_end_anchor_on(pc);
        }
    }

    bspline_spiro_build(pc);
}

static void bspline_spiro_end_anchor_on(PenTool *const pc)
{

    using Geom::X;
    using Geom::Y;
    pc->p[2] = pc->p[3] + (1./3)*(pc->p[0] - pc->p[3]);
    pc->p[2] = Geom::Point(pc->p[2][X] + 0.005,pc->p[2][Y] + 0.005);
    SPCurve *tmpCurve = new SPCurve();
    SPCurve *lastSeg = new SPCurve();
    Geom::Point C(0,0);
    if(!pc->sa || pc->sa->curve->is_empty()){
        tmpCurve = pc->green_curve->create_reverse();
        if(pc->green_curve->get_segment_count()==0)return;
        Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const*>(&*tmpCurve->last_segment());
        if(pc->bspline){
            C = tmpCurve->last_segment()->finalPoint() + (1./3)*(tmpCurve->last_segment()->initialPoint() - tmpCurve->last_segment()->finalPoint());
            C = Geom::Point(C[X] + 0.005,C[Y] + 0.005);
        }else{
            C =  pc->p[3] + (Geom::Point)(pc->p[3] - pc->p[2] );
        }
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
    }else {
        tmpCurve = pc->sa->curve->copy();
        if(!pc->sa->start) tmpCurve = tmpCurve->create_reverse();
        Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const*>(&*tmpCurve->last_segment());
        if(pc->bspline){
            C = tmpCurve->last_segment()->finalPoint() + (1./3)*(tmpCurve->last_segment()->initialPoint() - tmpCurve->last_segment()->finalPoint());
            C = Geom::Point(C[X] + 0.005,C[Y] + 0.005);
        }else{
            C =  pc->p[3] + (Geom::Point)(pc->p[3] - pc->p[2] );
        }
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

static void bspline_spiro_end_anchor_off(PenTool *const pc)
{

    SPCurve *tmpCurve = new SPCurve();
    SPCurve *lastSeg = new SPCurve();
    pc->p[2] = pc->p[3];
    if(!pc->sa || pc->sa->curve->is_empty()){

        tmpCurve = pc->green_curve->create_reverse();
        if(pc->green_curve->get_segment_count()==0)return;
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
    }else {
        tmpCurve = pc->sa->curve->copy();
        if(!pc->sa->start) tmpCurve = tmpCurve->create_reverse();
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


//spanish: preparates the curves for its trasformation into BSline curves.
static void bspline_spiro_build(PenTool *const pc)
{
    if(!pc->spiro && !pc->bspline)
        return;

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
        //spanish: cerramos la curva si estan cerca los puntos finales de la curva
        if(Geom::are_near(curve->first_path()->initialPoint(), curve->last_path()->finalPoint())){
            curve->closepath_current();
        }
        //TODO: CALL TO CLONED FUNCTION SPIRO::doEffect IN lpe-spiro.cpp
        //For example
        //using namespace Inkscape::LivePathEffect;
        //LivePathEffectObject *lpeobj = static_cast<LivePathEffectObject*> (curve);
        //Effect *spr = static_cast<Effect*> ( new LPEbspline(lpeobj) );
        //spr->doEffect(curve);
        if(pc->bspline){
            bspline_doEffect(curve);
        }else{
            spiro_doEffect(curve);
        }

        sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->blue_bpath), curve);   
        sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(pc->blue_bpath), pc->blue_color, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
        sp_canvas_item_show(pc->blue_bpath);
        curve->unref();
        pc->blue_curve->reset();
        //We hide the holders that doesn't contribute anything
        if(pc->spiro){
            sp_canvas_item_show(pc->c1);
            SP_CTRL(pc->c1)->moveto(pc->p[0]);
        }else
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
    //spanish: comentado en funcion "doEffect" de src/live_effects/lpe-bspline.cpp
    if(curve->get_segment_count() < 2)
        return;
    Geom::PathVector const original_pathv = curve->get_pathvector();
    curve->reset();

    for(Geom::PathVector::const_iterator path_it = original_pathv.begin(); path_it != original_pathv.end(); ++path_it) {
        if (path_it->empty())
            continue;

        Geom::Path::const_iterator curve_it1 = path_it->begin();      // incoming curve
        Geom::Path::const_iterator curve_it2 = ++(path_it->begin());         // outgoing curve
        Geom::Path::const_iterator curve_endit = path_it->end_default(); // this determines when the loop has to stop
        SPCurve *nCurve = new SPCurve();
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
        if (path_it->closed()) {
            const Geom::Curve &closingline = path_it->back_closed(); // the closing line segment is always of type Geom::LineSegment.
            if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
                curve_endit = path_it->end_open();
            }
        }
        while ( curve_it2 != curve_endit )
        {
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
            SPCurve *lineHelper = new SPCurve();
            lineHelper->moveto(pointAt2);
            lineHelper->lineto(nextPointAt1);
            SBasisHelper  = lineHelper->first_segment()->toSBasis();
            lineHelper->reset();
            delete lineHelper;
            previousNode = node;
            node = SBasisHelper.valueAt(0.5);
            SPCurve *curveHelper = new SPCurve();
            curveHelper->moveto(previousNode);
            curveHelper->curveto(pointAt1, pointAt2, node);
            nCurve->append_continuous(curveHelper, 0.0625);
            curveHelper->reset();
            delete curveHelper;
            ++curve_it1;
            ++curve_it2;
        }
        SPCurve *curveHelper = new SPCurve();
        curveHelper->moveto(node);
        Geom::Point startNode(0,0);
        if (path_it->closed()) {
            SPCurve * start = new SPCurve();
            start->moveto(path_it->begin()->initialPoint());
            start->lineto(path_it->begin()->finalPoint());
            Geom::D2< Geom::SBasis > SBasisStart = start->first_segment()->toSBasis();
            SPCurve *lineHelper = new SPCurve();
            cubic = dynamic_cast<Geom::CubicBezier const*>(&*path_it->begin());
            if(cubic){
                lineHelper->moveto(SBasisStart.valueAt(Geom::nearest_point((*cubic)[1],*start->first_segment())));
            }else{
                lineHelper->moveto(start->first_segment()->initialPoint());
            }
            start->reset();
            delete start;

            SPCurve * end = new SPCurve();
            end->moveto(curve_it1->initialPoint());
            end->lineto(curve_it1->finalPoint());
            Geom::D2< Geom::SBasis > SBasisEnd = end->first_segment()->toSBasis();
            cubic = dynamic_cast<Geom::CubicBezier const*>(&*curve_it1);
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
            startNode = SBasisHelper.valueAt(0.5);
            curveHelper->curveto(nextPointAt1, nextPointAt2, startNode);
            nCurve->append_continuous(curveHelper, 0.0625);
            nCurve->move_endpoints(startNode,startNode);
        }else{
            SPCurve * start = new SPCurve();
            start->moveto(path_it->begin()->initialPoint());
            start->lineto(path_it->begin()->finalPoint());
            startNode = start->first_segment()->initialPoint();
            start->reset();
            delete start;
            curveHelper->curveto(nextPointAt1, nextPointAt2, nextPointAt3);
            nCurve->append_continuous(curveHelper, 0.0625);
            nCurve->move_endpoints(startNode,nextPointAt3);
        }
        curveHelper->reset();
        delete curveHelper;
        if (path_it->closed()) {
            nCurve->closepath_current();
        }
        curve->append(nCurve,false);
        nCurve->reset();
        delete nCurve;
    }
}

//Spiro function cloned from lpe-spiro.cpp
//spanish: comentado en funcion "doEffect" de src/live_effects/lpe-spiro.cpp
static void spiro_doEffect(SPCurve * curve)
{
    using Geom::X;
    using Geom::Y;

    Geom::PathVector const original_pathv = curve->get_pathvector();
    guint len = curve->get_segment_count() + 2;

    curve->reset();
    Spiro::spiro_cp *path = g_new (Spiro::spiro_cp, len);
    int ip = 0;

    for(Geom::PathVector::const_iterator path_it = original_pathv.begin(); path_it != original_pathv.end(); ++path_it) {
        if (path_it->empty())
            continue;

        {
            Geom::Point p = path_it->front().pointAt(0);
            path[ip].x = p[X];
            path[ip].y = p[Y];
            path[ip].ty = '{' ;
            ip++;
        }

        Geom::Path::const_iterator curve_it1 = path_it->begin();
        Geom::Path::const_iterator curve_it2 = ++(path_it->begin());

        Geom::Path::const_iterator curve_endit = path_it->end_default();
        if (path_it->closed()) {
            const Geom::Curve &closingline = path_it->back_closed(); 
            if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
                curve_endit = path_it->end_open();
            }
        }

        while ( curve_it2 != curve_endit )
        {
            Geom::Point p = curve_it1->finalPoint();
            path[ip].x = p[X];
            path[ip].y = p[Y];

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

        Geom::Point p = curve_it1->finalPoint();
        path[ip].x = p[X];
        path[ip].y = p[Y];
        if (path_it->closed()) {
            Geom::NodeType nodetype = Geom::get_nodetype(*curve_it1, path_it->front());
            switch (nodetype) {
                case Geom::NODE_NONE:
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
            path[ip].ty = '}';
            ip++;
        }

        int sp_len = ip;
        Spiro::spiro_run(path, sp_len, *curve);
        ip = 0;
    }

    g_free (path);
}

static void spdc_pen_set_subsequent_point(PenTool *const pc, Geom::Point const p, bool statusbar, guint status)
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
        if (pc->p[1] != pc->p[0] || pc->spiro) {
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
        if(pc->spiro || pc->bspline){
            message = is_curve ?
            _("<b>Curve segment</b>: angle %3.2f&#176;, distance %s; with <b>Shift</b> to cusp node, <b>Enter</b> to finish the path" ):
            _("<b>Line segment</b>: angle %3.2f&#176;, distance %s; with <b>Shift</b> to cusp node, <b>Enter</b> to finish the path");        
        }
        spdc_pen_set_angle_distance_status_message(pc, p, 0, message);
    }
}

static void spdc_pen_set_ctrl(PenTool *const pc, Geom::Point const p, guint const state)
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
        if ( ( ( pc->mode == PenTool::MODE_CLICK ) && ( state & GDK_CONTROL_MASK ) ) ||
             ( ( pc->mode == PenTool::MODE_DRAG ) &&  !( state & GDK_SHIFT_MASK  ) ) ) {
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

static void spdc_pen_finish_segment(PenTool *const pc, Geom::Point const p, guint const state)
{
    if (pc->polylines_paraxial) {
        pen_last_paraxial_dir = pen_next_paraxial_direction(pc, p, pc->p[0], state);
    }

    ++pc->num_clicks;

    if (!pc->red_curve->is_empty()) {
        bspline_spiro(pc,(state & GDK_SHIFT_MASK));
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

static void spdc_pen_finish(PenTool *const pc, gboolean const closed)
{
    if (pc->expecting_clicks_for_LPE > 1) {
        // don't let the path be finished before we have collected the required number of mouse clicks
        return;
    }

    pc->num_clicks = 0;
    pen_disable_events(pc);

    SPDesktop *const desktop = pc->desktop;
    pc->message_context->clear();
    desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Drawing finished"));
    if(pc->spiro || pc->bspline) pc->blue_curve->reset();
    //spanish para cancelar linea sin un segmento creado
    pc->red_curve->reset();
    spdc_concat_colors_and_flush(pc, closed);
    pc->sa = NULL;
    pc->ea = NULL;

    pc->npoints = 0;
    pc->state = PenTool::POINT;

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

static void pen_disable_events(PenTool *const pc) {
  pc->events_disabled++;
}

static void pen_enable_events(PenTool *const pc) {
  g_return_if_fail(pc->events_disabled != 0);

  pc->events_disabled--;
}

void sp_pen_context_wait_for_LPE_mouse_clicks(PenTool *pc, Inkscape::LivePathEffect::EffectType effect_type,
                                         unsigned int num_clicks, bool use_polylines)
{
    if (effect_type == Inkscape::LivePathEffect::INVALID_LPE)
        return;

    pc->waiting_LPE_type = effect_type;
    pc->expecting_clicks_for_LPE = num_clicks;
    pc->polylines_only = use_polylines;
    pc->polylines_paraxial = false; // TODO: think if this is correct for all cases
}

void sp_pen_context_cancel_waiting_for_LPE(PenTool *pc)
{
    pc->waiting_LPE_type = Inkscape::LivePathEffect::INVALID_LPE;
    pc->expecting_clicks_for_LPE = 0;
    sp_pen_context_set_polyline_mode(pc);
}

static int pen_next_paraxial_direction(const PenTool *const pc,
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

void pen_set_to_nearest_horiz_vert(const PenTool *const pc, Geom::Point &pt, guint const state, bool snap)
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

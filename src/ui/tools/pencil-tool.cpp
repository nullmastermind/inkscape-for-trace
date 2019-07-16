// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * Pencil event context implementation.
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
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gdk/gdkkeysyms.h>

#include "ui/tools/pencil-tool.h"
#include <2geom/bezier-utils.h>
#include <2geom/sbasis-to-bezier.h>
#include <2geom/svg-path-parser.h>

#include "desktop.h"
#include "inkscape.h"

#include "context-fns.h"
#include "desktop-style.h"
#include "message-context.h"
#include "message-stack.h"
#include "selection-chemistry.h"
#include "selection.h"
#include "snap.h"

#include "display/canvas-bpath.h"
#include "display/curve.h"
#include "display/sp-canvas.h"

#include "live_effects/lpe-powerstroke-interpolators.h"
#include "live_effects/lpe-powerstroke.h"

#include "object/sp-lpe-item.h"
#include "object/sp-path.h"
#include "style.h"

#include "ui/pixmaps/cursor-pencil.xpm"

#include "svg/svg.h"

#include "ui/draw-anchor.h"
#include "ui/tool/event-utils.h"

#include "xml/node.h"
#include "xml/sp-css-attr.h"
#include <glibmm/i18n.h>
// #include <thread>
// #include <chrono>

namespace Inkscape {
namespace UI {
namespace Tools {

static Geom::Point pencil_drag_origin_w(0, 0);
static bool pencil_within_tolerance = false;

static bool in_svg_plane(Geom::Point const &p) { return Geom::LInfty(p) < 1e18; }
const double HANDLE_CUBIC_GAP = 0.01;

const std::string& PencilTool::getPrefsPath() {
	return PencilTool::prefsPath;
}

const std::string PencilTool::prefsPath = "/tools/freehand/pencil";

PencilTool::PencilTool()
    : FreehandBase(cursor_pencil_xpm)
    , p()
    , _npoints(0)
    , _state(SP_PENCIL_CONTEXT_IDLE)
    , _req_tangent(0, 0)
    , _is_drawing(false)
    , sketch_n(0)
    , _curve(nullptr)
{
}

void PencilTool::setup() {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/tools/freehand/pencil/selcue")) {
        this->enableSelectionCue();
    }
    this->_curve = new SPCurve();

    FreehandBase::setup();

    this->_is_drawing = false;
    this->anchor_statusbar = false;
}



PencilTool::~PencilTool() {
    if (this->_curve) {
        this->_curve->unref();
    }
}

void PencilTool::_extinput(GdkEvent *event) {
    if (gdk_event_get_axis (event, GDK_AXIS_PRESSURE, &this->pressure)) {
        this->pressure = CLAMP (this->pressure, DDC_MIN_PRESSURE, DDC_MAX_PRESSURE);
        is_tablet = true;
    } else {
        this->pressure = DDC_DEFAULT_PRESSURE;
        is_tablet = false;
    }
}

/** Snaps new node relative to the previous node. */
void PencilTool::_endpointSnap(Geom::Point &p, guint const state) {
    if ((state & GDK_CONTROL_MASK)) { //CTRL enables constrained snapping
        if (this->_npoints > 0) {
            spdc_endpoint_snap_rotation(this, p, this->p[0], state);
        }
    } else {
        if (!(state & GDK_SHIFT_MASK)) { //SHIFT disables all snapping, except the angular snapping above
                                         //After all, the user explicitly asked for angular snapping by
                                         //pressing CTRL
            boost::optional<Geom::Point> origin = this->_npoints > 0 ? this->p[0] : boost::optional<Geom::Point>();
            spdc_endpoint_snap_free(this, p, origin, state);
        }
    }
}

/**
 * Callback for handling all pencil context events.
 */
bool PencilTool::root_handler(GdkEvent* event) {
    bool ret = false;
    this->_extinput(event);
    switch (event->type) {
        case GDK_BUTTON_PRESS:
            ret = this->_handleButtonPress(event->button);
            break;

        case GDK_MOTION_NOTIFY:
            ret = this->_handleMotionNotify(event->motion);
            break;

        case GDK_BUTTON_RELEASE:
            ret = this->_handleButtonRelease(event->button);
            break;

        case GDK_KEY_PRESS:
            ret = this->_handleKeyPress(event->key);
            break;

        case GDK_KEY_RELEASE:
            ret = this->_handleKeyRelease(event->key);
            break;

        default:
            break;
    }
    if (!ret) {
        ret = FreehandBase::root_handler(event);
    }

    return ret;
}

bool PencilTool::_handleButtonPress(GdkEventButton const &bevent) {
    bool ret = false;
    if ( bevent.button == 1  && !this->space_panning) {
        Inkscape::Selection *selection = desktop->getSelection();

        if (Inkscape::have_viable_layer(desktop, defaultMessageContext()) == false) {
            return true;
        }

        if (!this->grab) {
            /* Grab mouse, so release will not pass unnoticed */
            this->grab = SP_CANVAS_ITEM(desktop->acetate);
            sp_canvas_item_grab(this->grab, ( GDK_KEY_PRESS_MASK | GDK_BUTTON_PRESS_MASK   |
                                            GDK_BUTTON_RELEASE_MASK |
                                            GDK_POINTER_MOTION_MASK  ),
                                nullptr, bevent.time);
        }

        Geom::Point const button_w(bevent.x, bevent.y);

        /* Find desktop coordinates */
        Geom::Point p = this->desktop->w2d(button_w);

        /* Test whether we hit any anchor. */
        SPDrawAnchor *anchor = spdc_test_inside(this, button_w);

        pencil_drag_origin_w = Geom::Point(bevent.x,bevent.y);
        pencil_within_tolerance = true;
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        tablet_enabled = prefs->getBool("/tools/freehand/pencil/pressure", false);
        switch (this->_state) {
            case SP_PENCIL_CONTEXT_ADDLINE:
                /* Current segment will be finished with release */
                ret = true;
                break;
            default:
                /* Set first point of sequence */
                SnapManager &m = desktop->namedview->snap_manager;
                if (bevent.state & GDK_CONTROL_MASK) {
                    m.setup(desktop, true);
                    if (!(bevent.state & GDK_SHIFT_MASK)) {
                        m.freeSnapReturnByRef(p, Inkscape::SNAPSOURCE_NODE_HANDLE);
                      }
                    spdc_create_single_dot(this, p, "/tools/freehand/pencil", bevent.state);
                    m.unSetup();
                    ret = true;
                    break;
                }
                if (anchor) {
                    p = anchor->dp;
                    //Put the start overwrite curve always on the same direction
                    if (anchor->start) {
                        this->sa_overwrited =  anchor->curve->create_reverse();
                    } else {
                        this->sa_overwrited =  anchor->curve->copy();
                    }
                    desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Continuing selected path"));
                } else {
                    m.setup(desktop, true);
                    if (tablet_enabled && this->pressure) {
                        // This is the first click of a new curve; deselect item so that
                        // this curve is not combined with it (unless it is drawn from its
                        // anchor, which is handled by the sibling branch above)
                        selection->clear();
                        desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Creating new path"));
                    } else if (!(bevent.state & GDK_SHIFT_MASK) ) {
                        // This is the first click of a new curve; deselect item so that
                        // this curve is not combined with it (unless it is drawn from its
                        // anchor, which is handled by the sibling branch above)
                        selection->clear();
                        desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Creating new path"));
                        m.freeSnapReturnByRef(p, Inkscape::SNAPSOURCE_NODE_HANDLE);
                    } else if (selection->singleItem() && SP_IS_PATH(selection->singleItem())) {
                        desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Appending to selected path"));
                        m.freeSnapReturnByRef(p, Inkscape::SNAPSOURCE_NODE_HANDLE);
                    }
                    m.unSetup();
                }
                this->sa = anchor;
                this->_setStartpoint(p);
                ret = true;
                break;
        }

        this->_is_drawing = true;
    }
    return ret;
}

bool PencilTool::_handleMotionNotify(GdkEventMotion const &mevent) {
    if ((mevent.state & GDK_CONTROL_MASK) && (mevent.state & GDK_BUTTON1_MASK)) {
        // mouse was accidentally moved during Ctrl+click;
        // ignore the motion and create a single point
        this->_is_drawing = false;
        return true;
    }
    bool ret = false;

    if (this->space_panning || (mevent.state & GDK_BUTTON2_MASK) || (mevent.state & GDK_BUTTON3_MASK)) {
        // allow scrolling
        return ret;
    }

    /* Test whether we hit any anchor. */
    SPDrawAnchor *anchor = spdc_test_inside(this, pencil_drag_origin_w);
    if (this->pressure == 0.0 && tablet_enabled && !anchor) {
        // tablet event was accidentally fired without press;
        return ret;
    }
    
    if ( ( mevent.state & GDK_BUTTON1_MASK ) && !this->grab && this->_is_drawing) {
        /* Grab mouse, so release will not pass unnoticed */
        this->grab = SP_CANVAS_ITEM(desktop->acetate);
        sp_canvas_item_grab(this->grab, ( GDK_KEY_PRESS_MASK | GDK_BUTTON_PRESS_MASK   |
                                        GDK_BUTTON_RELEASE_MASK |
                                        GDK_POINTER_MOTION_MASK  ),
                            nullptr, mevent.time);
    }

    /* Find desktop coordinates */
    Geom::Point p = desktop->w2d(Geom::Point(mevent.x, mevent.y));
    
    
    
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (pencil_within_tolerance) {
        gint const tolerance = prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100);
        if ( Geom::LInfty( Geom::Point(mevent.x,mevent.y) - pencil_drag_origin_w ) < tolerance ) {
            return false;   // Do not drag if we're within tolerance from origin.
        }
    }

    // Once the user has moved farther than tolerance from the original location
    // (indicating they intend to move the object, not click), then always process the
    // motion notify coordinates as given (no snapping back to origin)
    pencil_within_tolerance = false;

    anchor = spdc_test_inside(this, Geom::Point(mevent.x,mevent.y));


    switch (this->_state) {
        case SP_PENCIL_CONTEXT_ADDLINE:
            if (is_tablet) {
                this->_state = SP_PENCIL_CONTEXT_FREEHAND;
                return false;
            }
            /* Set red endpoint */
            if (anchor) {
                p = anchor->dp;
            } else {
                Geom::Point ptnr(p);
                this->_endpointSnap(ptnr, mevent.state);
                p = ptnr;
            }
            this->_setEndpoint(p);
            ret = true;
            break;
        default:
            /* We may be idle or already freehand */
            if ( (mevent.state & GDK_BUTTON1_MASK) && this->_is_drawing ) {
                if (this->_state == SP_PENCIL_CONTEXT_IDLE) {
                    sp_event_context_discard_delayed_snap_event(this);
                }
                this->_state = SP_PENCIL_CONTEXT_FREEHAND;

                if ( !this->sa && !this->green_anchor ) {
                    /* Create green anchor */
                    this->green_anchor = sp_draw_anchor_new(this, this->green_curve, TRUE, this->p[0]);
                }
                if (anchor) {
                    p = anchor->dp;
                }
                if ( this->_npoints != 0) { // buttonpress may have happened before we entered draw context!
                    if (this->ps.empty()) {
                        // Only in freehand mode we have to add the first point also to this->ps (apparently)
                        // - We cannot add this point in spdc_set_startpoint, because we only need it for freehand
                        // - We cannot do this in the button press handler because at that point we don't know yet
                        //   whether we're going into freehand mode or not
                        this->ps.push_back(this->p[0]);
                        if (tablet_enabled) {
                            this->_wps.push_back(0);
                        }
                    }
                    this->_addFreehandPoint(p, mevent.state);
                    ret = true;
                }
                if (anchor && !this->anchor_statusbar) {
                    this->message_context->set(Inkscape::NORMAL_MESSAGE, _("<b>Release</b> here to close and finish the path."));
                    this->anchor_statusbar = true;
                    this->ea = anchor;
                } else if (!anchor && this->anchor_statusbar) {
                    this->message_context->clear();
                    this->anchor_statusbar = false;
                    this->ea = nullptr;
                } else if (!anchor) {
                    this->message_context->set(Inkscape::NORMAL_MESSAGE, _("Drawing a freehand path"));
                    this->ea = nullptr;
                }

            } else {
                if (anchor && !this->anchor_statusbar) {
                    this->message_context->set(Inkscape::NORMAL_MESSAGE, _("<b>Drag</b> to continue the path from this point."));
                    this->anchor_statusbar = true;
                } else if (!anchor && this->anchor_statusbar) {
                    this->message_context->clear();
                    this->anchor_statusbar = false;
                }
            }

            // Show the pre-snap indicator to communicate to the user where we would snap to if he/she were to
            // a) press the mousebutton to start a freehand drawing, or
            // b) release the mousebutton to finish a freehand drawing
            if (!tablet_enabled && !this->sp_event_context_knot_mouseover()) {
                SnapManager &m = desktop->namedview->snap_manager;
                m.setup(desktop, true);
                m.preSnap(Inkscape::SnapCandidatePoint(p, Inkscape::SNAPSOURCE_NODE_HANDLE));
                m.unSetup();
            }
            break;
    }
    return ret;
}

bool PencilTool::_handleButtonRelease(GdkEventButton const &revent) {
    bool ret = false;

    if ( revent.button == 1 && this->_is_drawing && !this->space_panning) {
        this->_is_drawing = false;

        /* Find desktop coordinates */
        Geom::Point p = desktop->w2d(Geom::Point(revent.x, revent.y));

        /* Test whether we hit any anchor. */
        SPDrawAnchor *anchor = spdc_test_inside(this, Geom::Point(revent.x, revent.y));

        switch (this->_state) {
            case SP_PENCIL_CONTEXT_IDLE:
                /* Releasing button in idle mode means single click */
                /* We have already set up start point/anchor in button_press */
                if (!(revent.state & GDK_CONTROL_MASK) && !is_tablet) {
                    // Ctrl+click creates a single point so only set context in ADDLINE mode when Ctrl isn't pressed
                    this->_state = SP_PENCIL_CONTEXT_ADDLINE;
                }
                /*Or select the down item if we are in tablet mode*/
                if (is_tablet) {
                    using namespace Inkscape::LivePathEffect;
                    SPItem * item = sp_event_context_find_item (desktop, Geom::Point(revent.x, revent.y), FALSE, FALSE);
                    if (item && (!this->white_item || item != white_item)) {
                        Effect* lpe = SP_LPE_ITEM(item)->getCurrentLPE();
                        if (lpe) {
                            LPEPowerStroke* ps = static_cast<LPEPowerStroke*>(lpe);
                            if (ps) {
                                desktop->selection->clear();
                                desktop->selection->add(item);
                            }
                        }
                    }
                }
                break;
            case SP_PENCIL_CONTEXT_ADDLINE:
                /* Finish segment now */
                if (anchor) {
                    p = anchor->dp;
                } else {
                    this->_endpointSnap(p, revent.state);
                }
                this->ea = anchor;
                this->_setEndpoint(p);
                this->_finishEndpoint();
                this->_state = SP_PENCIL_CONTEXT_IDLE;
                sp_event_context_discard_delayed_snap_event(this);
                break;
            case SP_PENCIL_CONTEXT_FREEHAND:
                if (revent.state & GDK_MOD1_MASK && !tablet_enabled) {
                    /* sketch mode: interpolate the sketched path and improve the current output path with the new interpolation. don't finish sketch */
                    this->_sketchInterpolate();

                    if (this->green_anchor) {
                        this->green_anchor = sp_draw_anchor_destroy(this->green_anchor);
                    }

                    this->_state = SP_PENCIL_CONTEXT_SKETCH;
                } else {
                    /* Finish segment now */
                    /// \todo fixme: Clean up what follows (Lauris)
                    if (anchor) {
                        p = anchor->dp;
                    } else {
                        Geom::Point p_end = p;
                        if (!tablet_enabled) {
                            this->_endpointSnap(p_end, revent.state);
                            if (p_end != p) {
                                // then we must have snapped!
                                this->_addFreehandPoint(p_end, revent.state);
                            }
                        }
                    }

                    this->ea = anchor;
                    /* Write curves to object */
                    desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Finishing freehand"));
                    this->_interpolate();
                    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
                    if (tablet_enabled) {
                        gint prevmode = prefs->getInt("/tools/freehand/pencil/freehand-mode", 0);
                        prefs->setInt("/tools/freehand/pencil/freehand-mode", 0);
                        spdc_concat_colors_and_flush(this, FALSE);
                        prefs->setInt("/tools/freehand/pencil/freehand-mode", prevmode);
                    } else {
                        spdc_concat_colors_and_flush(this, FALSE);
                    }
                    this->points.clear();
                    this->sa = nullptr;
                    this->ea = nullptr;
                    this->ps.clear();
                    this->_wps.clear();
                    if (this->green_anchor) {
                        this->green_anchor = sp_draw_anchor_destroy(this->green_anchor);
                    }
                    this->_state = SP_PENCIL_CONTEXT_IDLE;
                    // reset sketch mode too
                    this->sketch_n = 0;
                }
                break;
            case SP_PENCIL_CONTEXT_SKETCH:
            default:
                break;
        }

        if (this->grab) {
            /* Release grab now */
            sp_canvas_item_ungrab(this->grab);
            this->grab = nullptr;
        }

        ret = true;
    }
    return ret;
}

void PencilTool::_cancel() {
    if (this->grab) {
        /* Release grab now */
        sp_canvas_item_ungrab(this->grab);
        this->grab = nullptr;
    }

    this->_is_drawing = false;
    this->_state = SP_PENCIL_CONTEXT_IDLE;
    sp_event_context_discard_delayed_snap_event(this);

    this->red_curve->reset();
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(this->red_bpath), nullptr);
    for (auto i:this->green_bpaths) {
        sp_canvas_item_destroy(i);
    }
    this->green_bpaths.clear();
    this->green_curve->reset();
    if (this->green_anchor) {
        this->green_anchor = sp_draw_anchor_destroy(this->green_anchor);
    }

    this->message_context->clear();
    this->message_context->flash(Inkscape::NORMAL_MESSAGE, _("Drawing cancelled"));

    this->desktop->canvas->endForcedFullRedraws();
}

bool PencilTool::_handleKeyPress(GdkEventKey const &event) {
    bool ret = false;

    switch (get_latin_keyval(&event)) {
        case GDK_KEY_Up:
        case GDK_KEY_Down:
        case GDK_KEY_KP_Up:
        case GDK_KEY_KP_Down:
            // Prevent the zoom field from activation.
            if (!Inkscape::UI::held_only_control(event)) {
                ret = true;
            }
            break;
        case GDK_KEY_Escape:
            if (this->_npoints != 0) {
                // if drawing, cancel, otherwise pass it up for deselecting
                if (this->_state != SP_PENCIL_CONTEXT_IDLE) {
                    this->_cancel();
                    ret = true;
                }
            }
            break;
        case GDK_KEY_z:
        case GDK_KEY_Z:
            if (Inkscape::UI::held_only_control(event) && this->_npoints != 0) {
                // if drawing, cancel, otherwise pass it up for undo
                if (this->_state != SP_PENCIL_CONTEXT_IDLE) {
                    this->_cancel();
                    ret = true;
                }
            }
            break;
        case GDK_KEY_g:
        case GDK_KEY_G:
            if (Inkscape::UI::held_only_shift(event)) {
                this->desktop->selection->toGuides();
                ret = true;
            }
            break;
        case GDK_KEY_Alt_L:
        case GDK_KEY_Alt_R:
        case GDK_KEY_Meta_L:
        case GDK_KEY_Meta_R:
            if (this->_state == SP_PENCIL_CONTEXT_IDLE) {
                this->desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("<b>Sketch mode</b>: holding <b>Alt</b> interpolates between sketched paths. Release <b>Alt</b> to finalize."));
            }
            break;
        default:
            break;
    }
    return ret;
}

bool PencilTool::_handleKeyRelease(GdkEventKey const &event) {
    bool ret = false;

    switch (get_latin_keyval(&event)) {
        case GDK_KEY_Alt_L:
        case GDK_KEY_Alt_R:
        case GDK_KEY_Meta_L:
        case GDK_KEY_Meta_R:
            if (this->_state == SP_PENCIL_CONTEXT_SKETCH) {
                spdc_concat_colors_and_flush(this, FALSE);
                this->sketch_n = 0;
                this->sa = nullptr;
                this->ea = nullptr;
                if (this->green_anchor) {
                    this->green_anchor = sp_draw_anchor_destroy(this->green_anchor);
                }
                this->_state = SP_PENCIL_CONTEXT_IDLE;
                sp_event_context_discard_delayed_snap_event(this);
                this->desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Finishing freehand sketch"));
                ret = true;
            }
            break;
        default:
            break;
    }
    return ret;
}

/**
 * Reset points and set new starting point.
 */
void PencilTool::_setStartpoint(Geom::Point const &p) {
    this->_npoints = 0;
    this->red_curve_is_valid = false;
    if (in_svg_plane(p)) {
        this->p[this->_npoints++] = p;
    }
}

/**
 * Change moving endpoint position.
 * <ul>
 * <li>Ctrl constrains to moving to H/V direction, snapping in given direction.
 * <li>Otherwise we snap freely to whatever attractors are available.
 * </ul>
 *
 * Number of points is (re)set to 2 always, 2nd point is modified.
 * We change RED curve.
 */
void PencilTool::_setEndpoint(Geom::Point const &p) {
    if (this->_npoints == 0) {
        return;
        /* May occur if first point wasn't in SVG plane (e.g. weird w2d transform, perhaps from bad
         * zoom setting).
         */
    }
    g_return_if_fail( this->_npoints > 0 );

    this->red_curve->reset();
    if ( ( p == this->p[0] )
         || !in_svg_plane(p) )
    {
        this->_npoints = 1;
    } else {
        this->p[1] = p;
        this->_npoints = 2;

        this->red_curve->moveto(this->p[0]);
        this->red_curve->lineto(this->p[1]);
        this->red_curve_is_valid = true;
        if (!tablet_enabled) {
            sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(this->red_bpath), this->red_curve);
        }
    }
}

/**
 * Finalize addline.
 *
 * \todo
 * fixme: I'd like remove red reset from concat colors (lauris).
 * Still not sure, how it will make most sense.
 */
void PencilTool::_finishEndpoint() {
    if (this->red_curve->is_unset() || 
        this->red_curve->first_point() == this->red_curve->second_point())
    {
        this->red_curve->reset();
        sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(this->red_bpath), nullptr);
    } else {
        /* Write curves to object. */
        spdc_concat_colors_and_flush(this, FALSE);
        this->sa = nullptr;
        this->ea = nullptr;
    }
}

static inline double square(double const x) { return x * x; }

void PencilTool::addPowerStrokePencil(bool force, gint tolsimplify)
{

    static int pscounter = 16;
    if (pscounter > 15 || force) {
        pscounter = 0;
    } else {
        pscounter++;
        return;
    }
    using namespace Inkscape::LivePathEffect;

    if (this->_curve && this->ps.size() > 1) {
        // Example og work with std::future
        // Retain for other works
        /* std::future_status status;
        bool stop = false;
        bool nofuture = false;
        try {
            status = future.wait_for(std::chrono::seconds(0));
        } catch (const std::future_error& e) {
            stop = true;
            if (e.code() == std::future_errc::no_state) {
                nofuture = true;
            } else {
                std::cout << "Caught a future_error with code \"" << e.code()
                << nofuture << future.valid() << "\"\nMessage: \"" << e.what() << "\"\n";
            }
        }
        if (nofuture || status == std::future_status::ready) {
            if (!stop && status == std::future_status::ready) { */

        SPDocument *document = SP_ACTIVE_DOCUMENT;
        if (!document) {
            return;
        }
        const gchar *id = "power_stroke_preview";
        SPObject *toremove = document->getObjectById(id);
        using namespace Inkscape::LivePathEffect;
        if (toremove) {
            toremove->getRepr()->setAttribute("id", "tmp_power_stroke_preview");
        }
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        double tol = prefs->getDoubleLimited("/tools/freehand/pencil/base-simplify", 25.0, 0.0, 100.0) * 0.4;
        double tolerance_sq = 0.02 * square(this->desktop->w2d().descrim() * tol) * exp(0.2 * tol - 2);
        int n_points = this->ps.size();
        // worst case gives us a segment per point
        int max_segs = 4 * n_points;
        std::vector<Geom::Point> b(max_segs);
        SPCurve *curvepressure = new SPCurve();

        int const n_segs = Geom::bezier_fit_cubic_r(b.data(), this->ps.data(), n_points, tolerance_sq, max_segs);
        if (n_segs > 0) {
            /* Fit and draw and reset state */
            curvepressure->moveto(b[0]);
            for (int c = 0; c < n_segs; c++) {
                curvepressure->curveto(b[4 * c + 1], b[4 * c + 2], b[4 * c + 3]);
            }
        }
        Geom::Path path = curvepressure->get_pathvector()[0];
        int original_size = path.size();
        if (!path.empty()) {
            Geom::Affine transform_coordinate = SP_ITEM(SP_ACTIVE_DESKTOP->currentLayer())->i2dt_affine().inverse();
            path *= transform_coordinate;
            // std::vector<Geom::Point> points_preview = this->points;
            // points_preview.push_back(Geom::Point(path.size() - 1, points_preview[points_preview.size()-1][Geom::Y]));
            // future = std::async(std::launch::async, [path, points_preview] {
            using namespace Inkscape::LivePathEffect;
            SPDocument *document = SP_ACTIVE_DOCUMENT;
            if (!document) {
                return;
                // return true;
            }
            Inkscape::XML::Document *xml_doc = document->getReprDoc();
            Inkscape::XML::Node *pp = nullptr;
            pp = xml_doc->createElement("svg:path");
            pp->setAttribute("sodipodi:insensitive", "true");
            gchar *pvector_str = sp_svg_write_path(path);
            if (pvector_str) {
                pp->setAttribute("d", pvector_str);
                g_free(pvector_str);
            }
            pp->setAttribute("id", "power_stroke_preview");
            Inkscape::GC::release(pp);

            SPShape *powerpreview = SP_SHAPE(SP_ITEM(SP_ACTIVE_DESKTOP->currentLayer())->appendChildRepr(pp));
            SPLPEItem *lpeitem = dynamic_cast<SPLPEItem *>(powerpreview);
            if (!lpeitem) {
                return;
                // return true;
            }

            tol = prefs->getDoubleLimited("/tools/freehand/pencil/tolerance", 10.0, 0.0, 100.0) + tolsimplify;
            tol = tol / (100.0 * (102.0 - tol));
            std::ostringstream threshold;
            threshold << tol;
            Effect::createAndApply(SIMPLIFY, desktop->doc(), SP_ITEM(lpeitem));
            Effect *lpe = lpeitem->getCurrentLPE();
            if (lpe) {
                Glib::ustring pref_path = "/live_effects/simplify/smooth_angles";
                bool valid = prefs->getEntry(pref_path).isValid();
                if (!valid) {
                    lpe->getRepr()->setAttribute("smooth_angles", "0");
                }
                lpe->getRepr()->setAttribute("threshold", threshold.str());
            }
            sp_lpe_item_update_patheffect(lpeitem, false, true);
            curvepressure = powerpreview->getCurve();
            path = curvepressure->get_pathvector()[0];
            powerStrokeInterpolate(path);
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            Glib::ustring pref_path_pp = "/live_effects/powerstroke/powerpencil";
            prefs->setBool(pref_path_pp, true);
            Effect::createAndApply(POWERSTROKE, SP_ACTIVE_DESKTOP->doc(), lpeitem);
            lpe = lpeitem->getCurrentLPE();
            Inkscape::LivePathEffect::LPEPowerStroke *pspreview = static_cast<LPEPowerStroke *>(lpe);
            if (pspreview) {
                sp_lpe_item_enable_path_effects(lpeitem, false);
                Glib::ustring pref_path = "/live_effects/powerstroke/interpolator_type";
                bool valid = prefs->getEntry(pref_path).isValid();
                if (!valid) {
                    pspreview->getRepr()->setAttribute("interpolator_type", "CentripetalCatmullRom");
                }
                gint cap = prefs->getInt("/live_effects/powerstroke/powerpencilcap", 4);
                pspreview->getRepr()->setAttribute("start_linecap_type", LineCapTypeConverter.get_key(cap));
                pspreview->getRepr()->setAttribute("end_linecap_type", LineCapTypeConverter.get_key(cap));
                pspreview->getRepr()->setAttribute("sort_points", "true");
                pspreview->offset_points.param_set_and_write_new_value(this->points);
                sp_lpe_item_enable_path_effects(lpeitem, true);
                sp_lpe_item_update_patheffect(lpeitem, false, true);
                if (toremove) {
                    using namespace Inkscape::LivePathEffect;
                    Effect *lpe = SP_LPE_ITEM(toremove)->getCurrentLPE();
                    SP_LPE_ITEM(toremove)->removeCurrentPathEffect(true);
                    LivePathEffectObject *lpeobj = lpe->getLPEObj();
                    if (lpeobj) {
                        SP_OBJECT(lpeobj)->deleteObject(true);
                        lpeobj = nullptr;
                    }
                    lpe = SP_LPE_ITEM(toremove)->getCurrentLPE();
                    SP_LPE_ITEM(toremove)->removeCurrentPathEffect(true);
                    lpeobj = lpe->getLPEObj();
                    if (lpeobj) {
                        SP_OBJECT(lpeobj)->deleteObject(true);
                        lpeobj = nullptr;
                    }
                    toremove->deleteObject(true);
                    toremove = nullptr;
                }
                if (!pspreview->has_exception && path.size() != powerpreview->getCurve()->get_pathvector()[0].size()) {
                    pp->setAttribute("style", "fill:#888888;opacity:1;fill-rule:nonzero;stroke:none;");
                } else if (tolsimplify < 61) { // run 3 times
                    addPowerStrokePencil(true, tolsimplify + 10);
                }
            }
            if (curvepressure) {
                curvepressure->unref();
            }
            prefs->setBool(pref_path_pp, false);
            // return true;
            // });
        }
    }
}

void PencilTool::_addFreehandPoint(Geom::Point const &p, guint /*state*/) {
    g_assert( this->_npoints > 0 );
    g_return_if_fail(unsigned(this->_npoints) < G_N_ELEMENTS(this->p));

    if ( ( p != this->p[ this->_npoints - 1 ] )
         && in_svg_plane(p) )
    {
        this->p[this->_npoints++] = p;
        this->_fitAndSplit();
        this->ps.push_back(p);
        if (tablet_enabled) {
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            double min = prefs->getIntLimited("/tools/freehand/pencil/minpressure", 10, 0, 100) / 100.0;
            double max = prefs->getIntLimited("/tools/freehand/pencil/maxpressure", 40, 0, 100) / 100.0;
            Geom::Affine transform_coordinate = SP_ITEM(SP_ACTIVE_DESKTOP->currentLayer())->i2dt_affine();
            if (min > max) {
                min = max;
            }
            if (this->pressure < 0.25) {
                this->_wps.push_back(0);
                return;
            }
            double dezoomify_factor = 0.05 * 1000 / SP_EVENT_CONTEXT(this)->desktop->current_zoom();
            double pressure_shrunk = (((this->pressure - 0.25) * 1.25) * (max - min)) + min;
            double pressure_computed = pressure_shrunk * (dezoomify_factor / 5.0);
            this->_wps.push_back(pressure_computed);
            this->addPowerStrokePencil(false, 30);
            sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(this->red_bpath), nullptr);
            for (auto i:this->green_bpaths) {
                sp_canvas_item_destroy(i);
            }
            this->green_bpaths.clear();
        }
    }
}

void PencilTool::powerStrokeInterpolate(Geom::Path path)
{
    size_t ps_size = this->ps.size();
    if ( ps_size <= 1 ) {
        return;
    }
    using Geom::X;
    using Geom::Y;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    SPItem *item = selection ? selection->singleItem() : nullptr;
    gint points_size = this->_wps.size();
    gint path_size = path.size();
    std::vector<Geom::Point> tmp_points;
    Geom::Point previous = Geom::Point(Geom::infinity(), 0);
    size_t i = 0;
    size_t previ = 0;
    bool increase = true;
    for (auto pressure : this->_wps) {
        Geom::Point pp = Geom::Point();
        pp[Geom::X] = (path_size / (double)points_size) * i;
        pp[Geom::Y] = pressure;
        if (pressure == 0 || (path_size > 1 && (pp[Geom::X] < 1 || pp[Geom::X] > path_size - 1))) {
            ++i;
            continue;
        }
        if (std::abs(pp[Geom::X] - previous[Geom::X]) > 0.2 && std::abs(pp[Geom::Y] - previous[Geom::Y]) > 0.5) {
            if (previous[Geom::Y] < pp[Geom::Y]) {
                if (increase && tmp_points.size() > 1) {
                    tmp_points.pop_back();
                }
                increase = true;
            } else {
                if (!increase && tmp_points.size() > 1) {
                    tmp_points.pop_back();
                }
                increase = false;
            }
            previous = pp;
            tmp_points.push_back(pp);
        }

        ++i;
    }
    this->points = tmp_points;
    tmp_points.clear();
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(this->red_bpath), nullptr);
}

void PencilTool::_interpolate() {
    size_t ps_size = this->ps.size();
    if ( ps_size <= 1 ) {
        return;
    }
    
    using Geom::X;
    using Geom::Y;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double tol = prefs->getDoubleLimited("/tools/freehand/pencil/tolerance", 10.0, 1.0, 100.0) * 0.4;
    bool simplify = prefs->getInt("/tools/freehand/pencil/simplify", 0);
    if(simplify){
        double tol2 = prefs->getDoubleLimited("/tools/freehand/pencil/base-simplify", 25.0, 0.0, 100.0) * 0.4;
        tol = std::min(tol,tol2);
    }
    this->green_curve->reset();
    this->red_curve->reset();
    this->red_curve_is_valid = false;
    double tolerance_sq = square(this->desktop->w2d().descrim() * tol) * exp(0.2 * tol - 2);

    g_assert(is_zero(this->_req_tangent) || is_unit_vector(this->_req_tangent));

    int n_points = this->ps.size();

    // worst case gives us a segment per point
    int max_segs = 4 * n_points;

    std::vector<Geom::Point> b(max_segs);
    int const n_segs = Geom::bezier_fit_cubic_r(b.data(), this->ps.data(), n_points, tolerance_sq, max_segs);
    if (n_segs > 0) {
        /* Fit and draw and reset state */
        this->green_curve->moveto(b[0]);
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        guint mode = prefs->getInt("/tools/freehand/pencil/freehand-mode", 0);
        for (int c = 0; c < n_segs; c++) {
            // if we are in BSpline we modify the trace to create adhoc nodes 
            if (mode == 2) {
                Geom::Point point_at1 = b[4 * c + 0] + (1./3) * (b[4 * c + 3] - b[4 * c + 0]);
                point_at1 = Geom::Point(point_at1[X] + HANDLE_CUBIC_GAP, point_at1[Y] + HANDLE_CUBIC_GAP);
                Geom::Point point_at2 = b[4 * c + 3] + (1./3) * (b[4 * c + 0] - b[4 * c + 3]);
                point_at2 = Geom::Point(point_at2[X] + HANDLE_CUBIC_GAP, point_at2[Y] + HANDLE_CUBIC_GAP);
                this->green_curve->curveto(point_at1,point_at2,b[4*c+3]);
            } else {
                this->green_curve->curveto(b[4 * c + 1], b[4 * c + 2], b[4 * c + 3]);
            }
        }
        sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(this->red_bpath), this->green_curve);
        /* Fit and draw and copy last point */
        g_assert(!this->green_curve->is_empty());
        /* Set up direction of next curve. */
        {
            Geom::Curve const * last_seg = this->green_curve->last_segment();
            g_assert( last_seg );      // Relevance: validity of (*last_seg)
            this->p[0] = last_seg->finalPoint();
            this->_npoints = 1;
            Geom::Curve *last_seg_reverse = last_seg->reverse();
            Geom::Point const req_vec( -last_seg_reverse->unitTangentAt(0) );
            delete last_seg_reverse;
            this->_req_tangent = ( ( Geom::is_zero(req_vec) || !in_svg_plane(req_vec) )
                                ? Geom::Point(0, 0)
                                : Geom::unit_vector(req_vec) );
        }
    }
}


/* interpolates the sketched curve and tweaks the current sketch interpolation*/
void PencilTool::_sketchInterpolate() {
    if ( this->ps.size() <= 1 ) {
        return;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double tol = prefs->getDoubleLimited("/tools/freehand/pencil/tolerance", 10.0, 1.0, 100.0) * 0.4;
    bool simplify = prefs->getInt("/tools/freehand/pencil/simplify", 0);
    if(simplify){
        double tol2 = prefs->getDoubleLimited("/tools/freehand/pencil/base-simplify", 25.0, 1.0, 100.0) * 0.4;
        tol = std::min(tol,tol2);
    }
    double tolerance_sq = 0.02 * square(this->desktop->w2d().descrim() * tol) * exp(0.2 * tol - 2);
    
    bool average_all_sketches = prefs->getBool("/tools/freehand/pencil/average_all_sketches", true);

    g_assert(is_zero(this->_req_tangent) || is_unit_vector(this->_req_tangent));

    this->red_curve->reset();
    this->red_curve_is_valid = false;

    int n_points = this->ps.size();

    // worst case gives us a segment per point
    int max_segs = 4 * n_points;

    std::vector<Geom::Point> b(max_segs);

    int const n_segs = Geom::bezier_fit_cubic_r(b.data(), this->ps.data(), n_points, tolerance_sq, max_segs);

    if (n_segs > 0) {
        Geom::Path fit(b[0]);

        for (int c = 0; c < n_segs; c++) {
            fit.appendNew<Geom::CubicBezier>(b[4 * c + 1], b[4 * c + 2], b[4 * c + 3]);
        }

        Geom::Piecewise<Geom::D2<Geom::SBasis> > fit_pwd2 = fit.toPwSb();

        if (this->sketch_n > 0) {
            double t;

            if (average_all_sketches) {
                // Average = (sum of all) / n
                //         = (sum of all + new one) / n+1
                //         = ((old average)*n + new one) / n+1
                t = this->sketch_n / (this->sketch_n + 1.);
            } else {
                t = 0.5;
            }

            this->sketch_interpolation = Geom::lerp(t, fit_pwd2, this->sketch_interpolation);

            // simplify path, to eliminate small segments
            Path path;
            path.LoadPathVector(Geom::path_from_piecewise(this->sketch_interpolation, 0.01));
            path.Simplify(0.5);

            Geom::PathVector *pathv = path.MakePathVector();
            this->sketch_interpolation = (*pathv)[0].toPwSb();
            delete pathv;
        } else {
            this->sketch_interpolation = fit_pwd2;
        }

        this->sketch_n++;

        this->green_curve->reset();
        this->green_curve->set_pathvector(Geom::path_from_piecewise(this->sketch_interpolation, 0.01));
        if (!tablet_enabled) {
            sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(this->red_bpath), this->green_curve);
        }
        /* Fit and draw and copy last point */
        g_assert(!this->green_curve->is_empty());

        /* Set up direction of next curve. */
        {
            Geom::Curve const * last_seg = this->green_curve->last_segment();
            g_assert( last_seg );      // Relevance: validity of (*last_seg)
            this->p[0] = last_seg->finalPoint();
            this->_npoints = 1;
            Geom::Curve *last_seg_reverse = last_seg->reverse();
            Geom::Point const req_vec( -last_seg_reverse->unitTangentAt(0) );
            delete last_seg_reverse;
            this->_req_tangent = ( ( Geom::is_zero(req_vec) || !in_svg_plane(req_vec) )
                                ? Geom::Point(0, 0)
                                : Geom::unit_vector(req_vec) );
        }
    }

    this->ps.clear();
    this->points.clear();
    this->_wps.clear();
}

void PencilTool::_fitAndSplit() {
    g_assert( this->_npoints > 1 );

    double const tolerance_sq = 0;

    Geom::Point b[4];
    g_assert(is_zero(this->_req_tangent)
             || is_unit_vector(this->_req_tangent));
    Geom::Point const tHatEnd(0, 0);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int const n_segs = Geom::bezier_fit_cubic_full(b, nullptr, this->p, this->_npoints,
                                                this->_req_tangent, tHatEnd,
                                                tolerance_sq, 1);
    if ( n_segs > 0
         && unsigned(this->_npoints) < G_N_ELEMENTS(this->p) )
    {
        /* Fit and draw and reset state */

        this->red_curve->reset();
        this->red_curve->moveto(b[0]);
        using Geom::X;
        using Geom::Y;
            // if we are in BSpline we modify the trace to create adhoc nodes
        guint mode = prefs->getInt("/tools/freehand/pencil/freehand-mode", 0);
        if(mode == 2){
            Geom::Point point_at1 = b[0] + (1./3)*(b[3] - b[0]);
            point_at1 = Geom::Point(point_at1[X] + HANDLE_CUBIC_GAP, point_at1[Y] + HANDLE_CUBIC_GAP);
            Geom::Point point_at2 = b[3] + (1./3)*(b[0] - b[3]);
            point_at2 = Geom::Point(point_at2[X] + HANDLE_CUBIC_GAP, point_at2[Y] + HANDLE_CUBIC_GAP);
            this->red_curve->curveto(point_at1,point_at2,b[3]);
        }else{
            this->red_curve->curveto(b[1], b[2], b[3]);
        }
        if (!tablet_enabled) {
            sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(this->red_bpath), this->red_curve);
        }
        this->red_curve_is_valid = true;
    } else {
        /* Fit and draw and copy last point */

        g_assert(!this->red_curve->is_empty());

        /* Set up direction of next curve. */
        {
            Geom::Curve const * last_seg = this->red_curve->last_segment();
            g_assert( last_seg );      // Relevance: validity of (*last_seg)
            this->p[0] = last_seg->finalPoint();
            this->_npoints = 1;
            Geom::Curve *last_seg_reverse = last_seg->reverse();
            Geom::Point const req_vec( -last_seg_reverse->unitTangentAt(0) );
            delete last_seg_reverse;
            this->_req_tangent = ( ( Geom::is_zero(req_vec) || !in_svg_plane(req_vec) )
                                ? Geom::Point(0, 0)
                                : Geom::unit_vector(req_vec) );
        }


        this->green_curve->append_continuous(this->red_curve, 0.0625);
        SPCurve *curve = this->red_curve->copy();

        /// \todo fixme:
        SPCanvasItem *cshape = sp_canvas_bpath_new(this->desktop->getSketch(), curve, true);
        curve->unref();

        this->highlight_color = SP_ITEM(this->desktop->currentLayer())->highlight_color();
        if((unsigned int)prefs->getInt("/tools/nodes/highlight_color", 0xff0000ff) == this->highlight_color){
            this->green_color = 0x00ff007f;
        } else {
            this->green_color = this->highlight_color;
        }
        sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(cshape), this->green_color, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);

        this->green_bpaths.push_back(cshape);

        this->red_curve_is_valid = false;
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

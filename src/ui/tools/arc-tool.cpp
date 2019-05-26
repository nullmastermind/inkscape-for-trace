// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Ellipse drawing context.
 */
/* Authors:
 *   Mitsuru Oka
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <johan@shouraizou.nl>
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2000-2006 Authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include <glibmm/i18n.h>
#include <gdk/gdkkeysyms.h>

#include "context-fns.h"
#include "desktop-style.h"
#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "message-context.h"
#include "preferences.h"
#include "selection.h"
#include "snap.h"
#include "verbs.h"

#include "display/sp-canvas.h"
#include "display/sp-canvas-item.h"

#include "include/macros.h"

#include "object/sp-ellipse.h"
#include "object/sp-namedview.h"

#include "ui/pixmaps/cursor-ellipse.xpm"

#include "ui/tools/arc-tool.h"
#include "ui/shape-editor.h"
#include "ui/tools/tool-base.h"

#include "xml/repr.h"
#include "xml/node-event-vector.h"

using Inkscape::DocumentUndo;

namespace Inkscape {
namespace UI {
namespace Tools {

const std::string& ArcTool::getPrefsPath() {
	return ArcTool::prefsPath;
}

const std::string ArcTool::prefsPath = "/tools/shapes/arc";


ArcTool::ArcTool()
    : ToolBase(cursor_ellipse_xpm)
    , arc(nullptr)
{
}

void ArcTool::finish() {
    sp_canvas_item_ungrab(SP_CANVAS_ITEM(desktop->acetate));
    this->finishItem();
    this->sel_changed_connection.disconnect();

    ToolBase::finish();
}

ArcTool::~ArcTool() {
    this->enableGrDrag(false);

    this->sel_changed_connection.disconnect();

    delete this->shape_editor;
    this->shape_editor = nullptr;

    /* fixme: This is necessary because we do not grab */
    if (this->arc) {
        this->finishItem();
    }
}

/**
 * Callback that processes the "changed" signal on the selection;
 * destroys old and creates new knotholder.
 */
void ArcTool::selection_changed(Inkscape::Selection* selection) {
    this->shape_editor->unset_item();
    this->shape_editor->set_item(selection->singleItem());
}

void ArcTool::setup() {
    ToolBase::setup();

    Inkscape::Selection *selection = this->desktop->getSelection();

    this->shape_editor = new ShapeEditor(this->desktop);

    SPItem *item = this->desktop->getSelection()->singleItem();
    if (item) {
        this->shape_editor->set_item(item);
    }

    this->sel_changed_connection.disconnect();
    this->sel_changed_connection = selection->connectChanged(
        sigc::mem_fun(this, &ArcTool::selection_changed)
    );

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/tools/shapes/selcue")) {
        this->enableSelectionCue();
    }

    if (prefs->getBool("/tools/shapes/gradientdrag")) {
        this->enableGrDrag();
    }
}

bool ArcTool::item_handler(SPItem* item, GdkEvent* event) {
    switch (event->type) {
        case GDK_BUTTON_PRESS:
            if (event->button.button == 1 && !this->space_panning) {
                Inkscape::setup_for_drag_start(desktop, this, event);
            }
            break;
            // motion and release are always on root (why?)
        default:
            break;
    }

    return ToolBase::item_handler(item, event);
}

bool ArcTool::root_handler(GdkEvent* event) {
    static bool dragging;

    Inkscape::Selection *selection = desktop->getSelection();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    this->tolerance = prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100);

    bool handled = false;

    switch (event->type) {
        case GDK_BUTTON_PRESS:
            if (event->button.button == 1 && !this->space_panning) {
                dragging = true;

                this->center = Inkscape::setup_for_drag_start(desktop, this, event);

                /* Snap center */
                SnapManager &m = desktop->namedview->snap_manager;
                m.setup(desktop);
                m.freeSnapReturnByRef(this->center, Inkscape::SNAPSOURCE_NODE_HANDLE);

                sp_canvas_item_grab(SP_CANVAS_ITEM(desktop->acetate),
                                    GDK_KEY_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                                    GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_PRESS_MASK,
                                    nullptr, event->button.time);
                handled = true;
                m.unSetup();
            }
            break;
        case GDK_MOTION_NOTIFY:
            if (dragging && (event->motion.state & GDK_BUTTON1_MASK) && !this->space_panning) {
                if ( this->within_tolerance
                     && ( abs( (gint) event->motion.x - this->xp ) < this->tolerance )
                     && ( abs( (gint) event->motion.y - this->yp ) < this->tolerance ) ) {
                    break; // do not drag if we're within tolerance from origin
                }
                // Once the user has moved farther than tolerance from the original location
                // (indicating they intend to draw, not click), then always process the
                // motion notify coordinates as given (no snapping back to origin)
                this->within_tolerance = false;

                Geom::Point const motion_w(event->motion.x, event->motion.y);
                Geom::Point motion_dt(desktop->w2d(motion_w));

                this->drag(motion_dt, event->motion.state);

                gobble_motion_events(GDK_BUTTON1_MASK);

                handled = true;
            } else if (!this->sp_event_context_knot_mouseover()){
                SnapManager &m = desktop->namedview->snap_manager;
                m.setup(desktop);

                Geom::Point const motion_w(event->motion.x, event->motion.y);
                Geom::Point motion_dt(desktop->w2d(motion_w));
                m.preSnap(Inkscape::SnapCandidatePoint(motion_dt, Inkscape::SNAPSOURCE_NODE_HANDLE));
                m.unSetup();
            }
            break;
        case GDK_BUTTON_RELEASE:
            this->xp = this->yp = 0;
            if (event->button.button == 1 && !this->space_panning) {
                dragging = false;
                sp_event_context_discard_delayed_snap_event(this);

                if (!this->within_tolerance) {
                    // we've been dragging, finish the arc
                    this->finishItem();
                } else if (this->item_to_select) {
                    // no dragging, select clicked item if any
                    if (event->button.state & GDK_SHIFT_MASK) {
                        selection->toggle(this->item_to_select);
                    } else {
                        selection->set(this->item_to_select);
                    }
                } else {
                    // click in an empty space
                    selection->clear();
                }

                this->xp = 0;
                this->yp = 0;
                this->item_to_select = nullptr;
                handled = true;
            }
            sp_canvas_item_ungrab(SP_CANVAS_ITEM(desktop->acetate));
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
                    if (!dragging) {
                        sp_event_show_modifier_tip(this->defaultMessageContext(), event,
                                                   _("<b>Ctrl</b>: make circle or integer-ratio ellipse, snap arc/segment angle"),
                                                   _("<b>Shift</b>: draw around the starting point"),
                                                   nullptr);
                    }
                    break;

                case GDK_KEY_x:
                case GDK_KEY_X:
                    if (MOD__ALT_ONLY(event)) {
                        desktop->setToolboxFocusTo ("arc-rx");
                        handled = true;
                    }
                    break;

                case GDK_KEY_Escape:
                    if (dragging) {
                        dragging = false;
                        sp_event_context_discard_delayed_snap_event(this);
                        // if drawing, cancel, otherwise pass it up for deselecting
                        this->cancel();
                        handled = true;
                    }
                    break;

                case GDK_KEY_space:
                    if (dragging) {
                        sp_canvas_item_ungrab(SP_CANVAS_ITEM(desktop->acetate));
                        dragging = false;
                        sp_event_context_discard_delayed_snap_event(this);

                        if (!this->within_tolerance) {
                            // we've been dragging, finish the arc
                            this->finishItem();
                        }
                        // do not return true, so that space would work switching to selector
                    }
                    break;

                case GDK_KEY_Delete:
                case GDK_KEY_KP_Delete:
                case GDK_KEY_BackSpace:
                    handled = this->deleteSelectedDrag(MOD__CTRL_ONLY(event));
                    break;

                default:
                    break;
            }
            break;

        case GDK_KEY_RELEASE:
            switch (event->key.keyval) {
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

    if (!handled) {
    	handled = ToolBase::root_handler(event);
    }

    return handled;
}

void ArcTool::drag(Geom::Point pt, guint state) {
    if (!this->arc) {
        if (Inkscape::have_viable_layer(desktop, defaultMessageContext()) == false) {
            return;
        }

        // Create object
        Inkscape::XML::Document *xml_doc = desktop->doc()->getReprDoc();
        Inkscape::XML::Node *repr = xml_doc->createElement("svg:path");
        repr->setAttribute("sodipodi:type", "arc");

        // Set style
        sp_desktop_apply_style_tool(desktop, repr, "/tools/shapes/arc", false);

        this->arc = SP_GENERICELLIPSE(desktop->currentLayer()->appendChildRepr(repr));
        Inkscape::GC::release(repr);
        this->arc->transform = SP_ITEM(desktop->currentLayer())->i2doc_affine().inverse();
        this->arc->updateRepr();

        desktop->canvas->forceFullRedrawAfterInterruptions(5);
    }

    bool ctrl_save = false;

    if ((state & GDK_MOD1_MASK) && (state & GDK_CONTROL_MASK) && !(state & GDK_SHIFT_MASK)) {
        // if Alt is pressed without Shift in addition to Control, temporarily drop the CONTROL mask
        // so that the ellipse is not constrained to integer ratios
        ctrl_save = true;
        state = state ^ GDK_CONTROL_MASK;
    }

    Geom::Rect r = Inkscape::snap_rectangular_box(desktop, this->arc, pt, this->center, state);

    if (ctrl_save) {
        state = state ^ GDK_CONTROL_MASK;
    }

    Geom::Point dir = r.dimensions() / 2;

    if (state & GDK_MOD1_MASK) {
        /* With Alt let the ellipse pass through the mouse pointer */
        Geom::Point c = r.midpoint();

        if (!ctrl_save) {
            if (fabs(dir[Geom::X]) > 1E-6 && fabs(dir[Geom::Y]) > 1E-6) {
                Geom::Affine const i2d ( (this->arc)->i2dt_affine() );
                Geom::Point new_dir = pt * i2d - c;
                new_dir[Geom::X] *= dir[Geom::Y] / dir[Geom::X];
                double lambda = new_dir.length() / dir[Geom::Y];
                r = Geom::Rect (c - lambda*dir, c + lambda*dir);
            }
        } else {
            /* with Alt+Ctrl (without Shift) we generate a perfect circle
               with diameter click point <--> mouse pointer */
                double l = dir.length();
                Geom::Point d (l, l);
                r = Geom::Rect (c - d, c + d);
        }
    }

    this->arc->position_set(
        r.midpoint()[Geom::X], r.midpoint()[Geom::Y],
        r.dimensions()[Geom::X] / 2, r.dimensions()[Geom::Y] / 2);

    double rdimx = r.dimensions()[Geom::X];
    double rdimy = r.dimensions()[Geom::Y];

    Inkscape::Util::Quantity rdimx_q = Inkscape::Util::Quantity(rdimx, "px");
    Inkscape::Util::Quantity rdimy_q = Inkscape::Util::Quantity(rdimy, "px");
    Glib::ustring xs = rdimx_q.string(desktop->namedview->display_units);
    Glib::ustring ys = rdimy_q.string(desktop->namedview->display_units);

    if (state & GDK_CONTROL_MASK) {
        int ratio_x, ratio_y;
        bool is_golden_ratio = false;

        if (fabs (rdimx) > fabs (rdimy)) {
            if (fabs(rdimx / rdimy - goldenratio) < 1e-6) {
                is_golden_ratio = true;
            }

            ratio_x = (int) rint (rdimx / rdimy);
            ratio_y = 1;
        } else {
            if (fabs(rdimy / rdimx - goldenratio) < 1e-6) {
                is_golden_ratio = true;
            }

            ratio_x = 1;
            ratio_y = (int) rint (rdimy / rdimx);
        }

        if (!is_golden_ratio) {
            this->message_context->setF(Inkscape::IMMEDIATE_MESSAGE,
                    _("<b>Ellipse</b>: %s &#215; %s (constrained to ratio %d:%d); with <b>Shift</b> to draw around the starting point"),
                    xs.c_str(), ys.c_str(), ratio_x, ratio_y);
        } else {
            if (ratio_y == 1) {
                this->message_context->setF(Inkscape::IMMEDIATE_MESSAGE,
                        _("<b>Ellipse</b>: %s &#215; %s (constrained to golden ratio 1.618 : 1); with <b>Shift</b> to draw around the starting point"),
                        xs.c_str(), ys.c_str());
            } else {
                this->message_context->setF(Inkscape::IMMEDIATE_MESSAGE,
                        _("<b>Ellipse</b>: %s &#215; %s (constrained to golden ratio 1 : 1.618); with <b>Shift</b> to draw around the starting point"),
                        xs.c_str(), ys.c_str());
            }
        }
    } else {
        this->message_context->setF(Inkscape::IMMEDIATE_MESSAGE, _("<b>Ellipse</b>: %s &#215; %s; with <b>Ctrl</b> to make circle, integer-ratio, or golden-ratio ellipse; with <b>Shift</b> to draw around the starting point"), xs.c_str(), ys.c_str());
    }
}

void ArcTool::finishItem() {
    this->message_context->clear();

    if (this->arc != nullptr) {
        if (this->arc->rx.computed == 0 || this->arc->ry.computed == 0) {
            this->cancel(); // Don't allow the creating of zero sized arc, for example when the start and and point snap to the snap grid point
            return;
        }

        this->arc->updateRepr();
        this->arc->doWriteTransform(this->arc->transform, nullptr, true);

        desktop->canvas->endForcedFullRedraws();

        desktop->getSelection()->set(this->arc);

        DocumentUndo::done(desktop->getDocument(), SP_VERB_CONTEXT_ARC, _("Create ellipse"));

        this->arc = nullptr;
    }
}

void ArcTool::cancel() {
    desktop->getSelection()->clear();
    sp_canvas_item_ungrab(SP_CANVAS_ITEM(desktop->acetate));

    if (this->arc != nullptr) {
        this->arc->deleteObject();
        this->arc = nullptr;
    }

    this->within_tolerance = false;
    this->xp = 0;
    this->yp = 0;
    this->item_to_select = nullptr;

    desktop->canvas->endForcedFullRedraws();

    DocumentUndo::cancel(desktop->getDocument());
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

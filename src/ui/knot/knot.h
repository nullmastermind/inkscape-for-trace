// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_KNOT_H
#define SEEN_SP_KNOT_H

/** \file
 * Declarations for SPKnot: Desktop-bound visual control object.
 */
/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/point.h>
#include <sigc++/sigc++.h>
#include <glibmm/ustring.h>

#include "knot-enums.h"
#include "display/control/canvas-item-enums.h"
#include "enums.h"

class SPDesktop;
class SPItem;

typedef struct _GdkCursor GdkCursor;
typedef union _GdkEvent GdkEvent;
typedef unsigned int guint32;

#define SP_KNOT(obj) (dynamic_cast<SPKnot*>(static_cast<SPKnot*>(obj)))
#define SP_IS_KNOT(obj) (dynamic_cast<const SPKnot*>(static_cast<const SPKnot*>(obj)) != NULL)

namespace Inkscape {
class CanvasItemCtrl;
}

/**
 * Desktop-bound visual control object.
 *
 * A knot is a draggable object, with callbacks to change something by
 * dragging it, visuably represented by a canvas item (mostly square).
 *
 * See also KnotHolderEntity.
 * See also ControlPoint (which does the same kind of things).
 */
class SPKnot {
public:
    SPKnot(SPDesktop *desktop, char const *tip, Inkscape::CanvasItemCtrlType type, Glib::ustring const & name = Glib::ustring("unknown"));
    virtual ~SPKnot();

    SPKnot(SPKnot const&) = delete;
    SPKnot& operator=(SPKnot const&) = delete;

    int ref_count; // FIXME encapsulation

    SPDesktop *desktop  = nullptr;                  /**< Desktop we are on. */
    Inkscape::CanvasItemCtrl *ctrl = nullptr;       /**< Our CanvasItemCtrl. */
    SPItem *owner       = nullptr;                  /**< Optional Owner Item */
    SPItem *sub_owner   = nullptr;                  /**< Optional SubOwner Item */
    unsigned int flags  = SP_KNOT_VISIBLE;

    unsigned int size   = 9;                        /**< Always square. Must be odd. */
    bool size_set       = false;                    /**< Use default size unless explicitly set. */
    double angle        = 0.0;                      /**< Angle of mesh handle. */
    Geom::Point pos;                                /**< Our desktop coordinates. */
    Geom::Point grabbed_rel_pos;                    /**< Grabbed relative position. */
    Geom::Point drag_origin;                        /**< Origin of drag. */
    SPAnchorType anchor = SP_ANCHOR_CENTER;         /**< Anchor. */

    bool grabbed        = false;
    bool moved          = false;
    int  xp             = 0.0;                      /**< Where drag started */
    int  yp             = 0.0;                      /**< Where drag started */
    int  tolerance      = 0;
    bool within_tolerance = false;
    bool transform_escaped = false; // true iff resize or rotate was cancelled by esc.

    Inkscape::CanvasItemCtrlShape shape = Inkscape::CANVAS_ITEM_CTRL_SHAPE_SQUARE;   /**< Shape type. */
    bool shape_set      = false;                    /**< Use default shape unless explicitly set. */
    Inkscape::CanvasItemCtrlMode mode = Inkscape::CANVAS_ITEM_CTRL_MODE_XOR;

    guint32 fill[SP_KNOT_VISIBLE_STATES];
    guint32 stroke[SP_KNOT_VISIBLE_STATES];
    unsigned char *image[SP_KNOT_VISIBLE_STATES];

    GdkCursor *cursor[SP_KNOT_VISIBLE_STATES];

    GdkCursor *saved_cursor = nullptr;
    void* pixbuf            = nullptr;

    char *tip               = nullptr;

    sigc::connection _event_connection;

    double pressure         = 0.0;    /**< The tablet pen pressure when the knot is being dragged. */

    // FIXME: signals should NOT need to emit the object they came from, the callee should
    // be able to figure that out
    sigc::signal<void, SPKnot *, unsigned int> click_signal;
    sigc::signal<void, SPKnot*, unsigned int> doubleclicked_signal;
    sigc::signal<void, SPKnot*, unsigned int> mousedown_signal;
    sigc::signal<void, SPKnot*, unsigned int> grabbed_signal;
    sigc::signal<void, SPKnot *, unsigned int> ungrabbed_signal;
    sigc::signal<void, SPKnot *, Geom::Point const &, unsigned int> moved_signal;
    sigc::signal<bool, SPKnot*, GdkEvent*> event_signal;

    sigc::signal<bool, SPKnot*, Geom::Point*, unsigned int> request_signal;


    //TODO: all the members above should eventualle become private, accessible via setters/getters
    void setSize(unsigned int i);
    void setShape(Inkscape::CanvasItemCtrlShape s);
    void setAnchor(unsigned int i);
    void setMode(Inkscape::CanvasItemCtrlMode m);
    void setPixbuf(void* p);
    void setAngle(double i);

    void setFill(guint32 normal, guint32 mouseover, guint32 dragging, guint32 selected);
    void setStroke(guint32 normal, guint32 mouseover, guint32 dragging, guint32 selected);
    void setImage(unsigned char* normal, unsigned char* mouseover, unsigned char* dragging, unsigned char* selected);

    void setCursor(GdkCursor* normal, GdkCursor* mouseover, GdkCursor* dragging, GdkCursor* selected);

    /**
     * Show knot on its canvas.
     */
    void show();

    /**
     * Hide knot on its canvas.
     */
    void hide();

    /**
     * Set flag in knot, with side effects.
     */
    void setFlag(unsigned int flag, bool set);

    /**
     * Update knot's pixbuf and set its control state.
     */
    void updateCtrl();

    /**
     * Request or set new position for knot.
     */
    void requestPosition(Geom::Point const &pos, unsigned int state);

    /**
     * Update knot for dragging and tell canvas an item was grabbed.
     */
    void startDragging(Geom::Point const &p, int x, int y, guint32 etime);

    /**
     * Move knot to new position and emits "moved" signal.
     */
    void setPosition(Geom::Point const &p, unsigned int state);

    /**
     * Move knot to new position, without emitting a MOVED signal.
     */
    void moveto(Geom::Point const &p);
    /**
     * Select knot.
     */
    void selectKnot(bool select);

    /**
     * Returns position of knot.
     */
    Geom::Point position() const;

    /**
     * Event handler (from CanvasItem's).
     */
    bool eventHandler(GdkEvent *event);

    bool is_visible()   { return (flags & SP_KNOT_VISIBLE)   != 0; }
    bool is_selected()  { return (flags & SP_KNOT_SELECTED)  != 0; }
    bool is_mouseover() { return (flags & SP_KNOT_MOUSEOVER) != 0; }
    bool is_dragging()  { return (flags & SP_KNOT_DRAGGING)  != 0; }
    bool is_grabbed()   { return (flags & SP_KNOT_GRABBED)   != 0; }

private:
    /**
     * Set knot control state (dragging/mouseover/normal).
     */
    void _setCtrlState();
};

void knot_ref(SPKnot* knot);
void knot_unref(SPKnot* knot);

void sp_knot_handler_request_position(GdkEvent *event, SPKnot *knot);

#endif // SEEN_SP_KNOT_H

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

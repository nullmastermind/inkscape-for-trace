// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Selection and transformation context
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2010      authors
 * Copyright (C) 2006      Johan Engelen <johan@shouraizou.nl>
 * Copyright (C) 1999-2005 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#include <cstring>
#include <string>

#include <gtkmm/widget.h>
#include <gdk/gdkkeysyms.h>
#include <glibmm/i18n.h>

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "include/macros.h"
#include "message-stack.h"
#include "rubberband.h"
#include "selection-chemistry.h"
#include "selection-describer.h"
#include "selection.h"
#include "seltrans.h"

#include "display/drawing-item.h"
#include "display/control/canvas-item-catchall.h"
#include "display/control/canvas-item-drawing.h"

#include "object/box3d.h"
#include "style.h"

#include "ui/cursor-utils.h"
#include "ui/modifiers.h"

#include "ui/tools-switch.h"
#include "ui/tools/select-tool.h"

#include "ui/widget/canvas.h"

#ifdef WITH_DBUS
#include "extension/dbus/document-interface.h"
#endif


using Inkscape::DocumentUndo;
using Inkscape::Modifiers::Modifier;

namespace Inkscape {
namespace UI {
namespace Tools {

static gint rb_escaped = 0; // if non-zero, rubberband was canceled by esc, so the next button release should not deselect
static gint drag_escaped = 0; // if non-zero, drag was canceled by esc

const std::string& SelectTool::getPrefsPath() {
    return SelectTool::prefsPath;
}

const std::string SelectTool::prefsPath = "/tools/select";

SelectTool::SelectTool()
    : ToolBase("select.svg")
    , dragging(false)
    , moved(false)
    , button_press_state(0)
    , cycling_wrap(true)
    , item(nullptr)
    , _seltrans(nullptr)
    , _describer(nullptr)
{
}

//static gint xp = 0, yp = 0; // where drag started
//static gint tolerance = 0;
//static bool within_tolerance = false;
static bool is_cycling = false;


SelectTool::~SelectTool() {
    this->enableGrDrag(false);

    if (grabbed) {
        grabbed->ungrab();
        grabbed = nullptr;
    }

    delete this->_seltrans;
    this->_seltrans = nullptr;

    delete this->_describer;
    this->_describer = nullptr;
    g_free(no_selection_msg);

    if (item) {
        sp_object_unref(item);
        item = nullptr;
    }

    forced_redraws_stop();
}

void SelectTool::setup() {
    ToolBase::setup();

    auto select_click = Modifier::get(Modifiers::Type::SELECT_ADD_TO)->get_label();
    auto select_scroll = Modifier::get(Modifiers::Type::SELECT_CYCLE)->get_label();

    // cursors in select context
    Gtk::Widget *w = desktop->getCanvas();
    if (w->get_window()) {
        // Window may not be open when tool is setup for the first time!
        _cursor_mouseover = load_svg_cursor(w->get_display(), w->get_window(), "select-mouseover.svg");
        _cursor_dragging  = load_svg_cursor(w->get_display(), w->get_window(), "select-dragging.svg");
        // Need to reload select.svg.
        load_svg_cursor(w->get_display(), w->get_window(), "select.svg");
    }
    
    no_selection_msg = g_strdup_printf(
        _("No objects selected. Click, %s+click, %s+scroll mouse on top of objects, or drag around objects to select."),
        select_click.c_str(), select_scroll.c_str());

    this->_describer = new Inkscape::SelectionDescriber(
                desktop->selection, 
                desktop->messageStack(),
                _("Click selection again to toggle scale/rotation handles."),
                no_selection_msg);

    this->_seltrans = new Inkscape::SelTrans(desktop);

    sp_event_context_read(this, "show");
    sp_event_context_read(this, "transform");

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    if (prefs->getBool("/tools/select/gradientdrag")) {
        this->enableGrDrag();
    }
}

void SelectTool::set(const Inkscape::Preferences::Entry& val) {
    Glib::ustring path = val.getEntryName();

    if (path == "show") {
        if (val.getString() == "outline") {
            this->_seltrans->setShow(Inkscape::SelTrans::SHOW_OUTLINE);
        } else {
            this->_seltrans->setShow(Inkscape::SelTrans::SHOW_CONTENT);
        }
    }
}

bool SelectTool::sp_select_context_abort() {
    Inkscape::SelTrans *seltrans = this->_seltrans;

    if (this->dragging) {
        if (this->moved) { // cancel dragging an object
            seltrans->ungrab();
            this->moved = FALSE;
            this->dragging = FALSE;
            sp_event_context_discard_delayed_snap_event(this);
            drag_escaped = 1;

            if (this->item) {
                // only undo if the item is still valid
                if (this->item->document) {
                    DocumentUndo::undo(desktop->getDocument());
                }

                sp_object_unref( this->item, nullptr);
            }
            this->item = nullptr;

            desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Move canceled."));
            return true;
        }
    } else {
        if (Inkscape::Rubberband::get(desktop)->is_started()) {
            Inkscape::Rubberband::get(desktop)->stop();
            rb_escaped = 1;
            defaultMessageContext()->clear();
            desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Selection canceled."));
            return true;
        }
    }
    return false;
}

static bool
key_is_a_modifier (guint key) {
    return (key == GDK_KEY_Alt_L ||
                key == GDK_KEY_Alt_R ||
                key == GDK_KEY_Control_L ||
                key == GDK_KEY_Control_R ||
                key == GDK_KEY_Shift_L ||
                key == GDK_KEY_Shift_R ||
                key == GDK_KEY_Meta_L ||  // Meta is when you press Shift+Alt (at least on my machine)
            key == GDK_KEY_Meta_R);
}

static void
sp_select_context_up_one_layer(SPDesktop *desktop)
{
    /* Click in empty place, go up one level -- but don't leave a layer to root.
     *
     * (Rationale: we don't usually allow users to go to the root, since that
     * detracts from the layer metaphor: objects at the root level can in front
     * of or behind layers.  Whereas it's fine to go to the root if editing
     * a document that has no layers (e.g. a non-Inkscape document).)
     *
     * Once we support editing SVG "islands" (e.g. <svg> embedded in an xhtml
     * document), we might consider further restricting the below to disallow
     * leaving a layer to go to a non-layer.
     */
    SPObject *const current_layer = desktop->currentLayer();
    if (current_layer) {
        SPObject *const parent = current_layer->parent;
        SPGroup *current_group = dynamic_cast<SPGroup *>(current_layer);
        if ( parent
             && ( parent->parent
                  || !( current_group
                        && ( SPGroup::LAYER == current_group->layerMode() ) ) ) )
        {
            desktop->setCurrentLayer(parent);
            if (current_group && (SPGroup::LAYER != current_group->layerMode())) {
                desktop->getSelection()->set(current_layer);
            }
        }
    }
}

bool SelectTool::item_handler(SPItem* item, GdkEvent* event) {
    gint ret = FALSE;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    tolerance = prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100);

    // make sure we still have valid objects to move around
    if (this->item && this->item->document == nullptr) {
        this->sp_select_context_abort();
    }

    switch (event->type) {
        case GDK_BUTTON_PRESS:
            if (event->button.button == 1 && !this->space_panning) {
                /* Left mousebutton */

                // save drag origin
                xp = (gint) event->button.x;
                yp = (gint) event->button.y;
                within_tolerance = true;

                // remember what modifiers were on before button press
                this->button_press_state = event->button.state;
                bool first_hit = Modifier::get(Modifiers::Type::SELECT_FIRST_HIT)->active(this->button_press_state);
                bool force_drag = Modifier::get(Modifiers::Type::SELECT_FORCE_DRAG)->active(this->button_press_state);
                bool always_box = Modifier::get(Modifiers::Type::SELECT_ALWAYS_BOX)->active(this->button_press_state);
                bool touch_path = Modifier::get(Modifiers::Type::SELECT_TOUCH_PATH)->active(this->button_press_state);

                // if shift or ctrl was pressed, do not move objects;
                // pass the event to root handler which will perform rubberband, shift-click, ctrl-click, ctrl-drag
                if (!(always_box || first_hit || touch_path)) {

                    this->dragging = TRUE;
                    this->moved = FALSE;

                    auto window = desktop->getCanvas()->get_window();
                    window->set_cursor(_cursor_dragging);

                    // remember the clicked item in this->item:
                    if (this->item) {
                        sp_object_unref(this->item, nullptr);
                        this->item = nullptr;
                    }

                    this->item = sp_event_context_find_item (desktop, Geom::Point(event->button.x, event->button.y), force_drag, FALSE);
                    sp_object_ref(this->item, nullptr);

                    rb_escaped = drag_escaped = 0;

                    if (grabbed) {
                        grabbed->ungrab();
                        grabbed = nullptr;
                    }

                    grabbed = desktop->getCanvasDrawing();
                    grabbed->grab(Gdk::KEY_PRESS_MASK      |
                                  Gdk::KEY_RELEASE_MASK    |
                                  Gdk::BUTTON_PRESS_MASK   |
                                  Gdk::BUTTON_RELEASE_MASK |
                                  Gdk::POINTER_MOTION_MASK );

                    ret = TRUE;
                }
            } else if (event->button.button == 3 && !this->dragging) {
                // right click; do not eat it so that right-click menu can appear, but cancel dragging & rubberband
                this->sp_select_context_abort();
            }
            break;
    

        case GDK_ENTER_NOTIFY: {
            if (!desktop->isWaitingCursor() && !this->dragging) {
                auto window = desktop->getCanvas()->get_window();
                window->set_cursor(_cursor_mouseover);
            }
            break;
        }
        case GDK_LEAVE_NOTIFY:
            if (!desktop->isWaitingCursor() && !this->dragging) {
                auto window = desktop->getCanvas()->get_window();
                window->set_cursor(this->cursor);
            }
            break;

        case GDK_KEY_PRESS:
            if (get_latin_keyval (&event->key) == GDK_KEY_space) {
                if (this->dragging && this->grabbed) {
                    /* stamping mode: show content mode moving */
                    _seltrans->stamp();
                    ret = TRUE;
                }
            } else if (get_latin_keyval (&event->key) == GDK_KEY_Tab) {
                if (this->dragging && this->grabbed) {
                    _seltrans->getNextClosestPoint(false);
                    ret = TRUE;
                }
            } else if (get_latin_keyval (&event->key) == GDK_KEY_ISO_Left_Tab) {
                if (this->dragging && this->grabbed) {
                    _seltrans->getNextClosestPoint(true);
                    ret = TRUE;
                }
            }
            break;

        default:
            break;
    }

    if (!ret) {
        ret = ToolBase::item_handler(item, event);
    }

    return ret;
}

void SelectTool::sp_select_context_cycle_through_items(Inkscape::Selection *selection, GdkEventScroll *scroll_event) {
    if ( this->cycling_items.empty() )
        return;

    Inkscape::DrawingItem *arenaitem;

    if(cycling_cur_item) {
        arenaitem = cycling_cur_item->get_arenaitem(desktop->dkey);
        arenaitem->setOpacity(0.3);
    }

    // Find next item and activate it


    std::vector<SPItem *>::iterator next = cycling_items.end();

    if ((scroll_event->direction == GDK_SCROLL_UP) ||
        (scroll_event->direction == GDK_SCROLL_SMOOTH && scroll_event->delta_y < 0)) {
        if (! cycling_cur_item) {
            next = cycling_items.begin();
        } else {
            next = std::find( cycling_items.begin(), cycling_items.end(), cycling_cur_item );
            g_assert (next != cycling_items.end());
            next++;
            if (next == cycling_items.end()) {
                if ( cycling_wrap ) {
                    next = cycling_items.begin();
                } else {
                    next--;
                }
            }
        }
    } else { 
        if (! cycling_cur_item) {
            next = cycling_items.end();
            next--;
        } else {
            next = std::find( cycling_items.begin(), cycling_items.end(), cycling_cur_item );
            g_assert (next != cycling_items.end());
            if (next == cycling_items.begin()){
                if ( cycling_wrap ) { 
                    next = cycling_items.end();
                    next--;
                }
            } else {
                next--;
            }
        }
    }

    this->cycling_cur_item = *next;
    g_assert(next != cycling_items.end());
    g_assert(cycling_cur_item != nullptr);

    arenaitem = cycling_cur_item->get_arenaitem(desktop->dkey);
    arenaitem->setOpacity(1.0);

    if (Modifier::get(Modifiers::Type::SELECT_ADD_TO)->active(scroll_event->state)) {
        selection->add(cycling_cur_item);
    } else {
        selection->set(cycling_cur_item);
    }
}

void SelectTool::sp_select_context_reset_opacities() {
    for (auto item : this->cycling_items_cmp) {
        if (item) {
            Inkscape::DrawingItem *arenaitem = item->get_arenaitem(desktop->dkey);
            arenaitem->setOpacity(SP_SCALE24_TO_FLOAT(item->style->opacity.value));
        } else {
            g_assert_not_reached();
        }
    }

    this->cycling_items_cmp.clear();
    this->cycling_cur_item = nullptr;
}

bool SelectTool::root_handler(GdkEvent* event) {
    SPItem *item = nullptr;
    SPItem *item_at_point = nullptr, *group_at_point = nullptr, *item_in_group = nullptr;
    gint ret = FALSE;

    Inkscape::Selection *selection = desktop->getSelection();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // make sure we still have valid objects to move around
    if (this->item && this->item->document == nullptr) {
        this->sp_select_context_abort();
    }
    forced_redraws_start(5);

    switch (event->type) {
        case GDK_2BUTTON_PRESS:
            if (event->button.button == 1) {
                if (!selection->isEmpty()) {
                    SPItem *clicked_item = selection->items().front();

                    if (dynamic_cast<SPGroup *>(clicked_item) && !dynamic_cast<SPBox3D *>(clicked_item)) { // enter group if it's not a 3D box
                        desktop->setCurrentLayer(clicked_item);
                        desktop->getSelection()->clear();
                        this->dragging = false;
                        sp_event_context_discard_delayed_snap_event(this);

                        
                    } else { // switch tool
                        Geom::Point const button_pt(event->button.x, event->button.y);
                        Geom::Point const p(desktop->w2d(button_pt));
                        tools_switch_by_item (desktop, clicked_item, p);
                    }
                } else {
                    sp_select_context_up_one_layer(desktop);
                }

                ret = TRUE;
            }
            break;

        case GDK_BUTTON_PRESS:
            if (event->button.button == 1 && !this->space_panning) {
                // save drag origin
                xp = (gint) event->button.x;
                yp = (gint) event->button.y;
                within_tolerance = true;

                Geom::Point const button_pt(event->button.x, event->button.y);
                Geom::Point const p(desktop->w2d(button_pt));

                if(Modifier::get(Modifiers::Type::SELECT_TOUCH_PATH)->active(event->button.state)) {
                    Inkscape::Rubberband::get(desktop)->setMode(RUBBERBAND_MODE_TOUCHPATH);
                } else {
                    Inkscape::Rubberband::get(desktop)->defaultMode();
                }

                Inkscape::Rubberband::get(desktop)->start(desktop, p);

                if (this->grabbed) {
                    grabbed->ungrab();
                    this->grabbed = nullptr;
                }

                grabbed = desktop->getCanvasCatchall();
                grabbed->grab(Gdk::KEY_PRESS_MASK      |
                              Gdk::KEY_RELEASE_MASK    |
                              Gdk::BUTTON_PRESS_MASK   |
                              Gdk::BUTTON_RELEASE_MASK |
                              Gdk::POINTER_MOTION_MASK );

                // remember what modifiers were on before button press
                this->button_press_state = event->button.state;

                this->moved = FALSE;

                rb_escaped = drag_escaped = 0;

                ret = TRUE;
            } else if (event->button.button == 3) {
                // right click; do not eat it so that right-click menu can appear, but cancel dragging & rubberband
                this->sp_select_context_abort();
            }
            break;

        case GDK_MOTION_NOTIFY:
        {
            tolerance = prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100);

            bool first_hit = Modifier::get(Modifiers::Type::SELECT_FIRST_HIT)->active(this->button_press_state);
            bool force_drag = Modifier::get(Modifiers::Type::SELECT_FORCE_DRAG)->active(this->button_press_state);
            bool always_box = Modifier::get(Modifiers::Type::SELECT_ALWAYS_BOX)->active(this->button_press_state);

            if ((event->motion.state & GDK_BUTTON1_MASK) && !this->space_panning) {
                Geom::Point const motion_pt(event->motion.x, event->motion.y);
                Geom::Point const p(desktop->w2d(motion_pt));
                if ( within_tolerance
                     && ( abs( (gint) event->motion.x - xp ) < tolerance )
                     && ( abs( (gint) event->motion.y - yp ) < tolerance ) ) {
                    break; // do not drag if we're within tolerance from origin
                }
                // Once the user has moved farther than tolerance from the original location
                // (indicating they intend to move the object, not click), then always process the
                // motion notify coordinates as given (no snapping back to origin)
                within_tolerance = false;

                if (first_hit || (force_drag && !always_box && !selection->isEmpty())) {
                    // if it's not click and ctrl or alt was pressed (the latter with some selection
                    // but not with shift) we want to drag rather than rubberband
                    this->dragging = TRUE;

                    auto window = desktop->getCanvas()->get_window();
                    window->set_cursor(_cursor_dragging);
                }

                if (this->dragging) {
                    /* User has dragged fast, so we get events on root (lauris)*/
                    // not only that; we will end up here when ctrl-dragging as well
                    // and also when we started within tolerance, but trespassed tolerance outside of item
                    if (Inkscape::Rubberband::get(desktop)->is_started()) {
                        Inkscape::Rubberband::get(desktop)->stop();
                    }
                    this->defaultMessageContext()->clear();

                    item_at_point = desktop->getItemAtPoint(Geom::Point(event->button.x, event->button.y), FALSE);

                    if (!item_at_point) { // if no item at this point, try at the click point (bug 1012200)
                        item_at_point = desktop->getItemAtPoint(Geom::Point(xp, yp), FALSE);
                    }

                    if (item_at_point || this->moved || force_drag) {
                        // drag only if starting from an item, or if something is already grabbed, or if alt-dragging
                        if (!this->moved) {
                            item_in_group = desktop->getItemAtPoint(Geom::Point(event->button.x, event->button.y), TRUE);
                            group_at_point = desktop->getGroupAtPoint(Geom::Point(event->button.x, event->button.y));

                            {
                                SPGroup *selGroup = dynamic_cast<SPGroup *>(selection->single());
                                if (selGroup && (selGroup->layerMode() == SPGroup::LAYER)) {
                                    group_at_point = selGroup;
                                }
                            }

                            // group-at-point is meant to be topmost item if it's a group,
                            // not topmost group of all items at point
                            if (group_at_point != item_in_group &&
                                !(group_at_point && item_at_point &&
                                  group_at_point->isAncestorOf(item_at_point))) {
                                group_at_point = nullptr;
                            }

                            // if neither a group nor an item (possibly in a group) at point are selected, set selection to the item at point
                            if ((!item_in_group || !selection->includes(item_in_group)) &&
                                (!group_at_point || !selection->includes(group_at_point)) && !force_drag) {
                                // select what is under cursor
                                if (!_seltrans->isEmpty()) {
                                    _seltrans->resetState();
                                }

                                // when simply ctrl-dragging, we don't want to go into groups
                                if (item_at_point && !selection->includes(item_at_point)) {
                                    selection->set(item_at_point);
                                }
                            } // otherwise, do not change selection so that dragging selected-within-group items, as well as alt-dragging, is possible

                            _seltrans->grab(p, -1, -1, FALSE, TRUE);
                            this->moved = TRUE;
                        }

                        if (!_seltrans->isEmpty()) {
                            sp_event_context_discard_delayed_snap_event(this);
                            _seltrans->moveTo(p, event->button.state);
                        }

                        desktop->scroll_to_point(p);
                        gobble_motion_events(GDK_BUTTON1_MASK);
                        ret = TRUE;
                    } else {
                        this->dragging = FALSE;
                        sp_event_context_discard_delayed_snap_event(this);
                        
                    }

                } else {
                    if (Inkscape::Rubberband::get(desktop)->is_started()) {
                        Inkscape::Rubberband::get(desktop)->move(p);

                        auto touch_path = Modifier::get(Modifiers::Type::SELECT_TOUCH_PATH)->get_label();
                        auto mode = Inkscape::Rubberband::get(desktop)->getMode();
                        if (mode == RUBBERBAND_MODE_TOUCHPATH) {
                            this->defaultMessageContext()->setF(Inkscape::NORMAL_MESSAGE,
                                _("<b>Draw over</b> objects to select them; release <b>%s</b> to switch to rubberband selection"), touch_path.c_str());
                        } else if (mode == RUBBERBAND_MODE_TOUCHRECT) {
                            this->defaultMessageContext()->setF(Inkscape::NORMAL_MESSAGE,
                                _("<b>Drag near</b> objects to select them; press <b>%s</b> to switch to touch selection"), touch_path.c_str());
                        } else {
                            this->defaultMessageContext()->setF(Inkscape::NORMAL_MESSAGE,
                                _("<b>Drag around</b> objects to select them; press <b>%s</b> to switch to touch selection"), touch_path.c_str());
                        }

                        gobble_motion_events(GDK_BUTTON1_MASK);
                    }
                }
            }
            break;
        }
        case GDK_BUTTON_RELEASE:
            xp = yp = 0;

            if ((event->button.button == 1) && (this->grabbed) && !this->space_panning) {
                if (this->dragging) {

                    if (this->moved) {
                        // item has been moved
                        _seltrans->ungrab();
                        this->moved = FALSE;
#ifdef WITH_DBUS
                        dbus_send_ping(desktop, this->item);
#endif
                    } else if (this->item && !drag_escaped) {
                        // item has not been moved -> simply a click, do selecting
                        if (!selection->isEmpty()) {
                            if(Modifier::get(Modifiers::Type::SELECT_ADD_TO)->active(event->button.state)) {
                                // with shift, toggle selection
                                _seltrans->resetState();
                                selection->toggle(this->item);
                            } else {
                                SPObject* single = selection->single();
                                SPGroup *singleGroup = dynamic_cast<SPGroup *>(single);
                                // without shift, increase state (i.e. toggle scale/rotation handles)
                                if (selection->includes(this->item)) {
                                    _seltrans->increaseState();
                                } else if (singleGroup && (singleGroup->layerMode() == SPGroup::LAYER) && single->isAncestorOf(this->item)) {
                                    _seltrans->increaseState();
                                } else {
                                    _seltrans->resetState();
                                    selection->set(this->item);
                                }
                            }
                        } else { // simple or shift click, no previous selection
                            _seltrans->resetState();
                            selection->set(this->item);
                        }
                    }

                    this->dragging = FALSE;

                    auto window = desktop->getCanvas()->get_window();
                    window->set_cursor(_cursor_mouseover);

                    sp_event_context_discard_delayed_snap_event(this);

                    if (this->item) {
                        sp_object_unref( this->item, nullptr);
                    }

                    this->item = nullptr;
                } else {
                    Inkscape::Rubberband *r = Inkscape::Rubberband::get(desktop);

                    if (r->is_started() && !within_tolerance) {
                        // this was a rubberband drag
                        std::vector<SPItem*> items;

                        if (r->getMode() == RUBBERBAND_MODE_RECT) {
                            Geom::OptRect const b = r->getRectangle();
                            items = desktop->getDocument()->getItemsInBox(desktop->dkey, (*b) * desktop->dt2doc());
                        } else if (r->getMode() == RUBBERBAND_MODE_TOUCHRECT) {
                            Geom::OptRect const b = r->getRectangle();
                            items = desktop->getDocument()->getItemsPartiallyInBox(desktop->dkey, (*b) * desktop->dt2doc());
                        } else if (r->getMode() == RUBBERBAND_MODE_TOUCHPATH) {
                            items = desktop->getDocument()->getItemsAtPoints(desktop->dkey, r->getPoints());
                        }

                        _seltrans->resetState();
                        r->stop();
                        this->defaultMessageContext()->clear();

                        if(Modifier::get(Modifiers::Type::SELECT_ADD_TO)->active(event->button.state)) {
                            // with shift, add to selection
                            selection->addList (items);
                        } else {
                            // without shift, simply select anew
                            selection->setList (items);
                        }

                    } else { // it was just a click, or a too small rubberband
                        r->stop();

                        bool add_to = Modifier::get(Modifiers::Type::SELECT_ADD_TO)->active(event->button.state);
                        bool in_groups = Modifier::get(Modifiers::Type::SELECT_IN_GROUPS)->active(event->button.state);
                        bool force_drag = Modifier::get(Modifiers::Type::SELECT_FORCE_DRAG)->active(event->button.state);

                        if (add_to && !rb_escaped && !drag_escaped) {
                            // this was a shift+click or alt+shift+click, select what was clicked upon

                            if (in_groups) {
                                // go into groups, honoring force_drag (Alt)
                                item = sp_event_context_find_item (desktop,
                                                   Geom::Point(event->button.x, event->button.y), force_drag, TRUE);
                            } else {
                                // don't go into groups, honoring Alt
                                item = sp_event_context_find_item (desktop,
                                                   Geom::Point(event->button.x, event->button.y), force_drag, FALSE);
                            }

                            if (item) {
                                selection->toggle(item);
                                item = nullptr;
                            }

                        } else if ((in_groups || force_drag) && !rb_escaped && !drag_escaped) { // ctrl+click, alt+click
                            item = sp_event_context_find_item (desktop,
                                         Geom::Point(event->button.x, event->button.y), force_drag, in_groups);

                            if (item) {
                                if (selection->includes(item)) {
                                    _seltrans->increaseState();
                                } else {
                                    _seltrans->resetState();
                                    selection->set(item);
                                }

                                item = nullptr;
                            }
                        } else { // click without shift, simply deselect, unless with Alt or something was cancelled
                            if (!selection->isEmpty()) {
                                if (!(rb_escaped) && !(drag_escaped) && !force_drag) {
                                    selection->clear();
                                }

                                rb_escaped = 0;
                            }
                        }
                    }

                    ret = TRUE;
                }
                if (grabbed) {
                    grabbed->ungrab();
                    grabbed = nullptr;
                }
                // Think is not necesary now
                // desktop->updateNow();
            }

            if (event->button.button == 1) {
                Inkscape::Rubberband::get(desktop)->stop(); // might have been started in another tool!
            }

            this->button_press_state = 0;
            break;

        case GDK_SCROLL: {

            GdkEventScroll *scroll_event = (GdkEventScroll*) event;

            // do nothing specific if alt was not pressed
            if ( ! Modifier::get(Modifiers::Type::SELECT_CYCLE)->active(scroll_event->state))
                break;

            is_cycling = true;

            /* Rebuild list of items underneath the mouse pointer */
            Geom::Point p = desktop->d2w(desktop->point()); 
            SPItem *item = desktop->getItemAtPoint(p, true, nullptr);
            this->cycling_items.clear();

            SPItem *tmp = nullptr;
            while(item != nullptr) {
                this->cycling_items.push_back(item);
                item = desktop->getItemAtPoint(p, true, item);
                if (selection->includes(item)) tmp = item;
            }

            /* Compare current item list with item list during previous scroll ... */
            bool item_lists_differ = this->cycling_items != this->cycling_items_cmp;

            if(item_lists_differ) {
                this->sp_select_context_reset_opacities();
                for (auto l : this->cycling_items_cmp) 
                    selection->remove(l); // deselects the previous content of the cycling loop
                this->cycling_items_cmp = (this->cycling_items);

                // set opacities in new stack
                for(auto item : this->cycling_items) {
                    if (item) {
                        Inkscape::DrawingItem *arenaitem = item->get_arenaitem(desktop->dkey);
                        arenaitem->setOpacity(0.3);
                    }
                }
            }
            if(!cycling_cur_item) cycling_cur_item = tmp;

            this->cycling_wrap = prefs->getBool("/options/selection/cycleWrap", true);

            // Cycle through the items underneath the mouse pointer, one-by-one
            this->sp_select_context_cycle_through_items(selection, scroll_event);

            ret = TRUE;

            GtkWindow *w =GTK_WINDOW(gtk_widget_get_toplevel( GTK_WIDGET(desktop->getCanvas()->gobj()) ));
            if (w) {
                gtk_window_present(w);
                desktop->getCanvas()->grab_focus();
            }
            break;
        }

        case GDK_KEY_PRESS: // keybindings for select context
            {
            {
            guint keyval = get_latin_keyval(&event->key);
            
                bool alt = ( MOD__ALT(event)
                                    || (keyval == GDK_KEY_Alt_L)
                                    || (keyval == GDK_KEY_Alt_R)
                                    || (keyval == GDK_KEY_Meta_L)
                                    || (keyval == GDK_KEY_Meta_R));

            if (!key_is_a_modifier (keyval)) {
                this->defaultMessageContext()->clear();
            } else if (this->grabbed || _seltrans->isGrabbed()) {

                if (Inkscape::Rubberband::get(desktop)->is_started()) {
                    // if Alt then change cursor to moving cursor:
                    if (Modifier::get(Modifiers::Type::SELECT_TOUCH_PATH)->active(event->key.state | keyval)) {
                        Inkscape::Rubberband::get(desktop)->setMode(RUBBERBAND_MODE_TOUCHPATH);
                    }
                } else {
                // do not change the statusbar text when mousekey is down to move or transform the object,
                // because the statusbar text is already updated somewhere else.
                   break;
                }
            } else {
                    Modifiers::responsive_tooltip(this->defaultMessageContext(), event, 6,
                        Modifiers::Type::SELECT_IN_GROUPS, Modifiers::Type::MOVE_CONFINE,
                        Modifiers::Type::SELECT_ADD_TO, Modifiers::Type::SELECT_TOUCH_PATH,
                        Modifiers::Type::SELECT_CYCLE, Modifiers::Type::SELECT_FORCE_DRAG);

                    // if Alt and nonempty selection, show moving cursor ("move selected"):
                    if (alt && !selection->isEmpty() && !desktop->isWaitingCursor()) {
                        auto window = desktop->getCanvas()->get_window();
                        window->set_cursor(_cursor_dragging);
                    }
                    //*/
                    break;
            }
            }

            gdouble const nudge = prefs->getDoubleLimited("/options/nudgedistance/value", 2, 0, 1000, "px"); // in px
            int const snaps = prefs->getInt("/options/rotationsnapsperpi/value", 12);
            auto const y_dir = desktop->yaxisdir();

            switch (get_latin_keyval (&event->key)) {
                case GDK_KEY_Left: // move selection left
                case GDK_KEY_KP_Left:
                    if (!MOD__CTRL(event)) { // not ctrl
                        gint mul = 1 + gobble_key_events( get_latin_keyval(&event->key), 0); // with any mask
                        
                        if (MOD__ALT(event)) { // alt
                            if (MOD__SHIFT(event)) {
                                desktop->getSelection()->moveScreen(mul*-10, 0); // shift
                            } else {
                                desktop->getSelection()->moveScreen(mul*-1, 0); // no shift
                            }
                        } else { // no alt
                            if (MOD__SHIFT(event)) {
                                desktop->getSelection()->move(mul*-10*nudge, 0); // shift
                            } else {
                                desktop->getSelection()->move(mul*-nudge, 0); // no shift
                            }
                        }
                        
                        ret = TRUE;
                    }
                    break;
                    
                case GDK_KEY_Up: // move selection up
                case GDK_KEY_KP_Up:
                    if (!MOD__CTRL(event)) { // not ctrl
                        gint mul = 1 + gobble_key_events(get_latin_keyval(&event->key), 0); // with any mask
                        mul *= -y_dir;
                        
                        if (MOD__ALT(event)) { // alt
                            if (MOD__SHIFT(event)) {
                                desktop->getSelection()->moveScreen(0, mul*10); // shift
                            } else {
                                desktop->getSelection()->moveScreen(0, mul*1); // no shift
                            }
                        } else { // no alt
                            if (MOD__SHIFT(event)) {
                                desktop->getSelection()->move(0, mul*10*nudge); // shift
                            } else {
                                desktop->getSelection()->move(0, mul*nudge); // no shift
                            }
                        }
                        
                        ret = TRUE;
                    }
                    break;
                    
                case GDK_KEY_Right: // move selection right
                case GDK_KEY_KP_Right:
                    if (!MOD__CTRL(event)) { // not ctrl
                        gint mul = 1 + gobble_key_events(get_latin_keyval(&event->key), 0); // with any mask
                        
                        if (MOD__ALT(event)) { // alt
                            if (MOD__SHIFT(event)) {
                                desktop->getSelection()->moveScreen(mul*10, 0); // shift
                            } else {
                                desktop->getSelection()->moveScreen(mul*1, 0); // no shift
                            }
                        } else { // no alt
                            if (MOD__SHIFT(event)) {
                                desktop->getSelection()->move(mul*10*nudge, 0); // shift
                            } else {
                                desktop->getSelection()->move(mul*nudge, 0); // no shift
                            }
                        }
                        
                        ret = TRUE;
                    }
                    break;
                    
                case GDK_KEY_Down: // move selection down
                case GDK_KEY_KP_Down:
                    if (!MOD__CTRL(event)) { // not ctrl
                        gint mul = 1 + gobble_key_events(get_latin_keyval(&event->key), 0); // with any mask
                        mul *= -y_dir;
                        
                        if (MOD__ALT(event)) { // alt
                            if (MOD__SHIFT(event)) {
                                desktop->getSelection()->moveScreen(0, mul*-10); // shift
                            } else {
                                desktop->getSelection()->moveScreen(0, mul*-1); // no shift
                            }
                        } else { // no alt
                            if (MOD__SHIFT(event)) {
                                desktop->getSelection()->move(0, mul*-10*nudge); // shift
                            } else {
                                desktop->getSelection()->move(0, mul*-nudge); // no shift
                            }
                        }
                        
                        ret = TRUE;
                    }
                    break;
                    
                case GDK_KEY_Escape:
                    if (!this->sp_select_context_abort()) {
                        selection->clear();
                    }
                    
                    ret = TRUE;
                    break;

                case GDK_KEY_a:
                case GDK_KEY_A:
                    if (MOD__CTRL_ONLY(event)) {
                        sp_edit_select_all(desktop);
                        ret = TRUE;
                    }
                    break;
                    
                case GDK_KEY_space:
                    /* stamping mode: show outline mode moving */
                    /* FIXME: Is next condition ok? (lauris) */
                    if (this->dragging && this->grabbed) {
                        _seltrans->stamp();
                        ret = TRUE;
                    }
                    break;
                    
                case GDK_KEY_x:
                case GDK_KEY_X:
                    if (MOD__ALT_ONLY(event)) {
                        desktop->setToolboxFocusTo ("select-x");
                        ret = TRUE;
                    }
                    break;
                    
                case GDK_KEY_bracketleft:
                    if (MOD__ALT(event)) {
                        gint mul = 1 + gobble_key_events(get_latin_keyval(&event->key), 0); // with any mask
                        selection->rotateScreen(-mul * y_dir);
                    } else if (MOD__CTRL(event)) {
                        selection->rotate(-90 * y_dir);
                    } else if (snaps) {
                        selection->rotate(-180.0/snaps * y_dir);
                    }
                    
                    ret = TRUE;
                    break;
                    
                case GDK_KEY_bracketright:
                    if (MOD__ALT(event)) {
                        gint mul = 1 + gobble_key_events(get_latin_keyval(&event->key), 0); // with any mask
                        selection->rotateScreen(mul * y_dir);
                    } else if (MOD__CTRL(event)) {
                        selection->rotate(90 * y_dir);
                    } else if (snaps) {
                        selection->rotate(180.0/snaps * y_dir);
                    }
                    
                    ret = TRUE;
                    break;

                case GDK_KEY_Return:
                    if (MOD__CTRL_ONLY(event)) {
                        if (selection->singleItem()) {
                            SPItem *clicked_item = selection->singleItem();
                            SPGroup *clickedGroup = dynamic_cast<SPGroup *>(clicked_item);
                            if ( (clickedGroup && (clickedGroup->layerMode() != SPGroup::LAYER)) || dynamic_cast<SPBox3D *>(clicked_item)) { // enter group or a 3D box
                                desktop->setCurrentLayer(clicked_item);
                                desktop->getSelection()->clear();
                            } else {
                                this->desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Selected object is not a group. Cannot enter."));
                            }
                        }
                        
                        ret = TRUE;
                    }
                    break;
                    
                case GDK_KEY_BackSpace:
                    if (MOD__CTRL_ONLY(event)) {
                        sp_select_context_up_one_layer(desktop);
                        ret = TRUE;
                    }
                    break;
                    
                case GDK_KEY_s:
                case GDK_KEY_S:
                    if (MOD__SHIFT_ONLY(event)) {
                        if (!selection->isEmpty()) {
                            _seltrans->increaseState();
                        }
                        
                        ret = TRUE;
                    }
                    break;
                    
                case GDK_KEY_g:
                case GDK_KEY_G:
                    if (MOD__SHIFT_ONLY(event)) {
                        desktop->selection->toGuides();
                        ret = true;
                    }
                    break;
                    
                default:
                    break;
            }
            break;
            }
        case GDK_KEY_RELEASE: {
            guint keyval = get_latin_keyval(&event->key);
            if (key_is_a_modifier (keyval)) {
                this->defaultMessageContext()->clear();
            }
            
            bool alt = ( MOD__ALT(event)
                         || (keyval == GDK_KEY_Alt_L)
                         || (keyval == GDK_KEY_Alt_R)
                         || (keyval == GDK_KEY_Meta_L)
                         || (keyval == GDK_KEY_Meta_R));

            if (Inkscape::Rubberband::get(desktop)->is_started()) {
                // if Alt then change cursor to moving cursor:
                if (alt) {
                    Inkscape::Rubberband::get(desktop)->defaultMode();
                }
            } else {
                if (alt) {
                    // quit cycle-selection and reset opacities
                    if (is_cycling) {
                        this->sp_select_context_reset_opacities();
                        is_cycling = false;
                    }
                }
            }

            // set cursor to default.
            if (!desktop->isWaitingCursor()) {
                // Do we need to reset the cursor here on key release ?
                //GdkWindow* window = gtk_widget_get_window (GTK_WIDGET (desktop->getCanvas()));
                //gdk_window_set_cursor(window, event_context->cursor);
            }
            break;
        }
        default:
            break;
    }

    if (!ret) {
        ret = ToolBase::root_handler(event);
    }

    return ret;
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

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Container for SPKnot visual handles.
 *
 * Authors:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   bulia byak <buliabyak@users.sf.net>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2001-2008 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "knotholder.h"

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "knot-holder-entity.h"
#include "knot.h"
#include "verbs.h"

#include "live_effects/effect.h"
#include "live_effects/lpeobject.h"

#include "object/box3d.h"
#include "object/sp-ellipse.h"
#include "object/sp-offset.h"
#include "object/sp-pattern.h"
#include "object/sp-rect.h"
#include "object/sp-shape.h"
#include "object/sp-spiral.h"
#include "object/sp-star.h"
#include "style.h"

#include "display/sp-canvas.h"

#include "ui/control-manager.h"
#include "ui/shape-editor.h"
#include "ui/tools-switch.h"
#include "ui/tools/arc-tool.h"
#include "ui/tools/node-tool.h"
#include "ui/tools/rect-tool.h"
#include "ui/tools/spiral-tool.h"
#include "ui/tools/tweak-tool.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

using Inkscape::ControlManager;
using Inkscape::DocumentUndo;

class SPDesktop;

KnotHolder::KnotHolder(SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler) :
    desktop(desktop),
    item(item),
    //XML Tree being used directly for item->getRepr() while it shouldn't be...
    repr(item ? item->getRepr() : nullptr),
    entity(),
    sizeUpdatedConn(),
    released(relhandler),
    local_change(FALSE),
    dragging(false),
    _edit_transform(Geom::identity())
{
    if (!desktop || !item) {
        g_print ("Error! Throw an exception, please!\n");
    }

    sp_object_ref(item);

    sizeUpdatedConn = ControlManager::getManager().connectCtrlSizeChanged(sigc::mem_fun(*this, &KnotHolder::updateControlSizes));
}

KnotHolder::~KnotHolder() {
    sp_object_unref(item);

    for (auto & i : entity)
    {
        delete i;
        i = NULL;
    }
    entity.clear(); // is this necessary?
    sizeUpdatedConn.disconnect();
}

void
KnotHolder::setEditTransform(Geom::Affine edit_transform)
{
    _edit_transform = edit_transform;
}

void KnotHolder::updateControlSizes()
{
    ControlManager &mgr = ControlManager::getManager();

    for (auto e : entity) {
        mgr.updateItem(e->knot->item);
    }
}

void KnotHolder::update_knots()
{
    for (auto e : entity) {
        e->update_knot();
    }
}

/**
 * Returns true if at least one of the KnotHolderEntities has the mouse hovering above it.
 */
bool KnotHolder::knot_mouseover() const {
    for (auto i : entity) {
        const SPKnot *knot = i->knot;

        if (knot && (knot->flags & SP_KNOT_MOUSEOVER)) {
            return true;
        }
    }

    return false;
}

void
KnotHolder::knot_mousedown_handler(SPKnot *knot, guint state)
{
    if (!(state & GDK_SHIFT_MASK)) {
        unselect_knots();
    }
    for(auto e : this->entity) {
        if (!(state & GDK_SHIFT_MASK)) {
            e->knot->selectKnot(false);
        }
        if (e->knot == knot) {
            if (!(e->knot->flags & SP_KNOT_SELECTED) || !(state & GDK_SHIFT_MASK)){
                e->knot->selectKnot(true);
            } else {
                e->knot->selectKnot(false);
            }
        }
    }
}

void
KnotHolder::knot_clicked_handler(SPKnot *knot, guint state)
{
    SPItem *saved_item = this->item;

    for(auto e : this->entity) {
        if (e->knot == knot)
            // no need to test whether knot_click exists since it's virtual now
            e->knot_click(state);
    }

    {
        SPShape *savedShape = dynamic_cast<SPShape *>(saved_item);
        if (savedShape) {
            savedShape->set_shape();
        }
    }

    this->update_knots();

    unsigned int object_verb = SP_VERB_NONE;

    // TODO extract duplicated blocks;
    if (dynamic_cast<SPRect *>(saved_item)) {
        object_verb = SP_VERB_CONTEXT_RECT;
    } else if (dynamic_cast<SPBox3D *>(saved_item)) {
        object_verb = SP_VERB_CONTEXT_3DBOX;
    } else if (dynamic_cast<SPGenericEllipse *>(saved_item)) {
        object_verb = SP_VERB_CONTEXT_ARC;
    } else if (dynamic_cast<SPStar *>(saved_item)) {
        object_verb = SP_VERB_CONTEXT_STAR;
    } else if (dynamic_cast<SPSpiral *>(saved_item)) {
        object_verb = SP_VERB_CONTEXT_SPIRAL;
    } else {
        SPOffset *offset = dynamic_cast<SPOffset *>(saved_item);
        if (offset) {
            if (offset->sourceHref) {
                object_verb = SP_VERB_SELECTION_LINKED_OFFSET;
            } else {
                object_verb = SP_VERB_SELECTION_DYNAMIC_OFFSET;
            }
        }
    }

    // for drag, this is done by ungrabbed_handler, but for click we must do it here

    if (saved_item) { //increasingly aggressive sanity checks
       if (saved_item->document) {
            // enum is unsigned so can't be less than SP_VERB_INVALID
            if (object_verb <= SP_VERB_LAST) {
                DocumentUndo::done(saved_item->document, object_verb,
                                   _("Change handle"));
            }
       }
    } // else { abort(); }
}

void
KnotHolder::transform_selected(Geom::Affine transform){
    for (auto & i : entity) {
        SPKnot *knot = i->knot;
        if (knot->flags & SP_KNOT_SELECTED) {
            knot_moved_handler(knot, knot->pos * transform , 0);
            knot->selectKnot(true);
        }
    }
}

void
KnotHolder::unselect_knots(){
    if (tools_isactive(desktop, TOOLS_NODES)) {
        Inkscape::UI::Tools::NodeTool *nt = static_cast<Inkscape::UI::Tools::NodeTool*>(desktop->event_context);
        if (nt) {
            for(auto i=nt->_shape_editors.begin();i!=nt->_shape_editors.end();++i){
                Inkscape::UI::ShapeEditor * shape_editor = i->second;
                if (shape_editor && shape_editor->has_knotholder()) {
                    KnotHolder * knotholder = shape_editor->knotholder;
                    if (knotholder) {
                        for(auto e : knotholder->entity) {
                            if (e->knot->flags & SP_KNOT_SELECTED) {
                                e->knot->selectKnot(false);
                            }
                        }
                    }
                }
            }
        }
    }
}

void
KnotHolder::knot_moved_handler(SPKnot *knot, Geom::Point const &p, guint state)
{
    if (this->dragging == false) {
    	this->dragging = true;
    }

    // this was a local change and the knotholder does not need to be recreated:
    this->local_change = TRUE;

    for(auto e : this->entity) {
        if (e->knot == knot) {
            Geom::Point const q = p * item->i2dt_affine().inverse() * _edit_transform.inverse();
            e->knot_set(q, e->knot->drag_origin * item->i2dt_affine().inverse() * _edit_transform.inverse(), state);
            break;
        }
    }

    SPShape *shape = dynamic_cast<SPShape *>(item);
    if (shape) {
        shape->set_shape();
    }

    this->update_knots();
}

void
KnotHolder::knot_ungrabbed_handler(SPKnot *knot, guint state)
{
    this->dragging = false;

    if (this->released) {
        this->released(this->item);
    } else {
        // if a point is dragged while not selected, it should select itself,
        // even if it was just unselected in the mousedown event handler.
        if (!(knot->flags & SP_KNOT_SELECTED)) {
            knot->selectKnot(true);
        } else {
            for(auto e : this->entity) {
                if (e->knot == knot) {
                    e->knot_ungrabbed(e->knot->position(), e->knot->drag_origin * item->i2dt_affine().inverse() * _edit_transform.inverse(), state);
                    break;
                }
            }
        }

        SPObject *object = (SPObject *) this->item;

        // Caution: this call involves a screen update, which may process events, and as a
        // result the knotholder may be destructed. So, after the updateRepr, we cannot use any
        // fields of this knotholder (such as this->item), but only values we have saved beforehand
        // (such as object).
        object->updateRepr();

        /* do cleanup tasks (e.g., for LPE items write the parameter values
         * that were changed by dragging the handle to SVG)
         */
        SPLPEItem *lpeItem = dynamic_cast<SPLPEItem *>(object);
        if (lpeItem) {
            // This writes all parameters to SVG. Is this sufficiently efficient or should we only
            // write the ones that were changed?
            Inkscape::LivePathEffect::Effect *lpe = lpeItem->getCurrentLPE();
            if (lpe) {
                LivePathEffectObject *lpeobj = lpe->getLPEObj();
                lpeobj->updateRepr();
            }
        }

        unsigned int object_verb = SP_VERB_NONE;

        // TODO extract duplicated blocks:
        if (dynamic_cast<SPRect *>(object)) {
            object_verb = SP_VERB_CONTEXT_RECT;
        } else if (dynamic_cast<SPBox3D *>(object)) {
            object_verb = SP_VERB_CONTEXT_3DBOX;
        } else if (dynamic_cast<SPGenericEllipse *>(object)) {
            object_verb = SP_VERB_CONTEXT_ARC;
        } else if (dynamic_cast<SPStar *>(object)) {
            object_verb = SP_VERB_CONTEXT_STAR;
        } else if (dynamic_cast<SPSpiral *>(object)) {
            object_verb = SP_VERB_CONTEXT_SPIRAL;
        } else {
            SPOffset *offset = dynamic_cast<SPOffset *>(object);
            if (offset) {
                if (offset->sourceHref) {
                    object_verb = SP_VERB_SELECTION_LINKED_OFFSET;
                } else {
                    object_verb = SP_VERB_SELECTION_DYNAMIC_OFFSET;
                }
            }
        }
        DocumentUndo::done(object->document, object_verb, _("Move handle"));
    }
}

void KnotHolder::add(KnotHolderEntity *e)
{
    // g_message("Adding a knot at %p", e);
    entity.push_back(e);
    updateControlSizes();
}

void KnotHolder::add_pattern_knotholder()
{
    if ((item->style->fill.isPaintserver()) && dynamic_cast<SPPattern *>(item->style->getFillPaintServer())) {
        PatternKnotHolderEntityXY *entity_xy = new PatternKnotHolderEntityXY(true);
        PatternKnotHolderEntityAngle *entity_angle = new PatternKnotHolderEntityAngle(true);
        PatternKnotHolderEntityScale *entity_scale = new PatternKnotHolderEntityScale(true);
        entity_xy->create(desktop, item, this, Inkscape::CTRL_TYPE_POINT,
                          // TRANSLATORS: This refers to the pattern that's inside the object
                          _("<b>Move</b> the pattern fill inside the object"),
                          SP_KNOT_SHAPE_CROSS);

        entity_scale->create(desktop, item, this, Inkscape::CTRL_TYPE_SIZER,
                             _("<b>Scale</b> the pattern fill; uniformly if with <b>Ctrl</b>"),
                             SP_KNOT_SHAPE_SQUARE, SP_KNOT_MODE_XOR);

        entity_angle->create(desktop, item, this, Inkscape::CTRL_TYPE_ROTATE,
                             _("<b>Rotate</b> the pattern fill; with <b>Ctrl</b> to snap angle"),
                             SP_KNOT_SHAPE_CIRCLE, SP_KNOT_MODE_XOR);

        entity.push_back(entity_xy);
        entity.push_back(entity_angle);
        entity.push_back(entity_scale);
    }

    if ((item->style->stroke.isPaintserver()) && dynamic_cast<SPPattern *>(item->style->getStrokePaintServer())) {
        PatternKnotHolderEntityXY *entity_xy = new PatternKnotHolderEntityXY(false);
        PatternKnotHolderEntityAngle *entity_angle = new PatternKnotHolderEntityAngle(false);
        PatternKnotHolderEntityScale *entity_scale = new PatternKnotHolderEntityScale(false);
        entity_xy->create(desktop, item, this, Inkscape::CTRL_TYPE_POINT,
                          // TRANSLATORS: This refers to the pattern that's inside the object
                          _("<b>Move</b> the pattern fill inside the object"),
                          SP_KNOT_SHAPE_CROSS);

        entity_scale->create(desktop, item, this, Inkscape::CTRL_TYPE_SIZER,
                             _("<b>Scale</b> the pattern fill; uniformly if with <b>Ctrl</b>"),
                             SP_KNOT_SHAPE_SQUARE, SP_KNOT_MODE_XOR);

        entity_angle->create(desktop, item, this, Inkscape::CTRL_TYPE_ROTATE,
                             _("<b>Rotate</b> the pattern fill; with <b>Ctrl</b> to snap angle"),
                             SP_KNOT_SHAPE_CIRCLE, SP_KNOT_MODE_XOR);

        entity.push_back(entity_xy);
        entity.push_back(entity_angle);
        entity.push_back(entity_scale);
    }
    updateControlSizes();
}

void KnotHolder::add_filter_knotholder() {
    FilterKnotHolderEntity *entity_tl = new FilterKnotHolderEntity(true);
    FilterKnotHolderEntity *entity_br = new FilterKnotHolderEntity(false);
    entity_tl->create(desktop, item, this, Inkscape::CTRL_TYPE_POINT,
        _("<b>Resize</b> the filter effect region"), SP_KNOT_SHAPE_DIAMOND);
    entity_br->create(desktop, item, this, Inkscape::CTRL_TYPE_POINT,
        _("<b>Resize</b> the filter effect region"), SP_KNOT_SHAPE_DIAMOND);
    entity.push_back(entity_tl);
    entity.push_back(entity_br);
    updateControlSizes();
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

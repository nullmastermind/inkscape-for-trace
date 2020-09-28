// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Inkscape::ShapeEditor
 * This is a container class which contains a knotholder for shapes.
 * It is attached to a single item.
 *//*
 * Authors: see git history
 *   bulia byak <buliabyak@users.sf.net>
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "shape-editor.h"

#include "desktop.h"
#include "document.h"
#include "live_effects/effect.h"
#include "object/sp-lpe-item.h"
#include "ui/knot/knot-holder.h"
#include "xml/node-event-vector.h"

namespace Inkscape {
namespace UI {

KnotHolder *createKnotHolder(SPItem *item, SPDesktop *desktop);
KnotHolder *createLPEKnotHolder(SPItem *item, SPDesktop *desktop);

bool ShapeEditor::_blockSetItem = false;

ShapeEditor::ShapeEditor(SPDesktop *dt, Geom::Affine edit_transform) :
    desktop(dt),
    knotholder(nullptr),
    lpeknotholder(nullptr),
    knotholder_listener_attached_for(nullptr),
    lpeknotholder_listener_attached_for(nullptr),
    _edit_transform(edit_transform)
{
}

ShapeEditor::~ShapeEditor() {
    unset_item();
}

void ShapeEditor::unset_item(bool keep_knotholder) {
    if (this->knotholder) {
        Inkscape::XML::Node *old_repr = this->knotholder->repr;
        if (old_repr && old_repr == knotholder_listener_attached_for) {
            sp_repr_remove_listener_by_data(old_repr, this);
            Inkscape::GC::release(old_repr);
            knotholder_listener_attached_for = nullptr;
        }

        if (!keep_knotholder) {
            delete this->knotholder;
            this->knotholder = nullptr;
        }
    }
    if (this->lpeknotholder) {
        Inkscape::XML::Node *old_repr = this->lpeknotholder->repr;
        if (old_repr && old_repr == lpeknotholder_listener_attached_for) {
            sp_repr_remove_listener_by_data(old_repr, this);
            Inkscape::GC::release(old_repr);
            lpeknotholder_listener_attached_for = nullptr;
        }

        if (!keep_knotholder) {
            delete this->lpeknotholder;
            this->lpeknotholder = nullptr;
        }
    }
}

bool ShapeEditor::has_knotholder() {
    return this->knotholder != nullptr || this->lpeknotholder != nullptr;
}

void ShapeEditor::update_knotholder() {
    if (this->knotholder)
        this->knotholder->update_knots();
    if (this->lpeknotholder)
        this->lpeknotholder->update_knots();
}

bool ShapeEditor::has_local_change() {
    return (this->knotholder && this->knotholder->local_change != 0) || (this->lpeknotholder && this->lpeknotholder->local_change != 0);
}

void ShapeEditor::decrement_local_change() {
    if (this->knotholder) {
        this->knotholder->local_change = FALSE;
    }
    if (this->lpeknotholder) {
        this->lpeknotholder->local_change = FALSE;
    }
}

void ShapeEditor::event_attr_changed(Inkscape::XML::Node * node, gchar const *name, gchar const *, gchar const *, bool, void *data)
{
    g_assert(data);
    ShapeEditor *sh = static_cast<ShapeEditor *>(data);
    bool changed_kh = false;

    if (sh->has_knotholder())
    {
        changed_kh = !sh->has_local_change();
        sh->decrement_local_change();
        if (changed_kh) {
            sh->reset_item();
        }
    }
}

static Inkscape::XML::NodeEventVector shapeeditor_repr_events = {
    nullptr, /* child_added */
    nullptr, /* child_removed */
    ShapeEditor::event_attr_changed,
    nullptr, /* content_changed */
    nullptr  /* order_changed */
};


void ShapeEditor::set_item(SPItem *item) {
    if (_blockSetItem) {
        return;
    }
    // this happens (and should only happen) when for an LPEItem having both knotholder and
    // nodepath the knotholder is adapted; in this case we don't want to delete the knotholder
    // since this freezes the handles
    unset_item(true);

    if (item) {
        Inkscape::XML::Node *repr;
        if (!this->knotholder) {
            // only recreate knotholder if none is present
            this->knotholder = createKnotHolder(item, desktop);
        }
        SPLPEItem *lpe = dynamic_cast<SPLPEItem *>(item);
        if (!(lpe &&
            lpe->getCurrentLPE() &&
            lpe->getCurrentLPE()->isVisible() &&
            lpe->getCurrentLPE()->providesKnotholder()))
        {
            delete this->lpeknotholder;
            this->lpeknotholder = nullptr;
        }
        if (!this->lpeknotholder) {
            // only recreate knotholder if none is present
            this->lpeknotholder = createLPEKnotHolder(item, desktop);
        }
        if (this->knotholder) {
            this->knotholder->setEditTransform(_edit_transform);
            this->knotholder->update_knots();
            // setting new listener
            repr = this->knotholder->repr;
            if (repr != knotholder_listener_attached_for) {
                Inkscape::GC::anchor(repr);
                sp_repr_add_listener(repr, &shapeeditor_repr_events, this);
                knotholder_listener_attached_for = repr;
            }
        }
        if (this->lpeknotholder) {
            this->lpeknotholder->setEditTransform(_edit_transform);
            this->lpeknotholder->update_knots();
            // setting new listener
            repr = this->lpeknotholder->repr;
            if (repr != lpeknotholder_listener_attached_for) {
                Inkscape::GC::anchor(repr);
                sp_repr_add_listener(repr, &shapeeditor_repr_events, this);
                lpeknotholder_listener_attached_for = repr;
            }
        }
    }
}


/** FIXME: This thing is only called when the item needs to be updated in response to repr change.
   Why not make a reload function in KnotHolder? */
void ShapeEditor::reset_item()
{
    if (knotholder) {
        SPObject *obj = desktop->getDocument()->getObjectByRepr(knotholder_listener_attached_for); /// note that it is not certain that this is an SPItem; it could be a LivePathEffectObject.
        set_item(SP_ITEM(obj));
    } else if (lpeknotholder) {
        SPObject *obj = desktop->getDocument()->getObjectByRepr(lpeknotholder_listener_attached_for); /// note that it is not certain that this is an SPItem; it could be a LivePathEffectObject.
        set_item(SP_ITEM(obj));
    }
}

/**
 * Returns true if this ShapeEditor has a knot above which the mouse currently hovers.
 */
bool ShapeEditor::knot_mouseover() const {
    if (this->knotholder) {
        return knotholder->knot_mouseover();
    }
    if (this->lpeknotholder) {
        return lpeknotholder->knot_mouseover();
    }

    return false;
}

} // namespace UI
} // namespace Inkscape

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

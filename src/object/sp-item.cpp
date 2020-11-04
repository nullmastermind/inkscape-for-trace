// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2001-2006 authors
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-item.h"

#include <glibmm/i18n.h>

#include "bad-uri-exception.h"
#include "svg/svg.h"
#include "print.h"
#include "display/drawing-item.h"
#include "attributes.h"
#include "document.h"

#include "inkscape.h"
#include "desktop.h"
#include "gradient-chemistry.h"
#include "conn-avoid-ref.h"
#include "conditions.h"
#include "filter-chemistry.h"

#include "sp-clippath.h"
#include "sp-desc.h"
#include "sp-guide.h"
#include "sp-hatch.h"
#include "sp-item-rm-unsatisfied-cns.h"
#include "sp-mask.h"
#include "sp-pattern.h"
#include "sp-rect.h"
#include "sp-root.h"
#include "sp-switch.h"
#include "sp-text.h"
#include "sp-textpath.h"
#include "sp-title.h"
#include "sp-use.h"

#include "style.h"
#include "uri.h"


#include "util/find-last-if.h"

#include "extract-uri.h"

#include "live_effects/lpeobject.h"
#include "live_effects/effect.h"
#include "live_effects/lpeobject-reference.h"

#include "util/units.h"

#define noSP_ITEM_DEBUG_IDLE

//#define OBJECT_TRACE

static SPItemView*          sp_item_view_list_remove(SPItemView     *list,
                                                     SPItemView     *view);


SPItem::SPItem() : SPObject() {

    sensitive = TRUE;
    bbox_valid = FALSE;

    _highlightColor = nullptr;

    transform_center_x = 0;
    transform_center_y = 0;

    freeze_stroke_width = false;
    _is_evaluated = true;
    _evaluated_status = StatusUnknown;

    transform = Geom::identity();
    // doc_bbox = Geom::OptRect();

    display = nullptr;

    clip_ref = nullptr;
    mask_ref = nullptr;

    style->signal_fill_ps_changed.connect(sigc::bind(sigc::ptr_fun(fill_ps_ref_changed), this));
    style->signal_stroke_ps_changed.connect(sigc::bind(sigc::ptr_fun(stroke_ps_ref_changed), this));

    avoidRef = nullptr;
}

SPItem::~SPItem() = default;

SPClipPath *SPItem::getClipObject() const { return clip_ref ? clip_ref->getObject() : nullptr; }

SPMask *SPItem::getMaskObject() const { return mask_ref ? mask_ref->getObject() : nullptr; }

SPMaskReference &SPItem::getMaskRef()
{
    if (!mask_ref) {
        mask_ref = new SPMaskReference(this);
        mask_ref->changedSignal().connect(sigc::bind(sigc::ptr_fun(mask_ref_changed), this));
    }

    return *mask_ref;
}

SPClipPathReference &SPItem::getClipRef()
{
    if (!clip_ref) {
        clip_ref = new SPClipPathReference(this);
        clip_ref->changedSignal().connect(sigc::bind(sigc::ptr_fun(clip_ref_changed), this));
    }

    return *clip_ref;
}

SPAvoidRef &SPItem::getAvoidRef()
{
    if (!avoidRef) {
        avoidRef = new SPAvoidRef(this);
    }
    return *avoidRef;
}

bool SPItem::isVisibleAndUnlocked() const {
    return (!isHidden() && !isLocked());
}

bool SPItem::isVisibleAndUnlocked(unsigned display_key) const {
    return (!isHidden(display_key) && !isLocked());
}

bool SPItem::isLocked() const {
    for (SPObject const *o = this; o != nullptr; o = o->parent) {
        SPItem const *item = dynamic_cast<SPItem const *>(o);
        if (item && !(item->sensitive)) {
            return true;
        }
    }
    return false;
}

void SPItem::setLocked(bool locked) {
    setAttribute("sodipodi:insensitive",
                 ( locked ? "1" : nullptr ));
    updateRepr();
    document->_emitModified();
}

bool SPItem::isHidden() const {
    if (!isEvaluated())
        return true;
    return style->display.computed == SP_CSS_DISPLAY_NONE;
}

void SPItem::setHidden(bool hide) {
    style->display.set = TRUE;
    style->display.value = ( hide ? SP_CSS_DISPLAY_NONE : SP_CSS_DISPLAY_INLINE );
    style->display.computed = style->display.value;
    style->display.inherit = FALSE;
    updateRepr();
}

bool SPItem::isHidden(unsigned display_key) const {
    if (!isEvaluated())
        return true;
    for ( SPItemView *view(display) ; view ; view = view->next ) {
        if ( view->key == display_key ) {
            g_assert(view->arenaitem != nullptr);
            for ( Inkscape::DrawingItem *arenaitem = view->arenaitem ;
                  arenaitem ; arenaitem = arenaitem->parent() )
            {
                if (!arenaitem->visible()) {
                    return true;
                }
            }
            return false;
        }
    }
    return true;
}

bool SPItem::isHighlightSet() const {
    return _highlightColor != nullptr;
}

guint32 SPItem::highlight_color() const {
    if (_highlightColor)
    {
        return atoi(_highlightColor) | 0x00000000;
    }
    else {
        SPItem const *item = dynamic_cast<SPItem const *>(parent);
        if (parent && (parent != this) && item)
        {
            return item->highlight_color();
        }
        else
        {
            static Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            return prefs->getInt("/tools/nodes/highlight_color", 0xff0000ff);
        }
    }
}

void SPItem::setEvaluated(bool evaluated) {
    _is_evaluated = evaluated;
    _evaluated_status = StatusSet;
}

void SPItem::resetEvaluated() {
    if ( StatusCalculated == _evaluated_status ) {
        _evaluated_status = StatusUnknown;
        bool oldValue = _is_evaluated;
        if ( oldValue != isEvaluated() ) {
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
        }
    } if ( StatusSet == _evaluated_status ) {
        SPSwitch *switchItem = dynamic_cast<SPSwitch *>(parent);
        if (switchItem) {
            switchItem->resetChildEvaluated();
        }
    }
}

bool SPItem::isEvaluated() const {
    if ( StatusUnknown == _evaluated_status ) {
        _is_evaluated = sp_item_evaluate(this);
        _evaluated_status = StatusCalculated;
    }
    return _is_evaluated;
}

bool SPItem::isExplicitlyHidden() const
{
    return (style->display.set
            && style->display.value == SP_CSS_DISPLAY_NONE);
}

void SPItem::setExplicitlyHidden(bool val) {
    style->display.set = val;
    style->display.value = ( val ? SP_CSS_DISPLAY_NONE : SP_CSS_DISPLAY_INLINE );
    style->display.computed = style->display.value;
    updateRepr();
}

void SPItem::setCenter(Geom::Point const &object_centre) {
    document->ensureUpToDate();

    // Copied from DocumentProperties::onDocUnitChange()
    gdouble viewscale = 1.0;
    Geom::Rect vb = this->document->getRoot()->viewBox;
    if ( !vb.hasZeroArea() ) {
        gdouble viewscale_w = this->document->getWidth().value("px") / vb.width();
        gdouble viewscale_h = this->document->getHeight().value("px")/ vb.height();
        viewscale = std::min(viewscale_h, viewscale_w);
    }

    // FIXME this is seriously wrong
    Geom::OptRect bbox = desktopGeometricBounds();
    if (bbox) {
        // object centre is document coordinates (i.e. in pixels), so we need to consider the viewbox
        // to translate to user units; transform_center_x/y is in user units
        transform_center_x = (object_centre[Geom::X] - bbox->midpoint()[Geom::X])/viewscale;
        if (Geom::are_near(transform_center_x, 0)) // rounding error
            transform_center_x = 0;
        transform_center_y = (object_centre[Geom::Y] - bbox->midpoint()[Geom::Y])/viewscale;
        if (Geom::are_near(transform_center_y, 0)) // rounding error
            transform_center_y = 0;
    }
}

void
SPItem::unsetCenter() {
    transform_center_x = 0;
    transform_center_y = 0;
}

bool SPItem::isCenterSet() const {
    return (transform_center_x != 0 || transform_center_y != 0);
}

// Get the item's transformation center in desktop coordinates (i.e. in pixels)
Geom::Point SPItem::getCenter() const {
    document->ensureUpToDate();

    // Copied from DocumentProperties::onDocUnitChange()
    gdouble viewscale = 1.0;
    Geom::Rect vb = this->document->getRoot()->viewBox;
    if ( !vb.hasZeroArea() ) {
        gdouble viewscale_w = this->document->getWidth().value("px") / vb.width();
        gdouble viewscale_h = this->document->getHeight().value("px")/ vb.height();
        viewscale = std::min(viewscale_h, viewscale_w);
    }

    // FIXME this is seriously wrong
    Geom::OptRect bbox = desktopGeometricBounds();
    if (bbox) {
        // transform_center_x/y are stored in user units, so we have to take the viewbox into account to translate to document coordinates
        return bbox->midpoint() + Geom::Point (transform_center_x*viewscale, transform_center_y*viewscale);

    } else {
        return Geom::Point(0, 0); // something's wrong!
    }

}

void
SPItem::scaleCenter(Geom::Scale const &sc) {
    transform_center_x *= sc[Geom::X];
    transform_center_y *= sc[Geom::Y];
}

namespace {

bool is_item(SPObject const &object) {
    return dynamic_cast<SPItem const *>(&object) != nullptr;
}

}

void SPItem::raiseToTop() {
    using Inkscape::Algorithms::find_last_if;

    auto topmost = find_last_if(++parent->children.iterator_to(*this), parent->children.end(), &is_item);
    if (topmost != parent->children.end()) {
        getRepr()->parent()->changeOrder( getRepr(), topmost->getRepr() );
    }
}

bool SPItem::raiseOne() {
    auto next_higher = std::find_if(++parent->children.iterator_to(*this), parent->children.end(), &is_item);
    if (next_higher != parent->children.end()) {
        Inkscape::XML::Node *ref = next_higher->getRepr();
        getRepr()->parent()->changeOrder(getRepr(), ref);
        return true;
    }
    return false;
}

bool SPItem::lowerOne() {
    using Inkscape::Algorithms::find_last_if;

    auto next_lower = find_last_if(parent->children.begin(), parent->children.iterator_to(*this), &is_item);
    if (next_lower != parent->children.iterator_to(*this)) {
        Inkscape::XML::Node *ref = nullptr;
        if (next_lower != parent->children.begin()) {
            next_lower--;
            ref = next_lower->getRepr();
        }
        getRepr()->parent()->changeOrder(getRepr(), ref);
        return true;
    }
    return false;
}

void SPItem::lowerToBottom() {
    auto bottom = std::find_if(parent->children.begin(), parent->children.iterator_to(*this), &is_item);
    if (bottom != parent->children.iterator_to(*this)) {
        Inkscape::XML::Node *ref = nullptr;
        if (bottom != parent->children.begin()) {
            bottom--;
            ref = bottom->getRepr();
        }
        parent->getRepr()->changeOrder(getRepr(), ref);
    }
}

void SPItem::moveTo(SPItem *target, bool intoafter) {

    Inkscape::XML::Node *target_ref = ( target ? target->getRepr() : nullptr );
    Inkscape::XML::Node *our_ref = getRepr();

    if (!target_ref) {
        // Assume move to the "first" in the top node, find the top node
        intoafter = false;
        SPObject* bottom = this->document->getObjectByRepr(our_ref->root())->firstChild();
        while(!dynamic_cast<SPItem*>(bottom->getNext())){
        	bottom = bottom->getNext();
        }
        target_ref = bottom->getRepr();
    }

    if (target_ref == our_ref) {
        // Move to ourself ignore
        return;
    }

    if (intoafter) {
        // Move this inside of the target at the end
        our_ref->parent()->removeChild(our_ref);
        target_ref->addChild(our_ref, nullptr);
    } else if (target_ref->parent() != our_ref->parent()) {
        // Change in parent, need to remove and add
        our_ref->parent()->removeChild(our_ref);
        target_ref->parent()->addChild(our_ref, target_ref);
    } else {
        // Same parent, just move
        our_ref->parent()->changeOrder(our_ref, target_ref);
    }
}

void SPItem::build(SPDocument *document, Inkscape::XML::Node *repr) {
	SPItem* object = this;

    object->readAttr(SPAttr::STYLE);
    object->readAttr(SPAttr::TRANSFORM);
    object->readAttr(SPAttr::CLIP_PATH);
    object->readAttr(SPAttr::MASK);
    object->readAttr(SPAttr::SODIPODI_INSENSITIVE);
    object->readAttr(SPAttr::TRANSFORM_CENTER_X);
    object->readAttr(SPAttr::TRANSFORM_CENTER_Y);
    object->readAttr(SPAttr::CONNECTOR_AVOID);
    object->readAttr(SPAttr::CONNECTION_POINTS);
    object->readAttr(SPAttr::INKSCAPE_HIGHLIGHT_COLOR);

    SPObject::build(document, repr);
}

void SPItem::release() {
	SPItem* item = this;

    // Note: do this here before the clip_ref is deleted, since calling
    // ensureUpToDate() for triggered routing may reference
    // the deleted clip_ref.
    delete item->avoidRef;

    // we do NOT disconnect from the changed signal of those before deletion.
    // The destructor will call *_ref_changed with NULL as the new value,
    // which will cause the hide() function to be called.
    delete item->clip_ref;
    delete item->mask_ref;

    SPObject::release();

    SPPaintServer *fill_ps = style->getFillPaintServer();
    SPPaintServer *stroke_ps = style->getStrokePaintServer();
    while (item->display) {
        if (fill_ps) {
            fill_ps->hide(item->display->arenaitem->key());
        }
        if (stroke_ps) {
            stroke_ps->hide(item->display->arenaitem->key());
        }
        item->display = sp_item_view_list_remove(item->display, item->display);
    }

    //item->_transformed_signal.~signal();
}

void SPItem::set(SPAttr key, gchar const* value) {
    SPItem *item = this;
    SPItem* object = item;

    switch (key) {
        case SPAttr::TRANSFORM: {
            Geom::Affine t;
            if (value && sp_svg_transform_read(value, &t)) {
                item->set_item_transform(t);
            } else {
                item->set_item_transform(Geom::identity());
            }
            break;
        }
        case SPAttr::CLIP_PATH: {
            auto uri = extract_uri(value);
            if (!uri.empty() || item->clip_ref) {
                item->getClipRef().try_attach(uri.c_str());
            }
            break;
        }
        case SPAttr::MASK: {
            auto uri = extract_uri(value);
            if (!uri.empty() || item->mask_ref) {
                item->getMaskRef().try_attach(uri.c_str());
            }
            break;
        }
        case SPAttr::SODIPODI_INSENSITIVE:
        {
            item->sensitive = !value;
            for (SPItemView *v = item->display; v != nullptr; v = v->next) {
                v->arenaitem->setSensitive(item->sensitive);
            }
            break;
        }
        case SPAttr::INKSCAPE_HIGHLIGHT_COLOR:
        {
            g_free(item->_highlightColor);
            if (value) {
                item->_highlightColor = g_strdup(value);
            } else {
                item->_highlightColor = nullptr;
            }
            break;
        }
        case SPAttr::CONNECTOR_AVOID:
            if (value || item->avoidRef) {
                item->getAvoidRef().setAvoid(value);
            }
            break;
        case SPAttr::TRANSFORM_CENTER_X:
            if (value) {
                item->transform_center_x = g_strtod(value, nullptr);
            } else {
                item->transform_center_x = 0;
            }
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::TRANSFORM_CENTER_Y:
            if (value) {
                item->transform_center_y = g_strtod(value, nullptr);
                item->transform_center_y *= -document->yaxisdir();
            } else {
                item->transform_center_y = 0;
            }
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::SYSTEM_LANGUAGE:
        case SPAttr::REQUIRED_FEATURES:
        case SPAttr::REQUIRED_EXTENSIONS:
            {
                item->resetEvaluated();
                // pass to default handler
            }
        default:
            if (SP_ATTRIBUTE_IS_CSS(key)) {
                style->clear(key);
                object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
            } else {
                SPObject::set(key, value);
            }
            break;
    }
}

void SPItem::clip_ref_changed(SPObject *old_clip, SPObject *clip, SPItem *item)
{
    item->bbox_valid = FALSE; // force a re-evaluation
    if (old_clip) {
        SPItemView *v;
        /* Hide clippath */
        for (v = item->display; v != nullptr; v = v->next) {
            SPClipPath *oldPath = dynamic_cast<SPClipPath *>(old_clip);
            g_assert(oldPath != nullptr);
            oldPath->hide(v->arenaitem->key());
        }
    }
    SPClipPath *clipPath = dynamic_cast<SPClipPath *>(clip);
    if (clipPath) {
        Geom::OptRect bbox = item->geometricBounds();
        for (SPItemView *v = item->display; v != nullptr; v = v->next) {
            if (!v->arenaitem->key()) {
                v->arenaitem->setKey(SPItem::display_key_new(3));
            }
            Inkscape::DrawingItem *ai = clipPath->show(
                                               v->arenaitem->drawing(),
                                               v->arenaitem->key());
            v->arenaitem->setClip(ai);
            clipPath->setBBox(v->arenaitem->key(), bbox);
            clip->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        }
    }
    item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void SPItem::mask_ref_changed(SPObject *old_mask, SPObject *mask, SPItem *item)
{
    item->bbox_valid = FALSE; // force a re-evaluation
    if (old_mask) {
        /* Hide mask */
        for (SPItemView *v = item->display; v != nullptr; v = v->next) {
            SPMask *maskItem = dynamic_cast<SPMask *>(old_mask);
            g_assert(maskItem != nullptr);
            maskItem->sp_mask_hide(v->arenaitem->key());
        }
    }
    SPMask *maskItem = dynamic_cast<SPMask *>(mask);
    if (maskItem) {
        Geom::OptRect bbox = item->geometricBounds();
        for (SPItemView *v = item->display; v != nullptr; v = v->next) {
            if (!v->arenaitem->key()) {
                v->arenaitem->setKey(SPItem::display_key_new(3));
            }
            Inkscape::DrawingItem *ai = maskItem->sp_mask_show(
                                           v->arenaitem->drawing(),
                                           v->arenaitem->key());
            v->arenaitem->setMask(ai);
            maskItem->sp_mask_set_bbox(v->arenaitem->key(), bbox);
            mask->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        }
    }
}

void SPItem::fill_ps_ref_changed(SPObject *old_ps, SPObject *ps, SPItem *item) {
    SPPaintServer *old_fill_ps = dynamic_cast<SPPaintServer *>(old_ps);
    if (old_fill_ps) {
        for (SPItemView *v =item->display; v != nullptr; v = v->next) {
            old_fill_ps->hide(v->arenaitem->key());
        }
    }

    SPPaintServer *new_fill_ps = dynamic_cast<SPPaintServer *>(ps);
    if (new_fill_ps) {
        Geom::OptRect bbox = item->geometricBounds();
        for (SPItemView *v = item->display; v != nullptr; v = v->next) {
            if (!v->arenaitem->key()) {
                v->arenaitem->setKey(SPItem::display_key_new(3));
            }
            Inkscape::DrawingPattern *pi = new_fill_ps->show(
                    v->arenaitem->drawing(), v->arenaitem->key(), bbox);
            v->arenaitem->setFillPattern(pi);
            if (pi) {
                new_fill_ps->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            }
        }
    }
}

void SPItem::stroke_ps_ref_changed(SPObject *old_ps, SPObject *ps, SPItem *item) {
    SPPaintServer *old_stroke_ps = dynamic_cast<SPPaintServer *>(old_ps);
    if (old_stroke_ps) {
        for (SPItemView *v =item->display; v != nullptr; v = v->next) {
            old_stroke_ps->hide(v->arenaitem->key());
        }
    }

    SPPaintServer *new_stroke_ps = dynamic_cast<SPPaintServer *>(ps);
    if (new_stroke_ps) {
        Geom::OptRect bbox = item->geometricBounds();
        for (SPItemView *v = item->display; v != nullptr; v = v->next) {
            if (!v->arenaitem->key()) {
                v->arenaitem->setKey(SPItem::display_key_new(3));
            }
            Inkscape::DrawingPattern *pi = new_stroke_ps->show(
                    v->arenaitem->drawing(), v->arenaitem->key(), bbox);
            v->arenaitem->setStrokePattern(pi);
            if (pi) {
                new_stroke_ps->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            }
        }
    }
}

void SPItem::update(SPCtx* ctx, guint flags) {

    SPItemCtx const *ictx = reinterpret_cast<SPItemCtx const *>(ctx);

    // Any of the modifications defined in sp-object.h might change bbox,
    // so we invalidate it unconditionally
    bbox_valid = FALSE;

    viewport = ictx->viewport; // Cache viewport

    if (flags & (SP_OBJECT_CHILD_MODIFIED_FLAG |
                 SP_OBJECT_MODIFIED_FLAG |
                 SP_OBJECT_STYLE_MODIFIED_FLAG) ) {
        if (flags & SP_OBJECT_MODIFIED_FLAG) {
            for (SPItemView *v = display; v != nullptr; v = v->next) {
                v->arenaitem->setTransform(transform);
            }
        }

        SPClipPath *clip_path = clip_ref ? clip_ref->getObject() : nullptr;
        SPMask *mask = mask_ref ? mask_ref->getObject() : nullptr;

        if ( clip_path || mask ) {
            Geom::OptRect bbox = geometricBounds();
            if (clip_path) {
                for (SPItemView *v = display; v != nullptr; v = v->next) {
                    clip_path->setBBox(v->arenaitem->key(), bbox);
                }
            }
            if (mask) {
                for (SPItemView *v = display; v != nullptr; v = v->next) {
                    mask->sp_mask_set_bbox(v->arenaitem->key(), bbox);
                }
            }
        }

        if (flags & SP_OBJECT_STYLE_MODIFIED_FLAG) {
            for (SPItemView *v = display; v != nullptr; v = v->next) {
                v->arenaitem->setOpacity(SP_SCALE24_TO_FLOAT(style->opacity.value));
                v->arenaitem->setAntialiasing(style->shape_rendering.computed == SP_CSS_SHAPE_RENDERING_CRISPEDGES ? 0 : 2);
                v->arenaitem->setIsolation( style->isolation.value );
                v->arenaitem->setBlendMode( style->mix_blend_mode.value );
                v->arenaitem->setVisible(!isHidden());
            }
        }
    }
    /* Update bounding box in user space, used for filter and objectBoundingBox units */
    if (style->filter.set && display) {
        Geom::OptRect item_bbox = geometricBounds();
        SPItemView *itemview = display;
        do {
            if (itemview->arenaitem)
                itemview->arenaitem->setItemBounds(item_bbox);
        } while ( (itemview = itemview->next) );
    }

    // Update libavoid with item geometry (for connector routing).
    if (avoidRef && document) {
        avoidRef->handleSettingChange();
    }
}

void SPItem::modified(unsigned int /*flags*/)
{
#ifdef OBJECT_TRACE
    objectTrace( "SPItem::modified" );
    objectTrace( "SPItem::modified", false );
#endif
}

Inkscape::XML::Node* SPItem::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
    SPItem *item = this;
    SPItem* object = item;

    // in the case of SP_OBJECT_WRITE_BUILD, the item should always be newly created,
    // so we need to add any children from the underlying object to the new repr
    if (flags & SP_OBJECT_WRITE_BUILD) {
        std::vector<Inkscape::XML::Node *>l;
        for (auto& child: object->children) {
            if (dynamic_cast<SPTitle *>(&child) || dynamic_cast<SPDesc *>(&child)) {
                Inkscape::XML::Node *crepr = child.updateRepr(xml_doc, nullptr, flags);
                if (crepr) {
                    l.push_back(crepr);
                }
            }
        }
        for (auto i = l.rbegin(); i!= l.rend(); ++i) {
            repr->addChild(*i, nullptr);
            Inkscape::GC::release(*i);
        }
    } else {
        for (auto& child: object->children) {
            if (dynamic_cast<SPTitle *>(&child) || dynamic_cast<SPDesc *>(&child)) {
                child.updateRepr(flags);
            }
        }
    }

    repr->setAttributeOrRemoveIfEmpty("transform", sp_svg_transform_write(item->transform));

    if (flags & SP_OBJECT_WRITE_EXT) {
        repr->setAttribute("sodipodi:insensitive", ( item->sensitive ? nullptr : "true" ));
        if (item->transform_center_x != 0)
            sp_repr_set_svg_double (repr, "inkscape:transform-center-x", item->transform_center_x);
        else
            repr->removeAttribute("inkscape:transform-center-x");
        if (item->transform_center_y != 0) {
            auto y = item->transform_center_y;
            y *= -document->yaxisdir();
            sp_repr_set_svg_double (repr, "inkscape:transform-center-y", y);
        } else
            repr->removeAttribute("inkscape:transform-center-y");
    }

    if (item->clip_ref){
        if (item->clip_ref->getObject()) {
            auto value = item->clip_ref->getURI()->cssStr();
            repr->setAttributeOrRemoveIfEmpty("clip-path", value);
        }
    }
    if (item->mask_ref){
        if (item->mask_ref->getObject()) {
            auto value = item->mask_ref->getURI()->cssStr();
            repr->setAttributeOrRemoveIfEmpty("mask", value);
        }
    }
    if (item->_highlightColor){
        repr->setAttribute("inkscape:highlight-color", item->_highlightColor);
    } else {
        repr->removeAttribute("inkscape:highlight-color");
    }

    SPObject::write(xml_doc, repr, flags);

    return repr;
}

// CPPIFY: make pure virtual
Geom::OptRect SPItem::bbox(Geom::Affine const & /*transform*/, SPItem::BBoxType /*type*/) const {
	//throw;
	return Geom::OptRect();
}

Geom::OptRect SPItem::geometricBounds(Geom::Affine const &transform) const
{
    Geom::OptRect bbox;

    // call the subclass method
    // CPPIFY
    //bbox = this->bbox(transform, SPItem::GEOMETRIC_BBOX);
    bbox = const_cast<SPItem*>(this)->bbox(transform, SPItem::GEOMETRIC_BBOX);

    return bbox;
}

Geom::OptRect SPItem::visualBounds(Geom::Affine const &transform, bool wfilter, bool wclip, bool wmask) const
{
    using Geom::X;
    using Geom::Y;

    Geom::OptRect bbox;


    SPFilter *filter = style ? style->getFilter() : nullptr;
    if (filter && wfilter) {
        // call the subclass method
    	// CPPIFY
    	//bbox = this->bbox(Geom::identity(), SPItem::VISUAL_BBOX);
    	bbox = const_cast<SPItem*>(this)->bbox(Geom::identity(), SPItem::GEOMETRIC_BBOX); // see LP Bug 1229971

        // default filer area per the SVG spec:
        SVGLength x, y, w, h;
        Geom::Point minp, maxp;
        x.set(SVGLength::PERCENT, -0.10, 0);
        y.set(SVGLength::PERCENT, -0.10, 0);
        w.set(SVGLength::PERCENT, 1.20, 0);
        h.set(SVGLength::PERCENT, 1.20, 0);

        // if area is explicitly set, override:
        if (filter->x._set)
            x = filter->x;
        if (filter->y._set)
            y = filter->y;
        if (filter->width._set)
            w = filter->width;
        if (filter->height._set)
            h = filter->height;

        double len_x = bbox ? bbox->width() : 0;
        double len_y = bbox ? bbox->height() : 0;

        x.update(12, 6, len_x);
        y.update(12, 6, len_y);
        w.update(12, 6, len_x);
        h.update(12, 6, len_y);

        if (filter->filterUnits == SP_FILTER_UNITS_OBJECTBOUNDINGBOX && bbox) {
            minp[X] = bbox->left() + x.computed * (x.unit == SVGLength::PERCENT ? 1.0 : len_x);
            maxp[X] = minp[X] + w.computed * (w.unit == SVGLength::PERCENT ? 1.0 : len_x);
            minp[Y] = bbox->top() + y.computed * (y.unit == SVGLength::PERCENT ? 1.0 : len_y);
            maxp[Y] = minp[Y] + h.computed * (h.unit == SVGLength::PERCENT ? 1.0 : len_y);
        } else if (filter->filterUnits == SP_FILTER_UNITS_USERSPACEONUSE) {
            minp[X] = x.computed;
            maxp[X] = minp[X] + w.computed;
            minp[Y] = y.computed;
            maxp[Y] = minp[Y] + h.computed;
        }
        bbox = Geom::OptRect(minp, maxp);
        *bbox *= transform;
    } else {
        // call the subclass method
    	// CPPIFY
    	//bbox = this->bbox(transform, SPItem::VISUAL_BBOX);
    	bbox = const_cast<SPItem*>(this)->bbox(transform, SPItem::VISUAL_BBOX);
    }
    if (clip_ref && clip_ref->getObject() && wclip) {
        SPItem *ownerItem = dynamic_cast<SPItem *>(clip_ref->getOwner());
        g_assert(ownerItem != nullptr);
        ownerItem->bbox_valid = FALSE;  // LP Bug 1349018
        bbox.intersectWith(clip_ref->getObject()->geometricBounds(transform));
    }
    if (mask_ref && mask_ref->getObject() && wmask) {
        bbox_valid = false;  // LP Bug 1349018
        bbox.intersectWith(mask_ref->getObject()->visualBounds(transform));
    }

    return bbox;
}

Geom::OptRect SPItem::bounds(BBoxType type, Geom::Affine const &transform) const
{
    if (type == GEOMETRIC_BBOX) {
        return geometricBounds(transform);
    } else {
        return visualBounds(transform);
    }
}

Geom::OptRect SPItem::documentPreferredBounds() const
{
    if (Inkscape::Preferences::get()->getInt("/tools/bounding_box") == 0) {
        return documentBounds(SPItem::VISUAL_BBOX);
    } else {
        return documentBounds(SPItem::GEOMETRIC_BBOX);
    }
}



Geom::OptRect SPItem::documentGeometricBounds() const
{
    return geometricBounds(i2doc_affine());
}

Geom::OptRect SPItem::documentVisualBounds() const
{
    if (!bbox_valid) {
        doc_bbox = visualBounds(i2doc_affine());
        bbox_valid = true;
    }
    return doc_bbox;
}
Geom::OptRect SPItem::documentBounds(BBoxType type) const
{
    if (type == GEOMETRIC_BBOX) {
        return documentGeometricBounds();
    } else {
        return documentVisualBounds();
    }
}

Geom::OptRect SPItem::desktopGeometricBounds() const
{
    return geometricBounds(i2dt_affine());
}

Geom::OptRect SPItem::desktopVisualBounds() const
{
    Geom::OptRect ret = documentVisualBounds();
    if (ret) {
        *ret *= document->doc2dt();
    }
    return ret;
}

Geom::OptRect SPItem::desktopPreferredBounds() const
{
    if (Inkscape::Preferences::get()->getInt("/tools/bounding_box") == 0) {
        return desktopBounds(SPItem::VISUAL_BBOX);
    } else {
        return desktopBounds(SPItem::GEOMETRIC_BBOX);
    }
}

Geom::OptRect SPItem::desktopBounds(BBoxType type) const
{
    if (type == GEOMETRIC_BBOX) {
        return desktopGeometricBounds();
    } else {
        return desktopVisualBounds();
    }
}

unsigned int SPItem::pos_in_parent() const {
    g_assert(parent != nullptr);
    g_assert(SP_IS_OBJECT(parent));

    unsigned int pos = 0;

    for (auto& iter: parent->children) {
        if (&iter == this) {
            return pos;
        }

        if (dynamic_cast<SPItem *>(&iter)) {
            pos++;
        }
    }

    g_assert_not_reached();
    return 0;
}

// CPPIFY: make pure virtual, see below!
void SPItem::snappoints(std::vector<Inkscape::SnapCandidatePoint> & /*p*/, Inkscape::SnapPreferences const */*snapprefs*/) const {
	//throw;
}
    /* This will only be called if the derived class doesn't override this.
     * see for example sp_genericellipse_snappoints in sp-ellipse.cpp
     * We don't know what shape we could be dealing with here, so we'll just
     * do nothing
     */

void SPItem::getSnappoints(std::vector<Inkscape::SnapCandidatePoint> &p, Inkscape::SnapPreferences const *snapprefs) const
{
    // Get the snappoints of the item
	// CPPIFY
	//this->snappoints(p, snapprefs);
	const_cast<SPItem*>(this)->snappoints(p, snapprefs);

    // Get the snappoints at the item's center
    if (snapprefs != nullptr && snapprefs->isTargetSnappable(Inkscape::SNAPTARGET_ROTATION_CENTER)) {
        p.emplace_back(getCenter(), Inkscape::SNAPSOURCE_ROTATION_CENTER, Inkscape::SNAPTARGET_ROTATION_CENTER);
    }

    // Get the snappoints of clipping paths and mask, if any
    std::list<SPObject const *> clips_and_masks;

    if (clip_ref) clips_and_masks.push_back(clip_ref->getObject());
    if (mask_ref) clips_and_masks.push_back(mask_ref->getObject());

    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    for (std::list<SPObject const *>::const_iterator o = clips_and_masks.begin(); o != clips_and_masks.end(); ++o) {
        if (*o) {
            // obj is a group object, the children are the actual clippers
            for(auto& child: (*o)->children) {
                SPItem *item = dynamic_cast<SPItem *>(const_cast<SPObject*>(&child));
                if (item) {
                    std::vector<Inkscape::SnapCandidatePoint> p_clip_or_mask;
                    // Please note the recursive call here!
                    item->getSnappoints(p_clip_or_mask, snapprefs);
                    // Take into account the transformation of the item being clipped or masked
                    for (const auto & p_orig : p_clip_or_mask) {
                        // All snappoints are in desktop coordinates, but the item's transformation is
                        // in document coordinates. Hence the awkward construction below
                        Geom::Point pt = desktop->dt2doc(p_orig.getPoint()) * i2dt_affine();
                        p.emplace_back(pt, p_orig.getSourceType(), p_orig.getTargetType());
                    }
                }
            }
        }
    }
}

// CPPIFY: make pure virtual
void SPItem::print(SPPrintContext* /*ctx*/) {
	//throw;
}

void SPItem::invoke_print(SPPrintContext *ctx)
{
    if ( !isHidden() ) {
    	if (!transform.isIdentity() || style->opacity.value != SP_SCALE24_MAX) {
			ctx->bind(transform, SP_SCALE24_TO_FLOAT(style->opacity.value));
			this->print(ctx);
			ctx->release();
    	} else {
    		this->print(ctx);
    	}
    }
}

const char* SPItem::displayName() const {
    return _("Object");
}

gchar* SPItem::description() const {
    return g_strdup("");
}

gchar *SPItem::detailedDescription() const {
        gchar* s = g_strdup_printf("<b>%s</b> %s",
                    this->displayName(), this->description());

	if (s && clip_ref && clip_ref->getObject()) {
		gchar *snew = g_strdup_printf (_("%s; <i>clipped</i>"), s);
		g_free (s);
		s = snew;
	}

	if (s && mask_ref && mask_ref->getObject()) {
		gchar *snew = g_strdup_printf (_("%s; <i>masked</i>"), s);
		g_free (s);
		s = snew;
	}

	if ( style && style->filter.href && style->filter.href->getObject() ) {
		const gchar *label = style->filter.href->getObject()->label();
		gchar *snew = nullptr;

		if (label) {
			snew = g_strdup_printf (_("%s; <i>filtered (%s)</i>"), s, _(label));
		} else {
			snew = g_strdup_printf (_("%s; <i>filtered</i>"), s);
		}

		g_free (s);
		s = snew;
	}

	return s;
}

bool SPItem::isFiltered() const {
	return (style && style->filter.href && style->filter.href->getObject());
}


SPObject* SPItem::isInMask() const {
    SPObject* parent = this->parent;
    while (parent && !dynamic_cast<SPMask *>(parent)) {
        parent = parent->parent;
    }
    return parent;
}

SPObject* SPItem::isInClipPath() const {
    SPObject* parent = this->parent;
    while (parent && !dynamic_cast<SPClipPath *>(parent)) {
        parent = parent->parent;
    }
    return parent;
}

unsigned SPItem::display_key_new(unsigned numkeys)
{
    static unsigned dkey = 0;

    dkey += numkeys;

    return dkey - numkeys;
}

// CPPIFY: make pure virtual
Inkscape::DrawingItem* SPItem::show(Inkscape::Drawing& /*drawing*/, unsigned int /*key*/, unsigned int /*flags*/) {
	//throw;
	return nullptr;
}

Inkscape::DrawingItem *SPItem::invoke_show(Inkscape::Drawing &drawing, unsigned key, unsigned flags)
{
    Inkscape::DrawingItem *ai = nullptr;

    ai = this->show(drawing, key, flags);

    if (ai != nullptr) {
        Geom::OptRect item_bbox = geometricBounds();

        display = sp_item_view_new_prepend(display, this, flags, key, ai);
        ai->setTransform(transform);
        ai->setOpacity(SP_SCALE24_TO_FLOAT(style->opacity.value));
        ai->setIsolation( style->isolation.value );
        ai->setBlendMode( style->mix_blend_mode.value );
        //ai->setCompositeOperator( style->composite_op.value );
        ai->setVisible(!isHidden());
        ai->setSensitive(sensitive);
        if (clip_ref && clip_ref->getObject()) {
            SPClipPath *cp = clip_ref->getObject();

            if (!display->arenaitem->key()) {
                display->arenaitem->setKey(display_key_new(3));
            }
            int clip_key = display->arenaitem->key();

            // Show and set clip
            Inkscape::DrawingItem *ac = cp->show(drawing, clip_key);
            ai->setClip(ac);

            // Update bbox, in case the clip uses bbox units
            cp->setBBox(clip_key, item_bbox);
            cp->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        }
        if (mask_ref && mask_ref->getObject()) {
            SPMask *mask = mask_ref->getObject();

            if (!display->arenaitem->key()) {
                display->arenaitem->setKey(display_key_new(3));
            }
            int mask_key = display->arenaitem->key();

            // Show and set mask
            Inkscape::DrawingItem *ac = mask->sp_mask_show(drawing, mask_key);
            ai->setMask(ac);

            // Update bbox, in case the mask uses bbox units
            mask->sp_mask_set_bbox(mask_key, item_bbox);
            mask->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        }

        SPPaintServer *fill_ps = style->getFillPaintServer();
        if (fill_ps) {
            if (!display->arenaitem->key()) {
                display->arenaitem->setKey(display_key_new(3));
            }
            int fill_key = display->arenaitem->key();

            Inkscape::DrawingPattern *ap = fill_ps->show(drawing, fill_key, item_bbox);
            ai->setFillPattern(ap);
            if (ap) {
                fill_ps->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            }
        }
        SPPaintServer *stroke_ps = style->getStrokePaintServer();
        if (stroke_ps) {
            if (!display->arenaitem->key()) {
                display->arenaitem->setKey(display_key_new(3));
            }
            int stroke_key = display->arenaitem->key();

            Inkscape::DrawingPattern *ap = stroke_ps->show(drawing, stroke_key, item_bbox);
            ai->setStrokePattern(ap);
            if (ap) {
                stroke_ps->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            }
        }
        ai->setItem(this);
        ai->setItemBounds(geometricBounds());
    }

    return ai;
}

// CPPIFY: make pure virtual
void SPItem::hide(unsigned int /*key*/) {
	//throw;
}

void SPItem::invoke_hide(unsigned key)
{
    this->hide(key);

    SPItemView *ref = nullptr;
    SPItemView *v = display;
    while (v != nullptr) {
        SPItemView *next = v->next;
        if (v->key == key) {
            if (clip_ref && clip_ref->getObject()) {
                (clip_ref->getObject())->hide(v->arenaitem->key());
                v->arenaitem->setClip(nullptr);
            }
            if (mask_ref && mask_ref->getObject()) {
                mask_ref->getObject()->sp_mask_hide(v->arenaitem->key());
                v->arenaitem->setMask(nullptr);
            }
            SPPaintServer *fill_ps = style->getFillPaintServer();
            if (fill_ps) {
                fill_ps->hide(v->arenaitem->key());
            }
            SPPaintServer *stroke_ps = style->getStrokePaintServer();
            if (stroke_ps) {
                stroke_ps->hide(v->arenaitem->key());
            }
            if (!ref) {
                display = v->next;
            } else {
                ref->next = v->next;
            }
            delete v->arenaitem;
            g_free(v);
        } else {
            ref = v;
        }
        v = next;
    }
}

// Adjusters

void SPItem::adjust_pattern(Geom::Affine const &postmul, bool set, PaintServerTransform pt)
{
    bool fill = (pt == TRANSFORM_FILL || pt == TRANSFORM_BOTH);
    if (fill && style && (style->fill.isPaintserver())) {
        SPObject *server = style->getFillPaintServer();
        SPPattern *serverPatt = dynamic_cast<SPPattern *>(server);
        if ( serverPatt ) {
            SPPattern *pattern = serverPatt->clone_if_necessary(this, "fill");
            pattern->transform_multiply(postmul, set);
        }
    }

    bool stroke = (pt == TRANSFORM_STROKE || pt == TRANSFORM_BOTH);
    if (stroke && style && (style->stroke.isPaintserver())) {
        SPObject *server = style->getStrokePaintServer();
        SPPattern *serverPatt = dynamic_cast<SPPattern *>(server);
        if ( serverPatt ) {
            SPPattern *pattern = serverPatt->clone_if_necessary(this, "stroke");
            pattern->transform_multiply(postmul, set);
        }
    }
}

void SPItem::adjust_hatch(Geom::Affine const &postmul, bool set, PaintServerTransform pt)
{
    bool fill = (pt == TRANSFORM_FILL || pt == TRANSFORM_BOTH);
    if (fill && style && (style->fill.isPaintserver())) {
        SPObject *server = style->getFillPaintServer();
        SPHatch *serverHatch = dynamic_cast<SPHatch *>(server);
        if (serverHatch) {
            SPHatch *hatch = serverHatch->clone_if_necessary(this, "fill");
            hatch->transform_multiply(postmul, set);
        }
    }

    bool stroke = (pt == TRANSFORM_STROKE || pt == TRANSFORM_BOTH);
    if (stroke && style && (style->stroke.isPaintserver())) {
        SPObject *server = style->getStrokePaintServer();
        SPHatch *serverHatch = dynamic_cast<SPHatch *>(server);
        if (serverHatch) {
            SPHatch *hatch = serverHatch->clone_if_necessary(this, "stroke");
            hatch->transform_multiply(postmul, set);
        }
    }
}

void SPItem::adjust_gradient( Geom::Affine const &postmul, bool set )
{
    if ( style && style->fill.isPaintserver() ) {
        SPPaintServer *server = style->getFillPaintServer();
        SPGradient *serverGrad = dynamic_cast<SPGradient *>(server);
        if ( serverGrad ) {

            /**
             * \note Bbox units for a gradient are generally a bad idea because
             * with them, you cannot preserve the relative position of the
             * object and its gradient after rotation or skew. So now we
             * convert them to userspace units which are easy to keep in sync
             * just by adding the object's transform to gradientTransform.
             * \todo FIXME: convert back to bbox units after transforming with
             * the item, so as to preserve the original units.
             */
            SPGradient *gradient = sp_gradient_convert_to_userspace( serverGrad, this, "fill" );

            sp_gradient_transform_multiply( gradient, postmul, set );
        }
    }

    if ( style && style->stroke.isPaintserver() ) {
        SPPaintServer *server = style->getStrokePaintServer();
        SPGradient *serverGrad = dynamic_cast<SPGradient *>(server);
        if ( serverGrad ) {
            SPGradient *gradient = sp_gradient_convert_to_userspace( serverGrad, this, "stroke");
            sp_gradient_transform_multiply( gradient, postmul, set );
        }
    }
}

void SPItem::adjust_stroke( gdouble ex )
{
    if (freeze_stroke_width) {
        return;
    }

    SPStyle *style = this->style;

    if (style && !Geom::are_near(ex, 1.0, Geom::EPSILON)) {
        style->stroke_width.computed *= ex;
        style->stroke_width.set = TRUE;

        if ( !style->stroke_dasharray.values.empty() ) {
            for (auto & value : style->stroke_dasharray.values) {
                value.value    *= ex;
                value.computed *= ex;
            }
            style->stroke_dashoffset.value    *= ex;
            style->stroke_dashoffset.computed *= ex;
        }

        updateRepr();
    }
}

/**
 * Find out the inverse of previous transform of an item (from its repr)
 */
Geom::Affine sp_item_transform_repr (SPItem *item)
{
    Geom::Affine t_old(Geom::identity());
    gchar const *t_attr = item->getRepr()->attribute("transform");
    if (t_attr) {
        Geom::Affine t;
        if (sp_svg_transform_read(t_attr, &t)) {
            t_old = t;
        }
    }

    return t_old;
}


void SPItem::adjust_stroke_width_recursive(double expansion)
{
    adjust_stroke (expansion);

// A clone's child is the ghost of its original - we must not touch it, skip recursion
    if ( !dynamic_cast<SPUse *>(this) ) {
        for (auto& o: children) {
            SPItem *item = dynamic_cast<SPItem *>(&o);
            if (item) {
                item->adjust_stroke_width_recursive(expansion);
            }
        }
    }
}

void SPItem::freeze_stroke_width_recursive(bool freeze)
{
    freeze_stroke_width = freeze;

// A clone's child is the ghost of its original - we must not touch it, skip recursion
    if ( !dynamic_cast<SPUse *>(this) ) {
        for (auto& o: children) {
            SPItem *item = dynamic_cast<SPItem *>(&o);
            if (item) {
                item->freeze_stroke_width_recursive(freeze);
            }
        }
    }
}

/**
 * Recursively adjust rx and ry of rects.
 */
static void
sp_item_adjust_rects_recursive(SPItem *item, Geom::Affine advertized_transform)
{
    SPRect *rect = dynamic_cast<SPRect *>(item);
    if (rect) {
    	rect->compensateRxRy(advertized_transform);
    }

    for(auto& o: item->children) {
        SPItem *itm = dynamic_cast<SPItem *>(&o);
        if (itm) {
            sp_item_adjust_rects_recursive(itm, advertized_transform);
        }
    }
}

void SPItem::adjust_paint_recursive(Geom::Affine advertized_transform, Geom::Affine t_ancestors, PaintServerType type)
{
// _Before_ full pattern/gradient transform: t_paint * t_item * t_ancestors
// _After_ full pattern/gradient transform: t_paint_new * t_item * t_ancestors * advertised_transform
// By equating these two expressions we get t_paint_new = t_paint * paint_delta, where:
    Geom::Affine t_item = sp_item_transform_repr (this);
    Geom::Affine paint_delta = t_item * t_ancestors * advertized_transform * t_ancestors.inverse() * t_item.inverse();

// Within text, we do not fork gradients, and so must not recurse to avoid double compensation;
// also we do not recurse into clones, because a clone's child is the ghost of its original -
// we must not touch it
    if (!(dynamic_cast<SPText *>(this) || dynamic_cast<SPUse *>(this))) {
        for (auto& o: children) {
            SPItem *item = dynamic_cast<SPItem *>(&o);
            if (item) {
                // At the level of the transformed item, t_ancestors is identity;
                // below it, it is the accumulated chain of transforms from this level to the top level
                item->adjust_paint_recursive(advertized_transform, t_item * t_ancestors, type);
            }
        }
    }

// We recursed into children first, and are now adjusting this object second;
// this is so that adjustments in a tree are done from leaves up to the root,
// and paintservers on leaves inheriting their values from ancestors could adjust themselves properly
// before ancestors themselves are adjusted, probably differently (bug 1286535)

    switch (type) {
        case PATTERN: {
            adjust_pattern(paint_delta);
            break;
        }
        case HATCH: {
            adjust_hatch(paint_delta);
            break;
        }
        default: {
            adjust_gradient(paint_delta);
        }
    }
}

// CPPIFY:: make pure virtual?
// Not all SPItems must necessarily have a set transform method!
Geom::Affine SPItem::set_transform(Geom::Affine const &transform) {
//	throw;
	return transform;
}

void SPItem::doWriteTransform(Geom::Affine const &transform, Geom::Affine const *adv, bool compensate)
{
    // calculate the relative transform, if not given by the adv attribute
    Geom::Affine advertized_transform;
    if (adv != nullptr) {
        advertized_transform = *adv;
    } else {
        advertized_transform = sp_item_transform_repr (this).inverse() * transform;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (compensate) {
        // recursively compensating for stroke scaling will not always work, because it can be scaled to zero or infinite
        // from which we cannot ever recover by applying an inverse scale; therefore we temporarily block any changes
        // to the strokewidth in such a case instead, and unblock these after the transformation
        // (as reported in https://bugs.launchpad.net/inkscape/+bug/825840/comments/4)
        if (!prefs->getBool("/options/transform/stroke", true)) {
            double const expansion = 1. / advertized_transform.descrim();
            if (expansion < 1e-9 || expansion > 1e9) {
                freeze_stroke_width_recursive(true);
                // This will only work if the item has a set_transform method (in this method adjust_stroke() will be called)
                // We will still have to apply the inverse scaling to other items, not having a set_transform method
                // such as ellipses and stars
                // PS: We cannot use this freeze_stroke_width_recursive() trick in all circumstances. For example, it will
                // break pasting objects within their group (because in such a case the transformation of the group will affect
                // the strokewidth, and has to be compensated for. See https://bugs.launchpad.net/inkscape/+bug/959223/comments/10)
            } else {
                adjust_stroke_width_recursive(expansion);
            }
        }

        // recursively compensate rx/ry of a rect if requested
        if (!prefs->getBool("/options/transform/rectcorners", true)) {
            sp_item_adjust_rects_recursive(this, advertized_transform);
        }

        // recursively compensate pattern fill if it's not to be transformed
        if (!prefs->getBool("/options/transform/pattern", true)) {
            adjust_paint_recursive(advertized_transform.inverse(), Geom::identity(), PATTERN);
        }
        if (!prefs->getBool("/options/transform/hatch", true)) {
            adjust_paint_recursive(advertized_transform.inverse(), Geom::identity(), HATCH);
        }

        /// \todo FIXME: add the same else branch as for gradients below, to convert patterns to userSpaceOnUse as well
        /// recursively compensate gradient fill if it's not to be transformed
        if (!prefs->getBool("/options/transform/gradient", true)) {
            adjust_paint_recursive(advertized_transform.inverse(), Geom::identity(), GRADIENT);
        } else {
            // this converts the gradient/pattern fill/stroke, if any, to userSpaceOnUse; we need to do
            // it here _before_ the new transform is set, so as to use the pre-transform bbox
            adjust_paint_recursive(Geom::identity(), Geom::identity(), GRADIENT);
        }

    } // endif(compensate)

    gint preserve = prefs->getBool("/options/preservetransform/value", false);
    Geom::Affine transform_attr (transform);

    // CPPIFY: check this code.
    // If onSetTransform is not overridden, CItem::onSetTransform will return the transform it was given as a parameter.
    // onSetTransform cannot be pure due to the fact that not all visible Items are transformable.
    SPLPEItem * lpeitem = SP_LPE_ITEM(this);
    if (lpeitem) {
        lpeitem->notifyTransform(transform);
    }
    if ( // run the object's set_transform (i.e. embed transform) only if:
        (dynamic_cast<SPText *>(this) && firstChild() && dynamic_cast<SPTextPath *>(firstChild())) ||
             (!preserve && // user did not chose to preserve all transforms
             (!clip_ref || !clip_ref->getObject()) && // the object does not have a clippath
             (!mask_ref || !mask_ref->getObject()) && // the object does not have a mask
             !(!transform.isTranslation() && style && style->getFilter())) // the object does not have a filter, or the transform is translation (which is supposed to not affect filters)
        )
    {
        transform_attr = this->set_transform(transform);
    }
    if (freeze_stroke_width) {
        freeze_stroke_width_recursive(false);
        if (compensate) {
            if (!prefs->getBool("/options/transform/stroke", true)) {
                // Recursively compensate for stroke scaling, depending on user preference
                // (As to why we need to do this, see the comment a few lines above near the freeze_stroke_width_recursive(true) call)
                double const expansion = 1. / advertized_transform.descrim();
                adjust_stroke_width_recursive(expansion);
            }
        }
    }
    // this avoid temporary scaling issues on display when near identity
    // this must be a bit grater than EPSILON * transform.descrim()
    double e = 1e-5 * transform.descrim();
    if (transform_attr.isIdentity(e)) {
        transform_attr = Geom::Affine();
    }
    set_item_transform(transform_attr);

    // Note: updateRepr comes before emitting the transformed signal since
    // it causes clone SPUse's copy of the original object to brought up to
    // date with the original.  Otherwise, sp_use_bbox returns incorrect
    // values if called in code handling the transformed signal.
    updateRepr();

    if (lpeitem && lpeitem->hasPathEffectRecursive()) {
        sp_lpe_item_update_patheffect(lpeitem, true, false);
    }

    // send the relative transform with a _transformed_signal
    _transformed_signal.emit(&advertized_transform, this);
}

// CPPIFY: see below, do not make pure?
gint SPItem::event(SPEvent* /*event*/) {
	return FALSE;
}

gint SPItem::emitEvent(SPEvent &event)
{
	return this->event(&event);
}

void SPItem::set_item_transform(Geom::Affine const &transform_matrix)
{
    if (!Geom::are_near(transform_matrix, transform, 1e-18)) {
        transform = transform_matrix;
        /* The SP_OBJECT_USER_MODIFIED_FLAG_B is used to mark the fact that it's only a
           transformation.  It's apparently not used anywhere else. */
        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_USER_MODIFIED_FLAG_B);
        sp_item_rm_unsatisfied_cns(*this);
    }
}

//void SPItem::convert_to_guides() const {
//	// CPPIFY: If not overridden, call SPItem::convert_to_guides() const, see below!
//	this->convert_to_guides();
//}


Geom::Affine i2anc_affine(SPObject const *object, SPObject const *const ancestor) {
    Geom::Affine ret(Geom::identity());
    g_return_val_if_fail(object != nullptr, ret);

    /* stop at first non-renderable ancestor */
    while ( object != ancestor && dynamic_cast<SPItem const *>(object) ) {
        SPRoot const *root = dynamic_cast<SPRoot const *>(object);
        if (root) {
            ret *= root->c2p;
        } else {
            SPItem const *item = dynamic_cast<SPItem const *>(object);
            g_assert(item != nullptr);
            ret *= item->transform;
        }
        object = object->parent;
    }
    return ret;
}

Geom::Affine
i2i_affine(SPObject const *src, SPObject const *dest) {
    g_return_val_if_fail(src != nullptr && dest != nullptr, Geom::identity());
    SPObject const *ancestor = src->nearestCommonAncestor(dest);
    return i2anc_affine(src, ancestor) * i2anc_affine(dest, ancestor).inverse();
}

Geom::Affine SPItem::getRelativeTransform(SPObject const *dest) const {
    return i2i_affine(this, dest);
}

Geom::Affine SPItem::i2doc_affine() const
{
    return i2anc_affine(this, nullptr);
}

Geom::Affine SPItem::i2dt_affine() const
{
    Geom::Affine ret(i2doc_affine());
    ret *= document->doc2dt();
    return ret;
}

// TODO should be named "set_i2dt_affine"
void SPItem::set_i2d_affine(Geom::Affine const &i2dt)
{
    Geom::Affine dt2p; /* desktop to item parent transform */
    if (parent) {
        dt2p = static_cast<SPItem *>(parent)->i2dt_affine().inverse();
    } else {
        dt2p = document->dt2doc();
    }

    Geom::Affine const i2p( i2dt * dt2p );
    set_item_transform(i2p);
}


Geom::Affine SPItem::dt2i_affine() const
{
    /* fixme: Implement the right way (Lauris) */
    return i2dt_affine().inverse();
}

/* Item views */

SPItemView *SPItem::sp_item_view_new_prepend(SPItemView *list, SPItem *item, unsigned flags, unsigned key, Inkscape::DrawingItem *drawing_item)
{
    g_assert(item != nullptr);
    g_assert(dynamic_cast<SPItem *>(item) != nullptr);
    g_assert(drawing_item != nullptr);

    SPItemView *new_view = g_new(SPItemView, 1);

    new_view->next = list;
    new_view->flags = flags;
    new_view->key = key;
    new_view->arenaitem = drawing_item;

    return new_view;
}

static SPItemView*
sp_item_view_list_remove(SPItemView *list, SPItemView *view)
{
    SPItemView *ret = list;
    if (view == list) {
        ret = list->next;
    } else {
        SPItemView *prev;
        prev = list;
        while (prev->next != view) prev = prev->next;
        prev->next = view->next;
    }

    delete view->arenaitem;
    g_free(view);

    return ret;
}

Inkscape::DrawingItem *SPItem::get_arenaitem(unsigned key)
{
    for ( SPItemView *iv = display ; iv ; iv = iv->next ) {
        if ( iv->key == key ) {
            return iv->arenaitem;
        }
    }

    return nullptr;
}

int sp_item_repr_compare_position(SPItem const *first, SPItem const *second)
{
    return sp_repr_compare_position(first->getRepr(),
                                    second->getRepr());
}

SPItem const *sp_item_first_item_child(SPObject const *obj)
{
    return sp_item_first_item_child( const_cast<SPObject *>(obj) );
}

SPItem *sp_item_first_item_child(SPObject *obj)
{
    SPItem *child = nullptr;
    for (auto& iter: obj->children) {
        SPItem *tmp = dynamic_cast<SPItem *>(&iter);
        if ( tmp ) {
            child = tmp;
            break;
        }
    }
    return child;
}

void SPItem::convert_to_guides() const {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int prefs_bbox = prefs->getInt("/tools/bounding_box", 0);

    Geom::OptRect bbox = (prefs_bbox == 0) ? desktopVisualBounds() : desktopGeometricBounds();
    if (!bbox) {
        g_warning ("Cannot determine item's bounding box during conversion to guides.\n");
        return;
    }

    std::list<std::pair<Geom::Point, Geom::Point> > pts;

    Geom::Point A((*bbox).min());
    Geom::Point C((*bbox).max());
    Geom::Point B(A[Geom::X], C[Geom::Y]);
    Geom::Point D(C[Geom::X], A[Geom::Y]);

    pts.emplace_back(A, B);
    pts.emplace_back(B, C);
    pts.emplace_back(C, D);
    pts.emplace_back(D, A);

    sp_guide_pt_pairs_to_guides(document, pts);
}

void SPItem::rotate_rel(Geom::Rotate const &rotation)
{
    Geom::Point center = getCenter();
    Geom::Translate const s(getCenter());
    Geom::Affine affine = Geom::Affine(s).inverse() * Geom::Affine(rotation) * Geom::Affine(s);

    // Rotate item.
    set_i2d_affine(i2dt_affine() * (Geom::Affine)affine);
    // Use each item's own transform writer, consistent with sp_selection_apply_affine()
    doWriteTransform(transform);

    // Restore the center position (it's changed because the bbox center changed)
    if (isCenterSet()) {
        setCenter(center * affine);
        updateRepr();
    }
}

void SPItem::scale_rel(Geom::Scale const &scale)
{
    Geom::OptRect bbox = desktopVisualBounds();
    if (bbox) {
        Geom::Translate const s(bbox->midpoint()); // use getCenter?
        set_i2d_affine(i2dt_affine() * s.inverse() * scale * s);
        doWriteTransform(transform);
    }
}

void SPItem::skew_rel(double skewX, double skewY)
{
    Geom::Point center = getCenter();
    Geom::Translate const s(getCenter());

    Geom::Affine const skew(1, skewY, skewX, 1, 0, 0);
    Geom::Affine affine = Geom::Affine(s).inverse() * skew * Geom::Affine(s);

    set_i2d_affine(i2dt_affine() * affine);
    doWriteTransform(transform);

    // Restore the center position (it's changed because the bbox center changed)
    if (isCenterSet()) {
        setCenter(center * affine);
        updateRepr();
    }
}

void SPItem::move_rel( Geom::Translate const &tr)
{
    set_i2d_affine(i2dt_affine() * tr);

    doWriteTransform(transform);
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

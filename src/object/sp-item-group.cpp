// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG <g> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2006 authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>
#include <cstring>
#include <string>

#include "attributes.h"
#include "document.h"
#include "document-undo.h"
#include "selection-chemistry.h"
#include "verbs.h"

#include "display/drawing-group.h"
#include "display/curve.h"
#include "live_effects/effect.h"
#include "live_effects/lpeobject.h"
#include "live_effects/lpeobject-reference.h"
#include "svg/svg.h"
#include "svg/css-ostringstream.h"
#include "xml/repr.h"
#include "xml/sp-css-attr.h"

#include "box3d.h"
#include "persp3d.h"
#include "sp-defs.h"
#include "sp-item-transform.h"
#include "sp-root.h"
#include "sp-rect.h"
#include "sp-offset.h"
#include "sp-clippath.h"
#include "sp-mask.h"
#include "sp-path.h"
#include "sp-use.h"
#include "sp-title.h"
#include "sp-desc.h"
#include "sp-switch.h"
#include "sp-textpath.h"
#include "sp-flowtext.h"
#include "style.h"

using Inkscape::DocumentUndo;

static void sp_group_perform_patheffect(SPGroup *group, SPGroup *top_group, Inkscape::LivePathEffect::Effect *lpe, bool write);

SPGroup::SPGroup() : SPLPEItem(),
    _expanded(false),
    _insert_bottom(false),
    _layer_mode(SPGroup::GROUP)
{
}

SPGroup::~SPGroup() = default;

void SPGroup::build(SPDocument *document, Inkscape::XML::Node *repr) {
    this->readAttr(SPAttr::INKSCAPE_GROUPMODE);

    SPLPEItem::build(document, repr);
}

void SPGroup::release() {
    if (this->_layer_mode == SPGroup::LAYER) {
        this->document->removeResource("layer", this);
    }

    SPLPEItem::release();
}

void SPGroup::child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref) {
    SPLPEItem::child_added(child, ref);

    SPObject *last_child = this->lastChild();
    if (last_child && last_child->getRepr() == child) {
        // optimization for the common special case where the child is being added at the end
        SPItem *item = dynamic_cast<SPItem *>(last_child);
        if ( item ) {
            /* TODO: this should be moved into SPItem somehow */
            SPItemView *v;

            for (v = this->display; v != nullptr; v = v->next) {
                Inkscape::DrawingItem *ac = item->invoke_show (v->arenaitem->drawing(), v->key, v->flags);

                if (ac) {
                    v->arenaitem->appendChild(ac);
                }
            }
        }
    } else {    // general case
        SPItem *item = dynamic_cast<SPItem *>(get_child_by_repr(child));
        if ( item ) {
            /* TODO: this should be moved into SPItem somehow */
            SPItemView *v;
            unsigned position = item->pos_in_parent();

            for (v = this->display; v != nullptr; v = v->next) {
                Inkscape::DrawingItem *ac = item->invoke_show (v->arenaitem->drawing(), v->key, v->flags);

                if (ac) {
                    v->arenaitem->prependChild(ac);
                    ac->setZOrder(position);
                }
            }
        }
    }
    this->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

/* fixme: hide (Lauris) */

void SPGroup::remove_child(Inkscape::XML::Node *child) {
    SPLPEItem::remove_child(child);

    this->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void SPGroup::order_changed (Inkscape::XML::Node *child, Inkscape::XML::Node *old_ref, Inkscape::XML::Node *new_ref)
{
    SPLPEItem::order_changed(child, old_ref, new_ref);

    SPItem *item = dynamic_cast<SPItem *>(get_child_by_repr(child));
    if ( item ) {
        /* TODO: this should be moved into SPItem somehow */
        SPItemView *v;
        unsigned position = item->pos_in_parent();
        for ( v = item->display ; v != nullptr ; v = v->next ) {
            v->arenaitem->setZOrder(position);
        }
    }

    this->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void SPGroup::update(SPCtx *ctx, unsigned int flags) {
    // std::cout << "SPGroup::update(): " << (getId()?getId():"null") << std::endl;
    SPItemCtx *ictx, cctx;

    ictx = (SPItemCtx *) ctx;
    cctx = *ictx;

    unsigned childflags = flags;

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
      childflags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    childflags &= SP_OBJECT_MODIFIED_CASCADE;
    std::vector<SPObject*> l=this->childList(true, SPObject::ActionUpdate);
    for(auto child : l){
        if (childflags || (child->uflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            SPItem *item = dynamic_cast<SPItem *>(child);
            if (item) {
                cctx.i2doc = item->transform * ictx->i2doc;
                cctx.i2vp = item->transform * ictx->i2vp;
                child->updateDisplay((SPCtx *)&cctx, childflags);
            } else {
                child->updateDisplay(ctx, childflags);
            }
        }

        sp_object_unref(child);
    }

    // For a group, we need to update ourselves *after* updating children.
    // this is because the group might contain shapes such as rect or ellipse,
    // which recompute their equivalent path (a.k.a curve) in the update callback,
    // and this is in turn used when computing bbox.
    SPLPEItem::update(ctx, flags);

    if (flags & SP_OBJECT_STYLE_MODIFIED_FLAG) {
        for (SPItemView *v = this->display; v != nullptr; v = v->next) {
            Inkscape::DrawingGroup *group = dynamic_cast<Inkscape::DrawingGroup *>(v->arenaitem);
            if( this->parent ) {
                this->context_style = this->parent->context_style;
            }
            group->setStyle(this->style, this->context_style);
        }
    }
}

void SPGroup::modified(guint flags) {
    //std::cout << "SPGroup::modified(): " << (getId()?getId():"null") << std::endl;
    SPLPEItem::modified(flags);
    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }

    flags &= SP_OBJECT_MODIFIED_CASCADE;

    if (flags & SP_OBJECT_STYLE_MODIFIED_FLAG) {
        for (SPItemView *v = this->display; v != nullptr; v = v->next) {
            Inkscape::DrawingGroup *group = dynamic_cast<Inkscape::DrawingGroup *>(v->arenaitem);
            group->setStyle(this->style);
        }
    }

    std::vector<SPObject*> l=this->childList(true);
    for(auto child : l){
        if (flags || (child->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->emitModified(flags);
        }

        sp_object_unref(child);
    }
}

Inkscape::XML::Node* SPGroup::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
    if (flags & SP_OBJECT_WRITE_BUILD) {
        std::vector<Inkscape::XML::Node *> l;

        if (!repr) {
            if (dynamic_cast<SPSwitch *>(this)) {
                repr = xml_doc->createElement("svg:switch");
            } else {
                repr = xml_doc->createElement("svg:g");
            }
        }

        for (auto& child: children) {
            if ( !dynamic_cast<SPTitle *>(&child) && !dynamic_cast<SPDesc *>(&child) ) {
                Inkscape::XML::Node *crepr = child.updateRepr(xml_doc, nullptr, flags);

                if (crepr) {
                    l.push_back(crepr);
                }
            }
        }
        for (auto i=l.rbegin();i!=l.rend();++i) {
            repr->addChild(*i, nullptr);
            Inkscape::GC::release(*i);
        }
    } else {
        for (auto& child: children) {
            if ( !dynamic_cast<SPTitle *>(&child) && !dynamic_cast<SPDesc *>(&child) ) {
                child.updateRepr(flags);
            }
        }
    }

    if ( flags & SP_OBJECT_WRITE_EXT ) {
        const char *value;
        if ( _layer_mode == SPGroup::LAYER ) {
            value = "layer";
        } else if ( _layer_mode == SPGroup::MASK_HELPER ) {
            value = "maskhelper";
        } else if ( flags & SP_OBJECT_WRITE_ALL ) {
            value = "group";
        } else {
            value = nullptr;
        }

        repr->setAttribute("inkscape:groupmode", value);
    }

    SPLPEItem::write(xml_doc, repr, flags);

    return repr;
}

Geom::OptRect SPGroup::bbox(Geom::Affine const &transform, SPItem::BBoxType bboxtype) const
{
    Geom::OptRect bbox;

    // TODO CPPIFY: replace this const_cast later
    std::vector<SPObject*> l = const_cast<SPGroup*>(this)->childList(false, SPObject::ActionBBox);
    for(auto o : l){
        SPItem *item = dynamic_cast<SPItem *>(o);
        if (item && !item->isHidden()) {
            Geom::Affine const ct(item->transform * transform);
            bbox |= item->bounds(bboxtype, ct);
        }
    }

    return bbox;
}

void SPGroup::print(SPPrintContext *ctx) {
    for(auto& child: children){
        SPObject *o = &child;
        SPItem *item = dynamic_cast<SPItem *>(o);
        if (item) {
            item->invoke_print(ctx);
        }
    }
}

const char *SPGroup::displayName() const {
    return _("Group");
}

gchar *SPGroup::description() const {
    gint len = this->getItemCount();
    return g_strdup_printf(
        ngettext(_("of <b>%d</b> object"), _("of <b>%d</b> objects"), len), len);
}

void SPGroup::set(SPAttr key, gchar const* value) {
    switch (key) {
        case SPAttr::INKSCAPE_GROUPMODE:
            if ( value && !strcmp(value, "layer") ) {
                this->setLayerMode(SPGroup::LAYER);
            } else if ( value && !strcmp(value, "maskhelper") ) {
                this->setLayerMode(SPGroup::MASK_HELPER);
            } else {
                this->setLayerMode(SPGroup::GROUP);
            }
            break;

        default:
            SPLPEItem::set(key, value);
            break;
    }
}

Inkscape::DrawingItem *SPGroup::show (Inkscape::Drawing &drawing, unsigned int key, unsigned int flags) {
    // std::cout << "SPGroup::show(): " << (getId()?getId():"null") << std::endl;
    Inkscape::DrawingGroup *ai;

    ai = new Inkscape::DrawingGroup(drawing);
    ai->setPickChildren(this->effectiveLayerMode(key) == SPGroup::LAYER);
    if( this->parent ) {
        this->context_style = this->parent->context_style;
    }
    ai->setStyle(this->style, this->context_style);

    this->_showChildren(drawing, ai, key, flags);
    return ai;
}

void SPGroup::hide (unsigned int key) {
    std::vector<SPObject*> l=this->childList(false, SPObject::ActionShow);
    for(auto o : l){
        SPItem *item = dynamic_cast<SPItem *>(o);
        if (item) {
            item->invoke_hide(key);
        }
    }

//    SPLPEItem::onHide(key);
}


void SPGroup::snappoints(std::vector<Inkscape::SnapCandidatePoint> &p, Inkscape::SnapPreferences const *snapprefs) const {
    for (auto& o: children)
    {
        SPItem const *item = dynamic_cast<SPItem const *>(&o);
        if (item) {
            item->getSnappoints(p, snapprefs);
        }
    }
}

/**
 * Helper function for ungrouping. Compensates the transform of linked items
 * (clones, linked offset, text-on-path, text with shape-inside) who's source is a
 * direct child of the group being ungrouped (or will be moved to a different
 * group or layer).
 *
 * @param item An object which may be linked to `expected_source`
 * @param expected_source An object who's transform attribute (but not its
 * i2doc transform) will change (later) due to moving to a different group
 * @param source_transform A transform which will be applied to
 * `expected_source` (later) and needs to be compensated in its linked items
 *
 * @post item and its representation are updated
 */
static void _ungroup_compensate_source_transform(SPItem *item, SPItem const *const expected_source,
                                                 Geom::Affine const &source_transform)
{
    if (!item || item->cloned) {
        return;
    }

    SPItem *source = nullptr;
    SPText *item_text = nullptr;
    SPOffset *item_offset = nullptr;
    SPUse *item_use = nullptr;

    if ((item_offset = dynamic_cast<SPOffset *>(item))) {
        source = sp_offset_get_source(item_offset);
    } else if ((item_text = dynamic_cast<SPText *>(item))) {
        source = item_text->get_first_shape_dependency();
    } else if (auto textpath = dynamic_cast<SPTextPath *>(item)) {
        item_text = dynamic_cast<SPText*>(textpath->parent);
        if (!item_text)
            return;
        item = item_text;
        source = sp_textpath_get_path_item(textpath);
    } else if ((item_use = dynamic_cast<SPUse *>(item))) {
        source = item_use->get_original();
    }

    if (source != expected_source) {
        return;
    }

    // FIXME: constructing a transform that would fully preserve the appearance of a
    // textpath if it is ungrouped with its path seems to be impossible in general
    // case. E.g. if the group was squeezed, to keep the ungrouped textpath squeezed
    // as well, we'll need to relink it to some "virtual" path which is inversely
    // stretched relative to the actual path, and then squeeze the textpath back so it
    // would both fit the actual path _and_ be squeezed as before. It's a bummer.

    auto const adv = item->transform.inverse() * source_transform * item->transform;
    double const scale = source_transform.descrim();

    if (item_text) {
        item_text->_adjustFontsizeRecursive(item_text, scale);
    } else if (item_offset) {
        item_offset->rad *= scale;
    } else if (item_use) {
        item->transform = Geom::Translate(item_use->x.computed, item_use->y.computed) * item->transform;
        item_use->x = 0;
        item_use->y = 0;
    }

    if (!item_use) {
        item->adjust_stroke_width_recursive(scale);
        item->adjust_paint_recursive(adv, Geom::identity(), SPItem::PATTERN);
        item->adjust_paint_recursive(adv, Geom::identity(), SPItem::HATCH);
        item->adjust_paint_recursive(adv, Geom::identity(), SPItem::GRADIENT);
    }

    item->transform = source_transform.inverse() * item->transform;
    item->updateRepr();
}

void sp_item_group_ungroup_handle_clones(SPItem *parent, Geom::Affine const g)
{
    // copy the list because the original may get invalidated
    auto hrefListCopy = parent->hrefList;

    for (auto *cobj : hrefListCopy) {
        _ungroup_compensate_source_transform(dynamic_cast<SPItem *>(cobj), parent, g);
    }
}

void
sp_item_group_ungroup (SPGroup *group, std::vector<SPItem*> &children, bool do_done)
{
    g_return_if_fail (group != nullptr);

    SPDocument *doc = group->document;
    SPRoot *root = doc->getRoot();
    SPObject *defs = root->defs;

    Inkscape::XML::Node *grepr = group->getRepr();

    g_return_if_fail (!strcmp (grepr->name(), "svg:g")
                   || !strcmp (grepr->name(), "svg:a")
                   || !strcmp (grepr->name(), "svg:switch")
                   || !strcmp (grepr->name(), "svg:svg"));

    // this converts the gradient/pattern fill/stroke on the group, if any, to userSpaceOnUse
    group->adjust_paint_recursive(Geom::identity(), Geom::identity());

    SPItem *pitem = dynamic_cast<SPItem *>(group->parent);
    g_assert(pitem);
    Inkscape::XML::Node *prepr = pitem->getRepr();

    {
        SPBox3D *box = dynamic_cast<SPBox3D *>(group);
        if (box) {
            group = box->convert_to_group();
        }
    }

    group->removeAllPathEffects(false);

    /* Step 1 - generate lists of children objects */
    std::vector<Inkscape::XML::Node *> items;
    std::vector<Inkscape::XML::Node *> objects;
    Geom::Affine const g = i2anc_affine(group, group->parent);

    if (!g.isIdentity()) {
        for (auto &child : group->children) {
            if (SPItem *citem = dynamic_cast<SPItem *>(&child)) {
                sp_item_group_ungroup_handle_clones(citem, g);
            }
        }
    }

    for (auto& child: group->children) {
        SPItem *citem = dynamic_cast<SPItem *>(&child);
        if (citem) {
            /* Merging of style */
            // this converts the gradient/pattern fill/stroke, if any, to userSpaceOnUse; we need to do
            // it here _before_ the new transform is set, so as to use the pre-transform bbox
            citem->adjust_paint_recursive(Geom::identity(), Geom::identity());

            child.style->merge( group->style );
            /*
             * fixme: We currently make no allowance for the case where child is cloned
             * and the group has any style settings.
             *
             * (This should never occur with documents created solely with the current
             * version of inkscape without using the XML editor: we usually apply group
             * style changes to children rather than to the group itself.)
             *
             * If the group has no style settings, then style->merge() should be a no-op. Otherwise
             * (i.e. if we change the child's style to compensate for its parent going away)
             * then those changes will typically be reflected in any clones of child,
             * whereas we'd prefer for Ungroup not to affect the visual appearance.
             *
             * The only way of preserving styling appearance in general is for child to
             * be put into a new group -- a somewhat surprising response to an Ungroup
             * command.  We could add a new groupmode:transparent that would mostly
             * hide the existence of such groups from the user (i.e. editing behaves as
             * if the transparent group's children weren't in a group), though that's
             * extra complication & maintenance burden and this case is rare.
             */

            // Merging transform
            citem->transform *= g;

            child.updateRepr();

            Inkscape::XML::Node *nrepr = child.getRepr()->duplicate(prepr->document());
            items.push_back(nrepr);

        } else {
            Inkscape::XML::Node *nrepr = child.getRepr()->duplicate(prepr->document());
            objects.push_back(nrepr);
        }
    }

    /* Step 2 - clear group */
    // remember the position of the group
    auto insert_after = group->getRepr()->prev();

    // the group is leaving forever, no heir, clones should take note; its children however are going to reemerge
    group->deleteObject(true, false);

    /* Step 3 - add nonitems */
    if (!objects.empty()) {
        Inkscape::XML::Node *last_def = defs->getRepr()->lastChild();
        for (auto i=objects.rbegin();i!=objects.rend();++i) {
            Inkscape::XML::Node *repr = *i;
            if (!sp_repr_is_meta_element(repr)) {
                defs->getRepr()->addChild(repr, last_def);
            }
            Inkscape::GC::release(repr);
        }
    }

    /* Step 4 - add items */
    for (auto *repr : items) {
        // add item
        prepr->addChild(repr, insert_after);
        insert_after = repr;

        // fill in the children list if non-null
        SPItem *item = static_cast<SPItem *>(doc->getObjectByRepr(repr));

        if (item) {
            item->doWriteTransform(item->transform, nullptr, false);
            children.insert(children.begin(),item);
            item->requestModified(SP_OBJECT_MODIFIED_FLAG);
        } else {
            g_assert_not_reached();
        }

        Inkscape::GC::release(repr);
    }
    if (do_done) {
        DocumentUndo::done(doc, SP_VERB_NONE, _("Ungroup"));
    }
}

/*
 * some API for list aspect of SPGroup
 */

std::vector<SPItem*> sp_item_group_item_list(SPGroup * group)
{
    std::vector<SPItem*> s;
    g_return_val_if_fail(group != nullptr, s);

    for (auto& o: group->children) {
        if ( dynamic_cast<SPItem *>(&o) ) {
            s.push_back((SPItem*)&o);
        }
    }
    return s;
}

SPObject *sp_item_group_get_child_by_name(SPGroup *group, SPObject *ref, const gchar *name)
{
    SPObject *child = (ref) ? ref->getNext() : group->firstChild();
    while ( child && strcmp(child->getRepr()->name(), name) ) {
        child = child->getNext();
    }
    return child;
}

void SPGroup::setLayerMode(LayerMode mode) {
    if ( _layer_mode != mode ) {
        if ( mode == LAYER ) {
            this->document->addResource("layer", this);
        } else if ( _layer_mode == LAYER ) {
            this->document->removeResource("layer", this);
        }
        _layer_mode = mode;
        _updateLayerMode();
    }
}

SPGroup::LayerMode SPGroup::layerDisplayMode(unsigned int dkey) const {
    std::map<unsigned int, LayerMode>::const_iterator iter;
    iter = _display_modes.find(dkey);
    if ( iter != _display_modes.end() ) {
        return (*iter).second;
    } else {
        return GROUP;
    }
}

void SPGroup::setExpanded(bool isexpanded) {
    if ( _expanded != isexpanded ){
        _expanded = isexpanded;
    }
}

void SPGroup::setInsertBottom(bool insertbottom) {
    if ( _insert_bottom != insertbottom) {
        _insert_bottom = insertbottom;
    }
}

void SPGroup::setLayerDisplayMode(unsigned int dkey, SPGroup::LayerMode mode) {
    if ( layerDisplayMode(dkey) != mode ) {
        _display_modes[dkey] = mode;
        _updateLayerMode(dkey);
    }
}

void SPGroup::_updateLayerMode(unsigned int display_key) {
    SPItemView *view;
    for ( view = this->display ; view ; view = view->next ) {
        if ( !display_key || view->key == display_key ) {
            Inkscape::DrawingGroup *g = dynamic_cast<Inkscape::DrawingGroup *>(view->arenaitem);
            if (g) {
                g->setPickChildren(effectiveLayerMode(view->key) == SPGroup::LAYER);
            }
        }
    }
}

void SPGroup::translateChildItems(Geom::Translate const &tr)
{
    if ( hasChildren() ) {
        for (auto& o: children) {
            SPItem *item = dynamic_cast<SPItem *>(&o);
            if ( item ) {
                item->move_rel(tr);
            }
        }
    }
}

// Recursively (or not) scale child items around a point
void SPGroup::scaleChildItemsRec(Geom::Scale const &sc, Geom::Point const &p, bool noRecurse)
{
    if ( hasChildren() ) {
        for (auto& o: children) {
            if ( SPDefs *defs = dynamic_cast<SPDefs *>(&o) ) { // select symbols from defs, ignore clips, masks, patterns
                for (auto& defschild: defs->children) {
                    SPGroup *defsgroup = dynamic_cast<SPGroup *>(&defschild);
                    if (defsgroup)
                        defsgroup->scaleChildItemsRec(sc, p, false);
                }
            } else if ( SPItem *item = dynamic_cast<SPItem *>(&o) ) {
                SPGroup *group = dynamic_cast<SPGroup *>(item);
                if (group && !dynamic_cast<SPBox3D *>(item)) {
                    /* Using recursion breaks clipping because transforms are applied
                       in coordinates for draws but nothing in defs is changed
                       instead change the transform on the entire group, and the transform
                       is applied after any references to clipping paths.  However NOT using
                       recursion apparently breaks as of r13544 other parts of Inkscape
                       involved with showing/modifying units.  So offer both for use
                       in different contexts.
                    */
                    if(noRecurse) {
                        // used for EMF import
                        Geom::Translate const s(p);
                        Geom::Affine final = s.inverse() * sc * s;
                        Geom::Affine tAff = item->i2dt_affine() * final;
                        item->set_i2d_affine(tAff);
                        tAff = item->transform;
                        // Eliminate common rounding error affecting EMF/WMF input.
                        // When the rounding error persists it converts the simple
                        //    transform=scale() to transform=matrix().
                        if(std::abs(tAff[4]) < 1.0e-5 && std::abs(tAff[5]) < 1.0e-5){
                           tAff[4] = 0.0;
                           tAff[5] = 0.0;
                        }
                        item->doWriteTransform(tAff, nullptr, true);
                    } else {
                        // used for other import
                        SPItem *sub_item = nullptr;
                        if (item->getClipObject()) {
                            sub_item = dynamic_cast<SPItem *>(item->getClipObject()->firstChild());
                        }
                        if (sub_item != nullptr) {
                            sub_item->doWriteTransform(sub_item->transform*sc, nullptr, true);
                        }
                        sub_item = nullptr;
                        if (item->getMaskObject()) {
                            sub_item = dynamic_cast<SPItem *>(item->getMaskObject()->firstChild());
                        }
                        if (sub_item != nullptr) {
                            sub_item->doWriteTransform(sub_item->transform*sc, nullptr, true);
                        }
                        item->doWriteTransform(sc.inverse()*item->transform*sc, nullptr, true);
                        group->scaleChildItemsRec(sc, p, false);
                    }
                } else {
//                    Geom::OptRect bbox = item->desktopVisualBounds();
//                    if (bbox) {  // test not needed, this was causing a failure to scale <circle> and <rect> in the clipboard, see LP Bug 1365451
                        // Scale item
                        Geom::Translate const s(p);
                        Geom::Affine final = s.inverse() * sc * s;

                        gchar const *conn_type = nullptr;
                        SPText *text_item = dynamic_cast<SPText *>(item);
                        bool is_text_path = text_item && text_item->firstChild() && dynamic_cast<SPTextPath *>(text_item->firstChild());
                        if (is_text_path) {
                            text_item->optimizeTextpathText();
                        } else {
                            SPFlowtext *flowText = dynamic_cast<SPFlowtext *>(item);
                            if (flowText) {
                                flowText->optimizeScaledText();
                            } else {
                                SPBox3D *box = dynamic_cast<SPBox3D *>(item);
                                if (box) {
                                    // Force recalculation from perspective
                                    box->position_set();
                                } else if (item->getAttribute("inkscape:connector-type") != nullptr
                                           && (item->getAttribute("inkscape:connection-start") == nullptr
                                               || item->getAttribute("inkscape:connection-end") == nullptr)) {
                                    // Remove and store connector type for transform if disconnected
                                    conn_type = item->getAttribute("inkscape:connector-type");
                                    item->removeAttribute("inkscape:connector-type");
                                }
                            }
                        }

                        Persp3D *persp = dynamic_cast<Persp3D *>(item);
                        if (persp) {
                            persp->apply_affine_transformation(final);
                        } else if (is_text_path && !item->transform.isIdentity()) {
                            // Save and reset current transform
                            Geom::Affine tmp(item->transform);
                            item->transform = Geom::Affine();
                            // Apply scale
                            item->set_i2d_affine(item->i2dt_affine() * sc);
                            item->doWriteTransform(item->transform, nullptr, true);
                            // Scale translation and restore original transform
                            tmp[4] *= sc[0];
                            tmp[5] *= sc[1];
                            item->doWriteTransform(tmp, nullptr, true);
                        } else if (dynamic_cast<SPUse *>(item)) {
                            // calculate the matrix we need to apply to the clone
                            // to cancel its induced transform from its original
                            Geom::Affine move = final.inverse() * item->transform * final;
                            item->doWriteTransform(move, &move, true);
                        } else {
                            item->doWriteTransform(item->transform*sc, nullptr, true);
                        }

                        if (conn_type != nullptr) {
                            item->setAttribute("inkscape:connector-type", conn_type);
                        }

                        if (item->isCenterSet() && !(final.isTranslation() || final.isIdentity())) {
                            item->scaleCenter(sc); // All coordinates have been scaled, so also the center must be scaled
                            item->updateRepr();
                        }
//                    }
                }
            }
        }
    }
}

gint SPGroup::getItemCount() const {
    gint len = 0;
    for (auto& child: children) {
        if (dynamic_cast<SPItem const *>(&child)) {
            len++;
        }
    }

    return len;
}

void SPGroup::_showChildren (Inkscape::Drawing &drawing, Inkscape::DrawingItem *ai, unsigned int key, unsigned int flags) {
    Inkscape::DrawingItem *ac = nullptr;
    std::vector<SPObject*> l=this->childList(false, SPObject::ActionShow);
    for(auto o : l){
        SPItem * child = dynamic_cast<SPItem *>(o);
        if (child) {
            ac = child->invoke_show (drawing, key, flags);
            if (ac) {
                ai->appendChild(ac);
            }
        }
    }
}

void SPGroup::update_patheffect(bool write) {
#ifdef GROUP_VERBOSE
    g_message("sp_group_update_patheffect: %p\n", lpeitem);
#endif
    std::vector<SPItem*> const item_list = sp_item_group_item_list(this);

    for (auto sub_item : item_list) {
        if (sub_item) {
            SPLPEItem *lpe_item = dynamic_cast<SPLPEItem *>(sub_item);
            if (lpe_item) {
                lpe_item->update_patheffect(write);
            }
        }
    }

    this->resetClipPathAndMaskLPE();
    if (hasPathEffect() && pathEffectsEnabled()) {
        PathEffectList path_effect_list(*this->path_effect_list);
        for (auto &lperef : path_effect_list) {
            LivePathEffectObject *lpeobj = lperef->lpeobject;
            if (lpeobj) {
                Inkscape::LivePathEffect::Effect *lpe = lpeobj->get_lpe();
                if (lpe && lpe->isVisible()) {
                    lpeobj->get_lpe()->doBeforeEffect_impl(this);
                    sp_group_perform_patheffect(this, this, lpe, write);
                    lpeobj->get_lpe()->doAfterEffect_impl(this, nullptr);
                }
            }
        }
    }
}

static void
sp_group_perform_patheffect(SPGroup *group, SPGroup *top_group, Inkscape::LivePathEffect::Effect *lpe, bool write)
{
    std::vector<SPItem*> const item_list = sp_item_group_item_list(group);
    for (auto sub_item : item_list) {
        SPGroup *sub_group = dynamic_cast<SPGroup *>(sub_item);
        if (sub_group) {
            sp_group_perform_patheffect(sub_group, top_group, lpe, write);
        } else {
            SPShape* sub_shape = dynamic_cast<SPShape *>(sub_item);
            //SPPath*  sub_path  = dynamic_cast<SPPath  *>(sub_item);
            SPItem* clipmaskto = dynamic_cast<SPItem  *>(sub_item);
            if (clipmaskto) {
                top_group->applyToClipPath(clipmaskto, lpe);
                top_group->applyToMask(clipmaskto, lpe);
            }
            if (sub_shape) {
                auto c = SPCurve::copy(sub_shape->curve());
                bool success = false;
                // only run LPEs when the shape has a curve defined
                if (c) {
                    lpe->pathvector_before_effect = c->get_pathvector();
                    c->transform(i2anc_affine(sub_shape, top_group));
                    sub_shape->setCurveInsync(c.get());
                    success = top_group->performOnePathEffect(c.get(), sub_shape, lpe);
                    c->transform(i2anc_affine(sub_shape, top_group).inverse());
                    Inkscape::XML::Node *repr = sub_item->getRepr();
                    if (c && success) {
                        sub_shape->setCurveInsync(c.get());
                        if (lpe->lpeversion.param_getSVGValue() != "0") { // we are on 1 or up
                            sub_shape->bbox_vis_cache_is_valid = false;
                            sub_shape->bbox_geom_cache_is_valid = false;
                        }
                        lpe->pathvector_after_effect = c->get_pathvector();
                        if (write) {
                            repr->setAttribute("d", sp_svg_write_path(lpe->pathvector_after_effect));
#ifdef GROUP_VERBOSE
                            g_message("sp_group_perform_patheffect writes 'd' attribute");
#endif
                        }
                    } else {
                        // LPE was unsuccessful or doeffect stack return null. Read the old 'd'-attribute.
                        if (gchar const * value = repr->attribute("d")) {
                            Geom::PathVector pv = sp_svg_read_pathv(value);
                            sub_shape->setCurve(std::make_unique<SPCurve>(pv));
                        }
                    }
                }
            }
        }
    }
    SPItem* clipmaskto = dynamic_cast<SPItem *>(group);
    if (clipmaskto) {
        top_group->applyToClipPath(clipmaskto, lpe);
        top_group->applyToMask(clipmaskto, lpe);
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

// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * Base class for live path effect items
 */
/*
 * Authors:
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Bastien Bouclet <bgkweb@gmail.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2008 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
#endif

#include <glibmm/i18n.h>

#include "bad-uri-exception.h"

#include "attributes.h"
#include "desktop.h"
#include "display/curve.h"
#include "inkscape.h"
#include "live_effects/effect.h"
#include "live_effects/lpe-bool.h"
#include "live_effects/lpe-clone-original.h"
#include "live_effects/lpe-copy_rotate.h"
#include "live_effects/lpe-lattice2.h"
#include "live_effects/lpe-measure-segments.h"
#include "live_effects/lpe-slice.h"
#include "live_effects/lpe-mirror_symmetry.h"
#include "message-stack.h"
#include "path-chemistry.h"
#include "sp-clippath.h"
#include "sp-ellipse.h"
#include "sp-item-group.h"
#include "sp-mask.h"
#include "sp-path.h"
#include "sp-rect.h"
#include "sp-root.h"
#include "svg/svg.h"
#include "ui/shape-editor.h"
#include "uri.h"

/* LPEItem base class */

static void lpeobject_ref_modified(SPObject *href, guint flags, SPLPEItem *lpeitem);
static void sp_lpe_item_create_original_path_recursive(SPLPEItem *lpeitem);
static void sp_lpe_item_cleanup_original_path_recursive(SPLPEItem *lpeitem, bool keep_paths, bool force = false, bool is_clip_mask = false);

typedef std::list<std::string> HRefList;
static std::string patheffectlist_svg_string(PathEffectList const & list);
static std::string hreflist_svg_string(HRefList const & list);

namespace {
    void clear_path_effect_list(PathEffectList* const l) {
        PathEffectList::iterator it =  l->begin();
        while ( it !=  l->end()) {
            (*it)->unlink();
            delete *it;
            it = l->erase(it);
        }
    }
}

SPLPEItem::SPLPEItem()
    : SPItem()
    , path_effects_enabled(1)
    , path_effect_list(new PathEffectList())
    , lpe_modified_connection_list(new std::list<sigc::connection>())
    , current_path_effect(nullptr)
    , lpe_helperpaths()
{
}

SPLPEItem::~SPLPEItem() = default;

void SPLPEItem::build(SPDocument *document, Inkscape::XML::Node *repr) {
    this->readAttr(SPAttr::INKSCAPE_PATH_EFFECT);

    SPItem::build(document, repr);
}

void SPLPEItem::release() {
    // disconnect all modified listeners:

    for (auto & mod_it : *this->lpe_modified_connection_list)
    {
        mod_it.disconnect();
    }

    delete this->lpe_modified_connection_list;
    this->lpe_modified_connection_list = nullptr;

    clear_path_effect_list(this->path_effect_list);
    // delete the list itself
    delete this->path_effect_list;
    this->path_effect_list = nullptr;

    SPItem::release();
}

void SPLPEItem::set(SPAttr key, gchar const* value) {
    switch (key) {
        case SPAttr::INKSCAPE_PATH_EFFECT:
            {
                this->current_path_effect = nullptr;

                // Disable the path effects while populating the LPE list
                sp_lpe_item_enable_path_effects(this, false);

                // disconnect all modified listeners:
                for (auto & mod_it : *this->lpe_modified_connection_list)
                {
                    mod_it.disconnect();
                }

                this->lpe_modified_connection_list->clear();
                clear_path_effect_list(this->path_effect_list);

                // Parse the contents of "value" to rebuild the path effect reference list
                if ( value ) {
                    std::istringstream iss(value);
                    std::string href;

                    while (std::getline(iss, href, ';'))
                    {
                        Inkscape::LivePathEffect::LPEObjectReference *path_effect_ref = new Inkscape::LivePathEffect::LPEObjectReference(this);

                        try {
                            path_effect_ref->link(href.c_str());
                        } catch (Inkscape::BadURIException &e) {
                            g_warning("BadURIException when trying to find LPE: %s", e.what());
                            path_effect_ref->unlink();
                            delete path_effect_ref;
                            path_effect_ref = nullptr;
                        }

                        this->path_effect_list->push_back(path_effect_ref);

                        if ( path_effect_ref->lpeobject && path_effect_ref->lpeobject->get_lpe() ) {
                            // connect modified-listener
                            this->lpe_modified_connection_list->push_back(
                                                path_effect_ref->lpeobject->connectModified(sigc::bind(sigc::ptr_fun(&lpeobject_ref_modified), this)) );
                        } else {
                            // something has gone wrong in finding the right patheffect.
                            g_warning("Unknown LPE type specified, LPE stack effectively disabled");
                            // keep the effect in the lpestack, so the whole stack is effectively disabled but maintained
                        }
                    }
                }

                sp_lpe_item_enable_path_effects(this, true);
            }
            break;

        default:
            SPItem::set(key, value);
            break;
    }
}

void SPLPEItem::update(SPCtx* ctx, unsigned int flags) {
    SPItem::update(ctx, flags);

    // update the helperpaths of all LPEs applied to the item
    // TODO: re-add for the new node tool
}

void SPLPEItem::modified(unsigned int flags) {
    //stop update when modified and make the effect update on the LPE transform method if the effect require it
    //if (SP_IS_GROUP(this) && (flags & SP_OBJECT_MODIFIED_FLAG) && (flags & SP_OBJECT_USER_MODIFIED_FLAG_B)) {
    //  sp_lpe_item_update_patheffect(this, true, false);
    //}
}

Inkscape::XML::Node* SPLPEItem::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
    if (flags & SP_OBJECT_WRITE_EXT) {
        if ( hasPathEffect() ) {
            repr->setAttributeOrRemoveIfEmpty("inkscape:path-effect", patheffectlist_svg_string(*this->path_effect_list));
        } else {
            repr->removeAttribute("inkscape:path-effect");
        }
    }

    SPItem::write(xml_doc, repr, flags);

    return repr;
}

/**
 * returns true when LPE was successful.
 */
bool SPLPEItem::performPathEffect(SPCurve *curve, SPShape *current, bool is_clip_or_mask) {

    if (!curve) {
        return false;
    }

    if (this->hasPathEffect() && this->pathEffectsEnabled()) {
        PathEffectList path_effect_list(*this->path_effect_list);
        size_t path_effect_list_size = path_effect_list.size();
        for (auto &lperef : path_effect_list) {
            LivePathEffectObject *lpeobj = lperef->lpeobject;
            if (!lpeobj) {
                /** \todo Investigate the cause of this.
                 * For example, this happens when copy pasting an object with LPE applied. Probably because the object is pasted while the effect is not yet pasted to defs, and cannot be found.
                */
                g_warning("SPLPEItem::performPathEffect - NULL lpeobj in list!");
                return false;
            }

            Inkscape::LivePathEffect::Effect *lpe = lpeobj->get_lpe();
            if (!lpe || !performOnePathEffect(curve, current, lpe, is_clip_or_mask)) {
                return false;
            }
            if (path_effect_list_size != this->path_effect_list->size()) {
                break;
            }
        }
    }
    return true;
}

/**
 * returns true when LPE was successful.
 */
bool SPLPEItem::performOnePathEffect(SPCurve *curve, SPShape *current, Inkscape::LivePathEffect::Effect *lpe, bool is_clip_or_mask) {
    if (!lpe) {
        /** \todo Investigate the cause of this.
         * Not sure, but I think this can happen when an unknown effect type is specified...
         */
        g_warning("SPLPEItem::performPathEffect - lpeobj with invalid lpe in the stack!");
        return false;
    }
    if (lpe->isVisible()) {
        if (lpe->acceptsNumClicks() > 0 && !lpe->isReady()) {
            // if the effect expects mouse input before being applied and the input is not finished
            // yet, we don't alter the path
            return false;
        }
        //if is not clip or mask or LPE apply to clip and mask
        if (!is_clip_or_mask || lpe->apply_to_clippath_and_mask) {
            lpe->setCurrentShape(current);
            if (!SP_IS_GROUP(this)) {
                lpe->pathvector_before_effect = curve->get_pathvector();
            }
            // To Calculate BBox on shapes and nested LPE
            current->setCurveInsync(curve);
            if (lpe->lpeversion.param_getSVGValue() != "0") { // we are on 1 or up
                current->bbox_vis_cache_is_valid = false;
                current->bbox_geom_cache_is_valid = false;
            }
            // Groups have their doBeforeEffect called elsewhere
            if (!SP_IS_GROUP(this) && !is_clip_or_mask) {
                lpe->doBeforeEffect_impl(this);
            }

            try {
                lpe->doEffect(curve);
                lpe->has_exception = false;
            }

            catch (std::exception & e) {
                g_warning("Exception during LPE %s execution. \n %s", lpe->getName().c_str(), e.what());
                if (SP_ACTIVE_DESKTOP && SP_ACTIVE_DESKTOP->messageStack()) {
                    SP_ACTIVE_DESKTOP->messageStack()->flash( Inkscape::WARNING_MESSAGE,
                                    _("An exception occurred during execution of the Path Effect.") );
                }
                lpe->doOnException(this);
                return false;
            }


            if (!SP_IS_GROUP(this)) {
                // To have processed the shape to doAfterEffect
                current->setCurveInsync(curve);
                if (curve) {
                    lpe->pathvector_after_effect = curve->get_pathvector();
                }
                lpe->doAfterEffect_impl(this, curve);
            }
        }
    }
    return true;
}

/**
 * returns true when LPE write unoptimiced
 */
bool SPLPEItem::optimizeTransforms()
{
    bool ret = true;
    if (dynamic_cast<SPGroup *>(this)) {
        return false;
    }
    auto* mask_path = this->getMaskObject();
    if(mask_path) {
        return false;
    }
    auto* clip_path = this->getClipObject();
    if(clip_path) {
        return false;
    }
    PathEffectList path_effect_list(*this->path_effect_list);
    for (auto &lperef : path_effect_list) {
        if (!lperef) {
            continue;
        }
        if (!ret) {
            break;
        }
        LivePathEffectObject *lpeobj = lperef->lpeobject;
        if (lpeobj) {
            Inkscape::LivePathEffect::Effect *lpe = lpeobj->get_lpe();
            if (lpe) {
                if (dynamic_cast<Inkscape::LivePathEffect::LPEMeasureSegments *>(lpe) ||
                    dynamic_cast<Inkscape::LivePathEffect::LPECloneOriginal *>(lpe) ||
                    dynamic_cast<Inkscape::LivePathEffect::LPEMirrorSymmetry *>(lpe) ||
                    dynamic_cast<Inkscape::LivePathEffect::LPESlice *>(lpe) ||
                    dynamic_cast<Inkscape::LivePathEffect::LPELattice2 *>(lpe) ||
                    dynamic_cast<Inkscape::LivePathEffect::LPEBool *>(lpe) ||
                    dynamic_cast<Inkscape::LivePathEffect::LPECopyRotate *>(lpe)) {
                    return false;
                }
            }
        }
    }
    gchar *classes = g_strdup(getRepr()->attribute("class"));
    if (classes) {
        Glib::ustring classdata = classes;
        size_t pos = classdata.find("UnoptimicedTransforms");
        if ( pos != std::string::npos ) {
            return false;
        }
    }
    g_free(classes);
    return true;
}

/**
 * notify tranbsform applied to a LPE
 */
void SPLPEItem::notifyTransform(Geom::Affine const &postmul)
{
    if (!pathEffectsEnabled())
        return;

    PathEffectList path_effect_list(*this->path_effect_list);
    for (auto &lperef : path_effect_list) {
        if (!lperef) {
            continue;
        }
        LivePathEffectObject *lpeobj = lperef->lpeobject;
        if (lpeobj) {
            Inkscape::LivePathEffect::Effect *lpe = lpeobj->get_lpe();
            if (lpe && !lpe->is_load) {
                lpe->transform_multiply(postmul, this);
            }
        }
    }
}

// CPPIFY: make pure virtual
void SPLPEItem::update_patheffect(bool /*write*/) {
    //throw;
}

/**
 * Calls any registered handlers for the update_patheffect action
 */
void
sp_lpe_item_update_patheffect (SPLPEItem *lpeitem, bool wholetree, bool write)
{
#ifdef SHAPE_VERBOSE
    g_message("sp_lpe_item_update_patheffect: %p\n", lpeitem);
#endif
    g_return_if_fail (lpeitem != nullptr);
    g_return_if_fail (SP_IS_OBJECT (lpeitem));
    g_return_if_fail (SP_IS_LPE_ITEM (lpeitem));

    // Do not check for LPE item to allow LPE work on clips/mask
    if (!lpeitem->pathEffectsEnabled())
        return;

    SPLPEItem *top = nullptr;

    if (wholetree) {
        SPLPEItem *prev_parent = lpeitem;
        SPLPEItem *parent = dynamic_cast<SPLPEItem*>(prev_parent->parent);
        while (parent && parent->hasPathEffectRecursive()) {
            prev_parent = parent;
            parent = dynamic_cast<SPLPEItem*>(prev_parent->parent);
        }
        top = prev_parent;
    }
    else {
        top = lpeitem;
    }
    top->update_patheffect(write);
}

/**
 * Gets called when any of the lpestack's lpeobject repr contents change: i.e. parameter change in any of the stacked LPEs
 */
static void
lpeobject_ref_modified(SPObject */*href*/, guint flags, SPLPEItem *lpeitem)
{
#ifdef SHAPE_VERBOSE
    g_message("lpeobject_ref_modified");
#endif
    if (flags != 29 && flags != 253) {
        sp_lpe_item_update_patheffect (lpeitem, true, true);
    }
}

static void
sp_lpe_item_create_original_path_recursive(SPLPEItem *lpeitem)
{
    g_return_if_fail(lpeitem != nullptr);

    SPClipPath *clip_path = SP_ITEM(lpeitem)->getClipObject();
    if(clip_path) {
        std::vector<SPObject*> clip_path_list = clip_path->childList(true);
        for (auto iter : clip_path_list) {
            SPLPEItem * clip_data = dynamic_cast<SPLPEItem *>(iter);
            sp_lpe_item_create_original_path_recursive(clip_data);
        }
    }

    SPMask *mask_path = SP_ITEM(lpeitem)->getMaskObject();
    if(mask_path) {
        std::vector<SPObject*> mask_path_list = mask_path->childList(true);
        for (auto iter : mask_path_list) {
            SPLPEItem * mask_data = dynamic_cast<SPLPEItem *>(iter);
            sp_lpe_item_create_original_path_recursive(mask_data);
        }
    }
    if (SP_IS_GROUP(lpeitem)) {
        std::vector<SPItem*> item_list = sp_item_group_item_list(SP_GROUP(lpeitem));
        for (auto subitem : item_list) {
            if (SP_IS_LPE_ITEM(subitem)) {
                sp_lpe_item_create_original_path_recursive(SP_LPE_ITEM(subitem));
            }
        }
    } else if (SPPath * path = dynamic_cast<SPPath *>(lpeitem)) {
        Inkscape::XML::Node *pathrepr = path->getRepr();
        if ( !pathrepr->attribute("inkscape:original-d") ) {
            if (gchar const * value = pathrepr->attribute("d")) {
                Geom::PathVector pv = sp_svg_read_pathv(value);
                pathrepr->setAttribute("inkscape:original-d", value);
                path->setCurveBeforeLPE(std::make_unique<SPCurve>(pv));
            }
        }
    } else if (SPShape * shape = dynamic_cast<SPShape *>(lpeitem)) {
        if (!shape->curveBeforeLPE()) {
            shape->setCurveBeforeLPE(shape->curve());
        }
    }
}

static void
sp_lpe_item_cleanup_original_path_recursive(SPLPEItem *lpeitem, bool keep_paths, bool force, bool is_clip_mask)
{
    g_return_if_fail(lpeitem != nullptr);
    SPItem  *item  = dynamic_cast<SPItem *>(lpeitem);
    if (!item) {
        return;
    }
    SPGroup *group = dynamic_cast<SPGroup*>(lpeitem);
    SPShape *shape = dynamic_cast<SPShape*>(lpeitem);
    SPPath  *path  = dynamic_cast<SPPath *>(lpeitem);
    SPClipPath *clip_path = item->getClipObject();
    if(clip_path) {
        std::vector<SPObject*> clip_path_list = clip_path->childList(true);
        for (auto iter : clip_path_list) {
            SPLPEItem* clip_data = dynamic_cast<SPLPEItem*>(iter);
            if (clip_data) {
                sp_lpe_item_cleanup_original_path_recursive(clip_data, keep_paths, lpeitem && !lpeitem->hasPathEffectRecursive(), true);
            }
        }
    }

    SPMask *mask_path = item->getMaskObject();
    if(mask_path) {
        std::vector<SPObject*> mask_path_list = mask_path->childList(true);
        for (auto iter : mask_path_list) {
            SPLPEItem* mask_data = dynamic_cast<SPLPEItem*>(iter);
            if (mask_data) {
                sp_lpe_item_cleanup_original_path_recursive(mask_data, keep_paths, lpeitem && !lpeitem->hasPathEffectRecursive(), true);
            }
        }
    }

    if (group) {
        std::vector<SPItem*> item_list = sp_item_group_item_list(SP_GROUP(lpeitem));
        for (auto iter : item_list) {
            SPLPEItem* subitem = dynamic_cast<SPLPEItem*>(iter);
            if (subitem) {
                sp_lpe_item_cleanup_original_path_recursive(subitem, keep_paths);
            }
        }
    } else if (path) {
        Inkscape::XML::Node *repr = lpeitem->getRepr();
        if (repr->attribute("inkscape:original-d") &&
            !lpeitem->hasPathEffectRecursive() &&
            (!is_clip_mask ||
            ( is_clip_mask && force)))
        {
            if (!keep_paths) {
                repr->setAttribute("d", repr->attribute("inkscape:original-d"));
            }
            repr->removeAttribute("inkscape:original-d");
            path->setCurveBeforeLPE(nullptr);
            if (!(shape->curve()->get_segment_count())) {
                repr->parent()->removeChild(repr);
            }
        } else {
            if (!keep_paths) {
                sp_lpe_item_update_patheffect(lpeitem, true, true);
            }
        }
    } else if (shape) {
        Inkscape::XML::Node *repr = lpeitem->getRepr();
        SPCurve const *c_lpe = shape->curve();
        if (c_lpe) {
            auto d_str = sp_svg_write_path(c_lpe->get_pathvector());
            {
                if (!lpeitem->hasPathEffectRecursive() &&
                    (!is_clip_mask ||
                    ( is_clip_mask && force)))
                {
                    if (!keep_paths) {
                        repr->removeAttribute("d");
                        shape->setCurveBeforeLPE(nullptr);
                    } else {
                        const char * id = repr->attribute("id");
                        const char * style = repr->attribute("style");
                        // remember the position of the item
                        gint pos = shape->getRepr()->position();
                        // remember parent
                        Inkscape::XML::Node *parent = shape->getRepr()->parent();
                        // remember class
                        char const *class_attr = shape->getRepr()->attribute("class");
                        // remember title
                        gchar *title = shape->title();
                        // remember description
                        gchar *desc = shape->desc();
                        // remember transformation
                        gchar const *transform_str = shape->getRepr()->attribute("transform");
                        // Mask
                        gchar const *mask_str = (gchar *) shape->getRepr()->attribute("mask");
                        // Clip path
                        gchar const *clip_str = (gchar *) shape->getRepr()->attribute("clip-path");

                        /* Rotation center */
                        gchar const *transform_center_x = shape->getRepr()->attribute("inkscape:transform-center-x");
                        gchar const *transform_center_y = shape->getRepr()->attribute("inkscape:transform-center-y");

                        // remember highlight color
                        guint32 highlight_color = 0;
                        if (shape->isHighlightSet())
                            highlight_color = shape->highlight_color();

                        // It's going to resurrect, so we delete without notifying listeners.
                        SPDocument * doc = shape->document;
                        shape->deleteObject(false);
                        Inkscape::XML::Document *xml_doc = doc->getReprDoc();
                        Inkscape::XML::Node *repr = xml_doc->createElement("svg:path");
                        // restore id
                        repr->setAttribute("id", id);
                        // restore class
                        repr->setAttribute("class", class_attr);
                        // restore transform
                        repr->setAttribute("transform", transform_str);
                        // restore clip
                        repr->setAttribute("clip-path", clip_str);
                        // restore mask
                        repr->setAttribute("mask", mask_str);
                        // restore transform_center_x
                        repr->setAttribute("inkscape:transform-center-x", transform_center_x);
                        // restore transform_center_y
                        repr->setAttribute("inkscape:transform-center-y", transform_center_y);
                        //restore d
                        repr->setAttribute("d", d_str);
                        //restore style
                        repr->setAttribute("style", style);
                        // add the new repr to the parent
                        parent->appendChild(repr);
                        SPObject* newObj = doc->getObjectByRepr(repr);
                        if (title && newObj) {
                            newObj->setTitle(title);
                            g_free(title);
                        }
                        if (desc && newObj) {
                            newObj->setDesc(desc);
                            g_free(desc);
                        }
                        if (highlight_color && newObj) {
                            SP_ITEM(newObj)->setHighlightColor( highlight_color );
                        }
                        // move to the saved position
                        repr->setPosition(pos > 0 ? pos : 0);
                        Inkscape::GC::release(repr);
                        lpeitem = dynamic_cast<SPLPEItem *>(newObj);
                    }
                } else {
                    if (!keep_paths) {
                        sp_lpe_item_update_patheffect(lpeitem, true, true);
                    }
                }
            }
        }
    }
}

void SPLPEItem::addPathEffect(std::string value, bool reset)
{
    if (!value.empty()) {
        // Apply the path effects here because in the casse of a group, lpe->resetDefaults
        // needs that all the subitems have their effects applied
        SPGroup *group = dynamic_cast<SPGroup *>(this); 
        if (group) {
            sp_lpe_item_update_patheffect(this, false, true);
        }
        // Disable the path effects while preparing the new lpe
        sp_lpe_item_enable_path_effects(this, false);

        // Add the new reference to the list of LPE references
        HRefList hreflist;
        for (PathEffectList::const_iterator it = this->path_effect_list->begin(); it != this->path_effect_list->end(); ++it)
        {
            hreflist.push_back( std::string((*it)->lpeobject_href) );
        }
        hreflist.push_back(value); // C++11: should be emplace_back std::move'd  (also the reason why passed by value to addPathEffect)

        this->setAttributeOrRemoveIfEmpty("inkscape:path-effect", hreflist_svg_string(hreflist));
        // Make sure that ellipse is stored as <svg:path>
        if( SP_IS_GENERICELLIPSE(this)) {
            SP_GENERICELLIPSE(this)->write( this->getRepr()->document(), this->getRepr(), SP_OBJECT_WRITE_EXT );
        }


        LivePathEffectObject *lpeobj = this->path_effect_list->back()->lpeobject;
        if (lpeobj && lpeobj->get_lpe()) {
            Inkscape::LivePathEffect::Effect *lpe = lpeobj->get_lpe();
            // Ask the path effect to reset itself if it doesn't have parameters yet
            if (reset) {
                // has to be called when all the subitems have their lpes applied
                lpe->resetDefaults(this);
            }
            // Moved here to fix #1299461, we can call previous function twice after
            // if anyone find necessary
            // make sure there is an original-d for paths!!!
            sp_lpe_item_create_original_path_recursive(this);
            // perform this once when the effect is applied
            lpe->doOnApply_impl(this);
        }

        //Enable the path effects now that everything is ready to apply the new path effect
        sp_lpe_item_enable_path_effects(this, true);

        // Apply the path effect
        sp_lpe_item_update_patheffect(this, true, true);
    }
}

void SPLPEItem::addPathEffect(LivePathEffectObject * new_lpeobj)
{
    const gchar * repr_id = new_lpeobj->getRepr()->attribute("id");
    gchar *hrefstr = g_strdup_printf("#%s", repr_id);
    this->addPathEffect(hrefstr, false);
    g_free(hrefstr);
}

/**
 *  If keep_path is true, the item should not be updated, effectively 'flattening' the LPE.
 */
void SPLPEItem::removeCurrentPathEffect(bool keep_paths)
{
    Inkscape::LivePathEffect::LPEObjectReference* lperef = this->getCurrentLPEReference();
    if (!lperef) {
        return;
    }
    if (Inkscape::LivePathEffect::Effect* effect_ = this->getCurrentLPE()) {
        effect_->keep_paths = keep_paths;
        effect_->doOnRemove(this);
        this->path_effect_list->remove(lperef); //current lpe ref is always our 'own' pointer from the path_effect_list
        this->setAttributeOrRemoveIfEmpty("inkscape:path-effect", patheffectlist_svg_string(*this->path_effect_list));
        if (!keep_paths) {
            // Make sure that ellipse is stored as <svg:circle> or <svg:ellipse> if possible.
            if( SP_IS_GENERICELLIPSE(this)) {
                SP_GENERICELLIPSE(this)->write( this->getRepr()->document(), this->getRepr(), SP_OBJECT_WRITE_EXT );
            }
        }
        sp_lpe_item_cleanup_original_path_recursive(this, keep_paths);
    }
}

/**
 *  If keep_path is true, the item should not be updated, effectively 'flattening' the LPE.
 */
void SPLPEItem::removeAllPathEffects(bool keep_paths)
{
    if (keep_paths) {
        if (path_effect_list->empty()) {
            return;
        }
    }
    PathEffectList path_effect_list(*this->path_effect_list);
    for (auto &lperef : path_effect_list) {
        if (!lperef) {
            continue;
        }
        LivePathEffectObject *lpeobj = lperef->lpeobject;
        if (lpeobj) {
            Inkscape::LivePathEffect::Effect * lpe = lpeobj->get_lpe();
            if (lpe) {
                lpe->keep_paths = keep_paths;
                lpe->doOnRemove(this);
            }
        }
    }
    clear_path_effect_list(this->path_effect_list);
    this->removeAttribute("inkscape:path-effect");
    if (!keep_paths) {
        // Make sure that ellipse is stored as <svg:circle> or <svg:ellipse> if possible.
        if (SP_IS_GENERICELLIPSE(this)) {
            SP_GENERICELLIPSE(this)->write(this->getRepr()->document(), this->getRepr(), SP_OBJECT_WRITE_EXT);
        }
    }
    sp_lpe_item_cleanup_original_path_recursive(this, keep_paths);

}

void SPLPEItem::downCurrentPathEffect()
{
    Inkscape::LivePathEffect::LPEObjectReference* lperef = getCurrentLPEReference();
    if (!lperef)
        return;

    PathEffectList new_list = *this->path_effect_list;
    PathEffectList::iterator cur_it = find( new_list.begin(), new_list.end(), lperef );
    if (cur_it != new_list.end()) {
        PathEffectList::iterator down_it = cur_it;
        ++down_it;
        if (down_it != new_list.end()) { // perhaps current effect is already last effect
            std::iter_swap(cur_it, down_it);
        }
    }

    this->setAttributeOrRemoveIfEmpty("inkscape:path-effect", patheffectlist_svg_string(new_list));

    sp_lpe_item_cleanup_original_path_recursive(this, false);
}

void SPLPEItem::upCurrentPathEffect()
{
    Inkscape::LivePathEffect::LPEObjectReference* lperef = getCurrentLPEReference();
    if (!lperef)
        return;

    PathEffectList new_list = *this->path_effect_list;
    PathEffectList::iterator cur_it = find( new_list.begin(), new_list.end(), lperef );
    if (cur_it != new_list.end() && cur_it != new_list.begin()) {
        PathEffectList::iterator up_it = cur_it;
        --up_it;
        std::iter_swap(cur_it, up_it);
    }

    this->setAttributeOrRemoveIfEmpty("inkscape:path-effect", patheffectlist_svg_string(new_list));

    sp_lpe_item_cleanup_original_path_recursive(this, false);
}

/** used for shapes so they can see if they should also disable shape calculation and read from d= */
bool SPLPEItem::hasBrokenPathEffect() const
{
    if (path_effect_list->empty()) {
        return false;
    }

    // go through the list; if some are unknown or invalid, return true
    PathEffectList path_effect_list(*this->path_effect_list);
    for (auto &lperef : path_effect_list) {
        LivePathEffectObject *lpeobj = lperef->lpeobject;
        if (!lpeobj || !lpeobj->get_lpe()) {
            return true;
        }
    }

    return false;
}

bool SPLPEItem::hasPathEffectOfType(int const type, bool is_ready) const
{
    if (path_effect_list->empty()) {
        return false;
    }

    for (PathEffectList::const_iterator it = path_effect_list->begin(); it != path_effect_list->end(); ++it)
    {
        LivePathEffectObject const *lpeobj = (*it)->lpeobject;
        if (lpeobj) {
            Inkscape::LivePathEffect::Effect const* lpe = lpeobj->get_lpe();
            if (lpe && (lpe->effectType() == type)) {
                if (is_ready || lpe->isReady()) {
                    return true;
                }
            }
        }
    }

    return false;
}

/**
 * returns true when any LPE apply to clip or mask.
 */
bool SPLPEItem::hasPathEffectOnClipOrMask(SPLPEItem * shape) const
{
    if (shape->hasPathEffectRecursive()) {
        return true;
    }
    if (!path_effect_list || path_effect_list->empty()) {
        return false;
    }

    PathEffectList path_effect_list(*this->path_effect_list);
    for (auto &lperef : path_effect_list) {
        LivePathEffectObject *lpeobj = lperef->lpeobject;
        if (!lpeobj) {
            continue;
        }
        Inkscape::LivePathEffect::Effect *lpe = lpeobj->get_lpe();
        if (lpe->apply_to_clippath_and_mask) {
            return true;
        }
    }
    return false;
}

/**
 * returns true when any LPE apply to clip or mask. recursive mode
 */
bool SPLPEItem::hasPathEffectOnClipOrMaskRecursive(SPLPEItem * shape) const
{
    SPLPEItem * parent_lpe_item = dynamic_cast<SPLPEItem *>(parent);
    if (parent_lpe_item) {
        return hasPathEffectOnClipOrMask(shape) || parent_lpe_item->hasPathEffectOnClipOrMaskRecursive(shape);
    }
    else {
        return hasPathEffectOnClipOrMask(shape);
    }
}

bool SPLPEItem::hasPathEffect() const
{
    if (!path_effect_list || path_effect_list->empty()) {
        return false;
    }

    // go through the list; if some are unknown or invalid, we are not an LPE item!
    PathEffectList path_effect_list(*this->path_effect_list);
    for (auto &lperef : path_effect_list) {
        LivePathEffectObject *lpeobj = lperef->lpeobject;
        if (!lpeobj || !lpeobj->get_lpe()) {
            return false;
        }
    }

    return true;
}

bool SPLPEItem::hasPathEffectRecursive() const
{
    SPLPEItem * parent_lpe_item = dynamic_cast<SPLPEItem *>(parent);
    if (parent_lpe_item) {
        return hasPathEffect() || parent_lpe_item->hasPathEffectRecursive();
    }
    else {
        return hasPathEffect();
    }
}

void
SPLPEItem::resetClipPathAndMaskLPE(bool fromrecurse)
{
    if (fromrecurse) {
        SPGroup*   group = dynamic_cast<SPGroup  *>(this);
        SPShape*   shape = dynamic_cast<SPShape  *>(this);
        if (group) {
            std::vector<SPItem*> item_list = sp_item_group_item_list(group);
            for (auto iter2 : item_list) {
                SPLPEItem * subitem = dynamic_cast<SPLPEItem *>(iter2);
                if (subitem) {
                    subitem->resetClipPathAndMaskLPE(true);
                }
            }
        } else if (shape) {
            shape->setCurveInsync(SPCurve::copy(shape->curveForEdit()));
            if (!hasPathEffectOnClipOrMaskRecursive(shape)) {
                shape->removeAttribute("inkscape:original-d");
                shape->setCurveBeforeLPE(nullptr);
            } else {
                // make sure there is an original-d for paths!!!
                sp_lpe_item_create_original_path_recursive(shape);
            }
        }
        return;
    }
    SPClipPath *clip_path = this->getClipObject();
    if(clip_path) {
        std::vector<SPObject*> clip_path_list = clip_path->childList(true);
        for (auto iter : clip_path_list) {
            SPGroup*   group = dynamic_cast<SPGroup  *>(iter);
            SPShape*   shape = dynamic_cast<SPShape  *>(iter);
            if (group) {
                std::vector<SPItem*> item_list = sp_item_group_item_list(group);
                for (auto iter2 : item_list) {
                    SPLPEItem * subitem = dynamic_cast<SPLPEItem *>(iter2);
                    if (subitem) {
                        subitem->resetClipPathAndMaskLPE(true);
                    }
                }
            } else if (shape) {
                shape->setCurveInsync(SPCurve::copy(shape->curveForEdit()));
                if (!hasPathEffectOnClipOrMaskRecursive(shape)) {
                    shape->removeAttribute("inkscape:original-d");
                    shape->setCurveBeforeLPE(nullptr);
                } else {
                    // make sure there is an original-d for paths!!!
                    sp_lpe_item_create_original_path_recursive(shape);
                }
            }
        }
    }
    SPMask *mask = this->getMaskObject();
    if(mask) {
        std::vector<SPObject*> mask_list = mask->childList(true);
        for (auto iter : mask_list) {
            SPGroup*   group = dynamic_cast<SPGroup  *>(iter);
            SPShape*   shape = dynamic_cast<SPShape  *>(iter);
            if (group) {
                std::vector<SPItem*> item_list = sp_item_group_item_list(group);
                for (auto iter2 : item_list) {
                    SPLPEItem * subitem = dynamic_cast<SPLPEItem *>(iter2);
                    if (subitem) {
                        subitem->resetClipPathAndMaskLPE(true);
                    }
                }
            } else if (shape) {
                shape->setCurveInsync(SPCurve::copy(shape->curveForEdit()));
                if (!hasPathEffectOnClipOrMaskRecursive(shape)) {
                    shape->removeAttribute("inkscape:original-d");
                    shape->setCurveBeforeLPE(nullptr);
                } else {
                    // make sure there is an original-d for paths!!!
                    sp_lpe_item_create_original_path_recursive(shape);
                }
            }
        }
    }
}

void
SPLPEItem::applyToClipPath(SPItem* to, Inkscape::LivePathEffect::Effect *lpe)
{
    if (lpe && !lpe->apply_to_clippath_and_mask) {
        return;
    }
    SPClipPath *clip_path = to->getClipObject();
    if(clip_path) {
        std::vector<SPObject*> clip_path_list = clip_path->childList(true);
        for (auto clip_data : clip_path_list) {
            applyToClipPathOrMask(SP_ITEM(clip_data), to, lpe);
        }
    }
}

void
SPLPEItem::applyToMask(SPItem* to, Inkscape::LivePathEffect::Effect *lpe)
{
    if (lpe && !lpe->apply_to_clippath_and_mask) {
        return;
    }
    SPMask *mask = to->getMaskObject();
    if(mask) {
        std::vector<SPObject*> mask_list = mask->childList(true);
        for (auto mask_data : mask_list) {
            applyToClipPathOrMask(SP_ITEM(mask_data), to, lpe);
        }
    }
}

void
SPLPEItem::applyToClipPathOrMask(SPItem *clip_mask, SPItem* to, Inkscape::LivePathEffect::Effect *lpe)
{
    SPGroup*   group = dynamic_cast<SPGroup  *>(clip_mask);
    SPShape*   shape = dynamic_cast<SPShape  *>(clip_mask);
    SPRoot *root = this->document->getRoot();
    if (group) {
        std::vector<SPItem*> item_list = sp_item_group_item_list(group);
        for (auto subitem : item_list) {
            applyToClipPathOrMask(subitem, to, lpe);
        }
    } else if (shape) {
        if (sp_version_inside_range(root->version.inkscape, 0, 1, 0, 92)) {
            shape->removeAttribute("inkscape:original-d");
        } else {
            auto c = SPCurve::copy(shape->curve());
            if (c) {
                bool success = false;
                try {
                    if (lpe) {
                        success = this->performOnePathEffect(c.get(), shape, lpe, true);
                    } else {
                        success = this->performPathEffect(c.get(), shape, true);
                    }
                } catch (std::exception & e) {
                    g_warning("Exception during LPE execution. \n %s", e.what());
                    if (SP_ACTIVE_DESKTOP && SP_ACTIVE_DESKTOP->messageStack()) {
                        SP_ACTIVE_DESKTOP->messageStack()->flash( Inkscape::WARNING_MESSAGE,
                                        _("An exception occurred during execution of the Path Effect.") );
                    }
                    success = false;
                }
                if (success && c) {
                    auto str = sp_svg_write_path(c->get_pathvector());
                    shape->setCurveInsync(std::move(c));
                    shape->setAttribute("d", str);
                } else {
                     // LPE was unsuccessful or doeffect stack return null.. Read the old 'd'-attribute.
                    if (gchar const * value = shape->getAttribute("d")) {
                        Geom::PathVector pv = sp_svg_read_pathv(value);
                        shape->setCurve(std::make_unique<SPCurve>(pv));
                    }
                }
                shape->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            }
        }
    }
}

Inkscape::LivePathEffect::Effect*
SPLPEItem::getPathEffectOfType(int type)
{
    PathEffectList path_effect_list(*this->path_effect_list);
    for (auto &lperef : path_effect_list) {
        LivePathEffectObject *lpeobj = lperef->lpeobject;
        if (lpeobj) {
            Inkscape::LivePathEffect::Effect* lpe = lpeobj->get_lpe();
            if (lpe && (lpe->effectType() == type)) {
                return lpe;
            }
        }
    }
    return nullptr;
}

Inkscape::LivePathEffect::Effect const*
SPLPEItem::getPathEffectOfType(int type) const
{
    std::list<Inkscape::LivePathEffect::LPEObjectReference *>::const_iterator i;
    for (i = path_effect_list->begin(); i != path_effect_list->end(); ++i) {
        LivePathEffectObject const *lpeobj = (*i)->lpeobject;
        if (lpeobj) {
            Inkscape::LivePathEffect::Effect const *lpe = lpeobj->get_lpe();
            if (lpe && (lpe->effectType() == type)) {
                return lpe;
            }
        }
    }
    return nullptr;
}

void SPLPEItem::editNextParamOncanvas(SPDesktop *dt)
{
    Inkscape::LivePathEffect::LPEObjectReference *lperef = this->getCurrentLPEReference();
    if (lperef && lperef->lpeobject && lperef->lpeobject->get_lpe()) {
        lperef->lpeobject->get_lpe()->editNextParamOncanvas(this, dt);
    }
}

void SPLPEItem::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) {
    SPItem::child_added(child, ref);

    if (this->hasPathEffectRecursive()) {
        SPObject *ochild = this->get_child_by_repr(child);

        if ( ochild && SP_IS_LPE_ITEM(ochild) ) {
            sp_lpe_item_create_original_path_recursive(SP_LPE_ITEM(ochild));
        }
    }
}
void SPLPEItem::remove_child(Inkscape::XML::Node * child) {
    if (this->hasPathEffectRecursive()) {
        SPObject *ochild = this->get_child_by_repr(child);

        if ( ochild && SP_IS_LPE_ITEM(ochild) ) {
            sp_lpe_item_cleanup_original_path_recursive(SP_LPE_ITEM(ochild), false);
        }
    }

    SPItem::remove_child(child);
}

static std::string patheffectlist_svg_string(PathEffectList const & list)
{
    HRefList hreflist;

    for (auto it : list)
    {
        hreflist.push_back( std::string(it->lpeobject_href) ); // C++11: use emplace_back
    }

    return hreflist_svg_string(hreflist);
}

/**
 *  THE function that should be used to generate any patheffectlist string.
 * one of the methods to change the effect list:
 *  - create temporary href list
 *  - populate the templist with the effects from the old list that you want to have and their order
 *  - call this function with temp list as param
 */
static std::string hreflist_svg_string(HRefList const & list)
{
    std::string r;
    bool semicolon_first = false;

    for (const auto & it : list)
    {
        if (semicolon_first) {
            r += ';';
        }

        semicolon_first = true;

        r += it;
    }

    return r;
}

// Return a copy of the effect list
PathEffectList SPLPEItem::getEffectList()
{
    return *path_effect_list;
}

// Return a copy of the effect list
PathEffectList const SPLPEItem::getEffectList() const
{
    return *path_effect_list;
}

Inkscape::LivePathEffect::LPEObjectReference* 
SPLPEItem::getPrevLPEReference(Inkscape::LivePathEffect::LPEObjectReference* lperef)
{
    Inkscape::LivePathEffect::LPEObjectReference* prev= nullptr;
    for (auto & it : *path_effect_list) {
        if (it->lpeobject_repr == lperef->lpeobject_repr) {
            break;
        }
        prev = it;
    }
    return prev;
}

Inkscape::LivePathEffect::LPEObjectReference* 
SPLPEItem::getNextLPEReference(Inkscape::LivePathEffect::LPEObjectReference* lperef)
{
    bool match = false;
    for (auto & it : *path_effect_list) {
        if (match) {
            return it;
        }
        if (it->lpeobject_repr == lperef->lpeobject_repr) {
            match = true;
        }
    }
    return nullptr;
}

size_t 
SPLPEItem::getLPEReferenceIndex(Inkscape::LivePathEffect::LPEObjectReference* lperef) const
{
    size_t counter = 0;
    for (auto & it : *path_effect_list) {
        if (it->lpeobject_repr == lperef->lpeobject_repr) {
            return counter;
        }
        counter++;
    }
    return Glib::ustring::npos;
}

Inkscape::LivePathEffect::LPEObjectReference* SPLPEItem::getCurrentLPEReference()
{
    if (!this->current_path_effect && !this->path_effect_list->empty()) {
        setCurrentPathEffect(this->path_effect_list->back());
    }

    return this->current_path_effect;
}

Inkscape::LivePathEffect::Effect* SPLPEItem::getCurrentLPE()
{
    Inkscape::LivePathEffect::LPEObjectReference* lperef = getCurrentLPEReference();

    if (lperef && lperef->lpeobject)
        return lperef->lpeobject->get_lpe();
    else
        return nullptr;
}

Inkscape::LivePathEffect::Effect* SPLPEItem::getPrevLPE(Inkscape::LivePathEffect::Effect* lpe)
{
    Inkscape::LivePathEffect::Effect* prev = nullptr;
    for (auto & it : *path_effect_list) {
        if (it->lpeobject == lpe->getLPEObj()) {
            break;
        }
        prev = it->lpeobject->get_lpe();
    }
    return prev;
}

Inkscape::LivePathEffect::Effect* SPLPEItem::getNextLPE(Inkscape::LivePathEffect::Effect* lpe)
{
    bool match = false;
    for (auto & it : *path_effect_list) {
        if (match) {
            return it->lpeobject->get_lpe();
        }
        if (it->lpeobject == lpe->getLPEObj()) {
            match = true;
        }
    }
    return nullptr;
}

size_t SPLPEItem::countLPEOfType(int const type, bool inc_hidden, bool is_ready) const
{
    size_t counter = 0;
    if (path_effect_list->empty()) {
        return counter;
    }

    for (PathEffectList::const_iterator it = path_effect_list->begin(); it != path_effect_list->end(); ++it)
    {
        LivePathEffectObject const *lpeobj = (*it)->lpeobject;
        if (lpeobj) {
            Inkscape::LivePathEffect::Effect const* lpe = lpeobj->get_lpe();
            if (lpe && (lpe->effectType() == type) && (lpe->is_visible || inc_hidden)) {
                if (is_ready || lpe->isReady()) {
                    counter++;
                }
            }
        }
    }

    return counter;
}

size_t 
SPLPEItem::getLPEIndex(Inkscape::LivePathEffect::Effect* lpe) const
{
    size_t counter = 0;
    for (auto & it : *path_effect_list) {
        if (it->lpeobject == lpe->getLPEObj()) {
            return counter;
        }
        counter++;
    }
    return Glib::ustring::npos;
}

bool SPLPEItem::setCurrentPathEffect(Inkscape::LivePathEffect::LPEObjectReference* lperef)
{
    for (auto & it : *path_effect_list) {
        if (it->lpeobject_repr == lperef->lpeobject_repr) {
            this->current_path_effect = it;  // current_path_effect should always be a pointer from the path_effect_list !
            return true;
        }
    }

    return false;
}

/**
 * Writes a new "inkscape:path-effect" string to xml, where the old_lpeobjects are substituted by the new ones.
 *  Note that this method messes up the item's \c PathEffectList.
 */
void SPLPEItem::replacePathEffects( std::vector<LivePathEffectObject const *> const &old_lpeobjs,
                                    std::vector<LivePathEffectObject const *> const &new_lpeobjs )
{
    HRefList hreflist;
    for (PathEffectList::const_iterator it = this->path_effect_list->begin(); it != this->path_effect_list->end(); ++it)
    {
        LivePathEffectObject const * current_lpeobj = (*it)->lpeobject;
        std::vector<LivePathEffectObject const *>::const_iterator found_it(std::find(old_lpeobjs.begin(), old_lpeobjs.end(), current_lpeobj));

        if ( found_it != old_lpeobjs.end() ) {
            std::vector<LivePathEffectObject const *>::difference_type found_index = std::distance (old_lpeobjs.begin(), found_it);
            const gchar * repr_id = new_lpeobjs[found_index]->getRepr()->attribute("id");
            gchar *hrefstr = g_strdup_printf("#%s", repr_id);
            hreflist.push_back( std::string(hrefstr) );
            g_free(hrefstr);
        }
        else {
            hreflist.push_back( std::string((*it)->lpeobject_href) );
        }
    }

    this->setAttributeOrRemoveIfEmpty("inkscape:path-effect", hreflist_svg_string(hreflist));
}

/**
 *  Check all effects in the stack if they are used by other items, and fork them if so.
 *  It is not recommended to fork the effects by yourself calling LivePathEffectObject::fork_private_if_necessary,
 *  use this method instead.
 *  Returns true if one or more effects were forked; returns false if nothing was done.
 */
bool SPLPEItem::forkPathEffectsIfNecessary(unsigned int nr_of_allowed_users, bool recursive)
{
    bool forked = false;
    SPGroup * group = dynamic_cast<SPGroup *>(this);
    if (group && recursive) {
        std::vector<SPItem*> item_list = sp_item_group_item_list(group);
        for (auto child:item_list) {
            SPLPEItem *lpeitem = dynamic_cast<SPLPEItem *>(child);
            if (lpeitem && lpeitem->forkPathEffectsIfNecessary(nr_of_allowed_users, recursive)) {
                forked = true;
            }
        }
    }

    if ( this->hasPathEffect() ) {
        // If one of the path effects is used by 2 or more items, fork it
        // so that each object has its own independent copy of the effect.
        // Note: replacing path effects messes up the path effect list

        // Clones of the LPEItem will increase the refcount of the lpeobjects.
        // Therefore, nr_of_allowed_users should be increased with the number of clones (i.e. refs to the lpeitem)
        nr_of_allowed_users += this->hrefcount;

        std::vector<LivePathEffectObject const *> old_lpeobjs, new_lpeobjs;
        PathEffectList effect_list = this->getEffectList();
        for (auto & it : effect_list)
        {
            LivePathEffectObject *lpeobj = it->lpeobject;
            if (lpeobj) {
                LivePathEffectObject *forked_lpeobj = lpeobj->fork_private_if_necessary(nr_of_allowed_users);
                if (forked_lpeobj && forked_lpeobj != lpeobj) {
                    forked = true;
                    forked_lpeobj->get_lpe()->is_load = true;
                    old_lpeobjs.push_back(lpeobj);
                    new_lpeobjs.push_back(forked_lpeobj);
                }
            }
        }

        if (forked) {
            this->replacePathEffects(old_lpeobjs, new_lpeobjs);
        }
    }

    return forked;
}

// Enable or disable the path effects of the item.
void sp_lpe_item_enable_path_effects(SPLPEItem *lpeitem, bool enable)
{
    if (enable) {
        lpeitem->path_effects_enabled++;
    }
    else {
        lpeitem->path_effects_enabled--;
    }
}

// Are the path effects enabled on this item ?
bool SPLPEItem::pathEffectsEnabled() const
{
    return path_effects_enabled > 0;
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

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Miscellaneous operations on selected items.
 */
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *   MenTaLguY <mental@rydia.net>
 *   bulia byak <buliabyak@users.sf.net>
 *   Andrius R. <knutux@gmail.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Martin Sucha <martin.sucha-inkscape@jts-sro.sk>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *   Tavmjong Bah <tavmjong@free.fr> (Symbol additions)
 *   Adrian Boguszewski
 *   Marc Jeanmougin
 *
 * Copyright (C) 1999-2016 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <boost/range/adaptor/reversed.hpp>
#include <cstring>
#include <map>
#include <string>

#include <glibmm/i18n.h>
#include <gtkmm/clipboard.h>

#include "selection-chemistry.h"

#include "file.h"

#include "context-fns.h"
#include "desktop-style.h"
#include "desktop.h"
#include "document-undo.h"
#include "gradient-drag.h"
#include "layer-fns.h"
#include "layer-manager.h"
#include "layer-model.h"
#include "message-stack.h"
#include "path-chemistry.h"
#include "selection.h"
#include "text-editing.h"
#include "text-chemistry.h"
#include "verbs.h"

#include "display/cairo-utils.h"
#include "display/curve.h"
#include "display/control/canvas-item-bpath.h"

#include "helper/png-write.h"

#include "io/resource.h"

#include "live_effects/effect.h"
#include "live_effects/parameter/originalpath.h"

#include "object/box3d.h"
#include "object/object-set.h"
#include "object/persp3d.h"
#include "object/sp-clippath.h"
#include "object/sp-conn-end.h"
#include "object/sp-defs.h"
#include "object/sp-ellipse.h"
#include "object/sp-flowregion.h"
#include "object/sp-flowtext.h"
#include "object/sp-gradient-reference.h"
#include "object/sp-image.h"
#include "object/sp-item-transform.h"
#include "object/sp-item.h"
#include "object/sp-line.h"
#include "object/sp-linear-gradient.h"
#include "object/sp-marker.h"
#include "object/sp-mask.h"
#include "object/sp-namedview.h"
#include "object/sp-offset.h"
#include "object/sp-path.h"
#include "object/sp-pattern.h"
#include "object/sp-polyline.h"
#include "object/sp-radial-gradient.h"
#include "object/sp-rect.h"
#include "object/sp-root.h"
#include "object/sp-spiral.h"
#include "object/sp-star.h"
#include "object/sp-symbol.h"
#include "object/sp-textpath.h"
#include "object/sp-tref.h"
#include "object/sp-tspan.h"
#include "object/sp-use.h"
#include "style.h"

#include "svg/svg-color.h"
#include "svg/svg.h"

#include "ui/clipboard.h"
#include "ui/tool/control-point-selection.h"
#include "ui/tool/multi-path-manipulator.h"
#include "ui/tools-switch.h"
#include "ui/tools/connector-tool.h"
#include "ui/tools/dropper-tool.h"
#include "ui/tools/gradient-tool.h"
#include "ui/tools/node-tool.h"
#include "ui/tools/text-tool.h"
#include "ui/widget/canvas.h"  // is_dragging()

#include "xml/rebase-hrefs.h"
#include "xml/simple-document.h"

// TODO FIXME: This should be moved into preference repr
SPCycleType SP_CYCLING = SP_CYCLE_FOCUS;

using Inkscape::DocumentUndo;
using Geom::X;
using Geom::Y;
using Inkscape::UI::Tools::GradientTool;
using Inkscape::UI::Tools::NodeTool;
using Inkscape::UI::Tools::TextTool;
using namespace Inkscape;

/* The clipboard handling is in ui/clipboard.cpp now. There are some legacy functions left here,
because the layer manipulation code uses them. It should be rewritten specifically
for that purpose. */


// helper for printing error messages, regardless of whether we have a GUI or not
// If desktop == NULL, errors will be shown on stderr
static void
selection_display_message(SPDesktop *desktop, Inkscape::MessageType msgType, Glib::ustring const &msg)
{
    if (desktop) {
        desktop->messageStack()->flash(msgType, msg);
    } else {
        if (msgType == Inkscape::IMMEDIATE_MESSAGE ||
            msgType == Inkscape::WARNING_MESSAGE ||
            msgType == Inkscape::ERROR_MESSAGE) {
            g_printerr("%s\n", msg.c_str());
        }
    }
}

namespace Inkscape {

void SelectionHelper::selectAll(SPDesktop *dt)
{
    NodeTool *nt = dynamic_cast<NodeTool*>(dt->event_context);
    if (nt) {
        if (!nt->_multipath->empty()) {
            nt->_multipath->selectSubpaths();
            return;
        }
    }
    sp_edit_select_all(dt);
}

void SelectionHelper::selectAllInAll(SPDesktop *dt)
{
    NodeTool *nt = dynamic_cast<NodeTool*>(dt->event_context);
    if (nt) {
        nt->_selected_nodes->selectAll();
    } else {
        sp_edit_select_all_in_all_layers(dt);
    }
}

void SelectionHelper::selectNone(SPDesktop *dt)
{
    NodeTool *nt = dynamic_cast<NodeTool*>(dt->event_context);
    if (nt && !nt->_selected_nodes->empty()) {
        nt->_selected_nodes->clear();
    } else if (!dt->getSelection()->isEmpty()) {
        dt->getSelection()->clear();
    } else {
        // If nothing selected switch to selection tool
        tools_switch(dt, TOOLS_SELECT);
    }
}

void SelectionHelper::selectSameFillStroke(SPDesktop *dt)
{
    sp_select_same_fill_stroke_style(dt, true, true, true);
}

void SelectionHelper::selectSameFillColor(SPDesktop *dt)
{
    sp_select_same_fill_stroke_style(dt, true, false, false);
}

void SelectionHelper::selectSameStrokeColor(SPDesktop *dt)
{
    sp_select_same_fill_stroke_style(dt, false, true, false);
}

void SelectionHelper::selectSameStrokeStyle(SPDesktop *dt)
{
    sp_select_same_fill_stroke_style(dt, false, false, true);
}

void SelectionHelper::selectSameObjectType(SPDesktop *dt)
{
    sp_select_same_object_type(dt);
}

void SelectionHelper::invert(SPDesktop *dt)
{
    NodeTool *nt = dynamic_cast<NodeTool*>(dt->event_context);
    if (nt) {
        nt->_multipath->invertSelectionInSubpaths();
    } else {
        sp_edit_invert(dt);
    }
}

void SelectionHelper::invertAllInAll(SPDesktop *dt)
{
    NodeTool *nt = dynamic_cast<NodeTool*>(dt->event_context);
    if (nt) {
        nt->_selected_nodes->invertSelection();
    } else {
        sp_edit_invert_in_all_layers(dt);
    }
}

void SelectionHelper::reverse(SPDesktop *dt)
{
    // TODO make this a virtual method of event context!
    NodeTool *nt = dynamic_cast<NodeTool*>(dt->event_context);
    if (nt) {
        nt->_multipath->reverseSubpaths();
    } else {
        dt->getSelection()->pathReverse();
    }
}

void SelectionHelper::selectNext(SPDesktop *dt)
{
    NodeTool *nt = dynamic_cast<NodeTool*>(dt->event_context);
    if (nt) {
        nt->_multipath->shiftSelection(1);
        return;
    }

    Inkscape::UI::Tools::ToolBase *ec = dt->event_context;
    GradientTool *gt = dynamic_cast<GradientTool*>(ec);
    if (gt && ec->_grdrag->isNonEmpty()) {
        Inkscape::UI::Tools::sp_gradient_context_select_next(ec);
    } else {
        sp_selection_item_next(dt);
    }
}

void SelectionHelper::selectPrev(SPDesktop *dt)
{
    NodeTool *nt = dynamic_cast<NodeTool*>(dt->event_context);
    if (nt) {
        nt->_multipath->shiftSelection(-1);
        return;
    }

    Inkscape::UI::Tools::ToolBase *ec = dt->event_context;
    GradientTool *gt = dynamic_cast<GradientTool*>(ec);
    if (gt && ec->_grdrag->isNonEmpty()) {
        Inkscape::UI::Tools::sp_gradient_context_select_prev(ec);
    } else {
        sp_selection_item_prev(dt);
    }
}

/*
 * Fixes the current selection, removing locked objects from it
 */
void SelectionHelper::fixSelection(SPDesktop *dt)
{
    if (!dt) {
        return;
    }

    Inkscape::Selection *selection = dt->getSelection();

    std::vector<SPItem*> items;

    auto selList = selection->items();

    for(auto i = boost::rbegin(selList); i != boost::rend(selList); ++i) {
        SPItem *item = *i;
        if( item &&
            !dt->isLayer(item) &&
            (!item->isLocked()))
        {
            items.push_back(item);
        }
    }

    selection->setList(items);
}

} // namespace Inkscape


/**
 * Copies repr and its inherited css style elements, along with the accumulated transform 'full_t',
 * then prepends the copy to 'clip'.
 */
static void sp_selection_copy_one(Inkscape::XML::Node *repr, Geom::Affine full_t, std::vector<Inkscape::XML::Node*> &clip, Inkscape::XML::Document* xml_doc)
{
    Inkscape::XML::Node *copy = repr->duplicate(xml_doc);

    // copy complete inherited style
    SPCSSAttr *css = sp_repr_css_attr_inherited(repr, "style");
    sp_repr_css_set(copy, css, "style");
    sp_repr_css_attr_unref(css);

    // write the complete accumulated transform passed to us
    // (we're dealing with unattached repr, so we write to its attr
    // instead of using sp_item_set_transform)
    copy->setAttributeOrRemoveIfEmpty("transform", sp_svg_transform_write(full_t));

    clip.insert(clip.begin(),copy);
}

static void sp_selection_copy_impl(std::vector<SPItem*> const &items, std::vector<Inkscape::XML::Node*> &clip, Inkscape::XML::Document* xml_doc)
{
    // Sort items:
    std::vector<SPItem*> sorted_items(items);
    sort(sorted_items.begin(),sorted_items.end(),sp_object_compare_position_bool);

    // Copy item reprs:
    for (auto item : sorted_items) {
        if (item) {
            sp_selection_copy_one(item->getRepr(), item->i2doc_affine(), clip, xml_doc);
        } else {
            g_assert_not_reached();
        }
    }
    reverse(clip.begin(),clip.end());
}

// TODO check if parent parameter should be changed to SPItem, of if the code should handle non-items.
static std::vector<Inkscape::XML::Node*> sp_selection_paste_impl(SPDocument *doc, SPObject *parent, std::vector<Inkscape::XML::Node*> &clip)
{
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();

    SPItem *parentItem = dynamic_cast<SPItem *>(parent);
    g_assert(parentItem != nullptr);

    std::vector<Inkscape::XML::Node*> copied;
    // add objects to document
    for (auto repr : clip) {
        Inkscape::XML::Node *copy = repr->duplicate(xml_doc);

        // premultiply the item transform by the accumulated parent transform in the paste layer
        Geom::Affine local(parentItem->i2doc_affine());
        if (!local.isIdentity()) {
            gchar const *t_str = copy->attribute("transform");
            Geom::Affine item_t(Geom::identity());
            if (t_str)
                sp_svg_transform_read(t_str, &item_t);
            item_t *= local.inverse();
            // (we're dealing with unattached repr, so we write to its attr instead of using sp_item_set_transform)
            copy->setAttributeOrRemoveIfEmpty("transform", sp_svg_transform_write(item_t));
        }

        parent->appendChildRepr(copy);
        copied.push_back(copy);
        Inkscape::GC::release(copy);
    }
    return copied;
}

static void sp_selection_delete_impl(std::vector<SPItem*> const &items, bool propagate = true, bool propagate_descendants = true)
{
    for (auto item : items) {
        sp_object_ref(item, nullptr);
    }
    for (auto item : items) {
        item->deleteObject(propagate, propagate_descendants);
        sp_object_unref(item, nullptr);
    }
}


void ObjectSet::deleteItems()
{
    if (desktop()) {
        if(dynamic_cast<TextTool*>(desktop()->event_context)) {
            if (Inkscape::UI::Tools::sp_text_delete_selection(desktop()->event_context)) {
                DocumentUndo::done(desktop()->getDocument(), SP_VERB_CONTEXT_TEXT,
                                   _("Delete text"));
                return;
            }
        }
        // This mirrors the copy() code in clipboard.cpp and allows ::cut() to
        // do the right thing for selected nodes.
        auto node_tool = dynamic_cast<Inkscape::UI::Tools::NodeTool *>(desktop()->event_context);
        if (node_tool && node_tool->_selected_nodes) {
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            // This takes care of undo internally
            node_tool->_multipath->deleteNodes(prefs->getBool("/tools/nodes/delete_preserves_shape", true));
            return;
        }
    }
    if (isEmpty()) {
        selection_display_message(desktop(),Inkscape::WARNING_MESSAGE, _("<b>Nothing</b> was deleted."));
        return;
    }

    std::vector<SPItem*> selected(items().begin(), items().end());
    clear();
    sp_selection_delete_impl(selected);
    if (SPDesktop *d = desktop()) {
        d->currentLayer()->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);

        /* a tool may have set up private information in it's selection context
         * that depends on desktop items.  I think the only sane way to deal with
         * this currently is to reset the current tool, which will reset it's
         * associated selection context.  For example: deleting an object
         * while moving it around the canvas.
         */
        tools_switch( d, tools_active( d ) );
    }

    if(document()) {
        DocumentUndo::done(document(), SP_VERB_EDIT_DELETE, _("Delete"));
    }
}



static void add_ids_recursive(std::vector<const gchar *> &ids, SPObject *obj)
{
    if (obj) {
        ids.push_back(obj->getId());

        if (dynamic_cast<SPGroup *>(obj)) {
            for (auto& child: obj->children) {
                add_ids_recursive(ids, &child);
            }
        }
    }
}

void ObjectSet::duplicate(bool suppressDone, bool duplicateLayer)
{
    if(duplicateLayer && !desktop() ){
        //TODO: understand why layer management is tied to desktop and not to document.
        return;
    }

    SPDocument *doc = document();

    if(!doc)
        return;

    Inkscape::XML::Document* xml_doc = doc->getReprDoc();

    // check if something is selected
    if (isEmpty() && !duplicateLayer) {
        selection_display_message(desktop(),Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to duplicate."));
        return;
    }
    std::vector<Inkscape::XML::Node*> reprs(xmlNodes().begin(), xmlNodes().end());

    if(duplicateLayer){
        reprs.clear();
        reprs.push_back(desktop()->currentLayer()->getRepr());
    }

    clear();

    // sorting items from different parents sorts each parent's subset without possibly mixing
    // them, just what we need
    sort(reprs.begin(),reprs.end(),sp_repr_compare_position_bool);

    std::vector<const gchar *> old_ids;
    std::vector<const gchar *> new_ids;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool relink_clones = prefs->getBool("/options/relinkclonesonduplicate/value");
    const bool fork_livepatheffects = prefs->getBool("/options/forklpeonduplicate/value", true);

    // check ref-d shapes, split in defs|internal|external
    // add external & defs to reprs
    auto text_refs = text_categorize_refs(doc, reprs.begin(), reprs.end(),
            static_cast<text_ref_t>(TEXT_REF_DEF | TEXT_REF_EXTERNAL | TEXT_REF_INTERNAL));
    for (auto const &ref : text_refs) {
        if (ref.second == TEXT_REF_DEF || ref.second == TEXT_REF_EXTERNAL) {
            reprs.push_back(doc->getObjectById(ref.first)->getRepr());
        }
    }

    std::vector<Inkscape::XML::Node*> copies;
    for(auto old_repr : reprs) {
        Inkscape::XML::Node *parent = old_repr->parent();
        Inkscape::XML::Node *copy = old_repr->duplicate(xml_doc);

        if (!duplicateLayer || sp_repr_is_def(old_repr)) {
            parent->appendChild(copy);
        } else if (sp_repr_is_layer(old_repr)) {
            parent->addChild(copy, old_repr);
        } else {
            // duplicateLayer, non-layer, non-def
            // external nodes -- append to new layer
            // text_relink will ignore extra nodes in layer children
            copies[0]->appendChild(copy);
        }

        if (relink_clones) {
            SPObject *old_obj = doc->getObjectByRepr(old_repr);
            SPObject *new_obj = doc->getObjectByRepr(copy);
            add_ids_recursive(old_ids, old_obj);
            add_ids_recursive(new_ids, new_obj);
        }

        if (fork_livepatheffects) {
            SPObject *new_obj = doc->getObjectByRepr(copy);
            SPLPEItem *newLPEObj = dynamic_cast<SPLPEItem *>(new_obj);
            if (newLPEObj) {
                newLPEObj->forkPathEffectsIfNecessary(1);
            }
        }

        copies.push_back(copy);
        Inkscape::GC::release(copy);
    }

    // Relink copied text nodes to copied reference shapes
    text_relink_refs(text_refs, reprs.begin(), reprs.end(), copies.begin());

    // copies contains def nodes, we don't want that in our selection
    std::vector<Inkscape::XML::Node*> newsel;
    if (!duplicateLayer) {
        // compute newsel, by removing def nodes from copies
        for (auto node : copies) {
            if (!sp_repr_is_def(node)) {
                newsel.push_back(node);
            }
        }
    }

    if (relink_clones) {

        g_assert(old_ids.size() == new_ids.size());

        for (unsigned int i = 0; i < old_ids.size(); i++) {
            const gchar *id = old_ids[i];
            SPObject *old_clone = doc->getObjectById(id);
            SPUse *use = dynamic_cast<SPUse *>(old_clone);
            SPOffset *offset = dynamic_cast<SPOffset *>(old_clone);
            SPText *text = dynamic_cast<SPText *>(old_clone);
            SPPath *path = dynamic_cast<SPPath *>(old_clone);
            if (use) {
                SPItem *orig = use->get_original();
                if (!orig) // orphaned
                    continue;
                for (unsigned int j = 0; j < old_ids.size(); j++) {
                    if (!strcmp(orig->getId(), old_ids[j])) {
                        // we have both orig and clone in selection, relink
                        // std::cout << id  << " old, its ori: " << orig->getId() << "; will relink:" << new_ids[i] << " to " << new_ids[j] << "\n";
                        SPObject *new_clone = doc->getObjectById(new_ids[i]);
                        new_clone->setAttribute("xlink:href", Glib::ustring("#") + new_ids[j]);
                        new_clone->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
                    }
                }
            } else if (offset) {
                gchar *source_href = offset->sourceHref;
                for (guint j = 0; j < old_ids.size(); j++) {
                    if (source_href && source_href[0]=='#' && !strcmp(source_href+1, old_ids[j])) {
                        doc->getObjectById(new_ids[i])->setAttribute("xlink:href", Glib::ustring("#") + new_ids[j]);
                    }
                }
            } else if (text) {
                SPTextPath *textpath = dynamic_cast<SPTextPath *>(text->firstChild());
                if (!textpath) continue;
                const gchar *source_href = sp_textpath_get_path_item(textpath)->getId();
                for (guint j = 0; j < old_ids.size(); j++) {
                    if (!strcmp(source_href, old_ids[j])) {
                        doc->getObjectById(new_ids[i])->firstChild()->setAttribute("xlink:href", Glib::ustring("#") + new_ids[j]);
                    }
                }
            } else if (path) {
                if (old_clone->getAttribute("inkscape:connection-start") != nullptr) {
                    const char *old_start = old_clone->getAttribute("inkscape:connection-start");
                    const char *old_end = old_clone->getAttribute("inkscape:connection-end");
                    SPObject *new_clone = doc->getObjectById(new_ids[i]);
                    for (guint j = 0; j < old_ids.size(); j++) {
                        if(old_start == Glib::ustring("#") + old_ids[j]) {
                            new_clone->setAttribute("inkscape:connection-start", Glib::ustring("#") + new_ids[j]);
                        }
                        if(old_end == Glib::ustring("#") + old_ids[j]) {
                            new_clone->setAttribute("inkscape:connection-end", Glib::ustring("#") + new_ids[j]);
                        }
                    }
                }
            }
        }
    }


    if ( !suppressDone ) {
        DocumentUndo::done(document(), SP_VERB_EDIT_DUPLICATE,
                           _("Duplicate"));
    }
    if(!duplicateLayer)
        setReprList(newsel);
    else{
        SPObject* new_layer = doc->getObjectByRepr(copies[0]);
        gchar* name = g_strdup_printf(_("%s copy"), new_layer->label());
        desktop()->layer_manager->renameLayer( new_layer, name, TRUE );
        g_free(name);
    }
}

void sp_edit_clear_all(Inkscape::Selection *selection)
{
    if (!selection)
        return;

    SPDocument *doc = selection->layers()->getDocument();
    selection->clear();

    SPGroup *group = dynamic_cast<SPGroup *>(selection->layers()->currentLayer());
    g_return_if_fail(group != nullptr);
    std::vector<SPItem*> items = sp_item_group_item_list(group);

    for(auto & item : items){
        item->deleteObject();
    }

    DocumentUndo::done(doc, SP_VERB_EDIT_CLEAR_ALL,
                       _("Delete all"));
}

/*
 * Return a list of SPItems that are the children of 'list'
 *
 * list - source list of items to search in
 * desktop - desktop associated with the source list
 * exclude - list of items to exclude from result
 * onlyvisible - TRUE includes only items visible on canvas
 * onlysensitive - TRUE includes only non-locked items
 * ingroups - TRUE to recursively get grouped items children
 */
std::vector<SPItem*> &get_all_items(std::vector<SPItem*> &list, SPObject *from, SPDesktop *desktop, bool onlyvisible, bool onlysensitive, bool ingroups, std::vector<SPItem*> const &exclude)
{
    for (auto& child: from->children) {
        SPItem *item = dynamic_cast<SPItem *>(&child);
        if (item &&
            !desktop->isLayer(item) &&
            (!onlysensitive || !item->isLocked()) &&
            (!onlyvisible || !desktop->itemIsHidden(item)) &&
            (exclude.empty() || exclude.end() == std::find(exclude.begin(), exclude.end(), &child))
            )
        {
            list.insert(list.begin(),item);
        }

        if (ingroups || (item && desktop->isLayer(item))) {
            list = get_all_items(list, &child, desktop, onlyvisible, onlysensitive, ingroups, exclude);
        }
    }

    return list;
}

static void sp_edit_select_all_full(SPDesktop *dt, bool force_all_layers, bool invert)
{
    if (!dt)
        return;

    Inkscape::Selection *selection = dt->getSelection();

    g_return_if_fail(dynamic_cast<SPGroup *>(dt->currentLayer()));

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    PrefsSelectionContext inlayer = (PrefsSelectionContext) prefs->getInt("/options/kbselection/inlayer", PREFS_SELECTION_LAYER);
    bool onlyvisible = prefs->getBool("/options/kbselection/onlyvisible", true);
    bool onlysensitive = prefs->getBool("/options/kbselection/onlysensitive", true);

    std::vector<SPItem*> items ;

    std::vector<SPItem*> exclude;
    if (invert) {
        exclude.insert(exclude.end(), selection->items().begin(), selection->items().end());
    }

    if (force_all_layers)
        inlayer = PREFS_SELECTION_ALL;

    switch (inlayer) {
        case PREFS_SELECTION_LAYER: {
        if ( (onlysensitive && dynamic_cast<SPItem *>(dt->currentLayer())->isLocked()) ||
             (onlyvisible && dt->itemIsHidden(dynamic_cast<SPItem *>(dt->currentLayer()))) )
        return;

        std::vector<SPItem*> all_items = sp_item_group_item_list(dynamic_cast<SPGroup *>(dt->currentLayer()));

        for (std::vector<SPItem*>::const_reverse_iterator i=all_items.rbegin();i!=all_items.rend();++i) {
            SPItem *item = *i;

            if (item && (!onlysensitive || !item->isLocked())) {
                if (!onlyvisible || !dt->itemIsHidden(item)) {
                    if (!dt->isLayer(item)) {
                        if (!invert || exclude.end() == std::find(exclude.begin(),exclude.end(),item)) {
                            items.push_back(item); // leave it in the list
                        }
                    }
                }
            }
        }

            break;
        }
        case PREFS_SELECTION_LAYER_RECURSIVE: {
            std::vector<SPItem*> x;
            items = get_all_items(x, dt->currentLayer(), dt, onlyvisible, onlysensitive, FALSE, exclude);
            break;
        }
        default: {
            std::vector<SPItem*> x;
            items = get_all_items(x, dt->currentRoot(), dt, onlyvisible, onlysensitive, FALSE, exclude);
            break;
    }
    }

    selection->setList(items);

}

void sp_edit_select_all(SPDesktop *desktop)
{
    sp_edit_select_all_full(desktop, false, false);
}

void sp_edit_select_all_in_all_layers(SPDesktop *desktop)
{
    sp_edit_select_all_full(desktop, true, false);
}

void sp_edit_invert(SPDesktop *desktop)
{
    sp_edit_select_all_full(desktop, false, true);
}

void sp_edit_invert_in_all_layers(SPDesktop *desktop)
{
    sp_edit_select_all_full(desktop, true, true);
}

Inkscape::XML::Node* ObjectSet::group() {
    SPDocument *doc = document();
    if(!doc)
        return nullptr;
    if (isEmpty()) {
        selection_display_message(desktop(), Inkscape::WARNING_MESSAGE, _("Select <b>some objects</b> to group."));
        return nullptr;
    }
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();
    Inkscape::XML::Node *group = xml_doc->createElement("svg:g");

    std::vector<Inkscape::XML::Node*> p(xmlNodes().begin(), xmlNodes().end());
    std::sort(p.begin(), p.end(), sp_repr_compare_position_bool);
    this->clear();

    // Remember the position and parent of the topmost object.
    gint topmost = p.back()->position();
    Inkscape::XML::Node *topmost_parent = p.back()->parent();

    for(auto current : p){
        if (current->parent() == topmost_parent) {
            Inkscape::XML::Node *spnew = current->duplicate(xml_doc);
            sp_repr_unparent(current);
            group->appendChild(spnew);
            Inkscape::GC::release(spnew);
            topmost --; // only reduce count for those items deleted from topmost_parent
        } else { // move it to topmost_parent first
            std::vector<Inkscape::XML::Node*> temp_clip;

            // At this point, current may already have no item, due to its being a clone whose original is already moved away
            // So we copy it artificially calculating the transform from its repr->attr("transform") and the parent transform
            gchar const *t_str = current->attribute("transform");
            Geom::Affine item_t(Geom::identity());
            if (t_str)
                sp_svg_transform_read(t_str, &item_t);
            item_t *= dynamic_cast<SPItem *>(doc->getObjectByRepr(current->parent()))->i2doc_affine();
            // FIXME: when moving both clone and original from a transformed group (either by
            // grouping into another parent, or by cut/paste) the transform from the original's
            // parent becomes embedded into original itself, and this affects its clones. Fix
            // this by remembering the transform diffs we write to each item into an array and
            // then, if this is clone, looking up its original in that array and pre-multiplying
            // it by the inverse of that original's transform diff.

            sp_selection_copy_one(current, item_t, temp_clip, xml_doc);
            sp_repr_unparent(current);

            // paste into topmost_parent (temporarily)
            std::vector<Inkscape::XML::Node*> copied = sp_selection_paste_impl(doc, doc->getObjectByRepr(topmost_parent), temp_clip);
            if (!temp_clip.empty())temp_clip.clear() ;
            if (!copied.empty()) { // if success,
                // take pasted object (now in topmost_parent)
                Inkscape::XML::Node *in_topmost = copied.back();
                // make a copy
                Inkscape::XML::Node *spnew = in_topmost->duplicate(xml_doc);
                // remove pasted
                sp_repr_unparent(in_topmost);
                // put its copy into group
                group->appendChild(spnew);
                Inkscape::GC::release(spnew);
                copied.clear();
            }
        }
    }

    // Add the new group to the topmost members' parent
    topmost_parent->addChildAtPos(group, topmost + 1);

    set(doc->getObjectByRepr(group));
    DocumentUndo::done(doc, SP_VERB_SELECTION_GROUP,
                       C_("Verb", "Group"));

    return group;
}


static bool clone_depth_descending(gconstpointer a, gconstpointer b) {
    SPUse *use_a = static_cast<SPUse *>(const_cast<gpointer>(a));
    SPUse *use_b = static_cast<SPUse *>(const_cast<gpointer>(b));
    int depth_a = use_a->cloneDepth();
    int depth_b = use_b->cloneDepth();
    return (depth_a==depth_b)?(a<b):(depth_a>depth_b);
}

void ObjectSet::popFromGroup(){
    if (isEmpty()) {
        selection_display_message(desktop(), Inkscape::WARNING_MESSAGE, _("<b>No objects selected</b> to pop out of group."));
        return;
    }

    auto item = items().begin(); // leaving this because it will be useful for
                                                                        // future implementation of complex pop ungrouping
    SPItem *obj = *item;
    SPItem *parent_group = static_cast<SPItem*>(obj->parent);
    if (!SP_IS_GROUP(parent_group) || SP_IS_LAYER(parent_group)) {
        selection_display_message(desktop(), Inkscape::WARNING_MESSAGE, _("Selection <b>not in a group</b>."));
        return;
    }
    if (parent_group->firstChild()->getNext() == nullptr) {
        std::vector<SPItem*> children;
        sp_item_group_ungroup(static_cast<SPGroup*>(parent_group), children, false);
    }
    else {
        toNextLayer(true);
        parent_group->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    }

    if(document())
        DocumentUndo::done(document(), SP_VERB_SELECTION_UNGROUP_POP_SELECTION,
                       _("Pop selection from group"));

}

static void ungroup_impl(ObjectSet *set)
{
    std::set<SPObject*> groups(set->groups().begin(),set->groups().end());

    std::vector<SPItem*> new_select;
    auto old_select = set->items();
    std::vector<SPItem*> items(old_select.begin(), old_select.end());

    // If any of the clones refer to the groups, unlink them and replace them with successors
    // in the items list.
    std::vector<SPUse*> clones_to_unlink;
    for (auto item : items) {
        SPUse *use = dynamic_cast<SPUse *>(item);

        SPItem *original = use;
        while (dynamic_cast<SPUse *>(original)) {
            original = dynamic_cast<SPUse *>(original)->get_original();
        }

        if (groups.find(original) !=  groups.end()) {
            clones_to_unlink.push_back(use);
        }
    }

    // Unlink clones beginning from those with highest clone depth.
    // This way we can be sure than no additional automatic unlinking happens,
    // and the items in the list remain valid
    std::sort(clones_to_unlink.begin(),clones_to_unlink.end(),clone_depth_descending);

    for (auto use:clones_to_unlink) {
        std::vector<SPItem*>::iterator items_node = std::find(items.begin(),items.end(), use);
        *items_node = use->unlink();
    }

    // do the actual work
    for (auto & item : items) {
        SPItem *obj = item;

        // ungroup only the groups marked earlier
        if (groups.find(item) != groups.end()) {
            std::vector<SPItem*> children;
            sp_item_group_ungroup(dynamic_cast<SPGroup *>(obj), children, false);
            // add the items resulting from ungrouping to the selection
            new_select.insert(new_select.end(),children.begin(),children.end());
            item = NULL; // zero out the original pointer, which is no longer valid
        } else {
            // if not a group, keep in the selection
            new_select.push_back(item);
        }
    }

    set->setList(new_select);
}

void ObjectSet::ungroup()
{
    if (isEmpty()) {
        if(desktop())
            selection_display_message(desktop(), Inkscape::WARNING_MESSAGE, _("Select a <b>group</b> to ungroup."));
        return;
    }

    if (boost::distance(groups()) == 0) {
        if(desktop())
            selection_display_message(desktop(), Inkscape::ERROR_MESSAGE, _("<b>No groups</b> to ungroup in the selection."));
        return;
    }

    ungroup_impl(this);
    if(document())
        DocumentUndo::done(document(), SP_VERB_SELECTION_UNGROUP,
                       _("Ungroup"));
}

// TODO replace it with ObjectSet::degroup_list

/** Replace all groups in the list with their member objects, recursively; returns a new list, frees old */
std::vector<SPItem*>
sp_degroup_list(std::vector<SPItem*> &items)
{
    std::vector<SPItem*> out;
    bool has_groups = false;
    for (auto item : items) {
        SPGroup *group = dynamic_cast<SPGroup *>(item);
        if (!group) {
            out.push_back(item);
        } else {
            has_groups = true;
            std::vector<SPItem*> members = sp_item_group_item_list(group);
            for (auto member : members) {
                out.push_back(member);
            }
            members.clear();
        }
    }

    if (has_groups) { // recurse if we unwrapped a group - it may have contained others
        out = sp_degroup_list(out);
    }

    return out;
}


/** If items in the list have a common parent, return it, otherwise return NULL */
static SPGroup *
sp_item_list_common_parent_group(const SPItemRange &items)
{
    if (items.empty()) {
        return nullptr;
    }
    SPObject *parent = items.front()->parent;
    // Strictly speaking this CAN happen, if user selects <svg> from Inkscape::XML editor
    if (!dynamic_cast<SPGroup *>(parent)) {
        return nullptr;
    }
    for (auto item=items.begin();item!=items.end();++item) {
        if((*item)==items.front())continue;
        if ((*item)->parent != parent) {
            return nullptr;
        }
    }

    return dynamic_cast<SPGroup *>(parent);
}

/** Finds out the minimum common bbox of the selected items. */
static Geom::OptRect
enclose_items(std::vector<SPItem*> const &items)
{
    g_assert(!items.empty());

    Geom::OptRect r;
    for (auto item : items) {
        r.unionWith(item->documentVisualBounds());
    }
    return r;
}

// TODO determine if this is intentionally different from SPObject::getPrev()
static SPObject *prev_sibling(SPObject *child)
{
    SPObject *prev = nullptr;
    if ( child && dynamic_cast<SPGroup *>(child->parent) ) {
        prev = child->getPrev();
    }
    return prev;
}

void ObjectSet::raise(bool skip_undo){

    if(isEmpty()){
        selection_display_message(desktop(), Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to raise."));
        return;
    }

    SPGroup const *group = sp_item_list_common_parent_group(items());
    if (!group) {
        if(desktop())
            selection_display_message(desktop(), Inkscape::ERROR_MESSAGE, _("You cannot raise/lower objects from <b>different groups</b> or <b>layers</b>."));
        return;
    }

    std::vector<SPItem*> items_copy(items().begin(), items().end());
    Inkscape::XML::Node *grepr = const_cast<Inkscape::XML::Node *>(items_copy.front()->parent->getRepr());

    /* Construct reverse-ordered list of selected children. */
    std::vector<SPItem*> rev(items_copy);
    sort(rev.begin(),rev.end(),sp_item_repr_compare_position_bool);

    // Determine the common bbox of the selected items.
    Geom::OptRect selected = enclose_items(items_copy);

    // Iterate over all objects in the selection (starting from top).
    if (selected) {
        for (auto child : rev) {
            // for each selected object, find the next sibling
            for (SPObject *newref = child->getNext(); newref; newref = newref->getNext()) {
                // if the sibling is an item AND overlaps our selection,
                SPItem *newItem = dynamic_cast<SPItem *>(newref);
                if (newItem) {
                    Geom::OptRect newref_bbox = newItem->documentVisualBounds();
                    if ( newref_bbox && selected->intersects(*newref_bbox) ) {
                        // AND if it's not one of our selected objects,
                        if ( std::find(items_copy.begin(),items_copy.end(),newref)==items_copy.end()) {
                            // move the selected object after that sibling
                            grepr->changeOrder(child->getRepr(), newref->getRepr());
                        }
                        break;
                    }
                }
            }
        }
    }
    if(document() && !skip_undo)
        DocumentUndo::done(document(), SP_VERB_SELECTION_RAISE,
                       //TRANSLATORS: "Raise" means "to raise an object" in the undo history
                       C_("Undo action", "Raise"));
}


void ObjectSet::raiseToTop(bool skip_undo) {
    if(isEmpty()){
        selection_display_message(desktop(), Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to raise."));
        return;
    }

    SPGroup const *group = sp_item_list_common_parent_group(items());
    if (!group) {
        selection_display_message(desktop(), Inkscape::ERROR_MESSAGE, _("You cannot raise/lower objects from <b>different groups</b> or <b>layers</b>."));
        return;
    }


    std::vector<Inkscape::XML::Node*> rl(xmlNodes().begin(), xmlNodes().end());
    sort(rl.begin(),rl.end(),sp_repr_compare_position_bool);

    for (auto repr : rl) {
        repr->setPosition(-1);
    }
    if (document() && !skip_undo) {
        DocumentUndo::done(document(), SP_VERB_SELECTION_TO_FRONT,
                         _("Raise to top"));
    }
}

void ObjectSet::lower(bool skip_undo){
    if(isEmpty()){
        selection_display_message(desktop(), Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to lower."));
        return;
    }

    SPGroup const *group = sp_item_list_common_parent_group(items());
    if (!group) {
        selection_display_message(desktop(), Inkscape::ERROR_MESSAGE, _("You cannot raise/lower objects from <b>different groups</b> or <b>layers</b>."));
        return;
    }

    std::vector<SPItem*> items_copy(items().begin(), items().end());
    Inkscape::XML::Node *grepr = const_cast<Inkscape::XML::Node *>(items_copy.front()->parent->getRepr());

    // Determine the common bbox of the selected items.
    Geom::OptRect selected = enclose_items(items_copy);

    /* Construct direct-ordered list of selected children. */
    std::vector<SPItem*> rev(items_copy);
    sort(rev.begin(),rev.end(),sp_item_repr_compare_position_bool);

    // Iterate over all objects in the selection (starting from top).
    if (selected) {
        for (std::vector<SPItem*>::const_reverse_iterator item=rev.rbegin();item!=rev.rend();++item) {
            SPObject *child = *item;
            // for each selected object, find the prev sibling
            for (SPObject *newref = prev_sibling(child); newref; newref = prev_sibling(newref)) {
                // if the sibling is an item AND overlaps our selection,
                SPItem *newItem = dynamic_cast<SPItem *>(newref);
                if (newItem) {
                    Geom::OptRect ref_bbox = newItem->documentVisualBounds();
                    if ( ref_bbox && selected->intersects(*ref_bbox) ) {
                        // AND if it's not one of our selected objects,
                        if (items_copy.end()==std::find(items_copy.begin(),items_copy.end(),newref)) {
                            // move the selected object before that sibling
                            SPObject *put_after = prev_sibling(newref);
                            if (put_after)
                                grepr->changeOrder(child->getRepr(), put_after->getRepr());
                            else
                                child->getRepr()->setPosition(0);
                        }
                        break;
                    }
                }
            }
        }
    }
    if(document() && !skip_undo)
        DocumentUndo::done(document(), SP_VERB_SELECTION_LOWER,
                       //TRANSLATORS: "Lower" means "to lower an object" in the undo history
                       C_("Undo action", "Lower"));
}


void ObjectSet::lowerToBottom(bool skip_undo){
    if(!document())
        return;
    if (isEmpty()) {
        selection_display_message(desktop(), Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to lower to bottom."));
        return;
    }

    SPGroup const *group = sp_item_list_common_parent_group(items());
    if (!group) {
        selection_display_message(desktop(), Inkscape::ERROR_MESSAGE, _("You cannot raise/lower objects from <b>different groups</b> or <b>layers</b>."));
        return;
    }

    std::vector<Inkscape::XML::Node*> rl(xmlNodes().begin(), xmlNodes().end());
    sort(rl.begin(),rl.end(),sp_repr_compare_position_bool);

    for (std::vector<Inkscape::XML::Node*>::const_reverse_iterator l=rl.rbegin();l!=rl.rend();++l) {
        gint minpos;
        SPObject *pp;
        Inkscape::XML::Node *repr = (*l);
        pp = document()->getObjectByRepr(repr->parent());
        minpos = 0;
        g_assert(dynamic_cast<SPGroup *>(pp));
        for (auto& pc: pp->children) {
            if(dynamic_cast<SPItem *>(&pc)) {
                break;
            }
            minpos += 1;
        }
        repr->setPosition(minpos);
    }
    if (document() && !skip_undo) {
        DocumentUndo::done(document(), SP_VERB_SELECTION_TO_BACK,
                           _("Lower to bottom"));
    }
}

void ObjectSet::stackUp(bool skip_undo) {
    if (isEmpty()) {
        selection_display_message(desktop(), Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to stack up."));
        return;
    }

    std::vector<SPItem*> selection(items().begin(), items().end());
    sort(selection.begin(), selection.end(), sp_item_repr_compare_position_bool);

    for (auto item: selection | boost::adaptors::reversed) {
        if (!item->raiseOne()) { // stop if top was reached
            if(document() && !skip_undo)
                DocumentUndo::cancel(document());
            selection_display_message(desktop(), Inkscape::WARNING_MESSAGE, _("We hit top."));
            return;
        }
    }

    if(document() && !skip_undo)
        DocumentUndo::done(document(), SP_VERB_SELECTION_STACK_UP,
           //TRANSLATORS: undo history: "stack up" means to raise an object of its ordinal position by 1
           C_("Undo action", "stack up"));
}

void ObjectSet::stackDown(bool skip_undo) {
    if (isEmpty()) {
        selection_display_message(desktop(), Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to stack down."));
        return;
    }

    std::vector<SPItem*> selection(items().begin(), items().end());
    sort(selection.begin(), selection.end(), sp_item_repr_compare_position_bool);

    for (auto item: selection) {
        if (!item->lowerOne()) { // stop if bottom was reached
            if(document() && !skip_undo)
                DocumentUndo::cancel(document());
            selection_display_message(desktop(), Inkscape::WARNING_MESSAGE, _("We hit bottom."));
            return;
        }
    }

    if(document() && !skip_undo)
        DocumentUndo::done(document(), SP_VERB_SELECTION_STACK_DOWN,
           //TRANSLATORS: undo history: "stack down" means to lower an object of its ordinal position by 1
           C_("Undo action", "stack down"));
}

void
sp_undo(SPDesktop *desktop, SPDocument *)
{
    // No re/undo while dragging, too dangerous.
    if (desktop->getCanvas()->is_dragging()) return;

    if (!DocumentUndo::undo(desktop->getDocument())) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Nothing to undo."));
    }
}

void
sp_redo(SPDesktop *desktop, SPDocument *)
{
    // No re/undo while dragging, too dangerous.
    if (desktop->getCanvas()->is_dragging()) return;

    if (!DocumentUndo::redo(desktop->getDocument())) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Nothing to redo."));
    }
}

void ObjectSet::cut()
{
    copy();
    deleteItems();
}

/**
 * \pre item != NULL
 */
SPCSSAttr *
take_style_from_item(SPObject *object)
{
    // CPPIFY:
    // This function should only take SPItems, but currently SPString is not an Item.

    // write the complete cascaded style, context-free
    SPCSSAttr *css = sp_css_attr_from_object(object, SP_STYLE_FLAG_ALWAYS);
    if (css == nullptr)
        return nullptr;

    if ((dynamic_cast<SPGroup *>(object) && object->firstChild()) ||
        (dynamic_cast<SPText *>(object) && object->firstChild() && object->firstChild()->getNext() == nullptr)) {
        // if this is a text with exactly one tspan child, merge the style of that tspan as well
        // If this is a group, merge the style of its topmost (last) child with style
        auto list = object->children | boost::adaptors::reversed;
        for (auto& element: list) {
            if (element.style ) {
                SPCSSAttr *temp = sp_css_attr_from_object(&element, SP_STYLE_FLAG_IFSET);
                if (temp) {
                    sp_repr_css_merge(css, temp);
                    sp_repr_css_attr_unref(temp);
                }
                break;
            }
        }
    }

    // Remove black-listed properties (those that should not be used in a default style)
    css = sp_css_attr_unset_blacklist(css);

    if (!(dynamic_cast<SPText *>(object) || dynamic_cast<SPTSpan *>(object) || dynamic_cast<SPTRef *>(object) || dynamic_cast<SPString *>(object))) {
        // do not copy text properties from non-text objects, it's confusing
        css = sp_css_attr_unset_text(css);
    }


    SPItem *item = dynamic_cast<SPItem *>(object);
    if (item) {
        // FIXME: also transform gradient/pattern fills, by forking? NO, this must be nondestructive
        double ex = item->i2doc_affine().descrim();
        if (ex != 1.0) {
            css = sp_css_attr_scale(css, ex);
        }
    }

    return css;
}

void ObjectSet::copy()
{
    Inkscape::UI::ClipboardManager *cm = Inkscape::UI::ClipboardManager::get();
    cm->copy(this);
}

void sp_selection_paste(SPDesktop *desktop, bool in_place)
{
    Inkscape::UI::ClipboardManager *cm = Inkscape::UI::ClipboardManager::get();
    if (cm->paste(desktop, in_place)) {
        DocumentUndo::done(desktop->getDocument(), SP_VERB_EDIT_PASTE, _("Paste"));
    }
}

void ObjectSet::pasteStyle()
{
    Inkscape::UI::ClipboardManager *cm = Inkscape::UI::ClipboardManager::get();
    if (cm->pasteStyle(this)) {
        DocumentUndo::done(document(), SP_VERB_EDIT_PASTE_STYLE, _("Paste style"));
    }
}

void ObjectSet::pastePathEffect()
{
    Inkscape::UI::ClipboardManager *cm = Inkscape::UI::ClipboardManager::get();
    if (cm->pastePathEffect(this)) {
        DocumentUndo::done(document(), SP_VERB_EDIT_PASTE_LIVEPATHEFFECT,
                           _("Paste live path effect"));
    }
}


static void sp_selection_remove_livepatheffect_impl(SPItem *item)
{
    if ( SPLPEItem *lpeitem = dynamic_cast<SPLPEItem*>(item) ) {
        if ( lpeitem->hasPathEffect() ) {
            lpeitem->removeAllPathEffects(false);
        }
    }
}

void ObjectSet::removeLPE()
{

    // check if something is selected
    if (isEmpty()) {
        if(desktop())
        desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to remove live path effects from."));
        return;
    }
    auto list= items();
    for (auto itemlist=list.begin();itemlist!=list.end();++itemlist) {
        SPItem *item = *itemlist;

        sp_selection_remove_livepatheffect_impl(item);

    }

    if(document())
        DocumentUndo::done(document(), SP_VERB_EDIT_REMOVE_LIVEPATHEFFECT,
                       _("Remove live path effect"));
}

void ObjectSet::removeFilter()
{

    // check if something is selected
    if (isEmpty()) {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to remove filters from."));
        return;
    }

    SPCSSAttr *css = sp_repr_css_attr_new();
    sp_repr_css_unset_property(css, "filter");
    sp_desktop_set_style(this, desktop(), css);
    sp_repr_css_attr_unref(css);
    if(document())
        DocumentUndo::done(document(), SP_VERB_EDIT_REMOVE_FILTER,
                       _("Remove filter"));
}


void ObjectSet::pasteSize(bool apply_x, bool apply_y)
{
    Inkscape::UI::ClipboardManager *cm = Inkscape::UI::ClipboardManager::get();
    if (cm->pasteSize(this, false, apply_x, apply_y)) {
        DocumentUndo::done(document(), SP_VERB_EDIT_PASTE_SIZE,
                           _("Paste size"));
    }
}

void ObjectSet::pasteSizeSeparately(bool apply_x, bool apply_y)
{
    Inkscape::UI::ClipboardManager *cm = Inkscape::UI::ClipboardManager::get();
    if (cm->pasteSize(this, true, apply_x, apply_y)) {
        DocumentUndo::done(document(), SP_VERB_EDIT_PASTE_SIZE_SEPARATELY,
                           _("Paste size separately"));
    }
}

/**
 * Ensures that the clones of objects are not modified when moving objects between layers.
 * Calls the same function as ungroup
 */
void sp_selection_change_layer_maintain_clones(std::vector<SPItem*> const &items,SPObject *where)
{
    for (auto item : items) {
        if (item) {
            SPItem *oldparent = dynamic_cast<SPItem *>(item->parent);
            SPItem *newparent = dynamic_cast<SPItem *>(where);
            sp_item_group_ungroup_handle_clones(item,
                    (oldparent->i2doc_affine())
                    *((newparent->i2doc_affine()).inverse()));
        }
    }
}

void ObjectSet::toNextLayer(bool skip_undo)
{
    if(!desktop())
        return;
    SPDesktop *dt=desktop(); //TODO make it desktop-independent

    // check if something is selected
    if (isEmpty()) {
        dt->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to move to the layer above."));
        return;
    }

    std::vector<SPItem*> items_copy(items().begin(), items().end());

    bool no_more = false; // Set to true, if no more layers above
    SPObject *next=Inkscape::next_layer(dt->currentRoot(), dt->currentLayer());
    if (next) {
        clear();
        sp_selection_change_layer_maintain_clones(items_copy,next);
        std::vector<Inkscape::XML::Node*> temp_clip;
        sp_selection_copy_impl(items_copy, temp_clip, dt->doc()->getReprDoc());
        sp_selection_delete_impl(items_copy, false, false);
        next=Inkscape::next_layer(dt->currentRoot(), dt->currentLayer()); // Fixes bug 1482973: crash while moving layers
        std::vector<Inkscape::XML::Node*> copied;
        if (next) {
            copied = sp_selection_paste_impl(dt->getDocument(), next, temp_clip);
        } else {
            copied = sp_selection_paste_impl(dt->getDocument(), dt->currentLayer(), temp_clip);
            no_more = true;
        }
        setReprList(copied);
        if (next) dt->setCurrentLayer(next);
        if ( !skip_undo ) {
            DocumentUndo::done(dt->getDocument(), SP_VERB_LAYER_MOVE_TO_NEXT,
                               _("Raise to next layer"));
        }
    } else {
        no_more = true;
    }

    if (no_more) {
        dt->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("No more layers above."));
    }

}

void ObjectSet::toPrevLayer(bool skip_undo)
{
    if(!desktop())
        return;
    SPDesktop *dt=desktop(); //TODO make it desktop-independent

    // check if something is selected
    if (isEmpty()) {
        dt->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to move to the layer below."));
        return;
    }

    std::vector<SPItem*> items_copy(items().begin(), items().end());

    bool no_more = false; // Set to true, if no more layers below
    SPObject *next=Inkscape::previous_layer(dt->currentRoot(), dt->currentLayer());
    if (next) {
        clear();
        sp_selection_change_layer_maintain_clones(items_copy,next);
        std::vector<Inkscape::XML::Node*> temp_clip;
        sp_selection_copy_impl(items_copy, temp_clip, dt->doc()->getReprDoc()); // we're in the same doc, so no need to copy defs
        sp_selection_delete_impl(items_copy, false, false);
        next=Inkscape::previous_layer(dt->currentRoot(), dt->currentLayer()); // Fixes bug 1482973: crash while moving layers
        std::vector<Inkscape::XML::Node*> copied;
        if (next) {
            copied = sp_selection_paste_impl(dt->getDocument(), next, temp_clip);
        } else {
            copied = sp_selection_paste_impl(dt->getDocument(), dt->currentLayer(), temp_clip);
            no_more = true;
        }
        setReprList( copied);
        if (next) dt->setCurrentLayer(next);
        if ( !skip_undo ) {
            DocumentUndo::done(dt->getDocument(), SP_VERB_LAYER_MOVE_TO_PREV,
                               _("Lower to previous layer"));
        }
    } else {
        no_more = true;
    }

    if (no_more) {
        dt->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("No more layers below."));
    }
}

void ObjectSet::toLayer(SPObject *moveto, bool skip_undo)
{
    if(!document())
        return;
    SPDesktop *dt = desktop();

    // check if something is selected
    if (isEmpty()) {
        if(dt)
            dt->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to move."));
        return;
    }

    std::vector<SPItem*> items_copy(items().begin(), items().end());

    if (moveto) {
        clear();
        sp_selection_change_layer_maintain_clones(items_copy,moveto);
        std::vector<Inkscape::XML::Node*> temp_clip;
        sp_selection_copy_impl(items_copy, temp_clip, document()->getReprDoc()); // we're in the same doc, so no need to copy defs
        sp_selection_delete_impl(items_copy, false, false);
        std::vector<Inkscape::XML::Node*> copied = sp_selection_paste_impl(document(), moveto, temp_clip);
        setReprList(copied);
        if (!temp_clip.empty()) temp_clip.clear();
        if (moveto && dt) dt->setCurrentLayer(moveto);
        if ( !skip_undo ) {
            DocumentUndo::done(document(), SP_VERB_LAYER_MOVE_TO,
                               _("Move selection to layer"));
        }
    }
}

static bool
object_set_contains_original(SPItem *item, ObjectSet *set)
{
    bool contains_original = false;

    SPItem *item_use = item;
    SPItem *item_use_first = item;
    SPUse *use = dynamic_cast<SPUse *>(item_use);
    while (use && item_use && !contains_original)
    {
        item_use = use->get_original();
        use = dynamic_cast<SPUse *>(item_use);
        contains_original |= set->includes(item_use);
        if (item_use == item_use_first)
            break;
    }

    // If it's a tref, check whether the object containing the character
    // data is part of the selection
    SPTRef *tref = dynamic_cast<SPTRef *>(item);
    if (!contains_original && tref) {
        contains_original = set->includes(tref->getObjectReferredTo());
    }

    return contains_original;
}


static bool
object_set_contains_both_clone_and_original(ObjectSet *set)
{
    bool clone_with_original = false;
    auto items = set->items();
    for (auto l=items.begin();l!=items.end() ;++l) {
        SPItem *item = *l;
        if (item) {
            clone_with_original |= object_set_contains_original(item, set);
            if (clone_with_original)
                break;
        }
    }
    return clone_with_original;
}

/** Apply matrix to the selection.  \a set_i2d is normally true, which means objects are in the
original transform, synced with their reprs, and need to jump to the new transform in one go. A
value of set_i2d==false is only used by seltrans when it's dragging objects live (not outlines); in
that case, items are already in the new position, but the repr is in the old, and this function
then simply updates the repr from item->transform.
 */

void ObjectSet::applyAffine(Geom::Affine const &affine, bool set_i2d, bool compensate,
                                bool adjust_transf_center)
{
    if (isEmpty())
        return;

    // For each perspective with a box in selection, check whether all boxes are selected and
    // unlink all non-selected boxes.
    Persp3D *persp;
    Persp3D *transf_persp;
    std::list<Persp3D *> plist = perspList();
    for (auto & i : plist) {
        persp = (Persp3D *) i;

        if (!persp->has_all_boxes_in_selection (this)) {
            std::list<SPBox3D *> selboxes = box3DList(persp);

            // create a new perspective as a copy of the current one and link the selected boxes to it
            transf_persp = Persp3D::create_xml_element (persp->document, persp->perspective_impl);

            for (auto & selboxe : selboxes)
                selboxe->switch_perspectives(persp, transf_persp);
        } else {
            transf_persp = persp;
        }

        transf_persp->apply_affine_transformation(affine);
    }
    auto items_copy = items();
    for (auto l=items_copy.begin();l!=items_copy.end() ;++l) {
        SPItem *item = *l;

        if( dynamic_cast<SPRoot *>(item) ) {
            // An SVG element cannot have a transform. We could change 'x' and 'y' in response
            // to a translation... but leave that for another day.
            if(desktop())
                desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Cannot transform an embedded SVG."));
            break;
        }

        Geom::Point old_center(0,0);
        if (set_i2d && item->isCenterSet())
            old_center = item->getCenter();

#if 0 /* Re-enable this once persistent guides have a graphical indication.
         At the time of writing, this is the only place to re-enable. */
        sp_item_update_cns(*item, desktop());
#endif

        // we're moving both a clone and its original or any ancestor in clone chain?
        bool transform_clone_with_original = object_set_contains_original(item, this);

        // ...both a text-on-path and its path?
        bool transform_textpath_with_path = ((dynamic_cast<SPText *>(item) && item->firstChild() && dynamic_cast<SPTextPath *>(item->firstChild()))
                                             && includes( sp_textpath_get_path_item(dynamic_cast<SPTextPath *>(item->firstChild())) ));

        // ...both a flowtext and its frame?
        bool transform_flowtext_with_frame = (dynamic_cast<SPFlowtext *>(item) && includes( dynamic_cast<SPFlowtext *>(item)->get_frame(nullptr))); // (only the first frame is checked so far)

        // ...both an offset and its source?
        bool transform_offset_with_source = (dynamic_cast<SPOffset *>(item) && dynamic_cast<SPOffset *>(item)->sourceHref) && includes( sp_offset_get_source(dynamic_cast<SPOffset *>(item)) );

        // If we're moving a connector, we want to detach it
        // from shapes that aren't part of the selection, but
        // leave it attached if they are
        if (Inkscape::UI::Tools::cc_item_is_connector(item)) {
            SPPath *path = dynamic_cast<SPPath *>(item);
            if (path) {
                SPItem *attItem[2] = {nullptr, nullptr};
                path->connEndPair.getAttachedItems(attItem);
                for (int n = 0; n < 2; ++n) {
                    if (!includes(attItem[n])) {
                        sp_conn_end_detach(item, n);
                    }
                }
            } else {
                g_assert_not_reached();
            }
        }

        // "clones are unmoved when original is moved" preference
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        int compensation = prefs->getInt("/options/clonecompensation/value", SP_CLONE_COMPENSATION_UNMOVED);
        bool prefs_unmoved = (compensation == SP_CLONE_COMPENSATION_UNMOVED);
        bool prefs_parallel = (compensation == SP_CLONE_COMPENSATION_PARALLEL);

        /* If this is a clone and it's selected along with its original, do not move it;
         * it will feel the transform of its original and respond to it itself.
         * Without this, a clone is doubly transformed, very unintuitive.
         *
         * Same for textpath if we are also doing ANY transform to its path: do not touch textpath,
         * letters cannot be squeezed or rotated anyway, they only refill the changed path.
         * Same for linked offset if we are also moving its source: do not move it. */
        if (transform_textpath_with_path) {
            // Restore item->transform field from the repr, in case it was changed by seltrans.
            item->readAttr(SPAttr::TRANSFORM);
        } else if (transform_flowtext_with_frame) {
            // apply the inverse of the region's transform to the <use> so that the flow remains
            // the same (even though the output itself gets transformed)
            for (auto& region: item->children) {
                if (dynamic_cast<SPFlowregion *>(&region) || dynamic_cast<SPFlowregionExclude *>(&region)) {
                    for (auto& itm: region.children) {
                        SPUse *use = dynamic_cast<SPUse *>(&itm);
                        if ( use ) {
                            use->doWriteTransform(item->transform.inverse(), nullptr, compensate);
                        }
                    }
                }
            }
        } else if (transform_clone_with_original || transform_offset_with_source) {
            // We are transforming a clone along with its original. The below matrix juggling is
            // necessary to ensure that they transform as a whole, i.e. the clone's induced
            // transform and its move compensation are both cancelled out.

            // restore item->transform field from the repr, in case it was changed by seltrans
            item->readAttr(SPAttr::TRANSFORM);

            // calculate the matrix we need to apply to the clone to cancel its induced transform from its original
            Geom::Affine parent2dt;
            {
                SPItem *parentItem = dynamic_cast<SPItem *>(item->parent);
                if (parentItem) {
                    parent2dt = parentItem->i2dt_affine();
                } else {
                    g_assert_not_reached();
                }
            }
            Geom::Affine t = parent2dt * affine * parent2dt.inverse();
            Geom::Affine t_inv = t.inverse();
            Geom::Affine result = t_inv * item->transform * t;

            if (transform_clone_with_original && (prefs_parallel || prefs_unmoved) && affine.isTranslation()) {
                // we need to cancel out the move compensation, too

                // find out the clone move, same as in sp_use_move_compensate
                Geom::Affine parent;
                {
                    SPUse *use = dynamic_cast<SPUse *>(item);
                    if (use) {
                        parent = use->get_parent_transform();
                    } else {
                        g_assert_not_reached();
                    }
                }
                Geom::Affine clone_move = parent.inverse() * t * parent;

                if (prefs_parallel) {
                    Geom::Affine move = result * clone_move * t_inv;
                    item->doWriteTransform(move, &move, compensate);

                } else if (prefs_unmoved) {
                    //if (dynamic_cast<SPUse *>(sp_use_get_original(dynamic_cast<SPUse *>(item))))
                    //    clone_move = Geom::identity();
                    Geom::Affine move = result * clone_move;
                    item->doWriteTransform(move, &t, compensate);
                }

            } else if (transform_offset_with_source && (prefs_parallel || prefs_unmoved) && affine.isTranslation()){
                Geom::Affine parent = item->transform;
                Geom::Affine offset_move = parent.inverse() * t * parent;

                if (prefs_parallel) {
                    Geom::Affine move = result * offset_move * t_inv;
                    item->doWriteTransform(move, &move, compensate);

                } else if (prefs_unmoved) {
                    Geom::Affine move = result * offset_move;
                    item->doWriteTransform(move, &t, compensate);
                }

            } else {
                // just apply the result
                item->doWriteTransform(result, &t, compensate);
            }

        } else {
            if (set_i2d) {
                item->set_i2d_affine(item->i2dt_affine() * (Geom::Affine)affine);
            }
            item->doWriteTransform(item->transform, nullptr, compensate);
        }

        if (adjust_transf_center) { // The transformation center should not be touched in case of pasting or importing, which is allowed by this if clause
            // if we're moving the actual object, not just updating the repr, we can transform the
            // center by the same matrix (only necessary for non-translations)
            if (set_i2d && item->isCenterSet() && !(affine.isTranslation() || affine.isIdentity())) {
                item->setCenter(old_center * affine);
                item->updateRepr();
            }
        }
    }
}

void ObjectSet::removeTransform()
{
    auto items = xmlNodes();
    for (auto l=items.begin();l!=items.end() ;++l) {
        (*l)->removeAttribute("transform");
    }

    if(document())
        DocumentUndo::done(document(), SP_VERB_OBJECT_FLATTEN,
                       _("Remove transform"));
}

void ObjectSet::setScaleAbsolute(double x0, double x1,double y0, double y1)
{
    if (isEmpty())
        return;

    Geom::OptRect bbox = visualBounds();
    if ( !bbox ) {
        return;
    }

    Geom::Translate const p2o(-bbox->min());

    Geom::Scale const newSize(x1 - x0,
                              y1 - y0);
    Geom::Scale const scale( newSize * Geom::Scale(bbox->dimensions()).inverse() );
    Geom::Translate const o2n(x0, y0);
    Geom::Affine const final( p2o * scale * o2n );

    applyAffine(final);
}

void ObjectSet::setScaleRelative(Geom::Point const &align, Geom::Scale const &scale)
{
    if (isEmpty())
        return;

    Geom::OptRect bbox = visualBounds();

    if ( !bbox ) {
        return;
    }

    // FIXME: ARBITRARY LIMIT: don't try to scale above 1 Mpx, it won't display properly and will crash sooner or later anyway
    if ( bbox->dimensions()[Geom::X] * scale[Geom::X] > 1e6  ||
         bbox->dimensions()[Geom::Y] * scale[Geom::Y] > 1e6 )
    {
        return;
    }

    Geom::Translate const n2d(-align);
    Geom::Translate const d2n(align);
    Geom::Affine const final( n2d * scale * d2n );
    applyAffine(final);
}

void ObjectSet::rotateRelative(Geom::Point const &center, double angle_degrees)
{
    Geom::Translate const d2n(center);
    Geom::Translate const n2d(-center);
    Geom::Rotate const rotate(Geom::Rotate::from_degrees(angle_degrees));
    Geom::Affine const final( Geom::Affine(n2d) * rotate * d2n );
    applyAffine(final);
}

void ObjectSet::skewRelative(Geom::Point const &align, double dx, double dy)
{
    Geom::Translate const d2n(align);
    Geom::Translate const n2d(-align);
    Geom::Affine const skew(1, dy,
                            dx, 1,
                            0, 0);
    Geom::Affine const final( n2d * skew * d2n );
    applyAffine(final);
}

void ObjectSet::moveRelative(Geom::Point const &move, bool compensate)
{
    applyAffine(Geom::Affine(Geom::Translate(move)), true, compensate);
}

void ObjectSet::moveRelative(double dx, double dy)
{
    applyAffine(Geom::Affine(Geom::Translate(dx, dy)));
}

/**
 * Rotates selected objects 90 degrees, either clock-wise or counter-clockwise, depending on the value of ccw.
 */
void ObjectSet::rotate90(bool ccw)
{
    if (isEmpty())
        return;

    auto items_copy = items();
    double y_dir = document() ? document()->yaxisdir() : 1;
    Geom::Rotate const rot_90(Geom::Point(0, ccw ? -y_dir : y_dir)); // pos. or neg. rotation, depending on the value of ccw
    for (auto l=items_copy.begin();l!=items_copy.end() ;++l) {
        SPItem *item = *l;
        if (item) {
            item->rotate_rel(rot_90);
        } else {
            g_assert_not_reached();
        }
    }

    if (document())
        DocumentUndo::done(document(),
                           ccw ? SP_VERB_OBJECT_ROTATE_90_CCW : SP_VERB_OBJECT_ROTATE_90_CW,
                           ccw ? _("Rotate 90\xc2\xb0 CCW") : _("Rotate 90\xc2\xb0 CW"));
}

void ObjectSet::rotate(gdouble const angle_degrees)
{
    if (isEmpty())
        return;

    boost::optional<Geom::Point> center_ = center();
    if (!center_) {
        return;
    }
    rotateRelative(*center_, angle_degrees);

    if (document())
        DocumentUndo::maybeDone(document(),
                                ( ( angle_degrees > 0 )
                                  ? "selector:rotate:ccw"
                                  : "selector:rotate:cw" ),
                                SP_VERB_CONTEXT_SELECT,
                                _("Rotate"));
}

/*
 * Selects all the visible items with the same fill and/or stroke color/style as the items in the current selection
 *
 * Params:
 * desktop - set the selection on this desktop
 * fill - select objects matching fill
 * stroke - select objects matching stroke
 */
void sp_select_same_fill_stroke_style(SPDesktop *desktop, gboolean fill, gboolean stroke, gboolean style)
{
    if (!desktop) {
        return;
    }

    if (!fill && !stroke && !style) {
        return;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool onlyvisible = prefs->getBool("/options/kbselection/onlyvisible", true);
    bool onlysensitive = prefs->getBool("/options/kbselection/onlysensitive", true);
    bool ingroups = TRUE;
    std::vector<SPItem*> x,y;
    std::vector<SPItem*> all_list = get_all_items(x, desktop->currentRoot(), desktop, onlyvisible, onlysensitive, ingroups, y);
    std::vector<SPItem*> all_matches;

    Inkscape::Selection *selection = desktop->getSelection();
    auto items = selection->items();

    std::vector<SPItem*> tmp;
    for (auto iter : all_list) {
        if(!SP_IS_GROUP(iter)){
            tmp.push_back(iter);
        }
    }
    all_list=tmp;

    for (auto sel_iter=items.begin();sel_iter!=items.end();++sel_iter) {
        SPItem *sel = *sel_iter;
        std::vector<SPItem*> matches = all_list;
        if (fill && stroke && style) {
            matches = sp_get_same_style(sel, matches);
        }
        else if (fill) {
            matches = sp_get_same_style(sel, matches, SP_FILL_COLOR);
        }
        else if (stroke) {
            matches = sp_get_same_style(sel, matches, SP_STROKE_COLOR);
        }
        else if (style) {
            matches = sp_get_same_style(sel, matches,SP_STROKE_STYLE_ALL);
        }
        all_matches.insert(all_matches.end(), matches.begin(),matches.end());
    }

    selection->clear();
    selection->setList(all_matches);

}


/*
 * Selects all the visible items with the same object type as the items in the current selection
 *
 * Params:
 * desktop - set the selection on this desktop
 */
void sp_select_same_object_type(SPDesktop *desktop)
{
    if (!desktop) {
        return;
    }


    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool onlyvisible = prefs->getBool("/options/kbselection/onlyvisible", true);
    bool onlysensitive = prefs->getBool("/options/kbselection/onlysensitive", true);
    bool ingroups = TRUE;
    std::vector<SPItem*> x,y;
    std::vector<SPItem*> all_list = get_all_items(x, desktop->currentRoot(), desktop, onlyvisible, onlysensitive, ingroups, y);
    std::vector<SPItem*> matches = all_list;

    Inkscape::Selection *selection = desktop->getSelection();

    auto items= selection->items();
    for (auto sel_iter=items.begin();sel_iter!=items.end();++sel_iter) {
        SPItem *sel = *sel_iter;
        if (sel) {
            matches = sp_get_same_object_type(sel, matches);
        } else {
            g_assert_not_reached();
        }
    }

    selection->clear();
    selection->setList(matches);

}



/*
 * Find all items in src list that have the same fill or stroke style as sel
 * Return the list of matching items
 */
std::vector<SPItem*> sp_get_same_fill_or_stroke_color(SPItem *sel, std::vector<SPItem*> &src, SPSelectStrokeStyleType type)
{
    std::vector<SPItem*> matches ;
    gboolean match = false;

    SPIPaint *sel_paint = sel->style->getFillOrStroke(type == SP_FILL_COLOR);

    for (std::vector<SPItem*>::const_reverse_iterator i=src.rbegin();i!=src.rend();++i) {
        SPItem *iter = *i;
        if (iter) {
            SPIPaint *iter_paint = iter->style->getFillOrStroke(type == SP_FILL_COLOR);
            match = false;
            if (sel_paint->isColor() && iter_paint->isColor() // color == color comparison doesn't seem to work here.
                && (sel_paint->value.color.toRGBA32(1.0) == iter_paint->value.color.toRGBA32(1.0))) {
                match = true;
            } else if (sel_paint->isPaintserver() && iter_paint->isPaintserver()) {

                SPPaintServer *sel_server =
                    (type == SP_FILL_COLOR) ? sel->style->getFillPaintServer() : sel->style->getStrokePaintServer();
                SPPaintServer *iter_server =
                    (type == SP_FILL_COLOR) ? iter->style->getFillPaintServer() : iter->style->getStrokePaintServer();

                if ((dynamic_cast<SPLinearGradient *>(sel_server) || dynamic_cast<SPRadialGradient *>(sel_server) ||
                     (dynamic_cast<SPGradient *>(sel_server) && dynamic_cast<SPGradient *>(sel_server)->getVector()->isSwatch()))
                    &&
                    (dynamic_cast<SPLinearGradient *>(iter_server) || dynamic_cast<SPRadialGradient *>(iter_server) ||
                     (dynamic_cast<SPGradient *>(iter_server) && dynamic_cast<SPGradient *>(iter_server)->getVector()->isSwatch()))) {
                    SPGradient *sel_vector = dynamic_cast<SPGradient *>(sel_server)->getVector();
                    SPGradient *iter_vector = dynamic_cast<SPGradient *>(iter_server)->getVector();
                    if (sel_vector == iter_vector) {
                        match = true;
                    }

                } else if (dynamic_cast<SPPattern *>(sel_server) && dynamic_cast<SPPattern *>(iter_server)) {
                    SPPattern *sel_pat = dynamic_cast<SPPattern *>(sel_server)->rootPattern();
                    SPPattern *iter_pat = dynamic_cast<SPPattern *>(iter_server)->rootPattern();
                    if (sel_pat == iter_pat) {
                        match = true;
                    }
                }
            } else if (sel_paint->isNone() && iter_paint->isNone()) {
                match = true;
            } else if (sel_paint->isNoneSet() && iter_paint->isNoneSet()) {
                match = true;
            }

            if (match) {
                matches.push_back(iter);
            }
        } else {
            g_assert_not_reached();
        }
    }

    return matches;
}

static bool item_type_match (SPItem *i, SPItem *j)
{
    if ( dynamic_cast<SPRect *>(i)) {
        return ( dynamic_cast<SPRect *>(j) );

    } else if (dynamic_cast<SPGenericEllipse *>(i)) {
        return (dynamic_cast<SPGenericEllipse *>(j));

    } else if (dynamic_cast<SPStar *>(i) || dynamic_cast<SPPolygon *>(i)) {
        return (dynamic_cast<SPStar *>(j) || dynamic_cast<SPPolygon *>(j)) ;

    } else if (dynamic_cast<SPSpiral *>(i)) {
        return (dynamic_cast<SPSpiral *>(j));

    } else if (dynamic_cast<SPPath *>(i) || dynamic_cast<SPLine *>(i) || dynamic_cast<SPPolyLine *>(i)) {
        return (dynamic_cast<SPPath *>(j) || dynamic_cast<SPLine *>(j) || dynamic_cast<SPPolyLine *>(j));

    } else if (dynamic_cast<SPText *>(i) || dynamic_cast<SPFlowtext *>(i) || dynamic_cast<SPTSpan *>(i) || dynamic_cast<SPTRef *>(i) || dynamic_cast<SPString *>(i)) {
        return (dynamic_cast<SPText *>(j) || dynamic_cast<SPFlowtext *>(j) || dynamic_cast<SPTSpan *>(j) || dynamic_cast<SPTRef *>(j) || dynamic_cast<SPString *>(j));

    }  else if (dynamic_cast<SPUse *>(i)) {
        return (dynamic_cast<SPUse *>(j)) ;

    } else if (dynamic_cast<SPImage *>(i)) {
        return (dynamic_cast<SPImage *>(j));

    } else if (dynamic_cast<SPOffset *>(i) && dynamic_cast<SPOffset *>(i)->sourceHref) {   // Linked offset
        return (dynamic_cast<SPOffset *>(j) && dynamic_cast<SPOffset *>(j)->sourceHref);

    }  else if (dynamic_cast<SPOffset *>(i) && !dynamic_cast<SPOffset *>(i)->sourceHref) { // Dynamic offset
        return (dynamic_cast<SPOffset *>(j) && !dynamic_cast<SPOffset *>(j)->sourceHref);

    }

    return false;
}

/*
 * Find all items in src list that have the same object type as sel by type
 * Return the list of matching items
 */
std::vector<SPItem*> sp_get_same_object_type(SPItem *sel, std::vector<SPItem*> &src)
{
    std::vector<SPItem*> matches;

    for (std::vector<SPItem*>::const_reverse_iterator i=src.rbegin();i!=src.rend();++i) {
        SPItem *item = *i;
        if (item && item_type_match(sel, item) && !item->cloned) {
            matches.push_back(item);
        }
    }
    return matches;
}

/*
 * Find all items in src list that have the same stroke style as sel by type
 * Return the list of matching items
 */
std::vector<SPItem*> sp_get_same_style(SPItem *sel, std::vector<SPItem*> &src, SPSelectStrokeStyleType type)
{
    std::vector<SPItem*> matches;
    bool match = false;

    SPStyle *sel_style = sel->style;

    if (type == SP_FILL_COLOR || type == SP_STYLE_ALL) {
        src = sp_get_same_fill_or_stroke_color(sel, src, SP_FILL_COLOR);
    }
    if (type == SP_STROKE_COLOR || type == SP_STYLE_ALL) {
        src = sp_get_same_fill_or_stroke_color(sel, src, SP_STROKE_COLOR);
    }

    /*
     * Stroke width needs to handle transformations, so call this function
     * to get the transformed stroke width
     */
    std::vector<SPItem*> objects;
    SPStyle *sel_style_for_width = nullptr;
    if (type == SP_STROKE_STYLE_WIDTH || type == SP_STROKE_STYLE_ALL || type==SP_STYLE_ALL ) {
        objects.push_back(sel);
        sel_style_for_width = new SPStyle(SP_ACTIVE_DOCUMENT);
        objects_query_strokewidth (objects, sel_style_for_width);
    }
    bool match_g;
    for (auto iter : src) {
        if (iter) {
            match_g=true;
            SPStyle *iter_style = iter->style;
            match = true;

            if (type == SP_STROKE_STYLE_WIDTH|| type == SP_STROKE_STYLE_ALL|| type==SP_STYLE_ALL) {
                match = (sel_style->stroke_width.set == iter_style->stroke_width.set);
                if (sel_style->stroke_width.set && iter_style->stroke_width.set) {
                    std::vector<SPItem*> objects;
                    objects.insert(objects.begin(),iter);
                    SPStyle tmp_style(SP_ACTIVE_DOCUMENT);
                    objects_query_strokewidth (objects, &tmp_style);

                    if (sel_style_for_width) {
                        match = (sel_style_for_width->stroke_width.computed == tmp_style.stroke_width.computed);
                    }
                }
            }
            match_g = match_g && match;
            if (type == SP_STROKE_STYLE_DASHES|| type == SP_STROKE_STYLE_ALL || type==SP_STYLE_ALL) {
                match = (sel_style->stroke_dasharray.set == iter_style->stroke_dasharray.set);
                if (sel_style->stroke_dasharray.set && iter_style->stroke_dasharray.set) {
                    match = (sel_style->stroke_dasharray == iter_style->stroke_dasharray);
                }
            }
            match_g = match_g && match;
            if (type == SP_STROKE_STYLE_MARKERS|| type == SP_STROKE_STYLE_ALL|| type==SP_STYLE_ALL) {
                match = true;
                int len = sizeof(sel_style->marker)/sizeof(SPIString);
                for (int i = 0; i < len; i++) {
                    if (g_strcmp0(sel_style->marker_ptrs[i]->value(),
                                  iter_style->marker_ptrs[i]->value())) {
                        match = false;
                        break;
                    }
                }
            }
            match_g = match_g && match;
            if (match_g) {
                while (iter->cloned) iter=dynamic_cast<SPItem *>(iter->parent);
                matches.insert(matches.begin(),iter);
            }
        } else {
            g_assert_not_reached();
        }
    }

    if( sel_style_for_width != nullptr ) delete sel_style_for_width;
    return matches;
}

// helper function:
static
Geom::Point
cornerFarthestFrom(Geom::Rect const &r, Geom::Point const &p){
    Geom::Point m = r.midpoint();
    unsigned i = 0;
    if (p[X] < m[X]) {
        i = 1;
    }
    if (p[Y] < m[Y]) {
        i = 3 - i;
    }
    return r.corner(i);
}

/**
\param  angle   the angle in "angular pixels", i.e. how many visible pixels must move the outermost point of the rotated object
*/
void ObjectSet::rotateScreen(double angle)
{
    if (isEmpty()||!desktop())
        return;

    Geom::OptRect bbox = visualBounds();
    boost::optional<Geom::Point> center_ = center();

    if ( !bbox || !center_ ) {
        return;
    }

    gdouble const zoom = desktop()->current_zoom();
    gdouble const zmove = angle / zoom;
    gdouble const r = Geom::L2(cornerFarthestFrom(*bbox, *center_) - *center_);

    gdouble const zangle = 180 * atan2(zmove, r) / M_PI;

    rotateRelative(*center_, zangle);

    DocumentUndo::maybeDone(document(),
                            ( (angle > 0)
                              ? "selector:rotate:ccw"
                              : "selector:rotate:cw" ),
                            SP_VERB_CONTEXT_SELECT,
                            _("Rotate by pixels"));
}

void ObjectSet::scale(double grow)
{
    if (isEmpty())
        return;

    Geom::OptRect bbox = visualBounds();
    if (!bbox) {
        return;
    }

    Geom::Point const center_(bbox->midpoint());

    // you can't scale "do nizhe pola" (below zero)
    double const max_len = bbox->maxExtent();
    if ( max_len + grow <= 1e-3 ) {
        return;
    }

    double const times = 1.0 + grow / max_len;
    setScaleRelative(center_, Geom::Scale(times, times));

    if (document()) {
            DocumentUndo::maybeDone(document(),
                                    ( (grow > 0)
                                      ? "selector:scale:larger"
                                      : "selector:scale:smaller" ),
                                    SP_VERB_CONTEXT_SELECT,
                                    _("Scale"));
    }
}

void ObjectSet::scaleScreen(double grow_pixels)
{
    if(!desktop())
        return;
    scale(grow_pixels / desktop()->current_zoom());
}

void ObjectSet::scaleTimes(double times)
{
    if (isEmpty())
        return;

    Geom::OptRect sel_bbox = visualBounds();

    if (!sel_bbox) {
        return;
    }

    Geom::Point const center_(sel_bbox->midpoint());
    setScaleRelative(center_, Geom::Scale(times, times));
    DocumentUndo::done(document(), SP_VERB_CONTEXT_SELECT,
                       _("Scale by whole factor"));
}

void ObjectSet::move(double dx, double dy)
{
    if (isEmpty()) {
        return;
    }

    moveRelative(dx, dy);

    if (document()) {
        if (dx == 0) {
            DocumentUndo::maybeDone(document(), "selector:move:vertical", SP_VERB_CONTEXT_SELECT,
                                    _("Move vertically"));
        } else if (dy == 0) {
            DocumentUndo::maybeDone(document(), "selector:move:horizontal", SP_VERB_CONTEXT_SELECT,
                                    _("Move horizontally"));
        } else {
            DocumentUndo::done(document(), SP_VERB_CONTEXT_SELECT,
                               _("Move"));
        }
    }
}

void ObjectSet::moveScreen(double dx, double dy)
{
    if (isEmpty() || !desktop()) {
        return;
    }

    // same as sp_selection_move but divide deltas by zoom factor
    gdouble const zoom = desktop()->current_zoom();
    gdouble const zdx = dx / zoom;
    gdouble const zdy = dy / zoom;
    moveRelative(zdx, zdy);

    SPDocument *doc = document();
    if (dx == 0) {
        DocumentUndo::maybeDone(doc, "selector:move:vertical", SP_VERB_CONTEXT_SELECT,
                                _("Move vertically by pixels"));
    } else if (dy == 0) {
        DocumentUndo::maybeDone(doc, "selector:move:horizontal", SP_VERB_CONTEXT_SELECT,
                                _("Move horizontally by pixels"));
    } else {
        DocumentUndo::done(doc, SP_VERB_CONTEXT_SELECT,
                           _("Move"));
    }
}



struct Forward {
    typedef SPObject *Iterator;

    static Iterator children(SPObject *o) { return o->firstChild(); }
    static Iterator siblings_after(SPObject *o) { return o->getNext(); }
    static void dispose(Iterator i) {}

    static SPObject *object(Iterator i) { return i; }
    static Iterator next(Iterator i) { return i->getNext(); }
    static bool isNull(Iterator i) {return (!i);}
};

struct ListReverse {
    typedef std::list<SPObject *> *Iterator;

    static Iterator children(SPObject *o) {
        return make_list(o, nullptr);
    }
    static Iterator siblings_after(SPObject *o) {
        return make_list(o->parent, o);
    }
    static void dispose(Iterator i) {
        delete i;
    }

    static SPObject *object(Iterator i) {
        return *(i->begin());
    }
    static Iterator next(Iterator i) { i->pop_front(); return i; }

    static bool isNull(Iterator i) {return i->empty();}

private:
    static std::list<SPObject *> *make_list(SPObject *object, SPObject *limit) {
        auto list = new std::list<SPObject *>;
        for (auto &child: object->children) {
            if (&child == limit) {
                break;
            }
            list->push_front(&child);
        }
        return list;
    }
};



template <typename D>
SPItem *next_item(SPDesktop *desktop, std::vector<SPObject *> &path, SPObject *root,
                  bool only_in_viewport, PrefsSelectionContext inlayer, bool onlyvisible, bool onlysensitive)
{
    typename D::Iterator children;
    typename D::Iterator iter;

    SPItem *found=nullptr;

    if (!path.empty()) {
        SPObject *object=path.back();
        path.pop_back();
        g_assert(object->parent == root);
        if (desktop->isLayer(object)) {
            found = next_item<D>(desktop, path, object, only_in_viewport, inlayer, onlyvisible, onlysensitive);
        }
        iter = children = D::siblings_after(object);
    } else {
        iter = children = D::children(root);
    }

    while ( !D::isNull(iter) && !found ) {
        SPObject *object=D::object(iter);
        if (desktop->isLayer(object)) {
            if (PREFS_SELECTION_LAYER != inlayer) { // recurse into sublayers
                std::vector<SPObject *> empt;
                found = next_item<D>(desktop, empt, object, only_in_viewport, inlayer, onlyvisible, onlysensitive);
            }
        } else {
            SPItem *item = dynamic_cast<SPItem *>(object);
            if ( item &&
                 ( !only_in_viewport || desktop->isWithinViewport(item) ) &&
                 ( !onlyvisible || !desktop->itemIsHidden(item)) &&
                 ( !onlysensitive || !item->isLocked()) &&
                 !desktop->isLayer(item) )
            {
                found = item;
            }
        }
        iter = D::next(iter);
    }

    D::dispose(children);

    return found;
}


template <typename D>
SPItem *next_item_from_list(SPDesktop *desktop, std::vector<SPItem*> const &items,
                            SPObject *root, bool only_in_viewport, PrefsSelectionContext inlayer, bool onlyvisible, bool onlysensitive)
{
    SPObject *current=root;
    for(auto item : items) {
        if ( root->isAncestorOf(item) &&
             ( !only_in_viewport || desktop->isWithinViewport(item) ) )
        {
            current = item;
            break;
        }
    }

    std::vector<SPObject *> path;
    while ( current != root ) {
        path.push_back(current);
        current = current->parent;
    }

    SPItem *next;
    // first, try from the current object
    next = next_item<D>(desktop, path, root, only_in_viewport, inlayer, onlyvisible, onlysensitive);

    if (!next) { // if we ran out of objects, start over at the root
        std::vector<SPObject *> empt;
        next = next_item<D>(desktop, empt, root, only_in_viewport, inlayer, onlyvisible, onlysensitive);
    }

    return next;
}

void
sp_selection_item_next(SPDesktop *desktop)
{
    g_return_if_fail(desktop != nullptr);
    Inkscape::Selection *selection = desktop->getSelection();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    PrefsSelectionContext inlayer = (PrefsSelectionContext)prefs->getInt("/options/kbselection/inlayer", PREFS_SELECTION_LAYER);
    bool onlyvisible = prefs->getBool("/options/kbselection/onlyvisible", true);
    bool onlysensitive = prefs->getBool("/options/kbselection/onlysensitive", true);

    SPObject *root;
    if (PREFS_SELECTION_ALL != inlayer) {
        root = selection->activeContext();
    } else {
        root = desktop->currentRoot();
    }

    std::vector<SPItem *> vec(selection->items().begin(), selection->items().end());
    SPItem *item=next_item_from_list<Forward>(desktop, vec, root, SP_CYCLING == SP_CYCLE_VISIBLE, inlayer, onlyvisible, onlysensitive);

    if (item) {
        selection->set(item, PREFS_SELECTION_LAYER_RECURSIVE == inlayer);
        if ( SP_CYCLING == SP_CYCLE_FOCUS ) {
            scroll_to_show_item(desktop, item);
        }
    }
}

void
sp_selection_item_prev(SPDesktop *desktop)
{
    SPDocument *document = desktop->getDocument();
    g_return_if_fail(document != nullptr);
    g_return_if_fail(desktop != nullptr);
    Inkscape::Selection *selection = desktop->getSelection();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    PrefsSelectionContext inlayer = (PrefsSelectionContext) prefs->getInt("/options/kbselection/inlayer", PREFS_SELECTION_LAYER);
    bool onlyvisible = prefs->getBool("/options/kbselection/onlyvisible", true);
    bool onlysensitive = prefs->getBool("/options/kbselection/onlysensitive", true);

    SPObject *root;
    if (PREFS_SELECTION_ALL != inlayer) {
        root = selection->activeContext();
    } else {
        root = desktop->currentRoot();
    }

    std::vector<SPItem *> vec(selection->items().begin(), selection->items().end());
    SPItem *item=next_item_from_list<ListReverse>(desktop, vec, root, SP_CYCLING == SP_CYCLE_VISIBLE, inlayer, onlyvisible, onlysensitive);

    if (item) {
        selection->set(item, PREFS_SELECTION_LAYER_RECURSIVE == inlayer);
        if ( SP_CYCLING == SP_CYCLE_FOCUS ) {
            scroll_to_show_item(desktop, item);
        }
    }
}

void sp_selection_next_patheffect_param(SPDesktop * dt)
{
    if (!dt) return;

    Inkscape::Selection *selection = dt->getSelection();
    if ( selection && !selection->isEmpty() ) {
        SPItem *item = selection->singleItem();
        if ( SPLPEItem *lpeitem = dynamic_cast<SPLPEItem*>(item) ) {
            if (lpeitem->hasPathEffect()) {
                lpeitem->editNextParamOncanvas(dt);
            } else {
                dt->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("The selection has no applied path effect."));
            }
        }
    }
}

/*bool has_path_recursive(SPObject *obj)
{
    if (!obj) return false;
    if (SP_IS_PATH(obj)) {
        return true;
    }
    if (SP_IS_GROUP(obj) || SP_IS_OBJECTGROUP(obj)) {
        for (SPObject *c = obj->children; c; c = c->next) {
            if (has_path_recursive(c)) return true;
        }
    }
    return false;
}*/

void ObjectSet::editMask(bool /*clip*/)
{
    return;
}




/**
 * If \a item is not entirely visible then adjust visible area to centre on the centre on of
 * \a item.
 */
void scroll_to_show_item(SPDesktop *desktop, SPItem *item)
{
    auto dbox = desktop->get_display_area();
    Geom::OptRect sbox = item->desktopVisualBounds();

    if ( sbox && dbox.contains(*sbox) == false ) {
        Geom::Point const s_dt = sbox->midpoint();
        Geom::Point const s_w = desktop->d2w(s_dt);
        Geom::Point const d_dt = dbox.midpoint();
        Geom::Point const d_w = desktop->d2w(d_dt);
        Geom::Point const moved_w( d_w - s_w );
        desktop->scroll_relative(moved_w);
    }
}

void ObjectSet::clone()
{
    if (document() == nullptr) {
        return;
    }

    Inkscape::XML::Document *xml_doc = document()->getReprDoc();

    // check if something is selected
    if (isEmpty()) {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select an <b>object</b> to clone."));
        return;
    }

    std::vector<Inkscape::XML::Node*> reprs(xmlNodes().begin(), xmlNodes().end());

    clear();

    // sorting items from different parents sorts each parent's subset without possibly mixing them, just what we need
    sort(reprs.begin(),reprs.end(),sp_repr_compare_position_bool);

    std::vector<Inkscape::XML::Node*> newsel;

    for(auto sel_repr : reprs){
        Inkscape::XML::Node *parent = sel_repr->parent();

        Inkscape::XML::Node *clone = xml_doc->createElement("svg:use");
        clone->setAttribute("x", "0");
        clone->setAttribute("y", "0");
        gchar *href_str = g_strdup_printf("#%s", sel_repr->attribute("id"));
        clone->setAttribute("xlink:href", href_str);
        g_free(href_str);

        clone->setAttribute("inkscape:transform-center-x", sel_repr->attribute("inkscape:transform-center-x"));
        clone->setAttribute("inkscape:transform-center-y", sel_repr->attribute("inkscape:transform-center-y"));

        // add the new clone to the top of the original's parent
        parent->appendChild(clone);

        newsel.push_back(clone);
        Inkscape::GC::release(clone);
    }

    DocumentUndo::done(document(), SP_VERB_EDIT_CLONE,
                       C_("Action", "Clone"));

    setReprList(newsel);
}

void ObjectSet::relink()
{
    if (isEmpty()) {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>clones</b> to relink."));
        return;
    }

    Inkscape::UI::ClipboardManager *cm = Inkscape::UI::ClipboardManager::get();
    auto newid = cm->getFirstObjectID();
    if (newid.empty()) {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Copy an <b>object</b> to clipboard to relink clones to."));
        return;
    }
    auto newref = "#" + newid;

    // Get a copy of current selection.
    bool relinked = false;
    auto items_= items();
    for (auto i=items_.begin();i!=items_.end();++i){
        SPItem *item = *i;

        if (dynamic_cast<SPUse *>(item)) {
            item->setAttribute("xlink:href", newref);
            item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            relinked = true;
        }
    }

    if (!relinked) {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("<b>No clones to relink</b> in the selection."));
    } else {
        DocumentUndo::done(document(), SP_VERB_EDIT_UNLINK_CLONE,
                           _("Relink clone"));
    }
}


bool ObjectSet::unlink(const bool skip_undo)
{
    if (isEmpty()) {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>clones</b> to unlink."));
        return false;
    }

    // Get a copy of current selection.
    std::vector<SPItem*> new_select;
    bool unlinked = false;
    std::vector<SPItem *> items_(items().begin(), items().end());

    for (auto i=items_.rbegin();i!=items_.rend();++i){
        SPItem *item = *i;

        ObjectSet tmp_set(document());
        tmp_set.set(item);
        auto *clip_obj = item->getClipObject();
        auto *mask_obj = item->getMaskObject();
        if (clip_obj) {
            SPUse *clipuse = dynamic_cast<SPUse *>(clip_obj);
            if (clipuse) {
                tmp_set.unsetMask(true,true);
                unlinked = tmp_set.unlink(true) || unlinked;
                tmp_set.setMask(true,false,true);
            }
            new_select.push_back(tmp_set.singleItem());
        } else if (mask_obj) {
            SPUse *maskuse = dynamic_cast<SPUse *>(mask_obj);
            if (maskuse) {
                tmp_set.unsetMask(false,true);
                unlinked = tmp_set.unlink(true) || unlinked;
                tmp_set.setMask(false,false,true);
            }
            new_select.push_back(tmp_set.singleItem());
        } else {
            if (dynamic_cast<SPText *>(item)) {
                SPObject *tspan = sp_tref_convert_to_tspan(item);

                if (tspan) {
                    item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
                }

                // Set unlink to true, and fall into the next if which
                // will include this text item in the new selection
                unlinked = true;
            }

            if (!(dynamic_cast<SPUse *>(item) || dynamic_cast<SPTRef *>(item))) {
                // keep the non-use item in the new selection
                new_select.push_back(item);
                continue;
            }

            SPItem *unlink = nullptr;
            SPUse *use = dynamic_cast<SPUse *>(item);
            if (use) {
                unlink = use->unlink();
                // Unable to unlink use (external or invalid href?)
                if (!unlink) {
                    new_select.push_back(item);
                    continue;
                }
            } else /*if (SP_IS_TREF(use))*/ {
                unlink = dynamic_cast<SPItem *>(sp_tref_convert_to_tspan(item));
                g_assert(unlink != nullptr);
            }

            unlinked = true;
            // Add ungrouped items to the new selection.
            new_select.push_back(unlink);
        }
    }

    if (!new_select.empty()) { // set new selection
        clear();
        setList(new_select);
    }
    if (!unlinked) {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("<b>No clones to unlink</b> in the selection."));
    }

    if (!skip_undo) {
        DocumentUndo::done(document(), SP_VERB_EDIT_UNLINK_CLONE,
                           _("Unlink clone"));
    }
    return unlinked;
}

bool ObjectSet::unlinkRecursive(const bool skip_undo, const bool force) {
    if (isEmpty()){
        if (desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>clones</b> to unlink."));
        return false;
    }
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool pathoperationsunlink = prefs->getBool("/options/pathoperationsunlink/value", true);
    if (!force && !pathoperationsunlink) {
        if (desktop() && !pathoperationsunlink) {
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Coulden't unlink see preference path operation unlink value."));
        }
        return false;
    }
    bool unlinked = false;
    ObjectSet tmp_set(document());
    std::vector<SPItem*> items_(items().begin(), items().end());
    for (auto& it:items_) {
        tmp_set.set(it);
        unlinked = tmp_set.unlink(true) || unlinked;
        it = tmp_set.singleItem();
        if (SP_IS_GROUP(it)) {
            std::vector<SPObject*> c = it->childList(false);
            tmp_set.setList(c);
            unlinked = tmp_set.unlinkRecursive(skip_undo, force) || unlinked;
        }
    }
    if (!unlinked) {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("<b>No clones to unlink</b> in the selection."));
    }
    if (!skip_undo) {
        DocumentUndo::done(document(), SP_VERB_EDIT_UNLINK_CLONE_RECURSIVE,
                                           _("Unlink clone recursively"));
    }
    setList(items_);
    return unlinked;
}

void ObjectSet::removeLPESRecursive(bool keep_paths) {
    if (isEmpty()){
        return;
    }

    ObjectSet tmp_set(document());
    std::vector<SPItem*> items_(items().begin(), items().end());
    for (auto& it:items_) {
        SPLPEItem *splpeitem = dynamic_cast<SPLPEItem *>(it);
        SPGroup *spgroup = dynamic_cast<SPGroup *>(it);
        if (spgroup) {
            std::vector<SPObject*> c = spgroup->childList(false);
            tmp_set.setList(c);
            tmp_set.removeLPESRecursive(keep_paths);
        } else if (splpeitem) {
            splpeitem->removeAllPathEffects(keep_paths);
        }
    }
    setList(items_);
}

void ObjectSet::cloneOriginal()
{
    SPItem *item = singleItem();

    gchar const *error = _("Select a <b>clone</b> to go to its original. Select a <b>linked offset</b> to go to its source. Select a <b>text on path</b> to go to the path. Select a <b>flowed text</b> to go to its frame.");

    // Check if other than two objects are selected

    auto items_= items();
    if (boost::distance(items_) != 1 || !item) {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, error);
        return;
    }

    SPItem *original = nullptr;
    SPUse *use = dynamic_cast<SPUse *>(item);
    if (use) {
        original = use->get_original();
    } else {
        SPOffset *offset = dynamic_cast<SPOffset *>(item);
        if (offset && offset->sourceHref) {
            original = sp_offset_get_source(offset);
        } else {
            SPText *text = dynamic_cast<SPText *>(item);
            SPTextPath *textpath = (text) ? dynamic_cast<SPTextPath *>(text->firstChild()) : nullptr;
            if (text && textpath) {
                original = sp_textpath_get_path_item(textpath);
            } else {
                SPFlowtext *flowtext = dynamic_cast<SPFlowtext *>(item);
                if (flowtext) {
                    original = flowtext->get_frame(nullptr); // first frame only
                }
            }
        }
    }

    if (original == nullptr) { // it's an object that we don't know what to do with
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, error);
        return;
    }

    if (!original) {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("<b>Cannot find</b> the object to select (orphaned clone, offset, textpath, flowed text?)"));
        return;
    }

    for (SPObject *o = original; o && !dynamic_cast<SPRoot *>(o); o = o->parent) {
        if (dynamic_cast<SPDefs *>(o)) {
            if(desktop())
                desktop()->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("The object you're trying to select is <b>not visible</b> (it is in &lt;defs&gt;)"));
            return;
        }
    }

    if (original) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        bool highlight = prefs->getBool("/options/highlightoriginal/value");
        if (highlight) {
            Geom::OptRect a = item->desktopVisualBounds();
            Geom::OptRect b = original->desktopVisualBounds();
            if ( a && b && desktop()) {
                // draw a flashing line between the objects
                auto curve = std::make_unique<SPCurve>();
                curve->moveto(a->midpoint());
                curve->lineto(b->midpoint());

                // We use a bpath as it supports dashes.
                auto canvas_item_bpath = new Inkscape::CanvasItemBpath(desktop()->getCanvasTemp(), curve.get());
                canvas_item_bpath->set_stroke(0x0000ddff);
                static std::vector<double> dashes = { 5, 3 };
                canvas_item_bpath->set_dashes(dashes);
                canvas_item_bpath->show();
                desktop()->add_temporary_canvasitem(canvas_item_bpath, 1000);
            }
        }

        clear();
        set(original);
        if (SP_CYCLING == SP_CYCLE_FOCUS && desktop()) {
            scroll_to_show_item(desktop(), original);
        }
    }
}

/**
* This applies the Fill Between Many LPE, and has it refer to the selection.
*/
void ObjectSet::cloneOriginalPathLPE(bool allow_transforms)
{

    Inkscape::SVGOStringStream os;
    SPObject * firstItem = nullptr;
    auto items_= items();
    bool multiple = false;
    for (auto i=items_.begin();i!=items_.end();++i){
        if (SP_IS_SHAPE(*i) || SP_IS_TEXT(*i) || SP_IS_GROUP(*i)) {
            if (firstItem) {
                os << "|";
                multiple = true;
            } else {
                firstItem = SP_ITEM(*i);
            }
            os << '#' << SP_ITEM(*i)->getId() << ",0,1";
        }
    }
    if (firstItem) {
        Inkscape::XML::Document *xml_doc = document()->getReprDoc();
        SPObject *parent = firstItem->parent;
        // create the LPE
        Inkscape::XML::Node *lpe_repr = xml_doc->createElement("inkscape:path-effect");
        if (multiple) {
            lpe_repr->setAttribute("effect", "fill_between_many");
            lpe_repr->setAttributeOrRemoveIfEmpty("linkedpaths", os.str());
            lpe_repr->setAttribute("applied", "true");
        } else {
            lpe_repr->setAttribute("effect", "clone_original");
            lpe_repr->setAttribute("linkeditem", ((Glib::ustring)"#" + (Glib::ustring)firstItem->getId()));
        }
        gchar const *method_str = allow_transforms ?  "d" : "bsplinespiro";
        lpe_repr->setAttribute("method", method_str);
        gchar const *allow_transforms_str = allow_transforms ? "true" : "false";
        lpe_repr->setAttribute("allow_transforms", allow_transforms_str);
        document()->getDefs()->getRepr()->addChild(lpe_repr, nullptr); // adds to <defs> and assigns the 'id' attribute
        std::string lpe_id_href = std::string("#") + lpe_repr->attribute("id");
        Inkscape::GC::release(lpe_repr);
        Inkscape::XML::Node* clone = nullptr;
        SPGroup *firstgroup = dynamic_cast<SPGroup *>(firstItem);
        if (firstgroup) {
            if (!multiple) {
                clone = firstgroup->getRepr()->duplicate(xml_doc);
            }
        } else {
            // create the new path
            clone = xml_doc->createElement("svg:path");
            clone->setAttribute("d", "M 0 0");
                
        }
        if (clone) {
            // add the new clone to the top of the original's parent
            parent->appendChildRepr(clone);
            // select the new object:
            set(clone);
            Inkscape::GC::release(clone);
            SPObject *clone_obj = document()->getObjectById(clone->attribute("id"));
            SPLPEItem *clone_lpeitem = dynamic_cast<SPLPEItem *>(clone_obj);
            if (clone_lpeitem) {
                clone_lpeitem->addPathEffect(lpe_id_href, false);
            }
            if (multiple) {
                DocumentUndo::done(document(), SP_VERB_EDIT_CLONE_ORIGINAL_PATH_LPE, _("Fill between many"));
            } else {
                DocumentUndo::done(document(), SP_VERB_EDIT_CLONE_ORIGINAL_PATH_LPE, _("Clone original"));
            }
        }
    } else {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select path(s) to fill."));
    }
}

void ObjectSet::toMarker(bool apply)
{
    // sp_selection_tile has similar code

    SPDocument *doc = document();
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();

    // check if something is selected
    if (isEmpty()) {
        if (desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE,
                                             _("Select <b>object(s)</b> to convert to marker."));
        return;
    }

    doc->ensureUpToDate();
    Geom::OptRect r = visualBounds();
    if (!r) {
        return;
    }

    std::vector<SPItem*> items_(items().begin(), items().end());
    sort(items_.begin(), items_.end(), sp_item_repr_compare_position_bool);

    // bottommost object, after sorting
    SPObject *parent = items_.front()->parent;

    Geom::Affine parent_transform;
    {
        SPItem *parentItem = dynamic_cast<SPItem *>(parent);
        if (parent) {
            parent_transform = parentItem->i2doc_affine();
        } else {
            g_assert_not_reached();
        }
    }

    // Create a list of duplicates, to be pasted inside marker element.
    std::vector<Inkscape::XML::Node*> repr_copies;
    for (auto *item : items_) {
        auto *dup = item->getRepr()->duplicate(xml_doc);
        repr_copies.push_back(dup);
    }

    Geom::Rect bbox(r->min() * doc->dt2doc(), r->max() * doc->dt2doc());

    // calculate the transform to be applied to objects to move them to 0,0
    // (alternative would be to define viewBox or set overflow:visible)
    Geom::Affine const move = Geom::Translate(-bbox.min());
    Geom::Point const center = bbox.dimensions() * 0.5;

    if (apply) {
        // Delete objects so that their clones don't get alerted;
        // the objects will be restored inside the marker element.
        for (auto item : items_){
            item->deleteObject(false);
        }
    }

    // Hack: Temporarily set clone compensation to unmoved, so that we can move clone-originals
    // without disturbing clones.
    // See ActorAlign::on_button_click() in src/ui/dialog/align-and-distribute.cpp
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int saved_compensation = prefs->getInt("/options/clonecompensation/value", SP_CLONE_COMPENSATION_UNMOVED);
    prefs->setInt("/options/clonecompensation/value", SP_CLONE_COMPENSATION_UNMOVED);

    gchar const *mark_id = generate_marker(repr_copies, bbox, doc, center, parent_transform * move);
    (void)mark_id;

    // restore compensation setting
    prefs->setInt("/options/clonecompensation/value", saved_compensation);



    DocumentUndo::done(doc, SP_VERB_EDIT_SELECTION_2_MARKER,
                       _("Objects to marker"));
}

static void sp_selection_to_guides_recursive(SPItem *item, bool wholegroups) {
    SPGroup *group = dynamic_cast<SPGroup *>(item);
    if (group && !dynamic_cast<SPBox3D *>(item) && !wholegroups) {
        std::vector<SPItem*> items=sp_item_group_item_list(group);
        for (auto item : items){
            sp_selection_to_guides_recursive(item, wholegroups);
        }
    } else {
        item->convert_to_guides();
    }
}

void ObjectSet::toGuides()
{
    SPDocument *doc = document();
    // we need to copy the list because it gets reset when objects are deleted
    std::vector<SPItem*> items_(items().begin(), items().end());

    if (isEmpty()) {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to convert to guides."));
        return;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool deleteitems = !prefs->getBool("/tools/cvg_keep_objects", false);
    bool wholegroups = prefs->getBool("/tools/cvg_convert_whole_groups", false);

    // If an object is earlier in the selection list than its clone, and it is deleted, then the clone will have changed
    // and its entry in the selection list is invalid (crash).
    // Therefore: first convert all, then delete all.

    for (auto item : items_){
        sp_selection_to_guides_recursive(item, wholegroups);
    }

    if (deleteitems) {
        clear();
        sp_selection_delete_impl(items_);
    }

    DocumentUndo::done(doc, SP_VERB_EDIT_SELECTION_2_GUIDES, _("Objects to guides"));
}

/*
 * Convert objects to <symbol>. How that happens depends on what is selected:
 *
 * 1) A random selection of objects will be embedded into a single <symbol> element.
 *
 * 2) Except, a single <g> will have its content directly embedded into a <symbol>; the 'id' and
 *    'style' of the <g> are transferred to the <symbol>.
 *
 * 3) Except, a single <g> with a transform that isn't a translation will keep the group when
 *    embedded into a <symbol> (with 'id' and 'style' transferred to <symbol>). This is because a
 *    <symbol> cannot have a transform. (If the transform is a pure translation, the translation
 *    is moved to the referencing <use> element that is created.)
 *
 * Possible improvements:
 *
 *   Move objects inside symbol so bbox corner at 0,0 (see marker/pattern)
 *
 *   For SVG2, set 'refX' 'refY' to object center (with compensating shift in <use>
 *   transformation).
 */
void ObjectSet::toSymbol()
{

    SPDocument *doc = document();
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();
    // Check if something is selected.
    if (isEmpty()) {
      if (desktop())
          desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>objects</b> to convert to symbol."));
      return;
    }

    doc->ensureUpToDate();

    std::vector<SPObject*> items_(objects().begin(), objects().end());
    sort(items_.begin(),items_.end(),sp_object_compare_position_bool);

    // Keep track of parent, this is where <use> will be inserted.
    Inkscape::XML::Node *the_first_repr = items_[0]->getRepr();
    Inkscape::XML::Node *the_parent_repr = the_first_repr->parent();

    // Find out if we have a single group
    bool single_group = false;
    SPGroup *the_group = nullptr;
    Geom::Affine transform;
    if( items_.size() == 1 ) {
        SPObject *object = items_[0];
        the_group = dynamic_cast<SPGroup *>(object);
        if ( the_group ) {
            single_group = true;

            if( !sp_svg_transform_read( object->getAttribute("transform"), &transform ))
                transform = Geom::identity();

            if( transform.isTranslation() ) {

                // Create new list from group children.
                items_ = object->childList(false);

                // Hack: Temporarily set clone compensation to unmoved, so that we can move clone-originals
                // without disturbing clones.
                // See ActorAlign::on_button_click() in src/ui/dialog/align-and-distribute.cpp
                Inkscape::Preferences *prefs = Inkscape::Preferences::get();
                int saved_compensation = prefs->getInt("/options/clonecompensation/value", SP_CLONE_COMPENSATION_UNMOVED);
                prefs->setInt("/options/clonecompensation/value", SP_CLONE_COMPENSATION_UNMOVED);

                // Remove transform on group, updating clones.
                the_group->doWriteTransform(Geom::identity());

                // restore compensation setting
                prefs->setInt("/options/clonecompensation/value", saved_compensation);
            }
        }
    }

    // Create new <symbol>
    Inkscape::XML::Node *defsrepr = doc->getDefs()->getRepr();
    Inkscape::XML::Node *symbol_repr = xml_doc->createElement("svg:symbol");
    Inkscape::XML::Node *title_repr = xml_doc->createElement("svg:title");

    defsrepr->appendChild(symbol_repr);
    bool settitle = false;
    // For a single group, copy relevant attributes.
    if( single_group ) {
        Glib::ustring id = the_group->getAttribute("id");
        symbol_repr->setAttribute("style",  the_group->getAttribute("style"));

        gchar * title = the_group->title();
        if (title) {
            symbol_repr->addChildAtPos(title_repr, 0);
            title_repr->appendChild(xml_doc->createTextNode(title));
            Inkscape::GC::release(title_repr);
        }
        g_free(title);

        gchar * desc = the_group->desc();
        if (desc) {
            Inkscape::XML::Node *desc_repr = xml_doc->createElement("svg:desc");
            desc_repr->setContent(desc);
            desc_repr->appendChild(xml_doc->createTextNode(desc));
            symbol_repr->addChildAtPos(desc_repr, 1);
            Inkscape::GC::release(desc_repr);
        }
        g_free(desc);
        symbol_repr->setAttribute("class",  the_group->getAttribute("class"));
        the_group->setAttribute("id", id + "_transform");
        symbol_repr->setAttribute("id", id);

        // This should eventually be replaced by 'refX' and 'refY' once SVG WG approves it.
        // It is done here for round-tripping
        symbol_repr->setAttribute("inkscape:transform-center-x",
                                  the_group->getAttribute("inkscape:transform-center-x"));
        symbol_repr->setAttribute("inkscape:transform-center-y",
                                  the_group->getAttribute("inkscape:transform-center-y"));

        the_group->removeAttribute("style");

    }

    // Move selected items to new <symbol>
    for (std::vector<SPObject*>::const_reverse_iterator i=items_.rbegin();i!=items_.rend();++i){
        gchar* title = (*i)->title();
        if (!single_group && !settitle && title) {
            symbol_repr->addChildAtPos(title_repr, 0);
            title_repr->appendChild(xml_doc->createTextNode(title));
            Inkscape::GC::release(title_repr);
            gchar * desc = (*i)->desc();
            if (desc) {
                Inkscape::XML::Node *desc_repr = xml_doc->createElement("svg:desc");
                desc_repr->appendChild(xml_doc->createTextNode(desc));
                symbol_repr->addChildAtPos(desc_repr, 1);
                Inkscape::GC::release(desc_repr);
            }
            g_free(desc);
            settitle = true;
        }
        g_free(title);
        Inkscape::XML::Node *repr = (*i)->getRepr();
        repr->parent()->removeChild(repr);
        symbol_repr->addChild(repr, nullptr);
    }

    if( single_group && transform.isTranslation() ) {
        the_group->deleteObject(true);
    }

    // Create <use> pointing to new symbol (to replace the moved objects).
    Inkscape::XML::Node *clone = xml_doc->createElement("svg:use");

    clone->setAttribute("xlink:href", Glib::ustring("#")+symbol_repr->attribute("id"));

    the_parent_repr->appendChild(clone);

    if( single_group && transform.isTranslation() ) {
        clone->setAttributeOrRemoveIfEmpty("transform", sp_svg_transform_write(transform));
    }

    // Change selection to new <use> element.
    set(clone);

    // Clean up
    Inkscape::GC::release(symbol_repr);

    DocumentUndo::done(doc, SP_VERB_EDIT_SYMBOL, _("Group to symbol"));
}

/*
 * Convert <symbol> to <g>. All <use> elements referencing symbol remain unchanged.
 */
void ObjectSet::unSymbol()
{
    SPDocument *doc = document();
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();
    // Check if something is selected.
    if (isEmpty()) {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select a <b>symbol</b> to extract objects from."));
        return;
    }

    SPObject* symbol = single();

    // Make sure we have only one object in selection.
    // Require that we really have a <symbol>.
    if( symbol == nullptr || !dynamic_cast<SPSymbol *>( symbol ))  {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select only one <b>symbol</b> in Symbol dialog to convert to group."));
        return;
    }

    doc->ensureUpToDate();

    // Create new <g> and insert in current layer
    Inkscape::XML::Node *group = xml_doc->createElement("svg:g");
    //TODO: Better handle if no desktop, currently go to defs without it
    if(desktop()) {
        desktop()->currentLayer()->getRepr()->appendChild(group);
    } else {
        symbol->parent->getRepr()->appendChild(group);
    }
    // Move all children of symbol to group
    std::vector<SPObject*> children = symbol->childList(false);

    // Converting a group to a symbol inserts a group for non-translational transform.
    // In converting a symbol back to a group we strip out the inserted group (or any other
    // group that only adds a transform to the symbol content).
    if( children.size() == 1 ) {
        SPObject *object = children[0];
        if ( dynamic_cast<SPGroup *>( object ) ) {
            if( object->getAttribute("style") == nullptr ||
                object->getAttribute("class") == nullptr ) {

                group->setAttribute("transform", object->getAttribute("transform"));
                children = object->childList(false);
            }
        }
    }

    for (std::vector<SPObject*>::const_reverse_iterator i=children.rbegin();i!=children.rend();++i){
        Inkscape::XML::Node *repr = (*i)->getRepr();
        repr->parent()->removeChild(repr);
        group->addChild(repr,nullptr);
    }

    // Copy relevant attributes
    group->setAttribute("style", symbol->getAttribute("style"));
    group->setAttribute("class", symbol->getAttribute("class"));
    group->setAttribute("title", symbol->getAttribute("title"));
    group->setAttribute("inkscape:transform-center-x",
                        symbol->getAttribute("inkscape:transform-center-x"));
    group->setAttribute("inkscape:transform-center-y",
                        symbol->getAttribute("inkscape:transform-center-y"));


    // Need to delete <symbol>; all <use> elements that referenced <symbol> should
    // auto-magically reference <g> (if <symbol> deleted after setting <g> 'id').
    Glib::ustring id = symbol->getAttribute("id");
    group->setAttribute("id", id);
    symbol->deleteObject(true);

    // Change selection to new <g> element.
    SPItem *group_item = static_cast<SPItem *>(document()->getObjectByRepr(group));
    set(group_item);

    // Clean up
    Inkscape::GC::release(group);

    DocumentUndo::done(doc, SP_VERB_EDIT_UNSYMBOL, _("Group from symbol"));
}

void ObjectSet::tile(bool apply)
{
    // toMarker has similar code

    SPDocument *doc = document();
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();

    // check if something is selected
    if (isEmpty()) {
        if (desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE,
                                             _("Select <b>object(s)</b> to convert to pattern."));
        return;
    }

    doc->ensureUpToDate();
    Geom::OptRect r = visualBounds();
    if ( !r ) {
        return;
    }

    std::vector<SPItem*> items_(items().begin(), items().end());

    sort(items_.begin(),items_.end(),sp_object_compare_position_bool);

    // bottommost object, after sorting
    SPObject *parent = items_[0]->parent;


    Geom::Affine parent_transform;
    {
        SPItem *parentItem = dynamic_cast<SPItem *>(parent);
        if (parentItem) {
            parent_transform = parentItem->i2doc_affine();
        } else {
            g_assert_not_reached();
        }
    }

    // remember the position of the first item
    gint pos = items_[0]->getRepr()->position();

    // create a list of duplicates
    std::vector<Inkscape::XML::Node*> repr_copies;
    for (auto item : items_){
        Inkscape::XML::Node *dup = item->getRepr()->duplicate(xml_doc);
        repr_copies.push_back(dup);
    }

    Geom::Rect bbox(r->min() * doc->dt2doc(), r->max() * doc->dt2doc());

    if (apply) {
        // delete objects so that their clones don't get alerted; this object will be restored shortly
        for (auto item : items_){
            item->deleteObject(false);
        }
    }

    // Hack: Temporarily set clone compensation to unmoved, so that we can move clone-originals
    // without disturbing clones.
    // See ActorAlign::on_button_click() in src/ui/dialog/align-and-distribute.cpp
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int saved_compensation = prefs->getInt("/options/clonecompensation/value", SP_CLONE_COMPENSATION_UNMOVED);
    prefs->setInt("/options/clonecompensation/value", SP_CLONE_COMPENSATION_UNMOVED);

    Geom::Affine move = Geom::Translate(- bbox.min());
    gchar const *pat_id = SPPattern::produce(repr_copies, bbox, doc,
                                       move.inverse() /* patternTransform */,
                                       parent_transform * move);

    // restore compensation setting
    prefs->setInt("/options/clonecompensation/value", saved_compensation);

    if (apply) {
        Inkscape::XML::Node *rect = xml_doc->createElement("svg:rect");
        gchar *style_str = g_strdup_printf("stroke:none;fill:url(#%s)", pat_id);
        rect->setAttribute("style", style_str);
        g_free(style_str);

        rect->setAttributeOrRemoveIfEmpty("transform", sp_svg_transform_write(parent_transform.inverse()));

        sp_repr_set_svg_double(rect, "width", bbox.width());
        sp_repr_set_svg_double(rect, "height", bbox.height());
        sp_repr_set_svg_double(rect, "x", bbox.left());
        sp_repr_set_svg_double(rect, "y", bbox.top());

        // restore parent and position
        parent->getRepr()->addChildAtPos(rect, pos);
        SPItem *rectangle = static_cast<SPItem *>(document()->getObjectByRepr(rect));

        Inkscape::GC::release(rect);

        clear();
        set(rectangle);
    }


    DocumentUndo::done(doc, SP_VERB_EDIT_TILE,
                       _("Objects to pattern"));
}

void ObjectSet::untile()
{

    SPDocument *doc = document();
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();

    // check if something is selected
    if (isEmpty()) {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select an <b>object with pattern fill</b> to extract objects from."));
        return;
    }

    std::vector<SPItem*> new_select;

    bool did = false;

    std::vector<SPItem*> items_(items().begin(), items().end());
    for (std::vector<SPItem*>::const_reverse_iterator i=items_.rbegin();i!=items_.rend();++i){
        SPItem *item = *i;

        SPStyle *style = item->style;

        if (!style || !style->fill.isPaintserver())
            continue;

        SPPaintServer *server = item->style->getFillPaintServer();

        SPPattern *basePat = dynamic_cast<SPPattern *>(server);
        if (!basePat) {
            continue;
        }

        did = true;

        SPPattern *pattern = basePat->rootPattern();

        Geom::Affine pat_transform = basePat->getTransform();
        pat_transform *= item->transform;

        for (auto& child: pattern->children) {
            if (dynamic_cast<SPItem *>(&child)) {
                Inkscape::XML::Node *copy = child.getRepr()->duplicate(xml_doc);
                SPItem *i = dynamic_cast<SPItem *>(item->parent->appendChildRepr(copy));

               // FIXME: relink clones to the new canvas objects
               // use SPObject::setid when mental finishes it to steal ids of

                // this is needed to make sure the new item has curve (simply requestDisplayUpdate does not work)
                doc->ensureUpToDate();

                if (i) {
                    Geom::Affine transform( i->transform * pat_transform );
                    i->doWriteTransform(transform);

                    new_select.push_back(i);
                } else {
                    g_assert_not_reached();
                }
            }
        }

        SPCSSAttr *css = sp_repr_css_attr_new();
        sp_repr_css_set_property(css, "fill", "none");
        sp_repr_css_change(item->getRepr(), css, "style");
    }

    if (!did) {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("<b>No pattern fills</b> in the selection."));
    } else {
        DocumentUndo::done(document(), SP_VERB_EDIT_UNTILE,
                           _("Pattern to objects"));
        setList(new_select);
    }
}

void ObjectSet::getExportHints(Glib::ustring &filename, float *xdpi, float *ydpi)
{
    if (isEmpty()) {
        return;
    }

    auto reprlst = xmlNodes();
    bool filename_search = TRUE;
    bool xdpi_search = TRUE;
    bool ydpi_search = TRUE;

    for (auto i=reprlst.begin();filename_search&&xdpi_search&&ydpi_search&&i!=reprlst.end();++i){
        gchar const *dpi_string;
        Inkscape::XML::Node *repr = *i;

        if (filename_search) {
            const gchar* tmp = repr->attribute("inkscape:export-filename");
            if (tmp){
                filename = tmp;
                filename_search = FALSE;
            }
            else{
                filename.clear();
            }
        }

        if (xdpi_search) {
            dpi_string = repr->attribute("inkscape:export-xdpi");
            if (dpi_string != nullptr) {
                *xdpi = atof(dpi_string);
                xdpi_search = FALSE;
            }
        }

        if (ydpi_search) {
            dpi_string = repr->attribute("inkscape:export-ydpi");
            if (dpi_string != nullptr) {
                *ydpi = atof(dpi_string);
                ydpi_search = FALSE;
            }
        }
    }
}

void sp_document_get_export_hints(SPDocument *doc, Glib::ustring &filename, float *xdpi, float *ydpi)
{
    Inkscape::XML::Node * repr = doc->getReprRoot();

    const gchar* tmp = repr->attribute("inkscape:export-filename");
    if(tmp)
    {
        filename = tmp;
    }
    else
    {
        filename.clear();
    }
    gchar const *dpi_string = repr->attribute("inkscape:export-xdpi");
    if (dpi_string != nullptr) {
        *xdpi = atof(dpi_string);
    }

    dpi_string = repr->attribute("inkscape:export-ydpi");
    if (dpi_string != nullptr) {
        *ydpi = atof(dpi_string);
    }
}

void ObjectSet::createBitmapCopy()
{

    SPDocument *doc = document();
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();

    // check if something is selected
    if (isEmpty()) {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to make a bitmap copy."));
        return;
    }
    if(desktop()){
        desktop()->messageStack()->flash(Inkscape::IMMEDIATE_MESSAGE, _("Rendering bitmap..."));
        // set "busy" cursor
        desktop()->setWaitingCursor();
    }

    // Get the bounding box of the selection
    doc->ensureUpToDate();
    Geom::OptRect bbox = documentBounds(SPItem::VISUAL_BBOX);
    if (!bbox) {
        if(desktop())
            desktop()->clearWaitingCursor();
        return; // exceptional situation, so not bother with a translatable error message, just quit quietly
    }

    // List of the items to show; all others will be hidden
    std::vector<SPItem*> items_(items().begin(), items().end());

    // Sort items so that the topmost comes last
    sort(items_.begin(),items_.end(),sp_item_repr_compare_position_bool);

    // Generate a random value from the current time (you may create bitmap from the same object(s)
    // multiple times, and this is done so that they don't clash)
    guint current = guint(g_get_monotonic_time() % 1024);
    // Create the filename.
    gchar *const basename = g_strdup_printf("%s-%s-%u.png",
                                            doc->getDocumentName(),
                                            items_[0]->getRepr()->attribute("id"),
                                            current);
    // Imagemagick is known not to handle spaces in filenames, so we replace anything but letters,
    // digits, and a few other chars, with "_"
    g_strcanon(basename, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.=+~$#@^&!?", '_');

    // Build the complete path by adding document base dir, if set, otherwise home dir
    gchar *directory = nullptr;
    if ( doc->getDocumentURI() ) {
        directory = g_path_get_dirname( doc->getDocumentURI() );
    }
    if (directory == nullptr) {
        directory = Inkscape::IO::Resource::homedir_path(nullptr);
    }
    gchar *filepath = g_build_filename(directory, basename, NULL);
    g_free(directory);

    //g_print("%s\n", filepath);

    // Remember parent and z-order of the topmost one
    gint pos = items_.back()->getRepr()->position();
    SPObject *parent_object = items_.back()->parent;
    Inkscape::XML::Node *parent = parent_object->getRepr();

    // Calculate resolution
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double res;
    int const prefs_res = prefs->getInt("/options/createbitmap/resolution", 0);
    int const prefs_min = prefs->getInt("/options/createbitmap/minsize", 0);
    if (0 < prefs_res) {
        // If it's given explicitly in prefs, take it
        res = prefs_res;
    } else if (0 < prefs_min) {
        // If minsize is given, look up minimum bitmap size (default 250 pixels) and calculate resolution from it
        res = Inkscape::Util::Quantity::convert(prefs_min, "in", "px") / MIN(bbox->width(), bbox->height());
    } else {
        float hint_xdpi = 0, hint_ydpi = 0;
        Glib::ustring hint_filename;
        // take resolution hint from the selected objects
        getExportHints(hint_filename, &hint_xdpi, &hint_ydpi);
        if (hint_xdpi != 0) {
            res = hint_xdpi;
        } else {
            // take resolution hint from the document
            sp_document_get_export_hints(doc, hint_filename, &hint_xdpi, &hint_ydpi);
            if (hint_xdpi != 0) {
                res = hint_xdpi;
            } else {
                // if all else fails, take the default 96 dpi
                res = Inkscape::Util::Quantity::convert(1, "in", "px");
            }
        }
    }

    // The width and height of the bitmap in pixels
    unsigned width = (unsigned) floor(bbox->width() * Inkscape::Util::Quantity::convert(res, "px", "in"));
    unsigned height =(unsigned) floor(bbox->height() * Inkscape::Util::Quantity::convert(res, "px", "in"));

    // Find out if we have to run an external filter
    gchar const *run = nullptr;
    Glib::ustring filter = prefs->getString("/options/createbitmap/filter");
    if (!filter.empty()) {
        // filter command is given;
        // see if we have a parameter to pass to it
        Glib::ustring param1 = prefs->getString("/options/createbitmap/filter_param1");
        if (!param1.empty()) {
            if (param1[param1.length() - 1] == '%') {
                // if the param string ends with %, interpret it as a percentage of the image's max dimension
                gchar p1[256];
                g_ascii_dtostr(p1, 256, ceil(g_ascii_strtod(param1.data(), nullptr) * MAX(width, height) / 100));
                // the first param is always the image filename, the second is param1
                run = g_strdup_printf("%s \"%s\" %s", filter.data(), filepath, p1);
            } else {
                // otherwise pass the param1 unchanged
                run = g_strdup_printf("%s \"%s\" %s", filter.data(), filepath, param1.data());
            }
        } else {
            // run without extra parameter
            run = g_strdup_printf("%s \"%s\"", filter.data(), filepath);
        }
    }

    // Calculate the matrix that will be applied to the image so that it exactly overlaps the source objects
    Geom::Affine eek;
    {
        SPItem *parentItem = dynamic_cast<SPItem *>(parent_object);
        if (parentItem) {
            eek = parentItem->i2doc_affine();
        } else {
            g_assert_not_reached();
        }
    }
    Geom::Affine t;

    double shift_x = bbox->left();
    double shift_y = bbox->top();
    if (res == Inkscape::Util::Quantity::convert(1, "in", "px")) { // for default 96 dpi, snap it to pixel grid
        shift_x = round(shift_x);
        shift_y = round(shift_y);
    }
    t = Geom::Translate(shift_x, shift_y) * eek.inverse();

    // TODO: avoid roundtrip via file
    // Do the export
    sp_export_png_file(doc, filepath,
                       bbox->min()[Geom::X], bbox->min()[Geom::Y],
                       bbox->max()[Geom::X], bbox->max()[Geom::Y],
                       width, height, res, res,
                       (guint32) 0xffffff00,
                       nullptr, nullptr,
                       true,  /*bool force_overwrite,*/
                       items_);

    // Run filter, if any
    if (run) {
        g_print("Running external filter: %s\n", run);
        int result = system(run);

        if(result == -1)
            g_warning("Could not run external filter: %s\n", run);
    }

    // Import the image back
    Inkscape::Pixbuf *pb = Inkscape::Pixbuf::create_from_file(filepath);
    if (pb) {
        // Create the repr for the image
        // TODO: avoid unnecessary roundtrip between data URI and decoded pixbuf
        Inkscape::XML::Node * repr = xml_doc->createElement("svg:image");
        sp_embed_image(repr, pb);
        if (res == Inkscape::Util::Quantity::convert(1, "in", "px")) { // for default 96 dpi, snap it to pixel grid
            sp_repr_set_svg_double(repr, "width", width);
            sp_repr_set_svg_double(repr, "height", height);
        } else {
            sp_repr_set_svg_double(repr, "width", bbox->width());
            sp_repr_set_svg_double(repr, "height", bbox->height());
        }

        // Write transform
        repr->setAttributeOrRemoveIfEmpty("transform", sp_svg_transform_write(t));

        // add the new repr to the parent
        parent->addChildAtPos(repr, pos + 1);

        // Set selection to the new image
        clear();
        add(repr);

        // Clean up
        Inkscape::GC::release(repr);
        delete pb;

        // Complete undoable transaction
        DocumentUndo::done(doc, SP_VERB_SELECTION_CREATE_BITMAP,
                           _("Create bitmap"));
    }
    if(desktop())
        desktop()->clearWaitingCursor();

    g_free(basename);
    g_free(filepath);
}

/* Creates a mask or clipPath from selection.
 * What is a clip group?
 * A clip group is a tangled mess of XML that allows an object inside a group
 * to clip the entire group using a few <use>s and generally irritating me.
 */

void ObjectSet::setClipGroup()
{
    SPDocument* doc = document();
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();

    if (isEmpty()) {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to create clippath or mask from."));
        return;
    }

    std::vector<Inkscape::XML::Node*> p(xmlNodes().begin(), xmlNodes().end());

    sort(p.begin(),p.end(),sp_repr_compare_position_bool);

    clear();

    int topmost = (p.back())->position();
    Inkscape::XML::Node *topmost_parent = (p.back())->parent();

    Inkscape::XML::Node *inner = xml_doc->createElement("svg:g");
    inner->setAttribute("inkscape:label", "Clip");

    for(auto current : p){
        if (current->parent() == topmost_parent) {
            Inkscape::XML::Node *spnew = current->duplicate(xml_doc);
            sp_repr_unparent(current);
            inner->appendChild(spnew);
            Inkscape::GC::release(spnew);
            topmost --; // only reduce count for those items deleted from topmost_parent
        } else { // move it to topmost_parent first
            std::vector<Inkscape::XML::Node*> temp_clip;

            // At this point, current may already have no item, due to its being a clone whose original is already moved away
            // So we copy it artificially calculating the transform from its repr->attr("transform") and the parent transform
            gchar const *t_str = current->attribute("transform");
            Geom::Affine item_t(Geom::identity());
            if (t_str)
                sp_svg_transform_read(t_str, &item_t);
            item_t *= SP_ITEM(doc->getObjectByRepr(current->parent()))->i2doc_affine();
            // FIXME: when moving both clone and original from a transformed group (either by
            // grouping into another parent, or by cut/paste) the transform from the original's
            // parent becomes embedded into original itself, and this affects its clones. Fix
            // this by remembering the transform diffs we write to each item into an array and
            // then, if this is clone, looking up its original in that array and pre-multiplying
            // it by the inverse of that original's transform diff.

            sp_selection_copy_one(current, item_t, temp_clip, xml_doc);
            sp_repr_unparent(current);

            // paste into topmost_parent (temporarily)
            std::vector<Inkscape::XML::Node*> copied = sp_selection_paste_impl(doc, doc->getObjectByRepr(topmost_parent), temp_clip);
            if (!copied.empty()) { // if success,
                // take pasted object (now in topmost_parent)
                Inkscape::XML::Node *in_topmost = copied.back();
                // make a copy
                Inkscape::XML::Node *spnew = in_topmost->duplicate(xml_doc);
                // remove pasted
                sp_repr_unparent(in_topmost);
                // put its copy into group
                inner->appendChild(spnew);
                Inkscape::GC::release(spnew);
            }
        }
    }

    Inkscape::XML::Node *outer = xml_doc->createElement("svg:g");
    outer->appendChild(inner);
    topmost_parent->addChildAtPos(outer, topmost + 1);

    Inkscape::XML::Node *clone = xml_doc->createElement("svg:use");
    clone->setAttribute("x", "0");
    clone->setAttribute("y", "0");
    clone->setAttribute("xlink:href", g_strdup_printf("#%s", inner->attribute("id")));

    clone->setAttribute("inkscape:transform-center-x", inner->attribute("inkscape:transform-center-x"));
    clone->setAttribute("inkscape:transform-center-y", inner->attribute("inkscape:transform-center-y"));

    std::vector<Inkscape::XML::Node*> templist;
    templist.push_back(clone);
    // add the new clone to the top of the original's parent
    gchar const *mask_id = SPClipPath::create(templist, doc);

    char* tmp = g_strdup_printf("url(#%s)", mask_id);
    outer->setAttribute("clip-path", tmp);
    g_free(tmp);

    Inkscape::GC::release(clone);

    set(outer);
    DocumentUndo::done(doc, SP_VERB_OBJECT_SET_CLIPPATH, _("Create Clip Group"));
}

/**
 * Creates a mask or clipPath from selection.
 * Two different modes:
 *  if applyToLayer, all selection is moved to DEFS as mask/clippath
 *       and is applied to current layer
 *  otherwise, topmost object is used as mask for other objects
 * If \a apply_clip_path parameter is true, clipPath is created, otherwise mask
 *
 */
 void ObjectSet::setMask(bool apply_clip_path, bool apply_to_layer, bool skip_undo)
{
    if(!desktop() && apply_to_layer)
        return;

    SPDocument *doc = document();
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();

    // check if something is selected
    bool is_empty = isEmpty();
    if ( apply_to_layer && is_empty) {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to create clippath or mask from."));
        return;
    } else if (!apply_to_layer && ( is_empty || boost::distance(items())==1 )) {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select mask object and <b>object(s)</b> to apply clippath or mask to."));
        return;
    }

    // FIXME: temporary patch to prevent crash!
    // Remove this when bboxes are fixed to not blow up on an item clipped/masked with its own clone
    bool clone_with_original = object_set_contains_both_clone_and_original(this);
    if (clone_with_original) {
        return; // in this version, you cannot clip/mask an object with its own clone
    }
    // /END FIXME

    doc->ensureUpToDate();

    // Comment out this section because we don't need it, I think.
    // Also updated comment code to work correctly if indeed it's needed.
    // To reactivate, remove the next line and uncomment the section.
    std::vector<SPItem*> items_(items().begin(), items().end());
    /*
    std::vector<SPItem*> items_prerect_(items().begin(), items().end());
    std::vector<SPItem*> items_;

    // convert any rects to paths
    for (std::vector<SPItem *>::const_iterator i = items_prerect_.begin(); i != items_prerect_.end(); ++i) {
        clear();
        if (dynamic_cast<SPRect *>(*i)) {
            add(*i);
            toCurves();
            items_.push_back(*items().begin());
        } else {
            items_.push_back(*i);
        }
    }
    clear();
    */
    sort(items_.begin(),items_.end(),sp_object_compare_position_bool);

    // See lp bug #542004
    clear();

    // create a list of duplicates
    std::vector<std::pair<Inkscape::XML::Node*, Geom::Affine>> mask_items;
    std::vector<SPItem*> apply_to_items;
    std::vector<SPItem*> items_to_delete;
    std::vector<SPItem*> items_to_select;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool topmost = prefs->getBool("/options/maskobject/topmost", true);
    bool remove_original = prefs->getBool("/options/maskobject/remove", true);
    int grouping = prefs->getInt("/options/maskobject/grouping", PREFS_MASKOBJECT_GROUPING_NONE);

    if (apply_to_layer) {
        // all selected items are used for mask, which is applied to a layer
        apply_to_items.push_back(SP_ITEM(desktop()->currentLayer()));
    }

    for (std::vector<SPItem*>::const_iterator i=items_.begin();i!=items_.end();++i) {
        if((!topmost && !apply_to_layer && *i == items_.front())
                || (topmost && !apply_to_layer && *i == items_.back())
                || apply_to_layer){

            Inkscape::XML::Node *dup = (*i)->getRepr()->duplicate(xml_doc);
            mask_items.emplace_back(dup, (*i)->i2doc_affine());

            if (remove_original) {
                items_to_delete.push_back(*i);
            }
            else {
                items_to_select.push_back(*i);
            }
            continue;
        }else{
            apply_to_items.push_back(*i);
            items_to_select.push_back(*i);
        }
    }

    items_.clear();

    if (grouping == PREFS_MASKOBJECT_GROUPING_ALL) {
        // group all those objects into one group
        // and apply mask to that
        ObjectSet* set = new ObjectSet(document());
        set->add(apply_to_items.begin(), apply_to_items.end());

        items_to_select.clear();

        Inkscape::XML::Node *group = set->group();
        group->setAttribute("inkscape:groupmode", "maskhelper");

        // apply clip/mask only to newly created group
        apply_to_items.clear();
        apply_to_items.push_back(dynamic_cast<SPItem*>(doc->getObjectByRepr(group)));

        items_to_select.push_back((SPItem*)(doc->getObjectByRepr(group)));

        delete set;
        Inkscape::GC::release(group);
    }
    if (grouping == PREFS_MASKOBJECT_GROUPING_SEPARATE) {
        items_to_select.clear();
    }


    gchar const *attributeName = apply_clip_path ? "clip-path" : "mask";
    for (std::vector<SPItem*>::const_reverse_iterator i = apply_to_items.rbegin(); i != apply_to_items.rend(); ++i) {
        SPItem *item = reinterpret_cast<SPItem *>(*i);
        std::vector<Inkscape::XML::Node*> mask_items_dup;
        std::map<Inkscape::XML::Node*, Geom::Affine> dup_transf;
        for (auto & mask_item : mask_items) {
            Inkscape::XML::Node *dup = (mask_item.first)->duplicate(xml_doc);
            mask_items_dup.push_back(dup);
            dup_transf[dup] = mask_item.second;
        }

        Inkscape::XML::Node *current = SP_OBJECT(*i)->getRepr();
        // Node to apply mask to
        Inkscape::XML::Node *apply_mask_to = current;

        if (grouping == PREFS_MASKOBJECT_GROUPING_SEPARATE) {
            // enclose current node in group, and apply crop/mask on that
            Inkscape::XML::Node *group = xml_doc->createElement("svg:g");
            // make a note we should ungroup this when unsetting mask
            group->setAttribute("inkscape:groupmode", "maskhelper");

            Inkscape::XML::Node *spnew = current->duplicate(xml_doc);
            current->parent()->addChild(group, current);
            sp_repr_unparent(current);
            group->appendChild(spnew);

            // Apply clip/mask to group instead
            apply_mask_to = group;

            items_to_select.push_back((SPItem*)doc->getObjectByRepr(group));
            Inkscape::GC::release(spnew);
            Inkscape::GC::release(group);
        }

        gchar const *mask_id = nullptr;
        if (apply_clip_path) {
            mask_id = SPClipPath::create(mask_items_dup, doc);
        } else {
            mask_id = sp_mask_create(mask_items_dup, doc);
        }

        // inverted object transform should be applied to a mask object,
        // as mask is calculated in user space (after applying transform)
        for (auto & it : mask_items_dup) {
            SPItem *clip_item = SP_ITEM(doc->getObjectByRepr(it));
            clip_item->doWriteTransform(dup_transf[it]);
            clip_item->doWriteTransform(clip_item->transform * item->i2doc_affine().inverse());
        }

        apply_mask_to->setAttribute(attributeName, Glib::ustring("url(#") + mask_id + ')');

    }

    for (auto i : items_to_delete) {
        SPObject *item = reinterpret_cast<SPObject*>(i);
        item->deleteObject(false);
        items_to_select.erase(std::remove(items_to_select.begin(), items_to_select.end(), item), items_to_select.end());
    }

    addList(items_to_select);
    if (!skip_undo) {
        if (apply_clip_path) {
            DocumentUndo::done(doc, SP_VERB_OBJECT_SET_CLIPPATH, _("Set clipping path"));
        } else {
            DocumentUndo::done(doc, SP_VERB_OBJECT_SET_MASK, _("Set mask"));
        }
    }
}

void ObjectSet::unsetMask(const bool apply_clip_path, const bool skip_undo) {
    SPDocument *doc = document();
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();

    // check if something is selected
    if (isEmpty()) {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to remove clippath or mask from."));
        return;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool remove_original = prefs->getBool("/options/maskobject/remove", true);
    bool ungroup_masked = prefs->getBool("/options/maskobject/ungrouping", true);
    doc->ensureUpToDate();

    gchar const *attributeName = apply_clip_path ? "clip-path" : "mask";
    std::map<SPObject*,SPItem*> referenced_objects;

    std::vector<SPItem*> items_(items().begin(), items().end());
    clear();

    std::vector<SPGroup *> items_to_ungroup;
    std::vector<SPItem*> items_to_select(items_);


    // SPObject* refers to a group containing the clipped path or mask itself,
    // whereas SPItem* refers to the item being clipped or masked
    for (auto i : items_){
        if (remove_original) {
            // remember referenced mask/clippath, so orphaned masks can be moved back to document
            SPItem *item = i;
            SPObject *obj_ref = nullptr;

            if (apply_clip_path) {
                obj_ref = item->getClipObject();
            } else {
                obj_ref = item->getMaskObject();
            }

            // collect distinct mask object (and associate with item to apply transform)
            if (obj_ref) {
                referenced_objects[obj_ref] = item;
            }
        }

        i->setAttribute(attributeName, "none");

        SPGroup *group = dynamic_cast<SPGroup *>(i);
        if (ungroup_masked && group) {
                // if we had previously enclosed masked object in group,
                // add it to list so we can ungroup it later

                // ungroup only groups we created when setting clip/mask
                if (group->layerMode() == SPGroup::MASK_HELPER) {
                    items_to_ungroup.push_back(group);
                }

        }
    }

    // restore mask objects into a document
    for (auto & referenced_object : referenced_objects) {
        SPObject *obj = referenced_object.first; // Group containing the clipped paths or masks
        std::vector<Inkscape::XML::Node *> items_to_move;
        for (auto& child: obj->children) {
            // Collect all clipped paths and masks within a single group
            Inkscape::XML::Node *copy = child.getRepr()->duplicate(xml_doc);
            if (copy->attribute("inkscape:original-d") && copy->attribute("inkscape:path-effect")) {
                copy->setAttribute("d", copy->attribute("inkscape:original-d"));
            } else if (copy->attribute("inkscape:original-d")) {
                copy->setAttribute("d", copy->attribute("inkscape:original-d"));
                copy->removeAttribute("inkscape:original-d");
            } else if (!copy->attribute("inkscape:path-effect") && !SP_IS_PATH(&child)) {
                copy->removeAttribute("d");
                copy->removeAttribute("inkscape:original-d");
            }
            items_to_move.push_back(copy);
        }

        if (!obj->isReferenced()) {
            // delete from defs if no other object references this mask
            obj->deleteObject(false);
        }

        // remember parent and position of the item to which the clippath/mask was applied
        Inkscape::XML::Node *parent = (referenced_object.second)->getRepr()->parent();
        Inkscape::XML::Node *ref_repr = referenced_object.second->getRepr();

        // Iterate through all clipped paths / masks
        for (auto i=items_to_move.rbegin();i!=items_to_move.rend();++i) {
            Inkscape::XML::Node *repr = *i;

            // insert into parent, restore pos
            parent->addChild(repr, ref_repr);

            SPItem *mask_item = dynamic_cast<SPItem *>(document()->getObjectByRepr(repr));
            if (!mask_item) {
                continue;
            }
            items_to_select.push_back(mask_item);

            // transform mask, so it is moved the same spot where mask was applied
            Geom::Affine transform(mask_item->transform);
            transform *= referenced_object.second->transform;
            mask_item->doWriteTransform(transform);
        }
    }

    // ungroup marked groups added when setting mask
    for (auto i=items_to_ungroup.rbegin();i!=items_to_ungroup.rend();++i) {
        SPGroup *group = *i;
        if (group) {
            items_to_select.erase(std::remove(items_to_select.begin(), items_to_select.end(), group), items_to_select.end());
            std::vector<SPItem*> children;
            sp_item_group_ungroup(group, children, false);
            items_to_select.insert(items_to_select.end(),children.rbegin(),children.rend());
        } else {
            g_assert_not_reached();
        }
    }

    // rebuild selection
    addList(items_to_select);
    if (!skip_undo) {
        if (apply_clip_path) {
            DocumentUndo::done(doc, SP_VERB_OBJECT_UNSET_CLIPPATH, _("Release clipping path"));
        } else {
            DocumentUndo::done(doc, SP_VERB_OBJECT_UNSET_MASK, _("Release mask"));
        }
    }
}

/**
 * \param with_margins margins defined in the xml under <sodipodi:namedview>
 *                     "fit-margin-..." attributes.  See SPDocument::fitToRect.
 * \return true if an undoable change should be recorded.
 */
bool ObjectSet::fitCanvas(bool with_margins, bool skip_undo)
{
    g_return_val_if_fail(document() != nullptr, false);

    if (isEmpty()) {
        if(desktop())
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to fit canvas to."));
        return false;
    }
    Geom::OptRect const bbox = documentBounds(SPItem::VISUAL_BBOX);
    if (bbox) {
        document()->fitToRect(*bbox, with_margins);
        if(!skip_undo)
            DocumentUndo::done(document(), SP_VERB_FIT_CANVAS_TO_SELECTION,
                            _("Fit Page to Selection"));
        return true;
    } else {
        return false;
    }
}

void ObjectSet::swapFillStroke()
{
    if (desktop() == nullptr) {
        return;
    }

    SPIPaint *paint;
    SPPaintServer *server;
    Glib::ustring _paintserver_id;

    auto list= items();
    for (auto itemlist=list.begin();itemlist!=list.end();++itemlist) {
        SPItem *item = *itemlist;

        SPCSSAttr *css = sp_repr_css_attr_new ();

        _paintserver_id.clear();
        paint = &(item->style->fill);
        if (paint->set && paint->isNone())
            sp_repr_css_set_property (css, "stroke", "none");
        else if (paint->set && paint->isColor()) {
            guint32 color = paint->value.color.toRGBA32(SP_SCALE24_TO_FLOAT (item->style->fill_opacity.value));
            gchar c[64];
            sp_svg_write_color (c, sizeof(c), color);
            sp_repr_css_set_property (css, "stroke", c);
        }
        else if (!paint->set)
            sp_repr_css_unset_property (css, "stroke");
        else if (paint->set && paint->isPaintserver()) {
            server = SP_STYLE_FILL_SERVER(item->style);
            if (server) {
                Inkscape::XML::Node *srepr = server->getRepr();
                _paintserver_id += "url(#";
                _paintserver_id += srepr->attribute("id");
                _paintserver_id += ")";
                sp_repr_css_set_property (css, "stroke", _paintserver_id.c_str());
            }
        }

        _paintserver_id.clear();
        paint = &(item->style->stroke);
        if (paint->set && paint->isNone())
            sp_repr_css_set_property (css, "fill", "none");
        else if (paint->set && paint->isColor()) {
            guint32 color = paint->value.color.toRGBA32(SP_SCALE24_TO_FLOAT (item->style->stroke_opacity.value));
            gchar c[64];
            sp_svg_write_color (c, sizeof(c), color);
            sp_repr_css_set_property (css, "fill", c);
        }
        else if (!paint->set)
            sp_repr_css_unset_property (css, "fill");
        else if (paint->set && paint->isPaintserver()) {
            server = SP_STYLE_STROKE_SERVER(item->style);
            if (server) {
                Inkscape::XML::Node *srepr = server->getRepr();
                _paintserver_id += "url(#";
                _paintserver_id += srepr->attribute("id");
                _paintserver_id += ")";
                sp_repr_css_set_property (css, "fill", _paintserver_id.c_str());
            }
        }

        Inkscape::ObjectSet set{};
        set.add(item);
        sp_desktop_set_style(&set, desktop(), css);

        sp_repr_css_attr_unref (css);
    }

    DocumentUndo::done(document(), SP_VERB_EDIT_SWAP_FILL_STROKE,
                _("Swap fill and stroke of an object"));
}

/**
 * Creates a linked fill between all the objects in the current selection using
 * the "Fill Between Many" LPE. After this method completes, the linked fill
 * created becomes the new selection, so as to facilitate quick styling of the
 * fill.
 *
 * All objects referred to must have an ID. If an ID does not exist, it will be
 * created.
 *
 * As an additional timesaver, the fill object is created below the bottommost
 * object in the selection.
 */
void ObjectSet::fillBetweenMany()
{
    if (isEmpty()) {
        if (desktop()) {
            desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>path(s)</b> to create fill between."));
        }

        return;
    }

    SPDocument *doc  = document();
    SPObject *defs   = doc->getDefs();
    SPObject *effect = nullptr;

    Inkscape::XML::Node *effectRepr = doc->getReprDoc()->createElement("inkscape:path-effect");
    Inkscape::XML::Node *fillRepr   = doc->getReprDoc()->createElement("svg:path");

    Glib::ustring acc;
    Glib::ustring pathTarget;

    for (auto&& item : items()) {
        // Force-assign id if there is none present
        if (!item->getId()) {
            gchar *id = sp_object_get_unique_id(item, nullptr);
            item->set(SPAttr::ID, id);
            item->updateRepr();
            g_free(id);
        }

        acc += "#";
        acc += item->getId();
        acc += ",0,1|";
    }

    effectRepr->setAttribute("effect", "fill_between_many");
    effectRepr->setAttribute("method", "originald");
    effectRepr->setAttribute("linkedpaths", acc.c_str());
    defs->appendChild(effectRepr);

    effect = doc->getObjectByRepr(effectRepr);
    pathTarget += "#";
    pathTarget += effect->getId();

    fillRepr->setAttribute("inkscape:original-d", "M 0,0");
    fillRepr->setAttribute("inkscape:path-effect", pathTarget.c_str());
    fillRepr->setAttribute("d", "M 0,0");

    // Get bottommost element in selection to create fill underneath
    auto&& items_ = std::vector<SPObject*>(items().begin(), items().end());
    SPObject *first = *std::min_element(items_.begin(), items_.end(), sp_object_compare_position_bool);
    SPObject *prev  = first->getPrev();

    first->parent->addChild(fillRepr, prev ? prev->getRepr() : nullptr);

    doc->ensureUpToDate();

    clear();
    add(fillRepr);

    DocumentUndo::done(doc, SP_VERB_SELECTION_FILL_BETWEEN_MANY, _("Create linked fill object between paths"));
}

/**
 * \param with_margins margins defined in the xml under <sodipodi:namedview>
 *                     "fit-margin-..." attributes.  See SPDocument::fitToRect.
 */
bool
fit_canvas_to_drawing(SPDocument *doc, bool with_margins)
{
    g_return_val_if_fail(doc != nullptr, false);

    doc->ensureUpToDate();
    SPItem const *const root = doc->getRoot();
    Geom::OptRect bbox = root->documentVisualBounds();
    if (bbox) {
        doc->fitToRect(*bbox, with_margins);
        return true;
    } else {
        return false;
    }
}

void
verb_fit_canvas_to_drawing(SPDesktop *desktop)
{
    if (fit_canvas_to_drawing(desktop->getDocument())) {
        DocumentUndo::done(desktop->getDocument(), SP_VERB_FIT_CANVAS_TO_DRAWING,
                           _("Fit Page to Drawing"));
    }
}

/**
 * Fits canvas to selection or drawing with margins from <sodipodi:namedview>
 * "fit-margin-..." attributes.  See SPDocument::fitToRect and
 * ui/dialog/page-sizer.
 */
void fit_canvas_to_selection_or_drawing(SPDesktop *desktop) {
    g_return_if_fail(desktop != nullptr);
    SPDocument *doc = desktop->getDocument();

    g_return_if_fail(doc != nullptr);
    g_return_if_fail(desktop->selection != nullptr);

    bool const changed = ( desktop->selection->isEmpty()
                           ? fit_canvas_to_drawing(doc, true)
                           : desktop->selection->fitCanvas(true,true));
    if (changed) {
        DocumentUndo::done(desktop->getDocument(), SP_VERB_FIT_CANVAS_TO_SELECTION_OR_DRAWING,
                           _("Fit Page to Selection or Drawing"));
    }
};

static void itemtree_map(void (*f)(SPItem *, SPDesktop *), SPObject *root, SPDesktop *desktop) {
    // don't operate on layers
    {
        SPItem *item = dynamic_cast<SPItem *>(root);
        if (item && !desktop->isLayer(item)) {
            f(item, desktop);
        }
    }
    for (auto& child: root->children) {
        //don't recurse into locked layers
        SPItem *item = dynamic_cast<SPItem *>(&child);
        if (!(item && desktop->isLayer(item) && item->isLocked())) {
            itemtree_map(f, &child, desktop);
        }
    }
}

static void unlock(SPItem *item, SPDesktop */*desktop*/) {
    if (item->isLocked()) {
        item->setLocked(FALSE);
    }
}

static void unhide(SPItem *item, SPDesktop *desktop) {
    if (desktop->itemIsHidden(item)) {
        item->setExplicitlyHidden(FALSE);
    }
}

static void process_all(void (*f)(SPItem *, SPDesktop *), SPDesktop *dt, bool layer_only) {
    if (!dt) return;

    SPObject *root;
    if (layer_only) {
        root = dt->currentLayer();
    } else {
        root = dt->currentRoot();
    }

    itemtree_map(f, root, dt);
}

void unlock_all(SPDesktop *dt) {
    process_all(&unlock, dt, true);
}

void unlock_all_in_all_layers(SPDesktop *dt) {
    process_all(&unlock, dt, false);
}

void unhide_all(SPDesktop *dt) {
    process_all(&unhide, dt, true);
}

void unhide_all_in_all_layers(SPDesktop *dt) {
    process_all(&unhide, dt, false);
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

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Here are handlers for modifying selections, specific to paths
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Jasper van de Gronde <th.v.d.gronde@hccnet.nl>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2008 Authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstring>
#include <string>

#include <glibmm/i18n.h>


#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "message-stack.h"
#include "path-chemistry.h"
#include "selection-chemistry.h"
#include "selection.h"
#include "text-editing.h"
#include "verbs.h"

#include "display/curve.h"

#include "object/box3d.h"
#include "object/object-set.h"
#include "object/sp-flowtext.h"
#include "object/sp-path.h"
#include "object/sp-text.h"
#include "style.h"

#include "ui/widget/canvas.h"  // Disable drawing during ops

#include "svg/svg.h"

#include "xml/repr.h"

using Inkscape::DocumentUndo;
using Inkscape::ObjectSet;


inline bool less_than_items(SPItem const *first, SPItem const *second)
{
    return sp_repr_compare_position(first->getRepr(),
                                    second->getRepr())<0;
}

void
ObjectSet::combine(bool skip_undo)
{
    //Inkscape::Selection *selection = desktop->getSelection();
    //Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    SPDocument *doc = document();
    std::vector<SPItem*> items_copy(items().begin(), items().end());
    
    if (items_copy.size() < 1) {
        if(desktop())
            desktop()->getMessageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to combine."));
        return;
    }

    if(desktop()){
        desktop()->messageStack()->flash(Inkscape::IMMEDIATE_MESSAGE, _("Combining paths..."));
        // set "busy" cursor
        desktop()->setWaitingCursor();
    }

    items_copy = sp_degroup_list (items_copy); // descend into any groups in selection

    std::vector<SPItem*> to_paths;
    for (std::vector<SPItem*>::const_reverse_iterator i = items_copy.rbegin(); i != items_copy.rend(); ++i) {
        if (!dynamic_cast<SPPath *>(*i) && !dynamic_cast<SPGroup *>(*i)) {
            to_paths.push_back(*i);
        }
    }
    std::vector<Inkscape::XML::Node*> converted;
    bool did = sp_item_list_to_curves(to_paths, items_copy, converted);
    for (auto i : converted)
        items_copy.push_back((SPItem*)doc->getObjectByRepr(i));

    items_copy = sp_degroup_list (items_copy); // converting to path may have added more groups, descend again

    sort(items_copy.begin(),items_copy.end(),less_than_items);
    assert(!items_copy.empty()); // cannot be NULL because of list length check at top of function

    // remember the position, id, transform and style of the topmost path, they will be assigned to the combined one
    gint position = 0;
    char const *transform = nullptr;
    char const *path_effect = nullptr;

    std::unique_ptr<SPCurve> curve;
    SPItem *first = nullptr;
    Inkscape::XML::Node *parent = nullptr; 

    if (did) {
        clear();
    }

    for (std::vector<SPItem*>::const_reverse_iterator i = items_copy.rbegin(); i != items_copy.rend(); ++i){

        SPItem *item = *i;
        SPPath *path = dynamic_cast<SPPath *>(item);
        if (!path) {
            continue;
        }

        if (!did) {
            clear();
            did = true;
        }

        auto c = SPCurve::copy(path->curveForEdit());
        if (first == nullptr) {  // this is the topmost path
            first = item;
            parent = first->getRepr()->parent();
            position = first->getRepr()->position();
            transform = first->getRepr()->attribute("transform");
            // FIXME: merge styles of combined objects instead of using the first one's style
            path_effect = first->getRepr()->attribute("inkscape:path-effect");
            //c->transform(item->transform);
            curve = std::move(c);
        } else {
            c->transform(item->getRelativeTransform(first));
            curve->append(*c);

            // reduce position only if the same parent
            if (item->getRepr()->parent() == parent) {
                position--;
            }
            // delete the object for real, so that its clones can take appropriate action
            item->deleteObject();
        }
    }


    if (did) {
        Inkscape::XML::Document *xml_doc = doc->getReprDoc();
        Inkscape::XML::Node *repr = xml_doc->createElement("svg:path");

        Inkscape::copy_object_properties(repr, first->getRepr());

        // delete the topmost.
        first->deleteObject(false);

        // restore id, transform, path effect, and style
        if (transform) {
            repr->setAttribute("transform", transform);
        }

        repr->setAttribute("inkscape:path-effect", path_effect);

        // set path data corresponding to new curve
        auto dstring = sp_svg_write_path(curve->get_pathvector());
        if (path_effect) {
            repr->setAttribute("inkscape:original-d", dstring);
        } else {
            repr->setAttribute("d", dstring);
        }

        // add the new group to the parent of the topmost
        // move to the position of the topmost, reduced by the number of deleted items
        parent->addChildAtPos(repr, position > 0 ? position : 0);

        if ( !skip_undo ) {
            DocumentUndo::done(doc, SP_VERB_SELECTION_COMBINE, 
                               _("Combine"));
        }
        set(repr);

        Inkscape::GC::release(repr);

    } else {
        if(desktop())
            desktop()->getMessageStack()->flash(Inkscape::ERROR_MESSAGE, _("<b>No path(s)</b> to combine in the selection."));
    }

    if(desktop())
        desktop()->clearWaitingCursor();
}

void
ObjectSet::breakApart(bool skip_undo)
{
    if (isEmpty()) {
        if(desktop())
            desktop()->getMessageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>path(s)</b> to break apart."));
        return;
    }
    if(desktop()){
        desktop()->messageStack()->flash(Inkscape::IMMEDIATE_MESSAGE, _("Breaking apart paths..."));
        // set "busy" cursor
        desktop()->setWaitingCursor();
        // disable redrawing during the break-apart operation for remarkable speedup for large paths
        desktop()->getCanvas()->set_drawing_disabled(true);
    }

    bool did = false;

    std::vector<SPItem*> itemlist(items().begin(), items().end());
    for (auto item : itemlist){

        SPPath *path = dynamic_cast<SPPath *>(item);
        if (!path) {
            continue;
        }

        auto curve = SPCurve::copy(path->curveForEdit());
        if (curve == nullptr) {
            continue;
        }
        did = true;

        Inkscape::XML::Node *parent = item->getRepr()->parent();
        gint pos = item->getRepr()->position();
        char const *id = item->getRepr()->attribute("id");

        // XML Tree being used directly here while it shouldn't be...
        gchar *style = g_strdup(item->getRepr()->attribute("style"));
        // XML Tree being used directly here while it shouldn't be...
        gchar *path_effect = g_strdup(item->getRepr()->attribute("inkscape:path-effect"));
        Geom::Affine transform = path->transform;
        // it's going to resurrect as one of the pieces, so we delete without advertisement
        item->deleteObject(false);


        auto list = curve->split();

        std::vector<Inkscape::XML::Node*> reprs;
        for (auto const &curve : list) {

            Inkscape::XML::Node *repr = parent->document()->createElement("svg:path");
            repr->setAttribute("style", style);

            repr->setAttribute("inkscape:path-effect", path_effect);

            auto str = sp_svg_write_path(curve->get_pathvector());
            if (path_effect)
                repr->setAttribute("inkscape:original-d", str);
            else
                repr->setAttribute("d", str);
            repr->setAttributeOrRemoveIfEmpty("transform", sp_svg_transform_write(transform));
            
            // add the new repr to the parent
            // move to the saved position
            parent->addChildAtPos(repr, pos);

            // if it's the first one, restore id
            if (curve == list.front())
                repr->setAttribute("id", id);

            reprs.push_back(repr);

            Inkscape::GC::release(repr);
        }
        setReprList(reprs);

        g_free(style);
        g_free(path_effect);
    }

    if (desktop()) {
        desktop()->getCanvas()->set_drawing_disabled(false);
        desktop()->clearWaitingCursor();
    }

    if (did) {
        if ( !skip_undo ) {
            DocumentUndo::done(document(), SP_VERB_SELECTION_BREAK_APART, 
                               _("Break apart"));
        }
    } else {
        if(desktop())
            desktop()->getMessageStack()->flash(Inkscape::ERROR_MESSAGE, _("<b>No path(s)</b> to break apart in the selection."));
    }
}

void ObjectSet::toCurves(bool skip_undo)
{
    if (isEmpty()) {
        if (desktop())
            desktop()->getMessageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to convert to path."));
        return;
    }
    
    bool did = false;
    if (desktop()) {
        desktop()->messageStack()->flash(Inkscape::IMMEDIATE_MESSAGE, _("Converting objects to paths..."));
        // set "busy" cursor
        desktop()->setWaitingCursor();
    }
    unlinkRecursive(true);
    std::vector<SPItem*> selected(items().begin(), items().end());
    std::vector<Inkscape::XML::Node*> to_select;
    std::vector<SPItem*> items(selected);

    did = sp_item_list_to_curves(items, selected, to_select);

    if (did) {
        setReprList(to_select);
        addList(selected);
    }

    if (desktop()) {
        desktop()->clearWaitingCursor();
    }
    if (did&& !skip_undo) {
        DocumentUndo::done(document(), SP_VERB_OBJECT_TO_CURVE, 
                               _("Object to path"));
    } else {
        if(desktop())
            desktop()->getMessageStack()->flash(Inkscape::ERROR_MESSAGE, _("<b>No objects</b> to convert to path in the selection."));
        return;
    }
}

/** Converts the selected items to LPEItems if they are not already so; e.g. SPRects) */
void ObjectSet::toLPEItems()
{

    if (isEmpty()) {
        return;
    }
    unlinkRecursive(true);
    std::vector<SPItem*> selected(items().begin(), items().end());
    std::vector<Inkscape::XML::Node*> to_select;
    clear();
    std::vector<SPItem*> items(selected);


    sp_item_list_to_curves(items, selected, to_select, true);

    setReprList(to_select);
    addList(selected);
}

bool
sp_item_list_to_curves(const std::vector<SPItem*> &items, std::vector<SPItem*>& selected, std::vector<Inkscape::XML::Node*> &to_select, bool skip_all_lpeitems)
{
    bool did = false;
    for (auto item : items){
        g_assert(item != nullptr);
        SPDocument *document = item->document;

        SPGroup *group = dynamic_cast<SPGroup *>(item);
        if ( skip_all_lpeitems &&
             dynamic_cast<SPLPEItem *>(item) && 
             !group ) // also convert objects in an SPGroup when skip_all_lpeitems is set.
        { 
            continue;
        }

        SPBox3D *box = dynamic_cast<SPBox3D *>(item);
        if (box) {
            // convert 3D box to ordinary group of paths; replace the old element in 'selected' with the new group
            Inkscape::XML::Node *repr = box->convert_to_group()->getRepr();
            
            if (repr) {
                to_select.insert(to_select.begin(),repr);
                did = true;
                selected.erase(remove(selected.begin(), selected.end(), item), selected.end());
            }

            continue;
        }
        // remember id
        char const *id = item->getRepr()->attribute("id");
        
        SPLPEItem *lpeitem = dynamic_cast<SPLPEItem *>(item);
        if (lpeitem) {
            lpeitem->removeAllPathEffects(true);
            SPObject *elemref = document->getObjectById(id);
            if (elemref != item) {
                selected.erase(remove(selected.begin(), selected.end(), item), selected.end());
                if (elemref) {
                    //If the LPE item is a shape is converted to a path so we need to reupdate the item
                    item = dynamic_cast<SPItem *>(elemref);
                    selected.push_back(item);
                    did = true;
                }
            }
        }
        
        SPPath *path = dynamic_cast<SPPath *>(item);
        if (path) {
            // remove connector attributes
            if (item->getAttribute("inkscape:connector-type") != nullptr) {
                item->removeAttribute("inkscape:connection-start");
                item->removeAttribute("inkscape:connection-start-point");
                item->removeAttribute("inkscape:connection-end");
                item->removeAttribute("inkscape:connection-end-point");
                item->removeAttribute("inkscape:connector-type");
                item->removeAttribute("inkscape:connector-curvature");
                did = true;
            }
            continue; // already a path, and no path effect
        }

        if (group) {
            std::vector<SPItem*> item_list = sp_item_group_item_list(group);
            
            std::vector<Inkscape::XML::Node*> item_to_select;
            std::vector<SPItem*> item_selected;
            
            if (sp_item_list_to_curves(item_list, item_selected, item_to_select))
                did = true;


            continue;
        }

        Inkscape::XML::Node *repr = sp_selected_item_to_curved_repr(item, 0);
        if (!repr)
            continue;

        did = true;
        selected.erase(remove(selected.begin(), selected.end(), item), selected.end());

        // remember the position of the item
        gint pos = item->getRepr()->position();
        // remember parent
        Inkscape::XML::Node *parent = item->getRepr()->parent();
        // remember class
        char const *class_attr = item->getRepr()->attribute("class");

        // It's going to resurrect, so we delete without notifying listeners.
        item->deleteObject(false);

        // restore id
        repr->setAttribute("id", id);
        // restore class
        repr->setAttribute("class", class_attr);
        // add the new repr to the parent
        parent->addChildAtPos(repr, pos);

        /* Buglet: We don't re-add the (new version of the) object to the selection of any other
         * desktops where it was previously selected. */
        to_select.insert(to_select.begin(),repr);
        Inkscape::GC::release(repr);
    }
    
    return did;
}

Inkscape::XML::Node *
sp_selected_item_to_curved_repr(SPItem *item, guint32 /*text_grouping_policy*/)
{
    if (!item)
        return nullptr;

    Inkscape::XML::Document *xml_doc = item->getRepr()->document();

    if (dynamic_cast<SPText *>(item) || dynamic_cast<SPFlowtext *>(item)) {
        // Special treatment for text: convert each glyph to separate path, then group the paths
        Inkscape::XML::Node *g_repr = xml_doc->createElement("svg:g");

        // Save original text for accessibility.
        Glib::ustring original_text = sp_te_get_string_multiline( item,
                                                                  te_get_layout(item)->begin(),
                                                                  te_get_layout(item)->end() );
        if( original_text.size() > 0 ) {
            g_repr->setAttributeOrRemoveIfEmpty("aria-label", original_text );
        }

        g_repr->setAttribute("transform", item->getRepr()->attribute("transform"));

        Inkscape::copy_object_properties(g_repr, item->getRepr());

        /* Whole text's style */
        Glib::ustring style_str =
            item->style->writeIfDiff(item->parent ? item->parent->style : nullptr); // TODO investigate possibility
        g_repr->setAttributeOrRemoveIfEmpty("style", style_str);

        Inkscape::Text::Layout::iterator iter = te_get_layout(item)->begin();
        do {
            Inkscape::Text::Layout::iterator iter_next = iter;
            iter_next.nextGlyph(); // iter_next is one glyph ahead from iter
            if (iter == iter_next)
                break;

            /* This glyph's style */
            SPObject *pos_obj = nullptr;
            te_get_layout(item)->getSourceOfCharacter(iter, &pos_obj);
            if (!pos_obj) // no source for glyph, abort
                break;
            while (dynamic_cast<SPString const *>(pos_obj) && pos_obj->parent) {
               pos_obj = pos_obj->parent;   // SPStrings don't have style
            }
            Glib::ustring style_str = pos_obj->style->writeIfDiff(
                pos_obj->parent ? pos_obj->parent->style : nullptr); // TODO investigate possibility

            // get path from iter to iter_next:
            auto curve = te_get_layout(item)->convertToCurves(iter, iter_next);
            iter = iter_next; // shift to next glyph
            if (!curve) { // error converting this glyph
                continue;
            }
            if (curve->is_empty()) { // whitespace glyph?
                continue;
            }

            Inkscape::XML::Node *p_repr = xml_doc->createElement("svg:path");

            p_repr->setAttribute("d", sp_svg_write_path(curve->get_pathvector()));

            p_repr->setAttributeOrRemoveIfEmpty("style", style_str);

            g_repr->appendChild(p_repr);

            Inkscape::GC::release(p_repr);

            if (iter == te_get_layout(item)->end())
                break;

        } while (true);

        return g_repr;
    }

    std::unique_ptr<SPCurve> curve;

    {
        SPShape *shape = dynamic_cast<SPShape *>(item);
        if (shape) {
            curve = SPCurve::copy(shape->curveForEdit());
        }
    }

    if (!curve)
        return nullptr;

    // Prevent empty paths from being added to the document
    // otherwise we end up with zomby markup in the SVG file
    if(curve->is_empty())
    {
        return nullptr;
    }

    Inkscape::XML::Node *repr = xml_doc->createElement("svg:path");

    Inkscape::copy_object_properties(repr, item->getRepr());

    /* Transformation */
    repr->setAttribute("transform", item->getRepr()->attribute("transform"));

    /* Style */
    Glib::ustring style_str =
        item->style->writeIfDiff(item->parent ? item->parent->style : nullptr); // TODO investigate possibility
    repr->setAttributeOrRemoveIfEmpty("style", style_str);

    /* Definition */
    repr->setAttribute("d", sp_svg_write_path(curve->get_pathvector()));
    return repr;
}


void
ObjectSet::pathReverse()
{
    if (isEmpty()) {
        if(desktop())
            desktop()->getMessageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>path(s)</b> to reverse."));
        return;
    }


    // set "busy" cursor
    if(desktop()){
        desktop()->setWaitingCursor();
        desktop()->messageStack()->flash(Inkscape::IMMEDIATE_MESSAGE, _("Reversing paths..."));
    }
    
    bool did = false;

    for (auto i = items().begin(); i != items().end(); ++i){

        SPPath *path = dynamic_cast<SPPath *>(*i);
        if (!path) {
            continue;
        }

        did = true;

        auto rcurve = path->curveForEdit()->create_reverse();

        auto str = sp_svg_write_path(rcurve->get_pathvector());
        if ( path->hasPathEffectRecursive() ) {
            path->setAttribute("inkscape:original-d", str);
        } else {
            path->setAttribute("d", str);
        }

        // reverse nodetypes order (Bug #179866)
        gchar *nodetypes = g_strdup(path->getRepr()->attribute("sodipodi:nodetypes"));
        if ( nodetypes ) {
            path->setAttribute("sodipodi:nodetypes", g_strreverse(nodetypes));
            g_free(nodetypes);
        }
    }
    if(desktop())
        desktop()->clearWaitingCursor();

    if (did) {
        DocumentUndo::done(document(), SP_VERB_SELECTION_REVERSE,
                           _("Reverse path"));
    } else {
        if(desktop())
            desktop()->getMessageStack()->flash(Inkscape::ERROR_MESSAGE, _("<b>No paths</b> to reverse in the selection."));
    }
}


/**
 * Copy generic attributes, like those from the "Object Properties" dialog,
 * but also style and transformation center.
 *
 * @param dest XML node to copy attributes to
 * @param src XML node to copy attributes from
 */
static void ink_copy_generic_attributes( //
    Inkscape::XML::Node *dest,           //
    Inkscape::XML::Node const *src)
{
    static char const *const keys[] = {
        // core
        "id",

        // clip & mask
        "clip-path",
        "mask",

        // style
        "style",

        // inkscape
        "inkscape:highlight-color",
        "inkscape:label",
        "inkscape:transform-center-x",
        "inkscape:transform-center-y",

        // interactivity
        "onclick",
        "onmouseover",
        "onmouseout",
        "onmousedown",
        "onmouseup",
        "onmousemove",
        "onfocusin",
        "onfocusout",
        "onload",
    };

    for (auto *key : keys) {
        auto *value = src->attribute(key);
        if (value) {
            dest->setAttribute(key, value);
        }
    }
}


/**
 * Copy generic child elements, like those from the "Object Properties" dialog
 * (title and description) but also XML comments.
 *
 * Does not check if children of the same type already exist in dest.
 *
 * @param dest XML node to copy children to
 * @param src XML node to copy children from
 */
static void ink_copy_generic_children( //
    Inkscape::XML::Node *dest,         //
    Inkscape::XML::Node const *src)
{
    static std::set<std::string> const names{
        // descriptive elements
        "svg:title",
        "svg:desc",
    };

    for (const auto *child = src->firstChild(); child != nullptr; child = child->next()) {
        // check if this child should be copied
        if (!(child->type() == Inkscape::XML::NodeType::COMMENT_NODE || //
              (child->name() && names.count(child->name())))) {
            continue;
        }

        auto dchild = child->duplicate(dest->document());
        dest->appendChild(dchild);
        dchild->release();
    }
}


/**
 * Copy generic object properties, like:
 * - id
 * - label
 * - title
 * - description
 * - style
 * - clip
 * - mask
 * - transformation center
 * - highlight color
 * - interactivity (event attributes)
 *
 * @param dest XML node to copy to
 * @param src XML node to copy from
 */
void Inkscape::copy_object_properties( //
    Inkscape::XML::Node *dest,         //
    Inkscape::XML::Node const *src)
{
    ink_copy_generic_attributes(dest, src);
    ink_copy_generic_children(dest, src);
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

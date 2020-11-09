// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_TEXT_CHEMISTRY_IMPL_H
#define SEEN_TEXT_CHEMISTRY_IMPL_H
/*
 * Text commands template implementation
 *
 * Authors:
 *   Iskren Chernev <iskren.chernev@gmail.com>
 *
 * Copyright (C) 2004 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <algorithm>
#include "style.h"
#include "xml/node.h"
#include "document.h"

Glib::ustring text_relink_shapes_str(gchar const *prop,
        std::map<Glib::ustring, Glib::ustring> const &old_to_new);

inline Inkscape::XML::Node *text_obj_or_node_to_node(SPObject *obj) {
    return obj->getRepr();
}

inline Inkscape::XML::Node *text_obj_or_node_to_node(Inkscape::XML::Node *node) {
    return node;
}

template<typename InIter>
text_refs_t text_categorize_refs(SPDocument *doc, InIter begin, InIter end, text_ref_t which) {
    text_refs_t res;
    std::set<Glib::ustring> int_ext;
    auto idVisitor = [which, &res, &int_ext](SPShapeReference *href) {
        auto ref_obj = href->getObject();
        if (!ref_obj)
            return;
        auto id = ref_obj->getId();
        if (sp_repr_is_def(ref_obj->getRepr())) {
            if (which & TEXT_REF_DEF) {
                res.emplace_back(id, TEXT_REF_DEF);
            }
        } else {
            int_ext.insert(id);
        }
    };
    // Visit all shape references, detect the refs, and put internal and
    // external ids in int_ext.
    for (auto it = begin; it != end; ++it) {
        sp_repr_visit_descendants(
                text_obj_or_node_to_node(*it),
                [doc, &idVisitor](Inkscape::XML::Node *crnt) {
            if (!(crnt->name() && strcmp("svg:text", crnt->name()) == 0)) {
                return true;
            }

            auto crnt_obj = doc->getObjectByRepr(crnt);
            assert(crnt_obj == doc->getObjectById(crnt->attribute("id")));
            {
                const auto &inside_ids = crnt_obj->style->shape_inside.hrefs;
                std::for_each(inside_ids.begin(), inside_ids.end(), idVisitor);

                const auto &subtract_ids = crnt_obj->style->shape_subtract.hrefs;
                std::for_each(subtract_ids.begin(), subtract_ids.end(), idVisitor);
            }
            // Do not recurse into svg:text elements children
            return false;
        });
    }

    if (!(which & (TEXT_REF_INTERNAL | TEXT_REF_EXTERNAL))) {
        // We already discovered the defs, bail out if nothing else is
        // required.
        return res;
    }
    // Visit all root elements, recursively and see which ones are in int_ext,
    // therefore discovering the internal ids.
    for (auto it = begin; it != end; ++it) {
        sp_repr_visit_descendants(
                text_obj_or_node_to_node(*it),
                [which, &res, &int_ext](Inkscape::XML::Node *crnt) {

            auto id = crnt->attribute("id");
            auto find_iter = id ? int_ext.find(id) : int_ext.end();
            if (find_iter != int_ext.end()) {
                if (which & TEXT_REF_INTERNAL) {
                    res.emplace_back(id, TEXT_REF_INTERNAL);
                }
                int_ext.erase(find_iter);
                // don't recurse into children of a matched element
                return false;
            }

            return true;
        });
    }

    // What is left in int_ext are the external ones
    if (which & TEXT_REF_EXTERNAL) {
        for (auto const &ext_id : int_ext) {
            res.emplace_back(ext_id, TEXT_REF_EXTERNAL);
        }
    }

    return res;
}

template<typename InIterOrig, typename InIterCopy>
void text_relink_refs(text_refs_t const &refs, InIterOrig origBegin, InIterOrig origEnd, InIterCopy copyBegin) {
    // get all ids of text refs
    std::set<Glib::ustring> all_refs;
    for (auto const &ref : refs) {
        all_refs.insert(ref.first);
    }

    // find a mapping from old ids to new ids
    std::map<Glib::ustring, Glib::ustring> old_to_new;
    for (auto itOrig = origBegin, itCopy = copyBegin; itOrig != origEnd; ++itOrig, ++itCopy) {
        sp_repr_visit_descendants(
                text_obj_or_node_to_node(*itOrig),
                text_obj_or_node_to_node(*itCopy),
                [&all_refs, &old_to_new](Inkscape::XML::Node *a, Inkscape::XML::Node *b) {
            if (a->attribute("id") && all_refs.find(a->attribute("id")) != all_refs.end()) {
                old_to_new[a->attribute("id")] = b->attribute("id");
                return false;
            }
            return true;
        });
    }

    if (all_refs.size() != old_to_new.size()) {
        std::cerr << "text_relink_refs: Failed to match all references! all:" << all_refs.size()
            << " matched:" << old_to_new.size() << std::endl;
    }
    // relink references
    for (auto itOrig = origBegin, itCopy = copyBegin; itOrig != origEnd; ++itOrig, ++itCopy) {
        sp_repr_visit_descendants(
                text_obj_or_node_to_node(*itCopy),
                [&old_to_new](Inkscape::XML::Node *crnt) {
            if (strcmp("svg:text", crnt->name()) == 0) {
                SPCSSAttr* css = sp_repr_css_attr (crnt, "style");
                for (auto &&prop : {"shape-inside", "shape-subtract"}) {
                    if (auto prop_str = sp_repr_css_property(css, prop, nullptr)) {
                        sp_repr_css_set_property(css, prop, text_relink_shapes_str(prop_str, old_to_new).c_str());
                    }
                }
                sp_repr_css_set (crnt, css, "style");
                return false;
            }
            return true;
        });
    }
}

#endif // SEEN_TEXT_CHEMISTRY_IMPL_H

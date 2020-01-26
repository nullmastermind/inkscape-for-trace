// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Routines for resolving ID clashes when importing or pasting.
 *
 * Authors:
 *   Stephen Silver <sasilver@users.sourceforge.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2008 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstdlib>
#include <cstring>
#include <list>
#include <map>
#include <string>
#include <utility>

#include "extract-uri.h"
#include "id-clash.h"

#include "live_effects/lpeobject.h"
#include "object/sp-gradient.h"
#include "object/sp-object.h"
#include "object/sp-paint-server.h"
#include "object/sp-root.h"
#include "style.h"

enum ID_REF_TYPE { REF_HREF, REF_STYLE, REF_SHAPES, REF_URL, REF_CLIPBOARD };

struct IdReference {
    ID_REF_TYPE type;
    SPObject *elem;
    const char *attr;  // property or href-like attribute
};

typedef std::map<Glib::ustring, std::list<IdReference> > refmap_type;

typedef std::pair<SPObject*, Glib::ustring> id_changeitem_type;
typedef std::list<id_changeitem_type> id_changelist_type;

const char *href_like_attributes[] = {
    "inkscape:connection-end",
    "inkscape:connection-start",
    "inkscape:href",
    "inkscape:path-effect",
    "inkscape:perspectiveID",
    "inkscape:tiled-clone-of",
    "xlink:href",
};
#define NUM_HREF_LIKE_ATTRIBUTES (sizeof(href_like_attributes) / sizeof(*href_like_attributes))

const SPIPaint SPStyle::* SPIPaint_members[] = {
    //&SPStyle::color,
    reinterpret_cast<SPIPaint SPStyle::*>(&SPStyle::fill),
    reinterpret_cast<SPIPaint SPStyle::*>(&SPStyle::stroke),
};
const char* SPIPaint_properties[] = {
    //"color",
    "fill",
    "stroke",
};
#define NUM_SPIPAINT_PROPERTIES (sizeof(SPIPaint_properties) / sizeof(*SPIPaint_properties))

const SPIShapes SPStyle::* SPIShapes_members[] = {
    reinterpret_cast<SPIShapes SPStyle::*>(&SPStyle::shape_inside),
    reinterpret_cast<SPIShapes SPStyle::*>(&SPStyle::shape_subtract),
};
const char *SPIShapes_properties[] = {
    "shape-inside",
    "shape-subtract",
};
#define NUM_SPISHAPES_PROPERTIES (sizeof(SPIShapes_properties) / sizeof(*SPIShapes_properties))

const char* other_url_properties[] = {
    "clip-path",
    "color-profile",
    "cursor",
    "marker-end",
    "marker-mid",
    "marker-start",
    "mask",
};
#define NUM_OTHER_URL_PROPERTIES (sizeof(other_url_properties) / sizeof(*other_url_properties))

const char* clipboard_properties[] = {
    //"color",
    "fill",
    "filter",
    "stroke",
    "marker-end",
    "marker-mid",
    "marker-start"
};
#define NUM_CLIPBOARD_PROPERTIES (sizeof(clipboard_properties) / sizeof(*clipboard_properties))

/**
 * Given an reference (idref), make it point to to_obj instead
 */
static void
fix_ref(IdReference const &idref, SPObject *to_obj, const char *old_id) {
    switch (idref.type) {
        case REF_HREF: {
            gchar *new_uri = g_strdup_printf("#%s", to_obj->getId());
            idref.elem->setAttribute(idref.attr, new_uri);
            g_free(new_uri);
            break;
        }
        case REF_STYLE: {
            sp_style_set_property_url(idref.elem, idref.attr, to_obj, false);
            break;
        }
        case REF_SHAPES: {
            SPCSSAttr* css = sp_repr_css_attr (idref.elem->getRepr(), "style");
            std::string prop = sp_repr_css_property (css, idref.attr, nullptr);
            std::string oid; oid.append("url(#").append(old_id).append(")");
            auto pos = prop.find(oid);
            if (pos != std::string::npos) {
                std::string nid; nid.append("url(#").append(to_obj->getId()).append(")");
                prop.replace(pos, oid.size(), nid);
                sp_repr_css_set_property (css, idref.attr, prop.c_str());
                sp_repr_css_set (idref.elem->getRepr(), css, "style");
            } else {
                std::cerr << "Failed to switch id -- shouldn't happen" << std::endl;
            }
            break;
        }
        case REF_URL: {
            gchar *url = g_strdup_printf("url(#%s)", to_obj->getId());
            idref.elem->setAttribute(idref.attr, url);
            g_free(url);
            break;
        }
        case REF_CLIPBOARD: {
            SPCSSAttr *style = sp_repr_css_attr(idref.elem->getRepr(), "style");
            gchar *url = g_strdup_printf("url(#%s)", to_obj->getId());
            sp_repr_css_set_property(style, idref.attr, url);
            g_free(url);
            Glib::ustring style_string;
            sp_repr_css_write_string(style, style_string);
            idref.elem->setAttributeOrRemoveIfEmpty("style", style_string);
            break;
        }
    }
}

/**
 *  Build a table of places where IDs are referenced, for a given element.
 *  FIXME: There are some types of references not yet dealt with here
 *         (e.g., ID selectors in CSS stylesheets, and references in scripts).
 */
static void
find_references(SPObject *elem, refmap_type &refmap)
{
    if (elem->cloned) return;
    Inkscape::XML::Node *repr_elem = elem->getRepr();
    if (!repr_elem) return;
    if (repr_elem->type() != Inkscape::XML::ELEMENT_NODE) return;

    /* check for references in inkscape:clipboard elements */
    if (!std::strcmp(repr_elem->name(), "inkscape:clipboard")) {
        SPCSSAttr *css = sp_repr_css_attr(repr_elem, "style");
        if (css) {
            for (auto attr : clipboard_properties) {
                const gchar *value = sp_repr_css_property(css, attr, nullptr);
                if (value) {
                    auto uri = extract_uri(value);
                    if (uri[0] == '#') {
                        IdReference idref = { REF_CLIPBOARD, elem, attr };
                        refmap[uri.c_str() + 1].push_back(idref);
                    }
                }
            }

        }
        return; // nothing more to do for inkscape:clipboard elements
    }

    /* check for xlink:href="#..." and similar */
    for (auto attr : href_like_attributes) {
        const gchar *val = repr_elem->attribute(attr);
        if (val && val[0] == '#') {
            std::string id(val+1);
            IdReference idref = { REF_HREF, elem, attr };
            refmap[id].push_back(idref);
        }
    }

    SPStyle *style = elem->style;

    /* check for url(#...) references in 'fill' or 'stroke' */
    for (unsigned i = 0; i < NUM_SPIPAINT_PROPERTIES; ++i) {
        const SPIPaint SPStyle::*prop = SPIPaint_members[i];
        const SPIPaint *paint = &(style->*prop);
        if (paint->isPaintserver() && paint->value.href) {
            const SPObject *obj = paint->value.href->getObject();
            if (obj) {
                const gchar *id = obj->getId();
                IdReference idref = { REF_STYLE, elem, SPIPaint_properties[i] };
                refmap[id].push_back(idref);
            }
        }
    }

    /* check for shape-inside/shape-subtract that contain multiple url(#..) each */
    for (unsigned i = 0; i < NUM_SPISHAPES_PROPERTIES; ++i) {
        const SPIShapes SPStyle::*prop = SPIShapes_members[i];
        const SPIShapes *shapes = &(style->*prop);
        for (const auto &shape_id : shapes->shape_ids) {
            IdReference idref = { REF_SHAPES, elem, SPIShapes_properties[i] };
            refmap[shape_id].push_back(idref);
        }
    }

    /* check for url(#...) references in 'filter' */
    const SPIFilter *filter = &(style->filter);
    if (filter->href) {
        const SPObject *obj = filter->href->getObject();
        if (obj) {
            const gchar *id = obj->getId();
            IdReference idref = { REF_STYLE, elem, "filter" };
            refmap[id].push_back(idref);
        }
    }

    /* check for url(#...) references in markers */
    const gchar *markers[4] = { "", "marker-start", "marker-mid", "marker-end" };
    for (unsigned i = SP_MARKER_LOC_START; i < SP_MARKER_LOC_QTY; i++) {
        const gchar *value = style->marker_ptrs[i]->value();
        if (value) {
            auto uri = extract_uri(value);
            if (uri[0] == '#') {
                IdReference idref = { REF_STYLE, elem, markers[i] };
                refmap[uri.c_str() + 1].push_back(idref);
            }
        }
    }

    /* check for other url(#...) references */
    for (auto attr : other_url_properties) {
        const gchar *value = repr_elem->attribute(attr);
        if (value) {
            auto uri = extract_uri(value);
            if (uri[0] == '#') {
                IdReference idref = { REF_URL, elem, attr };
                refmap[uri.c_str() + 1].push_back(idref);
            }
        }
    }

    // recurse
    for (auto& child: elem->children)
    {
        find_references(&child, refmap);
    }
}

/**
 *  Change any IDs that clash with IDs in the current document, and make
 *  a list of those changes that will require fixing up references.
 */
static void
change_clashing_ids(SPDocument *imported_doc, SPDocument *current_doc,
                    SPObject *elem, refmap_type const &refmap,
                    id_changelist_type *id_changes)
{
    const gchar *id = elem->getId();
    bool fix_clashing_ids = true;

    if (id && current_doc->getObjectById(id)) {
        // Choose a new ID.
        // To try to preserve any meaningfulness that the original ID
        // may have had, the new ID is the old ID followed by a hyphen
        // and one or more digits.

        if (SP_IS_GRADIENT(elem)) {
            SPObject *cd_obj =  current_doc->getObjectById(id);

            if (cd_obj && SP_IS_GRADIENT(cd_obj)) {
                SPGradient *cd_gr = SP_GRADIENT(cd_obj);
                if ( cd_gr->isEquivalent(SP_GRADIENT(elem))) {
                    fix_clashing_ids = false;
                 }
             }
        }

        LivePathEffectObject *lpeobj = dynamic_cast<LivePathEffectObject *>(elem);
        if (lpeobj) {
            SPObject *cd_obj = current_doc->getObjectById(id);
            LivePathEffectObject *cd_lpeobj = dynamic_cast<LivePathEffectObject *>(cd_obj);
            if (cd_lpeobj && lpeobj->is_similar(cd_lpeobj)) {
                fix_clashing_ids = false;
            }
        }

        if (fix_clashing_ids) {
            std::string old_id(id);
            std::string new_id(old_id + '-');
            for (;;) {
                new_id += "0123456789"[std::rand() % 10];
                const char *str = new_id.c_str();
                if (current_doc->getObjectById(str) == nullptr &&
                    imported_doc->getObjectById(str) == nullptr) break;
            }
            // Change to the new ID

            elem->setAttribute("id", new_id);
                // Make a note of this change, if we need to fix up refs to it
            if (refmap.find(old_id) != refmap.end())
                id_changes->push_back(id_changeitem_type(elem, old_id));
        }
    }


    // recurse
    for (auto& child: elem->children)
    {
        change_clashing_ids(imported_doc, current_doc, &child, refmap, id_changes);
    }
}

/**
 *  Fix up references to changed IDs.
 */
static void
fix_up_refs(refmap_type const &refmap, const id_changelist_type &id_changes)
{
    id_changelist_type::const_iterator pp;
    const id_changelist_type::const_iterator pp_end = id_changes.end();
    for (pp = id_changes.begin(); pp != pp_end; ++pp) {
        SPObject *obj = pp->first;
        refmap_type::const_iterator pos = refmap.find(pp->second);
        std::list<IdReference>::const_iterator it;
        const std::list<IdReference>::const_iterator it_end = pos->second.end();
        for (it = pos->second.begin(); it != it_end; ++it) {
            fix_ref(*it, obj, pp->second.c_str());
        }
    }
}

/**
 *  This function resolves ID clashes between the document being imported
 *  and the current open document: IDs in the imported document that would
 *  clash with IDs in the existing document are changed, and references to
 *  those IDs are updated accordingly.
 */
void
prevent_id_clashes(SPDocument *imported_doc, SPDocument *current_doc)
{
    refmap_type refmap;
    id_changelist_type id_changes;
    SPObject *imported_root = imported_doc->getRoot();

    find_references(imported_root, refmap);
    change_clashing_ids(imported_doc, current_doc, imported_root, refmap,
                        &id_changes);
    fix_up_refs(refmap, id_changes);
}

/*
 * Change any references of svg:def from_obj into to_obj
 */
void
change_def_references(SPObject *from_obj, SPObject *to_obj)
{
    refmap_type refmap;
    SPDocument *current_doc = from_obj->document;
    std::string old_id(from_obj->getId());

    find_references(current_doc->getRoot(), refmap);

    refmap_type::const_iterator pos = refmap.find(old_id);
    if (pos != refmap.end()) {
        std::list<IdReference>::const_iterator it;
        const std::list<IdReference>::const_iterator it_end = pos->second.end();
        for (it = pos->second.begin(); it != it_end; ++it) {
            fix_ref(*it, to_obj, from_obj->getId());
        }
    }
}

/*
 * Change the id of a SPObject to new_name
 * If there is an id clash then rename to something similar
 */
void rename_id(SPObject *elem, Glib::ustring const &new_name)
{
    if (new_name.empty()){
        g_message("Invalid Id, will not change.");
        return;
    }
    gchar *id = g_strdup(new_name.c_str()); //id is not empty here as new_name is check to be not empty
    g_strcanon (id, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.:", '_');
    Glib::ustring new_name2 = id; //will not fail as id can not be NULL, see length check on new_name
    if (!isalnum (new_name2[0])) {
        g_message("Invalid Id, will not change.");
        g_free (id);
        return;
    }

    SPDocument *current_doc = elem->document;
    refmap_type refmap;
    find_references(current_doc->getRoot(), refmap);

    std::string old_id(elem->getId());
    if (current_doc->getObjectById(id)) {
        // Choose a new ID.
        // To try to preserve any meaningfulness that the original ID
        // may have had, the new ID is the old ID followed by a hyphen
        // and one or more digits.
        new_name2 += '-';
        for (;;) {
            new_name2 += "0123456789"[std::rand() % 10];
            if (current_doc->getObjectById(new_name2) == nullptr)
                break;
        }
    }
    g_free (id);
    // Change to the new ID
    elem->setAttribute("id", new_name2);
    // Make a note of this change, if we need to fix up refs to it
    id_changelist_type id_changes;
    if (refmap.find(old_id) != refmap.end()) {
        id_changes.push_back(id_changeitem_type(elem, old_id));
    }

    fix_up_refs(refmap, id_changes);
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

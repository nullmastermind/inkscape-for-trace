// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Stock-items
 *
 * Stock Item management code
 *
 * Authors:
 *  John Cliff <simarilius@yahoo.com>
 *  Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright 2004 John Cliff
 *
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#define noSP_SS_VERBOSE

#include <cstring>
#include <glibmm/fileutils.h>

#include "path-prefix.h"

#include <xml/repr.h>
#include "inkscape.h"

#include "io/sys.h"
#include "io/resource.h"
#include "stock-items.h"

#include "object/sp-gradient.h"
#include "object/sp-pattern.h"
#include "object/sp-marker.h"
#include "object/sp-defs.h"

static SPDocument *load_paint_doc(char const *basename,
                                  Inkscape::IO::Resource::Type type = Inkscape::IO::Resource::PAINT)
{
    using namespace Inkscape::IO::Resource;

    for (Domain const domain : {SYSTEM, CREATE}) {
        auto const filename = get_path_string(domain, type, basename);
        if (Glib::file_test(filename, Glib::FILE_TEST_IS_REGULAR)) {
            auto doc = SPDocument::createNewDoc(filename.c_str(), false);
            if (doc) {
                doc->ensureUpToDate();
                return doc;
            }
        }
    }

    return nullptr;
}

// FIXME: these should be merged with the icon loading code so they
// can share a common file/doc cache.  This function should just
// take the dir to look in, and the file to check for, and cache
// against that, rather than the existing copy/paste code seen here.

static SPObject * sp_marker_load_from_svg(gchar const *name, SPDocument *current_doc)
{
    if (!current_doc) {
        return nullptr;
    }
    /* Try to load from document */
    static SPDocument *doc = load_paint_doc("markers.svg", Inkscape::IO::Resource::MARKERS);

    if (doc) {
        /* Get the marker we want */
        SPObject *object = doc->getObjectById(name);
        if (object && SP_IS_MARKER(object)) {
            SPDefs *defs = current_doc->getDefs();
            Inkscape::XML::Document *xml_doc = current_doc->getReprDoc();
            Inkscape::XML::Node *mark_repr = object->getRepr()->duplicate(xml_doc);
            defs->getRepr()->addChild(mark_repr, nullptr);
            SPObject *cloned_item = current_doc->getObjectByRepr(mark_repr);
            Inkscape::GC::release(mark_repr);
            return cloned_item;
        }
    }
    return nullptr;
}


static SPObject *
sp_pattern_load_from_svg(gchar const *name, SPDocument *current_doc)
{
    if (!current_doc) {
        return nullptr;
    }
    /* Try to load from document */
    static SPDocument *doc = load_paint_doc("patterns.svg");

    if (doc) {
        /* Get the pattern we want */
        SPObject *object = doc->getObjectById(name);
        if (object && SP_IS_PATTERN(object)) {
            SPDefs *defs = current_doc->getDefs();
            Inkscape::XML::Document *xml_doc = current_doc->getReprDoc();
            Inkscape::XML::Node *pat_repr = object->getRepr()->duplicate(xml_doc);
            defs->getRepr()->addChild(pat_repr, nullptr);
            Inkscape::GC::release(pat_repr);
            return object;
        }
    }
    return nullptr;
}


static SPObject *
sp_gradient_load_from_svg(gchar const *name, SPDocument *current_doc)
{
    if (!current_doc) {
        return nullptr;
    }
    /* Try to load from document */
    static SPDocument *doc = load_paint_doc("gradients.svg");

    if (doc) {
        /* Get the gradient we want */
        SPObject *object = doc->getObjectById(name);
        if (object && SP_IS_GRADIENT(object)) {
            SPDefs *defs = current_doc->getDefs();
            Inkscape::XML::Document *xml_doc = current_doc->getReprDoc();
            Inkscape::XML::Node *pat_repr = object->getRepr()->duplicate(xml_doc);
            defs->getRepr()->addChild(pat_repr, nullptr);
            Inkscape::GC::release(pat_repr);
            return object;
        }
    }
    return nullptr;
}

// get_stock_item returns a pointer to an instance of the desired stock object in the current doc
// if necessary it will import the object. Copes with name clashes through use of the inkscape:stockid property
// This should be set to be the same as the id in the library file.

SPObject *get_stock_item(gchar const *urn, gboolean stock)
{
    g_assert(urn != nullptr);

    /* check its an inkscape URN */
    if (!strncmp (urn, "urn:inkscape:", 13)) {

        gchar const *e = urn + 13;
        int a = 0;
        gchar * name = g_strdup(e);
        gchar *name_p = name;
        while (*name_p != ':' && *name_p != '\0'){
            name_p++;
            a++;
        }

        if (*name_p ==':') {
            name_p++;
        }

        gchar * base = g_strndup(e, a);

        SPDocument *doc = SP_ACTIVE_DOCUMENT;
        SPDefs *defs = doc->getDefs();
        if (!defs) {
            g_free(base);
            return nullptr;
        }
        SPObject *object = nullptr;
        if (!strcmp(base, "marker") && !stock) {
            for (auto& child: defs->children)
            {
                if (child.getRepr()->attribute("inkscape:stockid") &&
                    !strcmp(name_p, child.getRepr()->attribute("inkscape:stockid")) &&
                    SP_IS_MARKER(&child))
                {
                    object = &child;
                }
            }
        }
        else if (!strcmp(base,"pattern") && !stock)  {
            for (auto& child: defs->children)
            {
                if (child.getRepr()->attribute("inkscape:stockid") &&
                    !strcmp(name_p, child.getRepr()->attribute("inkscape:stockid")) &&
                    SP_IS_PATTERN(&child))
                {
                    object = &child;
                }
            }
        }
        else if (!strcmp(base,"gradient") && !stock)  {
            for (auto& child: defs->children)
            {
                if (child.getRepr()->attribute("inkscape:stockid") &&
                    !strcmp(name_p, child.getRepr()->attribute("inkscape:stockid")) &&
                    SP_IS_GRADIENT(&child))
                {
                    object = &child;
                }
            }
        }

        if (object == nullptr) {

            if (!strcmp(base, "marker"))  {
                object = sp_marker_load_from_svg(name_p, doc);
            }
            else if (!strcmp(base, "pattern"))  {
                object = sp_pattern_load_from_svg(name_p, doc);
            }
            else if (!strcmp(base, "gradient"))  {
                object = sp_gradient_load_from_svg(name_p, doc);
            }
        }

        g_free(base);
        g_free(name);

        if (object) {
            object->setAttribute("inkscape:isstock", "true");
        }

        return object;
    }

    else {

        SPDocument *doc = SP_ACTIVE_DOCUMENT;
        SPObject *object = doc->getObjectById(urn);

        return object;
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

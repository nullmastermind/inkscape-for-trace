// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/convert.h>
#include <glibmm/miscutils.h>
#include <glibmm/uriutils.h>
#include <glibmm/utility.h>

#include "../document.h"  /* Unfortunately there's a separate xml/document.h. */
#include "streq.h"

#include "io/dir-util.h"
#include "io/sys.h"

#include "object/sp-object.h"
#include "object/uri.h"

#include "xml/node.h"
#include "xml/rebase-hrefs.h"

using Inkscape::XML::AttributeRecord;
using Inkscape::XML::AttributeVector;

/**
 * Determine if a href needs rebasing.
 */
static bool href_needs_rebasing(char const *href)
{
    // RFC 3986 defines empty string relative URL as referring to the
    // containing document, rather than referring to the base URI.
    if (!href[0] || href[0] == '#') {
        return false;
    }

    // skip document-local queries
    if (href[0] == '?') {
        return false;
    }

    // skip absolute-path and network-path references
    if (href[0] == '/') {
        return false;
    }

    // Don't change non-file URIs (like data or http)
    auto scheme = Glib::make_unique_ptr_gfree(g_uri_parse_scheme(href));
    return !scheme || g_str_equal(scheme.get(), "file");
}

AttributeVector
Inkscape::XML::rebase_href_attrs(gchar const *const old_abs_base,
                                 gchar const *const new_abs_base,
                                 const AttributeVector &attributes)
{
    using Inkscape::Util::share_string;

    auto ret = attributes; // copy

    if (old_abs_base == new_abs_base) {
        return ret;
    }

    static GQuark const href_key = g_quark_from_static_string("xlink:href");
    static GQuark const absref_key = g_quark_from_static_string("sodipodi:absref");

    auto const find_record = [&ret](GQuark const key) {
        return find_if(ret.begin(), ret.end(), [key](auto const &attr) { return attr.key == key; });
    };

    auto href_it = find_record(href_key);
    if (href_it == ret.end() || !href_needs_rebasing(href_it->value.pointer())) {
        return ret;
    }

    auto uri = URI::from_href_and_basedir(href_it->value.pointer(), old_abs_base);
    auto abs_href = uri.toNativeFilename();

    auto absref_it = find_record(absref_key);
    if (absref_it != ret.end()) {
        if (g_file_test(abs_href.c_str(), G_FILE_TEST_EXISTS)) {
            if (!streq(abs_href.c_str(), absref_it->value.pointer())) {
                absref_it->value = share_string(abs_href.c_str());
            }
        } else if (g_file_test(absref_it->value.pointer(), G_FILE_TEST_EXISTS)) {
            uri = URI::from_native_filename(absref_it->value.pointer());
        }
    }

    std::string baseuri;
    if (new_abs_base && new_abs_base[0]) {
        baseuri = URI::from_dirname(new_abs_base).str();
    }

    auto new_href = uri.str(baseuri.c_str());
    href_it->value = share_string(new_href.c_str());

    return ret;
}

void Inkscape::XML::rebase_hrefs(SPDocument *const doc, gchar const *const new_base, bool const spns)
{
    using Inkscape::URI;

    std::string old_base_url_str = URI::from_dirname(doc->getDocumentBase()).str();
    std::string new_base_url_str;

    if (new_base) {
        new_base_url_str = URI::from_dirname(new_base).str();
    }

    /* TODO: Should handle not just image but also:
     *
     *    a, altGlyph, animElementAttrs, animate, animateColor, animateMotion, animateTransform,
     *    animation, audio, color-profile, cursor, definition-src, discard, feImage, filter,
     *    font-face-uri, foreignObject, glyphRef, handler, linearGradient, mpath, pattern,
     *    prefetch, radialGradient, script, set, textPath, tref, use, video
     *
     * (taken from the union of the xlink:href elements listed at
     * http://www.w3.org/TR/SVG11/attindex.html and
     * http://www.w3.org/TR/SVGMobile12/attributeTable.html).
     *
     * Also possibly some other attributes of type <URI> or <IRI> or list-thereof, or types like
     * <paint> that can include an IRI/URI, and stylesheets and style attributes.  (xlink:base is a
     * special case.  xlink:role and xlink:arcrole can be assumed to be already absolute, based on
     * http://www.w3.org/TR/SVG11/struct.html#xlinkRefAttrs .)
     *
     * Note that it may not useful to set sodipodi:absref for anything other than image.
     *
     * Note also that Inkscape only supports fragment hrefs (href="#pattern257") for many of these
     * cases. */
    std::vector<SPObject *> images = doc->getResourceList("image");
    for (auto image : images) {
        Inkscape::XML::Node *ir = image->getRepr();

        auto href_cstr = ir->attribute("xlink:href");
        if (!href_cstr) {
            continue;
        }

        if (!href_needs_rebasing(href_cstr)) {
            continue;
        }

        // make absolute
        URI url;
        try {
            url = URI(href_cstr, old_base_url_str.c_str());
        } catch (...) {
            continue;
        }

        // skip non-file URLs
        if (!url.hasScheme("file")) {
            continue;
        }

        // if path doesn't exist, use sodipodi:absref
        if (!g_file_test(url.toNativeFilename().c_str(), G_FILE_TEST_EXISTS)) {
            auto spabsref = ir->attribute("sodipodi:absref");
            if (spabsref && g_file_test(spabsref, G_FILE_TEST_EXISTS)) {
                url = URI::from_native_filename(spabsref);
            }
        } else if (spns) {
            ir->setAttributeOrRemoveIfEmpty("sodipodi:absref", url.toNativeFilename());
        }

        if (!spns) {
            ir->removeAttribute("sodipodi:absref");
        }

        auto href_str = url.str(new_base_url_str.c_str());
        href_str = Inkscape::uri_to_iri(href_str.c_str());

        ir->setAttribute("xlink:href", href_str);
    }

    doc->setDocumentBase(new_base);
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
// vi: set autoindent shiftwidth=4 tabstop=8 filetype=cpp expandtab softtabstop=4 fileencoding=utf-8 textwidth=99 :

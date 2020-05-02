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

#include "../document.h"  /* Unfortunately there's a separate xml/document.h. */
#include "streq.h"

#include "io/dir-util.h"
#include "io/sys.h"

#include "object/sp-object.h"
#include "object/uri.h"

#include "xml/node.h"
#include "xml/rebase-hrefs.h"

using Inkscape::XML::AttributeRecord;

/**
 * Determine if a href needs rebasing.
 */
static bool href_needs_rebasing(std::string const &href)
{
    bool ret = true;

    if ( href.empty() || (href[0] == '#') ) {
        ret = false;
        /* False (no change) is the right behaviour even when the base URI differs from the
         * document URI: RFC 3986 defines empty string relative URL as referring to the containing
         * document, rather than referring to the base URI. */
    } else {
        /* Don't change data or http hrefs. */
        std::string scheme = Glib::uri_parse_scheme(href);
        if ( !scheme.empty() ) {
            /* Assume it shouldn't be changed.  This is probably wrong if the scheme is `file'
             * (or if the scheme of the new base is non-file, though I believe that never
             * happens at the time of writing), but that's rare, and we won't try too hard to
             * handle this now: wait until after the freeze, then add liburiparser (or similar)
             * as a dependency and do it properly.  For now we'll just try to be simple (while
             * at least still correctly handling data hrefs). */
            ret = false;
        }
    }

    return ret;
}

Inkscape::Util::List<AttributeRecord const>
Inkscape::XML::rebase_href_attrs(gchar const *const old_abs_base,
                                 gchar const *const new_abs_base,
                                 Inkscape::Util::List<AttributeRecord const> attributes)
{
    using Inkscape::Util::List;
    using Inkscape::Util::cons;
    using Inkscape::Util::ptr_shared;
    using Inkscape::Util::share_string;


    if (old_abs_base == new_abs_base) {
        return attributes;
    }

    GQuark const href_key = g_quark_from_static_string("xlink:href");
    GQuark const absref_key = g_quark_from_static_string("sodipodi:absref");

    /* First search attributes for xlink:href and sodipodi:absref, putting the rest in ret.
     *
     * However, if we find that xlink:href doesn't need rebasing, then return immediately
     * with no change to attributes. */
    ptr_shared old_href;
    ptr_shared sp_absref;
    List<AttributeRecord const> ret;
    {
        for (List<AttributeRecord const> ai(attributes); ai; ++ai) {
            if (ai->key == href_key) {
                old_href = ai->value;
                if (!href_needs_rebasing(static_cast<char const *>(old_href))) {
                    return attributes;
                }
            } else if (ai->key == absref_key) {
                sp_absref = ai->value;
            } else {
                ret = cons(AttributeRecord(ai->key, ai->value), ret);
            }
        }
    }

    if (!old_href) {
        return attributes;
        /* We could instead return ret in this case, i.e. ensure that sodipodi:absref is cleared if
         * no xlink:href attribute.  However, retaining it might be more cautious.
         *
         * (For the usual case of not present, attributes and ret will be the same except
         * reversed.) */
    }

    auto uri = URI::from_href_and_basedir(static_cast<char const *>(old_href), old_abs_base);
    auto abs_href = uri.toNativeFilename();

    if (!Inkscape::IO::file_test(abs_href.c_str(), G_FILE_TEST_EXISTS) &&
        Inkscape::IO::file_test(sp_absref, G_FILE_TEST_EXISTS)) {
        uri = URI::from_native_filename(sp_absref);
    }

    std::string baseuri;
    if (new_abs_base && new_abs_base[0]) {
        baseuri = URI::from_dirname(new_abs_base).str();
    }

    auto new_href = uri.str(baseuri.c_str());

    ret = cons(AttributeRecord(href_key, share_string(new_href.c_str())), ret); // Check if this is safe/copied or if it is only held.
    if (sp_absref) {
        /* We assume that if there wasn't previously a sodipodi:absref attribute
         * then we shouldn't create one. */
        ret = cons(AttributeRecord(absref_key, ( streq(abs_href.c_str(), sp_absref)
                                                 ? sp_absref
                                                 : share_string(abs_href.c_str()) )),
                   ret);
    }

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

        // skip fragment URLs
        if (href_cstr[0] == '#') {
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

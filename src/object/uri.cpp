// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2003 MenTaLguY
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "uri.h"

#include <cstring>

#include <giomm/contenttype.h>
#include <giomm/file.h>
#include <glibmm/base64.h>
#include <glibmm/convert.h>
#include <glibmm/ustring.h>
#include <glibmm/miscutils.h>

#include "bad-uri-exception.h"

namespace Inkscape {

auto const URI_ALLOWED_NON_ALNUM = "!#$%&'()*+,-./:;=?@_~";

/**
 * Return true if the given URI string contains characters that need escaping.
 *
 * Note: It does not check if valid characters appear in invalid context (e.g.
 * '%' not followed by two hex digits).
 */
static bool uri_needs_escaping(char const *uri)
{
    for (auto *p = uri; *p; ++p) {
        if (!g_ascii_isalnum(*p) && !strchr(URI_ALLOWED_NON_ALNUM, *p)) {
            return true;
        }
    }
    return false;
}

URI::URI() {
    _impl = Impl::create(xmlCreateURI());
}

URI::URI(const URI &uri) {
    uri._impl->reference();
    _impl = uri._impl;
}

URI::URI(gchar const *preformed, char const *baseuri)
{
    xmlURIPtr uri;
    if (!preformed) {
        throw MalformedURIException();
    }

    // check for invalid characters, escape if needed
    xmlChar *escaped = nullptr;
    if (uri_needs_escaping(preformed)) {
        escaped = xmlURIEscapeStr(      //
            (xmlChar const *)preformed, //
            (xmlChar const *)URI_ALLOWED_NON_ALNUM);
        preformed = (decltype(preformed))escaped;
    }

    // make absolute
    xmlChar *full = nullptr;
    if (baseuri) {
        full = xmlBuildURI(             //
            (xmlChar const *)preformed, //
            (xmlChar const *)baseuri);
#if LIBXML_VERSION < 20905
        // libxml2 bug: "file:/some/file" instead of "file:///some/file"
        auto f = (gchar const *)full;
        if (f && g_str_has_prefix(f, "file:/") && f[6] != '/') {
            auto fixed = std::string(f, 6) + "//" + std::string(f + 6);
            xmlFree(full);
            full = (xmlChar *)xmlMemStrdup(fixed.c_str());
        }
#endif
        preformed = (decltype(preformed))full;
    }

    uri = xmlParseURI(preformed);

    if (full) {
        xmlFree(full);
    }
    if (escaped) {
        xmlFree(escaped);
    }
    if (!uri) {
        throw MalformedURIException();
    }
    _impl = Impl::create(uri);
}

URI::URI(char const *preformed, URI const &baseuri)
    : URI::URI(preformed, baseuri.str().c_str())
{
}

URI::~URI() {
    _impl->unreference();
}

URI &URI::operator=(URI const &uri) {
// No check for self-assignment needed, as _impl refcounting increments first.
    uri._impl->reference();
    _impl->unreference();
    _impl = uri._impl;
    return *this;
}

URI::Impl *URI::Impl::create(xmlURIPtr uri) {
    return new Impl(uri);
}

URI::Impl::Impl(xmlURIPtr uri)
: _refcount(1), _uri(uri) {}

URI::Impl::~Impl() {
    if (_uri) {
        xmlFreeURI(_uri);
        _uri = nullptr;
    }
}

void URI::Impl::reference() {
    _refcount++;
}

void URI::Impl::unreference() {
    if (!--_refcount) {
        delete this;
    }
}

// From RFC 2396:
//
// URI-reference = [ absoluteURI | relativeURI ] [ "#" fragment ]
// absoluteURI   = scheme ":" ( hier_part | opaque_part )
// relativeURI   = ( net_path | abs_path | rel_path ) [ "?" query ]
//
// hier_part     = ( net_path | abs_path ) [ "?" query ]
// opaque_part   = uric_no_slash *uric
//
// uric_no_slash = unreserved | escaped | ";" | "?" | ":" | "@" |
//                 "&" | "=" | "+" | "$" | ","
//
// net_path      = "//" authority [ abs_path ]
// abs_path      = "/"  path_segments
// rel_path      = rel_segment [ abs_path ]
//
// rel_segment   = 1*( unreserved | escaped |
//                     ";" | "@" | "&" | "=" | "+" | "$" | "," )
//
// authority     = server | reg_name

bool URI::isOpaque() const {
    return getOpaque() != nullptr;
}

bool URI::isRelative() const {
    return !_xmlURIPtr()->scheme;
}

bool URI::isNetPath() const {
    return isRelative() && _xmlURIPtr()->server;
}

bool URI::isRelativePath() const {
    if (isRelative() && !_xmlURIPtr()->server) {
        const gchar *path = getPath();
        return path && path[0] != '/';
    }
    return false;
}

bool URI::isAbsolutePath() const {
    if (isRelative() && !_xmlURIPtr()->server) {
        const gchar *path = getPath();
        return path && path[0] == '/';
    }
    return false;
}

const gchar *URI::getScheme() const {
    return (gchar *)_xmlURIPtr()->scheme;
}

const gchar *URI::getPath() const {
    return (gchar *)_xmlURIPtr()->path;
}

const gchar *URI::getQuery() const {
    return (gchar *)_xmlURIPtr()->query;
}

const gchar *URI::getFragment() const {
    return (gchar *)_xmlURIPtr()->fragment;
}

const gchar *URI::getOpaque() const {
    if (!isRelative() && !_xmlURIPtr()->server) {
        const gchar *path = getPath();
        if (path && path[0] != '/') {
            return path;
        }
    }
    return nullptr;
}

std::string URI::toNativeFilename() const
{ //
    auto uristr = str();

    // remove fragment identifier
    if (getFragment() != nullptr) {
        uristr.resize(uristr.find('#'));
    }

    return Glib::filename_from_uri(uristr);
}

/* TODO !!! proper error handling */
URI URI::from_native_filename(gchar const *path) {
    gchar *uri = g_filename_to_uri(path, nullptr, nullptr);
    URI result(uri);
    g_free( uri );
    return result;
}

URI URI::from_dirname(gchar const *path)
{
    std::string pathstr = path ? path : ".";

    if (!Glib::path_is_absolute(pathstr)) {
        pathstr = Glib::build_filename(Glib::get_current_dir(), pathstr);
    }

    auto uristr = Glib::filename_to_uri(pathstr);

    if (uristr[uristr.size() - 1] != '/') {
        uristr.push_back('/');
    }

    return URI(uristr.c_str());
}

URI URI::from_href_and_basedir(char const *href, char const *basedir)
{
    try {
        return URI(href, URI::from_dirname(basedir));
    } catch (...) {
        return URI();
    }
}

/**
 * Replacement for buggy xmlBuildRelativeURI
 * https://gitlab.gnome.org/GNOME/libxml2/merge_requests/12
 *
 * Special case: Don't cross filesystem root, e.g. drive letter on Windows.
 * This is an optimization to keep things practical, it's not required for correctness.
 *
 * @param uri an absolute URI
 * @param base an absolute URI without any ".." path segments
 * @return relative URI if possible, otherwise @a uri unchanged
 */
static std::string build_relative_uri(char const *uri, char const *base)
{
    size_t n_slash = 0;
    size_t i = 0;

    // find longest common prefix
    for (; uri[i]; ++i) {
        if (uri[i] != base[i]) {
            break;
        }

        if (uri[i] == '/') {
            ++n_slash;
        }
    }

    // URIs must share protocol://server/
    if (n_slash < 3) {
        return uri;
    }

    // Don't cross filesystem root
    if (n_slash == 3 && g_str_has_prefix(base, "file:///") && base[8]) {
        return uri;
    }

    std::string relative;

    for (size_t j = i; base[j]; ++j) {
        if (base[j] == '/') {
            relative += "../";
        }
    }

    while (uri[i - 1] != '/') {
        --i;
    }

    relative += (uri + i);

    if (relative.empty() && base[i]) {
        relative = "./";
    }

    return relative;
}

std::string URI::str(char const *baseuri) const
{
    std::string s;
    auto saveuri = xmlSaveUri(_xmlURIPtr());
    if (saveuri) {
        auto save = (const char *)saveuri;
        if (baseuri && baseuri[0]) {
            s = build_relative_uri(save, baseuri);
        } else {
            s = save;
        }
        xmlFree(saveuri);
    }
    return s;
}

std::string URI::getMimeType() const
{
    const char *path = getPath();

    if (path) {
        if (hasScheme("data")) {
            for (const char *p = path; *p; ++p) {
                if (*p == ';' || *p == ',') {
                    return std::string(path, p);
                }
            }
        } else {
            bool uncertain;
            auto type = Gio::content_type_guess(path, nullptr, 0, uncertain);
            return Gio::content_type_get_mime_type(type).raw();
        }
    }

    return "unknown/unknown";
}

std::string URI::getContents() const
{
    if (hasScheme("data")) {
        // handle data URIs

        const char *p = getPath();
        const char *tok = nullptr;

        // scan "[<media type>][;base64]," header
        for (; *p && *p != ','; ++p) {
            if (*p == ';') {
                tok = p + 1;
            }
        }

        // body follows after comma
        if (*p != ',') {
            g_critical("data URI misses comma");
        } else if (tok && strncmp("base64", tok, p - tok) == 0) {
            // base64 encoded body
            return Glib::Base64::decode(p + 1);
        } else {
            // raw body
            return p + 1;
        }
    } else {
        // handle non-data URIs with GVfs
        auto file = Gio::File::create_for_uri(str());

        gsize length = 0;
        char *buffer = nullptr;

        if (file->load_contents(buffer, length)) {
            auto contents = std::string(buffer, buffer + length);
            g_free(buffer);
            return contents;
        } else {
            g_critical("failed to load contents from %.100s", str().c_str());
        }
    }

    return "";
}

bool URI::hasScheme(const char *scheme) const
{
    const char *s = getScheme();
    return s && g_ascii_strcasecmp(s, scheme) == 0;
}

} // namespace Inkscape


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

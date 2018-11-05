/*
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2003 MenTaLguY
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
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
    const gchar *in = "";
    _impl = Impl::create(xmlParseURI(in));
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

bool URI::Impl::isOpaque() const {
    bool opq = !isRelative() && (getOpaque() != nullptr);
    return opq;
}

bool URI::Impl::isRelative() const {
    return !_uri->scheme;
}

bool URI::Impl::isNetPath() const {
    bool isNet = false;
    if ( isRelative() )
    {
        const gchar *path = getPath();
        isNet = path && path[0] == '\\' && path[1] == '\\';
    }
    return isNet;
}

bool URI::Impl::isRelativePath() const {
    bool isRel = false;
    if ( isRelative() )
    {
        const gchar *path = getPath();
        isRel = !path || path[0] != '\\';
    }
    return isRel;
}

bool URI::Impl::isAbsolutePath() const {
    bool isAbs = false;
    if ( isRelative() )
    {
        const gchar *path = getPath();
        isAbs = path && path[0] == '\\'&& path[1] != '\\';
    }
    return isAbs;
}

const gchar *URI::Impl::getScheme() const {
    return (gchar *)_uri->scheme;
}

const gchar *URI::Impl::getPath() const {
    return (gchar *)_uri->path;
}

const gchar *URI::Impl::getQuery() const {
    return (gchar *)_uri->query;
}

const gchar *URI::Impl::getFragment() const {
    return (gchar *)_uri->fragment;
}

const gchar *URI::Impl::getOpaque() const {
    return (gchar *)_uri->opaque;
}

/*
 * Returns the absolute path to an existing file referenced in this URI,
 * if the uri is data, the path is empty or the file doesn't exist, then
 * an empty string is returned.
 *
 * Does not check if the returned path is the local document's path (local)
 * and thus redundent. Caller is expected to check against the document's path.
 *
 * @param base directory name to use as base if this is not an absolute URL
 */
const std::string URI::getFullPath(std::string const &base) const {
    if (!_impl->getPath()) {
        return "";
    }

    URI url;

    if (!base.empty() && !getScheme()) {
        url = Inkscape::URI::from_href_and_basedir(str().c_str(), base.c_str());
    } else {
        url = *this;
    }

    if (!url.hasScheme("file")) {
        return "";
    }

    auto path = Glib::filename_from_uri(url.str());

    // Check the existence of the file
    if(! g_file_test(path.c_str(), G_FILE_TEST_EXISTS)
      || g_file_test(path.c_str(), G_FILE_TEST_IS_DIR) ) {
        path.clear();
    }
    return path;
}


/* TODO !!! proper error handling */
std::string URI::toNativeFilename() const
{ //
    return Glib::filename_from_uri(str());
}

URI URI::fromUtf8( gchar const* path ) {
    if ( !path ) {
        throw MalformedURIException();
    }
    Glib::ustring tmp;
    for ( int i = 0; path[i]; i++ )
    {
        gint one = 0x0ff & path[i];
        if ( ('a' <= one && one <= 'z')
             || ('A' <= one && one <= 'Z')
             || ('0' <= one && one <= '9')
             || one == '_'
             || one == '-'
             || one == '!'
             || one == '.'
             || one == '~'
             || one == '\''
             || one == '('
             || one == ')'
             || one == '*'
            ) {
            tmp += (gunichar)one;
        } else {
            gchar scratch[4];
            g_snprintf( scratch, 4, "%%%02X", one );
            tmp.append( scratch );
        }
    }
    return URI( tmp.data() );
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

gchar *URI::Impl::toString() const {
    xmlChar *string = xmlSaveUri(_uri);
    if (string) {
        /* hand the string off to glib memory management */
        gchar *glib_string = g_strdup((gchar *)string);
        xmlFree(string);
        return glib_string;
    } else {
        return nullptr;
    }
}

std::string URI::str(char const *baseuri) const
{
    std::string s;
    gchar *save = _impl->toString();
    if (save) {
        xmlChar *rel = nullptr;
        const char *latest = save;
        if (baseuri) {
            rel = xmlBuildRelativeURI((xmlChar *)save, (xmlChar *)baseuri);
            if (rel) {
                latest = (const char *)rel;
            }
        }
        s = latest;
        if (rel) {
            xmlFree(rel);
        }
        g_free(save);
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

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

#ifndef INKSCAPE_URI_H
#define INKSCAPE_URI_H

#include <libxml/uri.h>
#include <memory>
#include <string>

namespace Inkscape {

/**
 * Represents an URI as per RFC 2396.
 *
 * Typical use-cases of this class:
 * - converting between relative and absolute URIs
 * - converting URIs to/from filenames (alternative: Glib functions, but those only handle absolute paths)
 * - generic handling of data/file/http URIs (e.g. URI::getContents and URI::getMimeType)
 *
 * Wraps libxml2's URI functions. Direct usage of libxml2's C-API is discouraged if favor of
 * Inkscape::URI. (At the time of writing this, no de-factor standard C++ URI library exists, so
 * wrapping libxml2 seems like a good solution)
 *
 * Implementation detail: Immutable type, copies share a ref-counted data pointer.
 */
class URI {
public:

    /* Blank constructor */
    URI();

    /**
     * Constructor from a C-style ASCII string.
     *
     * @param preformed Properly quoted C-style string to be represented.
     * @param baseuri If @a preformed is a relative URI, use @a baseuri to make it absolute
     *
     * @throw MalformedURIException
     */
    explicit URI(char const *preformed, char const *baseuri = nullptr);
    explicit URI(char const *preformed, URI const &baseuri);

    /**
     * Determines if the URI represented is an 'opaque' URI.
     *
     * @return \c true if the URI is opaque, \c false if hierarchial.
     */
    bool isOpaque() const;

    /**
     * Determines if the URI represented is 'relative' as per RFC 2396.
     *
     * Relative URI references are distinguished by not beginning with a
     * scheme name.
     *
     * @return \c true if the URI is relative, \c false if it is absolute.
     */
    bool isRelative() const;

    /**
     * Determines if the relative URI represented is a 'net-path' as per RFC 2396.
     *
     * A net-path is one that starts with "//".
     *
     * @return \c true if the URI is relative and a net-path, \c false otherwise.
     */
    bool isNetPath() const;

    /**
     * Determines if the relative URI represented is a 'relative-path' as per RFC 2396.
     *
     * A relative-path is one that starts with no slashes.
     *
     * @return \c true if the URI is relative and a relative-path, \c false otherwise.
     */
    bool isRelativePath() const;

    /**
     * Determines if the relative URI represented is a 'absolute-path' as per RFC 2396.
     *
     * An absolute-path is one that starts with a single "/".
     *
     * @return \c true if the URI is relative and an absolute-path, \c false otherwise.
     */
    bool isAbsolutePath() const;

    /**
     * Return the scheme, e.g.\ "http", or \c NULL if this is not an absolute URI.
     */
    const char *getScheme() const;

    /**
     * Return the path.
     *
     * Example: "http://host/foo/bar?query#frag" -> "/foo/bar"
     *
     * For an opaque URI, this is identical to getOpaque()
     */
    const char *getPath() const;

    /**
     * Return the query, which is the part between "?" and the optional fragment hash ("#")
     */
    const char *getQuery() const;

    /**
     * Return the fragment, which is everything after "#"
     */
    const char *getFragment() const;

    /**
     * For an opaque URI, return everything between the scheme colon (":") and the optional
     * fragment hash ("#"). For non-opaque URIs, return NULL.
     */
    const char *getOpaque() const;

    /**
     * Construct a "file" URI from an absolute filename.
     */
    static URI from_native_filename(char const *path);

    /**
     * URI of a local directory. The URI path will end with a slash.
     */
    static URI from_dirname(char const *path);

    /**
     * Convenience function for the common use case given a xlink:href attribute and a local
     * directory as the document base. Returns an empty URI on failure.
     */
    static URI from_href_and_basedir(char const *href, char const *basedir);

    /**
     * Convert this URI to a native filename.
     *
     * Discards the fragment identifier.
     *
     * @throw Glib::ConvertError If this is not a "file" URI
     */
    std::string toNativeFilename() const;

    /**
     * Return the string representation of this URI
     *
     * @param baseuri Return a relative path if this URI shares protocol and host with @a baseuri
     */
    std::string str(char const *baseuri = nullptr) const;

    /**
     * Get the MIME type (e.g.\ "image/png")
     */
    std::string getMimeType() const;

    /**
     * Return the contents of the file
     *
     * @throw Glib::Error If the URL can't be read
     */
    std::string getContents() const;

    /**
     * Return a CSS formatted url value
     *
     * @param baseuri Return a relative path if this URI shares protocol and host with @a baseuri
     */
    std::string cssStr(char const *baseuri = nullptr) const {
        return "url(" + str(baseuri) + ")";
    }

    /**
     * True if the scheme equals the given string (not case sensitive)
     */
    bool hasScheme(const char *scheme) const;

private:
    std::shared_ptr<xmlURI> m_shared;

    void init(xmlURI *ptr) { m_shared.reset(ptr, xmlFreeURI); }

    xmlURI *_xmlURIPtr() const { return m_shared.get(); }
};

/**
 * Unescape the UTF-8 parts of the given URI.
 *
 * Does not decode non-UTF-8 escape sequences (e.g. reserved ASCII characters).
 * Does not do any IDN (internationalized domain name) decoding.
 *
 * @param uri URI or part of a URI
 * @return IRI equivalent of \c uri
 */
std::string uri_to_iri(const char *uri);

}  /* namespace Inkscape */

#endif

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

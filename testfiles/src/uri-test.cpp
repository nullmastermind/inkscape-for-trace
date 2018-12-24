// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Test Inkscape::URI
 */
/*
 * Authors:
 *   Thomas Holder
 *
 * Copyright (C) 2018 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "object/uri.h"
#include "gtest/gtest.h"

using Inkscape::URI;

#define BASE64_HELLO_WORLD_P1 "SGVsbG8g"
#define BASE64_HELLO_WORLD_P2 "V29ybGQ="
#define DATA_BASE64_HEADER "data:text/plain;charset=utf-8;base64,"
char const *DATA_BASE64_HELLO_WORLD = DATA_BASE64_HEADER BASE64_HELLO_WORLD_P1 BASE64_HELLO_WORLD_P2;
char const *DATA_BASE64_HELLO_WORLD_WRAPPED = DATA_BASE64_HEADER BASE64_HELLO_WORLD_P1 "\n" BASE64_HELLO_WORLD_P2;

char const *win_url_unc = "file://laptop/My%20Documents/FileSchemeURIs.doc";
char const *win_url_local = "file:///C:/Documents%20and%20Settings/davris/FileSchemeURIs.doc";
char const *win_filename_local = "C:\\Documents and Settings\\davris\\FileSchemeURIs.doc";

TEST(UriTest, Malformed)
{
    ASSERT_ANY_THROW(URI(nullptr));
    ASSERT_ANY_THROW(URI("nonhex-%XX"));
}

TEST(UriTest, GetPath)
{
    ASSERT_STREQ(URI().getPath(), nullptr);
    ASSERT_STREQ(URI("foo.svg").getPath(), "foo.svg");
    ASSERT_STREQ(URI("foo.svg#bar").getPath(), "foo.svg");
    ASSERT_STREQ(URI("#bar").getPath(), nullptr);
    ASSERT_STREQ(URI("scheme://host").getPath(), nullptr);
    ASSERT_STREQ(URI("scheme://host/path").getPath(), "/path");
    ASSERT_STREQ(URI("scheme://host/path?query").getPath(), "/path");
    ASSERT_STREQ(URI("scheme:/path").getPath(), "/path");
}

TEST(UriTest, FromDir)
{
#ifdef _WIN32
    ASSERT_EQ(URI::from_dirname("C:\\tmp").str(), "file:///C:/tmp/");
    ASSERT_EQ(URI::from_dirname("C:\\").str(), "file:///C:/");
    ASSERT_EQ(URI::from_href_and_basedir("uri.svg", "C:\\tmp").str(), "file:///C:/tmp/uri.svg");
#else
    ASSERT_EQ(URI::from_dirname("/").str(), "file:///");
    ASSERT_EQ(URI::from_dirname("/tmp").str(), "file:///tmp/");
    ASSERT_EQ(URI::from_href_and_basedir("uri.svg", "/tmp").str(), "file:///tmp/uri.svg");
#endif
}

TEST(UriTest, Str)
{
    ASSERT_EQ(URI().str(), "");
    ASSERT_EQ(URI("").str(), "");
    ASSERT_EQ(URI("", "http://a/b").str(), "http://a/b");

    ASSERT_EQ(URI("uri.svg").str(), "uri.svg");
    ASSERT_EQ(URI("tmp/uri.svg").str(), "tmp/uri.svg");
    ASSERT_EQ(URI("/tmp/uri.svg").str(), "/tmp/uri.svg");
    ASSERT_EQ(URI("../uri.svg").str(), "../uri.svg");

    ASSERT_EQ(URI("file:///tmp/uri.svg").str(), "file:///tmp/uri.svg");
    ASSERT_EQ(URI("uri.svg", "file:///tmp/").str(), "file:///tmp/uri.svg");
    ASSERT_EQ(URI("file:///tmp/uri.svg").str("file:///tmp/"), "uri.svg");
    ASSERT_EQ(URI("file:///tmp/up/uri.svg").str("file:///tmp/"), "up/uri.svg");
    ASSERT_EQ(URI("file:///tmp/uri.svg").str("file:///tmp/up/"), "../uri.svg");
    ASSERT_EQ(URI("file:///tmp/uri.svg").str("http://web/url"), "file:///tmp/uri.svg");
    ASSERT_EQ(URI("file:///tmp/uri.svg").str("http://web/url"), "file:///tmp/uri.svg");
    ASSERT_EQ(URI("foo/uri.svg", "http://web/a/b/c").str(), "http://web/a/b/foo/uri.svg");
    ASSERT_EQ(URI("foo/uri.svg", "http://web/a/b/c").str("http://web/a/"), "b/foo/uri.svg");
    ASSERT_EQ(URI("foo/uri.svg", "http://web/a/b/c").str("http://other/a/"), "http://web/a/b/foo/uri.svg");

    ASSERT_EQ(URI("http://web/").str("http://web/"), "");
    ASSERT_EQ(URI("http://web/").str("http://web/url"), "./");

    // special case: don't cross filesystem root
    ASSERT_EQ(URI("file:///a").str("file:///"), "a");
    ASSERT_EQ(URI("file:///ax/b").str("file:///ay/"), "file:///ax/b"); // special case
    ASSERT_EQ(URI("file:///C:/b").str("file:///D:/"), "file:///C:/b"); // special case
    ASSERT_EQ(URI("file:///C:/a/b").str("file:///C:/b/"), "../a/b");

    ASSERT_EQ(URI(win_url_unc).str(), win_url_unc);
    ASSERT_EQ(URI(win_url_unc).str("file://laptop/My%20Documents/"), "FileSchemeURIs.doc");
    ASSERT_EQ(URI(win_url_local).str(), win_url_local);
    ASSERT_EQ(URI(win_url_local).str("file:///C:/Documents%20and%20Settings/"), "davris/FileSchemeURIs.doc");
    ASSERT_EQ(URI(win_url_local).str(win_url_unc), win_url_local);
#ifdef _WIN32
    ASSERT_EQ(URI(win_url_local).toNativeFilename(), win_filename_local);
#else
    ASSERT_EQ(URI("file:///tmp/uri.svg").toNativeFilename(), "/tmp/uri.svg");
    ASSERT_EQ(URI("file:///tmp/x%20y.svg").toNativeFilename(), "/tmp/x y.svg");
    ASSERT_EQ(URI("file:///a/b#hash").toNativeFilename(), "/a/b");
#endif

    ASSERT_ANY_THROW(URI("http://a/b").toNativeFilename());
}

TEST(UriTest, StrDataScheme)
{
    ASSERT_EQ(URI("data:,text").str(), "data:,text");
    ASSERT_EQ(URI("data:,white%20space").str(), "data:,white%20space");
    ASSERT_EQ(URI("data:,umlaut-%C3%96").str(), "data:,umlaut-%C3%96");
    ASSERT_EQ(URI(DATA_BASE64_HELLO_WORLD).str(), DATA_BASE64_HELLO_WORLD);
}

TEST(UriTest, Escape)
{
    ASSERT_EQ(URI("data:,white space").str(), "data:,white%20space");
    ASSERT_EQ(URI("data:,white\nspace").str(), "data:,white%0Aspace");
    ASSERT_EQ(URI("data:,umlaut-\xC3\x96").str(), "data:,umlaut-%C3%96");
}

TEST(UriTest, GetContents)
{
    ASSERT_EQ(URI("data:,white space").getContents(), "white space");
    ASSERT_EQ(URI("data:,white%20space").getContents(), "white space");
    ASSERT_EQ(URI("data:,white\nspace").getContents(), "white\nspace");
    ASSERT_EQ(URI("data:,white%0Aspace").getContents(), "white\nspace");
    ASSERT_EQ(URI("data:,umlaut-%C3%96").getContents(), "umlaut-\xC3\x96");
    ASSERT_EQ(URI(DATA_BASE64_HELLO_WORLD).getContents(), "Hello World");
    ASSERT_EQ(URI(DATA_BASE64_HELLO_WORLD_WRAPPED).getContents(), "Hello World");

    ASSERT_ANY_THROW(URI().getContents());
}

TEST(UriTest, CssStr)
{
    ASSERT_EQ(URI("file:///tmp/uri.svg").cssStr(), "url(file:///tmp/uri.svg)");
    ASSERT_EQ(URI("uri.svg").cssStr(), "url(uri.svg)");
}

TEST(UriTest, GetMimeType)
{
    ASSERT_EQ(URI("data:image/png;base64,").getMimeType(), "image/png");
    ASSERT_EQ(URI("data:text/plain,xxx").getMimeType(), "text/plain");
    ASSERT_EQ(URI("file:///tmp/uri.png").getMimeType(), "image/png");
    ASSERT_EQ(URI("uri.png").getMimeType(), "image/png");
    ASSERT_EQ(URI("uri.svg").getMimeType(), "image/svg+xml");

    // can be "text/plain" or "text/*"
    ASSERT_EQ(URI("file:///tmp/uri.txt").getMimeType().substr(0, 5), "text/");
}

TEST(UriTest, HasScheme)
{
    ASSERT_FALSE(URI().hasScheme("file"));
    ASSERT_FALSE(URI("uri.svg").hasScheme("file"));
    ASSERT_FALSE(URI("uri.svg").hasScheme("data"));

    ASSERT_TRUE(URI("file:///uri.svg").hasScheme("file"));
    ASSERT_TRUE(URI("FILE:///uri.svg").hasScheme("file"));
    ASSERT_FALSE(URI("file:///uri.svg").hasScheme("data"));

    ASSERT_TRUE(URI("data:,").hasScheme("data"));
    ASSERT_TRUE(URI("DaTa:,").hasScheme("data"));
    ASSERT_FALSE(URI("data:,").hasScheme("file"));

    ASSERT_TRUE(URI("http://web/").hasScheme("http"));
    ASSERT_FALSE(URI("http://web/").hasScheme("file"));

    ASSERT_TRUE(URI::from_href_and_basedir("data:,white\nspace", "/tmp").hasScheme("data"));
}

TEST(UriTest, isOpaque)
{
    ASSERT_FALSE(URI().isOpaque());
    ASSERT_FALSE(URI("file:///uri.svg").isOpaque());
    ASSERT_FALSE(URI("/uri.svg").isOpaque());
    ASSERT_FALSE(URI("uri.svg").isOpaque());
    ASSERT_FALSE(URI("foo://bar/baz").isOpaque());
    ASSERT_FALSE(URI("foo://bar").isOpaque());
    ASSERT_FALSE(URI("foo:/bar").isOpaque());

    ASSERT_TRUE(URI("foo:bar").isOpaque());
    ASSERT_TRUE(URI("mailto:user@host.xy").isOpaque());
    ASSERT_TRUE(URI("news:comp.lang.java").isOpaque());
}

TEST(UriTest, isRelative)
{
    ASSERT_TRUE(URI().isRelative());

    ASSERT_FALSE(URI("http://web/uri.svg").isRelative());
    ASSERT_FALSE(URI("file:///uri.svg").isRelative());
    ASSERT_FALSE(URI("mailto:user@host.xy").isRelative());
    ASSERT_FALSE(URI("data:,").isRelative());

    ASSERT_TRUE(URI("//web/uri.svg").isRelative());
    ASSERT_TRUE(URI("/uri.svg").isRelative());
    ASSERT_TRUE(URI("uri.svg").isRelative());
    ASSERT_TRUE(URI("./uri.svg").isRelative());
    ASSERT_TRUE(URI("../uri.svg").isRelative());
}

TEST(UriTest, isNetPath)
{
    ASSERT_FALSE(URI().isNetPath());
    ASSERT_FALSE(URI("http://web/uri.svg").isNetPath());
    ASSERT_FALSE(URI("file:///uri.svg").isNetPath());
    ASSERT_FALSE(URI("/uri.svg").isNetPath());
    ASSERT_FALSE(URI("uri.svg").isNetPath());

    ASSERT_TRUE(URI("//web/uri.svg").isNetPath());
}

TEST(UriTest, isRelativePath)
{
    ASSERT_FALSE(URI("foo:bar").isRelativePath());
    ASSERT_TRUE(URI("foo%3Abar").isRelativePath());

    ASSERT_FALSE(URI("http://web/uri.svg").isRelativePath());
    ASSERT_FALSE(URI("//web/uri.svg").isRelativePath());
    ASSERT_FALSE(URI("/uri.svg").isRelativePath());

    ASSERT_TRUE(URI("uri.svg").isRelativePath());
    ASSERT_TRUE(URI("./uri.svg").isRelativePath());
    ASSERT_TRUE(URI("../uri.svg").isRelativePath());
}

TEST(UriTest, isAbsolutePath)
{
    ASSERT_FALSE(URI().isAbsolutePath());
    ASSERT_FALSE(URI("http://web/uri.svg").isAbsolutePath());
    ASSERT_FALSE(URI("//web/uri.svg").isAbsolutePath());
    ASSERT_FALSE(URI("uri.svg").isAbsolutePath());
    ASSERT_FALSE(URI("../uri.svg").isAbsolutePath());

    ASSERT_TRUE(URI("/uri.svg").isAbsolutePath());
}

TEST(UriTest, getScheme)
{
    ASSERT_STREQ(URI().getScheme(), nullptr);

    ASSERT_STREQ(URI("https://web/uri.svg").getScheme(), "https");
    ASSERT_STREQ(URI("file:///uri.svg").getScheme(), "file");
    ASSERT_STREQ(URI("data:,").getScheme(), "data");

    ASSERT_STREQ(URI("data").getScheme(), nullptr);
}

TEST(UriTest, getQuery)
{
    ASSERT_STREQ(URI().getQuery(), nullptr);
    ASSERT_STREQ(URI("uri.svg?a=b&c=d").getQuery(), "a=b&c=d");
    ASSERT_STREQ(URI("?a=b&c=d#hash").getQuery(), "a=b&c=d");
}

TEST(UriTest, getFragment)
{
    ASSERT_STREQ(URI().getFragment(), nullptr);
    ASSERT_STREQ(URI("uri.svg").getFragment(), nullptr);
    ASSERT_STREQ(URI("uri.svg#hash").getFragment(), "hash");
    ASSERT_STREQ(URI("?a=b&c=d#hash").getFragment(), "hash");
    ASSERT_STREQ(URI("urn:isbn:096139210x#hash").getFragment(), "hash");
}

TEST(UriTest, getOpaque)
{
    ASSERT_STREQ(URI().getOpaque(), nullptr);
    ASSERT_STREQ(URI("urn:isbn:096139210x#hash").getOpaque(), "isbn:096139210x");
    ASSERT_STREQ(URI("data:,foo").getOpaque(), ",foo");
}

TEST(UriTest, from_native_filename)
{
#ifdef _WIN32
    ASSERT_EQ(URI::from_native_filename(win_filename_local).str(), win_url_local);
#else
    ASSERT_EQ(URI::from_native_filename("/tmp/uri.svg").str(), "file:///tmp/uri.svg");
    ASSERT_EQ(URI::from_native_filename("/tmp/x y.svg").str(), "file:///tmp/x%20y.svg");
#endif
}

TEST(UriTest, uri_to_iri)
{
    // unescape UTF-8 (U+00D6)
    ASSERT_EQ(Inkscape::uri_to_iri("data:,umlaut-%C3%96"), "data:,umlaut-\xC3\x96");
    // don't unescape ASCII (U+003A)
    ASSERT_EQ(Inkscape::uri_to_iri("foo%3Abar"), "foo%3Abar");
    // sequence (U+00D6 U+1F37A U+003A)
    ASSERT_EQ(Inkscape::uri_to_iri("%C3%96%F0%9F%8D%BA%3A"), "\xC3\x96\xF0\x9F\x8D\xBA%3A");
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

/*
 * Test Inkscape::URI
 *
 * Copyright 2018 (c) Thomas Holder
 *
 */
#include "object/uri.h"
#include "gtest/gtest.h"

using Inkscape::URI;

#define BASE64_HELLO_WORLD_P1 "SGVsbG8g"
#define BASE64_HELLO_WORLD_P2 "V29ybGQ="
#define DATA_BASE64_HEADER "data:text/plain;charset=utf-8;base64,"
char const *DATA_BASE64_HELLO_WORLD = DATA_BASE64_HEADER BASE64_HELLO_WORLD_P1 BASE64_HELLO_WORLD_P2;
char const *DATA_BASE64_HELLO_WORLD_WRAPPED = DATA_BASE64_HEADER BASE64_HELLO_WORLD_P1 "\n" BASE64_HELLO_WORLD_P2;

TEST(UriTest, GetPath)
{
    ASSERT_STREQ(URI("foo.svg").getPath(), "foo.svg");
    ASSERT_STREQ(URI("foo.svg#bar").getPath(), "foo.svg");
    ASSERT_STREQ(URI("#bar").getPath(), nullptr);
}

TEST(UriTest, FromDir)
{
#ifdef _WIN32
    ASSERT_EQ(URI::from_dirname("C:\\tmp").str(), "file:///C:/tmp/");
    ASSERT_EQ(URI::from_href_and_basedir("uri.svg", "C:\\tmp").str(), "file:///C:/tmp/uri.svg");
#else
    ASSERT_EQ(URI::from_dirname("/tmp").str(), "file:///tmp/");
    ASSERT_EQ(URI::from_href_and_basedir("uri.svg", "/tmp").str(), "file:///tmp/uri.svg");
#endif
}

TEST(UriTest, Str)
{
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

    const char *win_url_unc = "file://laptop/My%20Documents/FileSchemeURIs.doc";
    const char *win_url_local = "file:///C:/Documents%20and%20Settings/davris/FileSchemeURIs.doc";

    ASSERT_EQ(URI(win_url_unc).str(), win_url_unc);
    ASSERT_EQ(URI(win_url_unc).str("file://laptop/My%20Documents/"), "FileSchemeURIs.doc");
    ASSERT_EQ(URI(win_url_local).str(), win_url_local);
    ASSERT_EQ(URI(win_url_local).str("file:///C:/Documents%20and%20Settings/"), "davris/FileSchemeURIs.doc");
    ASSERT_EQ(URI(win_url_local).str(win_url_unc), win_url_local);
#ifdef _WIN32
    ASSERT_EQ(URI(win_url_local).toNativeFilename(), "C:\\Documents and Settings\\davris\\FileSchemeURIs.doc");
#else
    ASSERT_EQ(URI("file:///tmp/uri.svg").toNativeFilename(), "/tmp/uri.svg");
    ASSERT_EQ(URI("file:///tmp/x%20y.svg").toNativeFilename(), "/tmp/x y.svg");
#endif
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

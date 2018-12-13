// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Test extract_uri
 */
/*
 * Authors:
 *   Thomas Holder
 *
 * Copyright (C) 2018 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "extract-uri.h"
#include "gtest/gtest.h"

TEST(ExtractUriTest, valid)
{
    ASSERT_EQ(extract_uri("url(#foo)"), "#foo");
    ASSERT_EQ(extract_uri("url( \t #foo \t )"), "#foo");
    ASSERT_EQ(extract_uri("url( '#foo' )"), "#foo");
    ASSERT_EQ(extract_uri("url('url(foo)')"), "url(foo)");
    ASSERT_EQ(extract_uri("url(\"foo(url)\")"), "foo(url)");
    ASSERT_EQ(extract_uri("url()bar"), "");
    ASSERT_EQ(extract_uri("url( )bar"), "");
    ASSERT_EQ(extract_uri("url(a b)"), "a b");
}

TEST(ExtractUriTest, legacy)
{
    ASSERT_EQ(extract_uri("url (foo)"), "foo");
}

TEST(ExtractUriTest, invalid)
{
    ASSERT_EQ(extract_uri("#foo"), "");
    ASSERT_EQ(extract_uri(" url(foo)"), "");
    ASSERT_EQ(extract_uri("url(#foo"), "");
    ASSERT_EQ(extract_uri("url('#foo'"), "");
    ASSERT_EQ(extract_uri("url('#foo)"), "");
    ASSERT_EQ(extract_uri("url #foo)"), "");
}

static char const *extract_end(char const *s)
{
    char const *end = nullptr;
    extract_uri(s, &end);
    return end;
}

TEST(ExtractUriTest, endptr)
{
    ASSERT_STREQ(extract_end(""), nullptr);
    ASSERT_STREQ(extract_end("url(invalid"), nullptr);
    ASSERT_STREQ(extract_end("url('invalid)"), nullptr);
    ASSERT_STREQ(extract_end("url(valid)"), "");
    ASSERT_STREQ(extract_end("url(valid)foo"), "foo");
    ASSERT_STREQ(extract_end("url('valid')bar"), "bar");
    ASSERT_STREQ(extract_end("url(  'valid'  )bar"), "bar");
    ASSERT_STREQ(extract_end("url(  valid  ) bar "), " bar ");
    ASSERT_STREQ(extract_end("url()bar"), "bar");
    ASSERT_STREQ(extract_end("url( )bar"), "bar");
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

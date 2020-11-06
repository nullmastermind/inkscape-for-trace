// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Test rebasing URI attributes
 */
/*
 * Authors:
 *   Thomas Holder
 *
 * Copyright (C) 2020 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "xml/rebase-hrefs.h"

#include <doc-per-case-test.h>
#include <gtest/gtest.h>

#include "object/sp-object.h"

using namespace Inkscape::XML;

#ifdef _WIN32
#define BASE_DIR_DIFFERENT_ROOT "D:\\foo\\bar"
#define BASE_DIR "C:\\foo\\bar"
#define BASE_URL "file:///C:/foo/bar"
#else
#define BASE_DIR_DIFFERENT_ROOT "/different/root"
#define BASE_DIR "/foo/bar"
#define BASE_URL "file://" BASE_DIR
#endif

static char const *const docString = R"""(
<svg xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink'>

<image id="img01" xlink:href=")""" BASE_URL R"""(/a.png" />
<image id="img02" xlink:href=")""" BASE_URL R"""(/c/b/a.png" />

<image id="img03" xlink:href="http://host/a.png" />
<image id="img04" xlink:href="data:text/plain,xxx" />

<image id="img05" xlink:href="" />
<image id="img06" xlink:href="#fragment" />
<image id="img07" xlink:href="?query" />
<image id="img08" xlink:href="/absolute/path" />
<image id="img09" xlink:href="//network/path" />

<image id="img10" xlink:href="b/a.png" />

<a id="a01" xlink:href=")""" BASE_URL R"""(/other.svg" />
<a id="a02" xlink:href="http://host/other.svg"></a>

</svg>
)""";

class ObjectTest : public DocPerCaseTest
{
public:
    std::unique_ptr<SPDocument> doc;

    ObjectTest() { doc.reset(SPDocument::createNewDocFromMem(docString, strlen(docString), false)); }

    void assert_nonfile_unchanged() const
    {
        ASSERT_STREQ(doc->getObjectById("img03")->getAttribute("xlink:href"), "http://host/a.png");
        ASSERT_STREQ(doc->getObjectById("img04")->getAttribute("xlink:href"), "data:text/plain,xxx");

        ASSERT_STREQ(doc->getObjectById("img05")->getAttribute("xlink:href"), "");
        ASSERT_STREQ(doc->getObjectById("img06")->getAttribute("xlink:href"), "#fragment");
        ASSERT_STREQ(doc->getObjectById("img07")->getAttribute("xlink:href"), "?query");
        ASSERT_STREQ(doc->getObjectById("img08")->getAttribute("xlink:href"), "/absolute/path");
        ASSERT_STREQ(doc->getObjectById("img09")->getAttribute("xlink:href"), "//network/path");
    }
};

TEST_F(ObjectTest, RebaseHrefs)
{
    rebase_hrefs(doc.get(), BASE_DIR G_DIR_SEPARATOR_S "c", false);
    assert_nonfile_unchanged();
    ASSERT_STREQ(doc->getObjectById("img01")->getAttribute("xlink:href"), "../a.png");
    ASSERT_STREQ(doc->getObjectById("img02")->getAttribute("xlink:href"), "b/a.png");

    // no base
    rebase_hrefs(doc.get(), nullptr, false);
    assert_nonfile_unchanged();
    ASSERT_STREQ(doc->getObjectById("img01")->getAttribute("xlink:href"), BASE_URL "/a.png");
    ASSERT_STREQ(doc->getObjectById("img02")->getAttribute("xlink:href"), BASE_URL "/c/b/a.png");

    rebase_hrefs(doc.get(), BASE_DIR, false);
    assert_nonfile_unchanged();
    ASSERT_STREQ(doc->getObjectById("img01")->getAttribute("xlink:href"), "a.png");
    ASSERT_STREQ(doc->getObjectById("img02")->getAttribute("xlink:href"), "c/b/a.png");

    // base with different root
    rebase_hrefs(doc.get(), BASE_DIR_DIFFERENT_ROOT, false);
    assert_nonfile_unchanged();
    ASSERT_STREQ(doc->getObjectById("img01")->getAttribute("xlink:href"), BASE_URL "/a.png");
    ASSERT_STREQ(doc->getObjectById("img02")->getAttribute("xlink:href"), BASE_URL "/c/b/a.png");
}

static std::map<std::string, std::string> rebase_attrs_test_helper(SPDocument *doc, char const *id,
                                                                   char const *old_base, char const *new_base)
{
    std::map<std::string, std::string> attributemap;
    auto attributes = rebase_href_attrs(old_base, new_base, doc->getObjectById(id)->getRepr()->attributeList());
    for (const auto &item : attributes) {
        attributemap[g_quark_to_string(item.key)] = item.value.pointer();
    }
    return attributemap;
}

TEST_F(ObjectTest, RebaseHrefAttrs)
{
    std::map<std::string, std::string> amap;

    amap = rebase_attrs_test_helper(doc.get(), "img01", BASE_DIR, BASE_DIR G_DIR_SEPARATOR_S "c");
    ASSERT_STREQ(amap["xlink:href"].c_str(), "../a.png");
    amap = rebase_attrs_test_helper(doc.get(), "img02", BASE_DIR, BASE_DIR G_DIR_SEPARATOR_S "c");
    ASSERT_STREQ(amap["xlink:href"].c_str(), "b/a.png");
    amap = rebase_attrs_test_helper(doc.get(), "img06", BASE_DIR, BASE_DIR G_DIR_SEPARATOR_S "c");
    ASSERT_STREQ(amap["xlink:href"].c_str(), "#fragment");
    amap = rebase_attrs_test_helper(doc.get(), "img10", BASE_DIR, BASE_DIR G_DIR_SEPARATOR_S "c");
    ASSERT_STREQ(amap["xlink:href"].c_str(), "../b/a.png");

    amap = rebase_attrs_test_helper(doc.get(), "a01", BASE_DIR, BASE_DIR G_DIR_SEPARATOR_S "c");
    ASSERT_STREQ(amap["xlink:href"].c_str(), "../other.svg");
    amap = rebase_attrs_test_helper(doc.get(), "a02", BASE_DIR, BASE_DIR G_DIR_SEPARATOR_S "c");
    ASSERT_STREQ(amap["xlink:href"].c_str(), "http://host/other.svg");

    amap = rebase_attrs_test_helper(doc.get(), "img01", BASE_DIR, BASE_DIR_DIFFERENT_ROOT);
    ASSERT_STREQ(amap["xlink:href"].c_str(), BASE_URL "/a.png");
}

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Test the API to the style element, access, read and write functions.
 *//*
 *
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2018 Authors
 *
 * Released under GNU GPL version 2 or later, read the file 'COPYING' for more information
 */

#include <gtest/gtest.h>
#include <doc-per-case-test.h>

#include <src/style.h>
#include <src/object/sp-root.h>
#include <src/object/sp-style-elem.h>

using namespace Inkscape;
using namespace Inkscape::XML;

class ObjectTest: public DocPerCaseTest {
public:
    ObjectTest() {
        char const *docString = "\
<svg xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink'>\
<style id='style01'>\
rect { fill: red; opacity:0.5; }\
#id1, #id2 { fill: red; stroke: #c0c0c0; }\
.cls1 { fill: red; opacity:1.0; }\
</style>\
<style id='style02'>\
rect { fill: green; opacity:1.0; }\
#id3, #id4 { fill: green; stroke: #606060; }\
.cls2 { fill: green; opacity:0.5; }\
</style>\
</svg>";
        doc = SPDocument::createNewDocFromMem(docString, static_cast<int>(strlen(docString)), false);
    }

    ~ObjectTest() override {
        doc->doUnref();
    }

    SPDocument *doc;
};

/*
 * Test sp-style-element objects created in document.
 */
TEST_F(ObjectTest, StyleElems) {
    ASSERT_TRUE(doc != nullptr);
    ASSERT_TRUE(doc->getRoot() != nullptr);

    SPRoot *root = doc->getRoot();
    ASSERT_TRUE(root->getRepr() != nullptr);

    SPStyleElem *one = dynamic_cast<SPStyleElem *>(doc->getObjectById("style01"));
    ASSERT_TRUE(one != nullptr);

    for(auto style: one->styles) {
        EXPECT_EQ(style->fill.get_value(), Glib::ustring("#ff0000"));
    }

    SPStyleElem *two = dynamic_cast<SPStyleElem *>(doc->getObjectById("style02"));
    ASSERT_TRUE(one != nullptr);

    for(auto style: two->styles) {
        EXPECT_EQ(style->fill.get_value(), Glib::ustring("#008000"));
    }
}

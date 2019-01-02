// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Combination style and object testing for cascading and flags.
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
#include <src/object/sp-rect.h>

using namespace Inkscape;
using namespace Inkscape::XML;

class ObjectTest: public DocPerCaseTest {
public:
    ObjectTest() {
        char const *docString = "\
<svg xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink'>\
<style>\
rect { fill: #808080; opacity:0.5; }\
.extra { opacity:1.0; }\
.overload { fill: #d0d0d0 !important; stroke: #c0c0c0 !important; }\
.font { font: italic bold 12px/30px Georgia, serif; }\
.exsize { stroke-width: 1ex; }\
.fosize { font-size: 15px; }\
</style>\
<g style='fill:blue; stroke-width:2px;font-size: 14px;'>\
  <rect id='one' style='fill:red; stroke:green;'/>\
  <rect id='two' style='stroke:green; stroke-width:4px;'/>\
  <rect id='three' class='extra' style='fill: #cccccc;'/>\
  <rect id='four' class='overload' style='fill:green;stroke:red !important;'/>\
  <rect id='five' class='font' style='font: 15px arial, sans-serif;'/>/\
  <rect id='six' style='stroke-width:1em;'/>\
  <rect id='seven' class='exsize'/>\
  <rect id='eight' class='fosize' style='stroke-width: 50%;'/>\
</g>\
</svg>";
        doc = SPDocument::createNewDocFromMem(docString, static_cast<int>(strlen(docString)), false);
    }

    ~ObjectTest() override {
        doc->doUnref();
    }

    SPDocument *doc;
};

/*
 * Test basic cascade values, that they are set correctly as we'd want to see them.
 */
TEST_F(ObjectTest, Styles) {
    ASSERT_TRUE(doc != nullptr);
    ASSERT_TRUE(doc->getRoot() != nullptr);

    SPRoot *root = doc->getRoot();
    ASSERT_TRUE(root->getRepr() != nullptr);
    ASSERT_TRUE(root->hasChildren());

    SPRect *one = dynamic_cast<SPRect *>(doc->getObjectById("one"));
    ASSERT_TRUE(one != nullptr);

    // TODO: Fix when Inkscape preserves colour names (i.e. 'red')
    EXPECT_EQ(one->style->fill.get_value(), Glib::ustring("#ff0000"));
    EXPECT_EQ(one->style->stroke.get_value(), Glib::ustring("#008000"));
    EXPECT_EQ(one->style->opacity.get_value(), Glib::ustring("0.5"));
    EXPECT_EQ(one->style->stroke_width.get_value(), Glib::ustring("2px"));

    SPRect *two = dynamic_cast<SPRect *>(doc->getObjectById("two"));
    ASSERT_TRUE(two != nullptr);

    EXPECT_EQ(two->style->fill.get_value(), Glib::ustring("#808080"));
    EXPECT_EQ(two->style->stroke.get_value(), Glib::ustring("#008000"));
    EXPECT_EQ(two->style->opacity.get_value(), Glib::ustring("0.5"));
    EXPECT_EQ(two->style->stroke_width.get_value(), Glib::ustring("4px"));

    SPRect *three = dynamic_cast<SPRect *>(doc->getObjectById("three"));
    ASSERT_TRUE(three != nullptr);

    EXPECT_EQ(three->style->fill.get_value(), Glib::ustring("#cccccc"));
    EXPECT_EQ(three->style->stroke.get_value(), Glib::ustring(""));
    EXPECT_EQ(three->style->opacity.get_value(), Glib::ustring("1"));
    EXPECT_EQ(three->style->stroke_width.get_value(), Glib::ustring("2px"));

    SPRect *four = dynamic_cast<SPRect *>(doc->getObjectById("four"));
    ASSERT_TRUE(four != nullptr);

    EXPECT_EQ(four->style->fill.get_value(), Glib::ustring("#d0d0d0"));
    EXPECT_EQ(four->style->stroke.get_value(), Glib::ustring("#ff0000"));
    EXPECT_EQ(four->style->opacity.get_value(), Glib::ustring("0.5"));
    EXPECT_EQ(four->style->stroke_width.get_value(), Glib::ustring("2px"));
}

/*
 * Test the origin flag for each of the values, should indicate where it came from.
 */
TEST_F(ObjectTest, StyleSource) {
    ASSERT_TRUE(doc != nullptr);
    ASSERT_TRUE(doc->getRoot() != nullptr);

    SPRoot *root = doc->getRoot();
    ASSERT_TRUE(root->getRepr() != nullptr);
    ASSERT_TRUE(root->hasChildren());

    SPRect *one = dynamic_cast<SPRect *>(doc->getObjectById("one"));
    ASSERT_TRUE(one != nullptr);

    EXPECT_EQ(one->style->fill.style_src, SP_STYLE_SRC_STYLE_PROP);
    EXPECT_EQ(one->style->stroke.style_src, SP_STYLE_SRC_STYLE_PROP);
    EXPECT_EQ(one->style->opacity.style_src, SP_STYLE_SRC_STYLE_SHEET);
    EXPECT_EQ(one->style->stroke_width.style_src, SP_STYLE_SRC_STYLE_PROP);

    SPRect *two = dynamic_cast<SPRect *>(doc->getObjectById("two"));
    ASSERT_TRUE(two != nullptr);

    EXPECT_EQ(two->style->fill.style_src, SP_STYLE_SRC_STYLE_SHEET);
    EXPECT_EQ(two->style->stroke.style_src, SP_STYLE_SRC_STYLE_PROP);
    EXPECT_EQ(two->style->opacity.style_src, SP_STYLE_SRC_STYLE_SHEET);
    EXPECT_EQ(two->style->stroke_width.style_src, SP_STYLE_SRC_STYLE_PROP);

    SPRect *three = dynamic_cast<SPRect *>(doc->getObjectById("three"));
    ASSERT_TRUE(three != nullptr);

    EXPECT_EQ(three->style->fill.style_src, SP_STYLE_SRC_STYLE_PROP);
    EXPECT_EQ(three->style->stroke.style_src, SP_STYLE_SRC_STYLE_PROP);
    EXPECT_EQ(three->style->opacity.style_src, SP_STYLE_SRC_STYLE_SHEET);
    EXPECT_EQ(three->style->stroke_width.style_src, SP_STYLE_SRC_STYLE_PROP);

    SPRect *four = dynamic_cast<SPRect *>(doc->getObjectById("four"));
    ASSERT_TRUE(four != nullptr);

    EXPECT_EQ(four->style->fill.style_src, SP_STYLE_SRC_STYLE_SHEET);
    EXPECT_EQ(four->style->stroke.style_src, SP_STYLE_SRC_STYLE_PROP);
    EXPECT_EQ(four->style->opacity.style_src, SP_STYLE_SRC_STYLE_SHEET);
    EXPECT_EQ(four->style->stroke_width.style_src, SP_STYLE_SRC_STYLE_PROP);
}

/*
 * Test the breaking up of the font property and recreation into separate properties.
 */
TEST_F(ObjectTest, StyleFont) {
    ASSERT_TRUE(doc != nullptr);
    ASSERT_TRUE(doc->getRoot() != nullptr);

    SPRoot *root = doc->getRoot();
    ASSERT_TRUE(root->getRepr() != nullptr);
    ASSERT_TRUE(root->hasChildren());

    SPRect *five = dynamic_cast<SPRect *>(doc->getObjectById("five"));
    ASSERT_TRUE(five != nullptr);

    // Font property is ALWAYS unset as it's converted into specific font css properties
    EXPECT_EQ(five->style->font.get_value(), Glib::ustring(""));
    EXPECT_EQ(five->style->font_size.get_value(), Glib::ustring("12px"));
    EXPECT_EQ(five->style->font_weight.get_value(), Glib::ustring("bold"));
    EXPECT_EQ(five->style->font_style.get_value(), Glib::ustring("italic"));
    EXPECT_EQ(five->style->font_family.get_value(), Glib::ustring("arial, sans-serif"));
}

/*
 * Test the consumption of font dependent lengths in SPILength, e.g. EM, EX and % units
 */
TEST_F(ObjectTest, StyleFontSizes) {
    ASSERT_TRUE(doc != nullptr);
    ASSERT_TRUE(doc->getRoot() != nullptr);

    SPRoot *root = doc->getRoot();
    ASSERT_TRUE(root->getRepr() != nullptr);
    ASSERT_TRUE(root->hasChildren());

    SPRect *six = dynamic_cast<SPRect *>(doc->getObjectById("six"));
    ASSERT_TRUE(six != nullptr);

    EXPECT_EQ(six->style->stroke_width.get_value(), Glib::ustring("1em"));
    EXPECT_EQ(six->style->stroke_width.computed, 14);

    SPRect *seven = dynamic_cast<SPRect *>(doc->getObjectById("seven"));
    ASSERT_TRUE(seven != nullptr);

    EXPECT_EQ(seven->style->stroke_width.get_value(), Glib::ustring("1ex"));
    EXPECT_EQ(seven->style->stroke_width.computed, 7);

    SPRect *eight = dynamic_cast<SPRect *>(doc->getObjectById("eight"));
    ASSERT_TRUE(eight != nullptr);

    EXPECT_EQ(eight->style->stroke_width.get_value(), Glib::ustring("50%"));
    EXPECT_EQ(eight->style->stroke_width.computed, 1); // Is this right?
}

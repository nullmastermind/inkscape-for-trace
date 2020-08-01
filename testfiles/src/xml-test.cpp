// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Test xml node
 */
/*
 * Authors:
 *   Ted Gould
 *
 * Copyright (C) 2020 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "gtest/gtest.h"
#include "xml/repr.h"

TEST(XmlTest, nodeiter)
{
    auto testdoc = std::shared_ptr<Inkscape::XML::Document>(sp_repr_read_buf("<svg><g/></svg>", SP_SVG_NS_URI));
    ASSERT_TRUE(testdoc);

    auto count = 0;
    for (auto child : *testdoc->root()->firstChild()) {
        ASSERT_STREQ(child->name(), "svg:g");
        count++;
    }
    ASSERT_EQ(count, 1);

    testdoc =
        std::shared_ptr<Inkscape::XML::Document>(sp_repr_read_buf("<svg><g/><g/><g><g/></g></svg>", SP_SVG_NS_URI));
    ASSERT_TRUE(testdoc);

    count = 0;
    for (auto child : *testdoc->root()->firstChild()) {
        ASSERT_STREQ(child->name(), "svg:g");
        count++;
    }
    ASSERT_EQ(count, 3);

    testdoc =
        std::shared_ptr<Inkscape::XML::Document>(sp_repr_read_buf("<svg><g/><g/><g><path/></g></svg>", SP_SVG_NS_URI));
    ASSERT_TRUE(testdoc);

    auto path = std::list<std::string>{"svg:g", "svg:path"};
    auto found = testdoc->root()->findChildPath(path);
    ASSERT_NE(found, nullptr);
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

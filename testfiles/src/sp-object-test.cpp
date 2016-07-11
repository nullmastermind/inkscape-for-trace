/*
 * Multiindex container for selection
 *
 * Authors:
 *   Adrian Boguszewski
 *
 * Copyright (C) 2016 Adrian Boguszewski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#include <gtest/gtest.h>
#include <src/sp-object.h>
#include <src/sp-item.h>
#include <src/xml/node.h>
#include <src/xml/text-node.h>
#include <doc-per-case-test.h>
#include <src/util/share.h>
#include <src/xml/simple-document.h>

using namespace Inkscape;
using namespace Inkscape::XML;

class SPObjectTest: public DocPerCaseTest {
public:
    SPObjectTest() {
        a = new SPItem();
        b = new SPItem();
        c = new SPItem();
        d = new SPItem();
        e = new SPItem();
        auto sd = new SimpleDocument();
        auto et = new TextNode(Util::share_string("e"), sd);
        auto dt = new TextNode(Util::share_string("d"), sd);
        auto ct = new TextNode(Util::share_string("c"), sd);
        auto bt = new TextNode(Util::share_string("b"), sd);
        auto at = new TextNode(Util::share_string("a"), sd);
        e->invoke_build(_doc, et, 0);
        d->invoke_build(_doc, dt, 0);
        c->invoke_build(_doc, ct, 0);
        b->invoke_build(_doc, bt, 0);
        a->invoke_build(_doc, at, 0);
    }
    ~SPObjectTest() {
        delete a;
        delete b;
        delete c;
        delete d;
        delete e;
    }
    SPObject* a;
    SPObject* b;
    SPObject* c;
    SPObject* d;
    SPObject* e;
};

TEST_F(SPObjectTest, Basics) {
    a->attach(c, a->lastChild());
    a->attach(b, nullptr);
    a->attach(d, c);
    EXPECT_TRUE(a->hasChildren());
    EXPECT_EQ(d, a->lastChild());
    auto children = a->childList(false);
    EXPECT_EQ(3, children.size());
    EXPECT_EQ(b, children[0]);
    EXPECT_EQ(c, children[1]);
    EXPECT_EQ(d, children[2]);
    a->attach(b, a->lastChild());
    EXPECT_EQ(3, a->_children.size());
    a->reorder(b, b);
    EXPECT_EQ(3, a->_children.size());
    EXPECT_EQ(b, &a->_children.front());
    EXPECT_EQ(d, &a->_children.back());
    a->reorder(b, d);
    EXPECT_EQ(3, a->_children.size());
    EXPECT_EQ(c, &a->_children.front());
    EXPECT_EQ(b, &a->_children.back());
    a->reorder(d, nullptr);
    EXPECT_EQ(3, a->_children.size());
    EXPECT_EQ(d, &a->_children.front());
    EXPECT_EQ(b, &a->_children.back());
    a->reorder(c, b);
    EXPECT_EQ(3, a->_children.size());
    EXPECT_EQ(d, &a->_children.front());
    EXPECT_EQ(c, &a->_children.back());
    a->detach(b);
    EXPECT_EQ(c, a->lastChild());
    children = a->childList(false);
    EXPECT_EQ(2, children.size());
    EXPECT_EQ(d, children[0]);
    EXPECT_EQ(c, children[1]);
    a->detach(b);
    EXPECT_EQ(2, a->childList(false).size());
    a->releaseReferences();
    EXPECT_FALSE(a->hasChildren());
}

TEST_F(SPObjectTest, Advanced) {
    a->attach(b, a->lastChild());
    a->attach(c, a->lastChild());
    a->attach(d, a->lastChild());
    a->attach(e, a->lastChild());
    EXPECT_EQ(e, a->get_child_by_repr(e->getRepr()));
    EXPECT_EQ(c, a->get_child_by_repr(c->getRepr()));
}

TEST_F(SPObjectTest, Tmp) {
    a->attach(b, a->lastChild());
    a->attach(c, a->lastChild());
    a->attach(d, a->lastChild());
    a->attach(e, a->lastChild());
    EXPECT_TRUE(a->hasChildren());
    a->releaseReferences();
    EXPECT_FALSE(a->hasChildren());
}
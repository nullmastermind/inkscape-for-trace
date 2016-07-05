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
#include <doc-per-case-test.h>

class SPObjectTest: public DocPerCaseTest {
public:
    SPObjectTest() {
        a = new SPItem();
        b = new SPItem();
        c = new SPItem();
        d = new SPItem();
        e = new SPItem();
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
    d->invoke_build(_doc, _doc->rroot, 1);
    c->invoke_build(_doc, _doc->rroot, 1);
    b->invoke_build(_doc, _doc->rroot, 1);
    a->attach(b, a->lastChild());
    a->attach(c, a->lastChild());
    a->attach(d, a->lastChild());
    EXPECT_TRUE(a->hasChildren());
    EXPECT_EQ(d, a->lastChild());
    auto children = a->childList(false);
    EXPECT_EQ(3, children.size());
    EXPECT_EQ(b, children[0]);
    EXPECT_EQ(c, children[1]);
    EXPECT_EQ(d, children[2]);
    b->reorder(d);
    children = a->childList(false);
    EXPECT_EQ(3, children.size());
    EXPECT_EQ(c, children[0]);
    EXPECT_EQ(d, children[1]);
    EXPECT_EQ(b, children[2]);
    a->detach(d);
    EXPECT_EQ(b, a->lastChild());
    children = a->childList(false);
    EXPECT_EQ(2, children.size());
    EXPECT_EQ(c, children[0]);
    EXPECT_EQ(b, children[1]);
    a->detach(c);
    a->detach(b);
    EXPECT_FALSE(a->hasChildren());
}

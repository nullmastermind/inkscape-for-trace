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
#include "object-set.h"
#include "document.h"
#include "xml/simple-document.h"

class ObjectSetTest: public testing::Test {
public:
    ObjectSetTest() {
        A = new SPObject();
        B = new SPObject();
        C = new SPObject();
        D = new SPObject();
        E = new SPObject();
        F = new SPObject();
        G = new SPObject();
        H = new SPObject();
        X = new SPObject();
    }
    ~ObjectSetTest() {
        delete X;
        delete H;
        delete G;
        delete F;
        delete E;
        delete D;
        delete C;
        delete B;
        delete A;
    }
    SPObject* A;
    SPObject* B;
    SPObject* C;
    SPObject* D;
    SPObject* E;
    SPObject* F;
    SPObject* G;
    SPObject* H;
    SPObject* X;
    ObjectSet set;
    ObjectSet set2;
};

TEST_F(ObjectSetTest, Basics) {
    EXPECT_EQ(0, set.size());
    set.add(A);
    EXPECT_EQ(1, set.size());
    EXPECT_TRUE(set.contains(A));
    set.add(B);
    set.add(C);
    EXPECT_EQ(3, set.size());
    EXPECT_TRUE(set.contains(B));
    EXPECT_TRUE(set.contains(C));
    EXPECT_FALSE(set.contains(D));
    EXPECT_FALSE(set.contains(X));
    set.remove(A);
    EXPECT_EQ(2, set.size());
    EXPECT_FALSE(set.contains(A));
    set.clear();
    EXPECT_EQ(0, set.size());
}

TEST_F(ObjectSetTest, Autoremoving) {
    SPObject* Q = new SPObject();
    // TODO temporary
    SPDocument *document = new SPDocument();
    Inkscape::XML::Node *rroot = new Inkscape::XML::SimpleDocument();
    Q->invoke_build(document, rroot, 0);
    set.add(Q);
    EXPECT_TRUE(set.contains(Q));
    EXPECT_EQ(1, set.size());
    Q->releaseReferences();
    EXPECT_EQ(0, set.size());
}

TEST_F(ObjectSetTest, BasicDescendants) {
    A->attach(B, nullptr);
    B->attach(C, nullptr);
    A->attach(D, nullptr);
    bool resultB = set.add(B);
    bool resultB2 = set.add(B);
    EXPECT_TRUE(resultB);
    EXPECT_FALSE(resultB2);
    EXPECT_TRUE(set.contains(B));
    bool resultC = set.add(C);
    EXPECT_FALSE(resultC);
    EXPECT_FALSE(set.contains(C));
    EXPECT_EQ(1, set.size());
    bool resultA = set.add(A);
    EXPECT_TRUE(resultA);
    EXPECT_EQ(1, set.size());
    EXPECT_TRUE(set.contains(A));
    EXPECT_FALSE(set.contains(B));
}

TEST_F(ObjectSetTest, AdvancedDescendants) {
    A->attach(B, nullptr);
    A->attach(C, nullptr);
    A->attach(X, nullptr);
    B->attach(D, nullptr);
    B->attach(E, nullptr);
    C->attach(F, nullptr);
    C->attach(G, nullptr);
    C->attach(H, nullptr);
    set.add(A);
//    bool resultF = set.remove(F);
//    EXPECT_TRUE(resultF);
    // TODO temporary
    set.remove(F);
    EXPECT_EQ(4, set.size());
    EXPECT_FALSE(set.contains(F));
    EXPECT_TRUE(set.contains(B));
    EXPECT_TRUE(set.contains(G));
    EXPECT_TRUE(set.contains(H));
    EXPECT_TRUE(set.contains(X));
    bool resultF2 = set.add(F);
    EXPECT_TRUE(resultF2);
    EXPECT_EQ(1, set.size());
    EXPECT_TRUE(set.contains(A));
}

TEST_F(ObjectSetTest, Removing) {
    A->attach(B, nullptr);
    A->attach(C, nullptr);
    A->attach(X, nullptr);
    B->attach(D, nullptr);
    B->attach(E, nullptr);
    C->attach(F, nullptr);
    C->attach(G, nullptr);
    C->attach(H, nullptr);
//    bool removeH = set.remove(H);
//    EXPECT_FALSE(removeH);
    // TODO temporary
    set.remove(H);
    set.add(A);
//    bool removeX = set.remove(X);
//    EXPECT_TRUE(removeX);
    // TODO temporary
    set.remove(X);
    EXPECT_EQ(2, set.size());
    EXPECT_TRUE(set.contains(B));
    EXPECT_TRUE(set.contains(C));
    EXPECT_FALSE(set.contains(X));
    EXPECT_FALSE(set.contains(A));
//    bool removeX2 = set.remove(X);
//    EXPECT_FALSE(removeX2);
    // TODO temporary
    set.remove(X);
    EXPECT_EQ(2, set.size());
//    bool removeA = set.remove(A);
//    EXPECT_FALSE(removeA);
    // TODO temporary
    set.remove(A);
    EXPECT_EQ(2, set.size());
//    bool removeC = set.remove(C);
//    EXPECT_TRUE(removeC);
    // TODO temporary
    set.remove(C);
    EXPECT_EQ(1, set.size());
    EXPECT_TRUE(set.contains(B));
    EXPECT_FALSE(set.contains(C));
}

TEST_F(ObjectSetTest, TwoSets) {
    SPObject* Q = new SPObject();
    // TODO temporary
    SPDocument *document = new SPDocument();
    Inkscape::XML::Node *rroot = new Inkscape::XML::SimpleDocument();
    Q->invoke_build(document, rroot, 0);
    A->attach(B, nullptr);
    A->attach(Q, nullptr);
    set.add(A);
    set2.add(A);
    EXPECT_EQ(1, set.size());
    EXPECT_EQ(1, set2.size());
    set.remove(B);
    EXPECT_EQ(1, set.size());
    EXPECT_TRUE(set.contains(Q));
    EXPECT_EQ(1, set2.size());
    EXPECT_TRUE(set2.contains(A));
    Q->releaseReferences();
    EXPECT_EQ(0, set.size());
    EXPECT_EQ(1, set2.size());
    EXPECT_TRUE(set2.contains(A));
}

TEST_F(ObjectSetTest, SetRemoving) {
    ObjectSet *objectSet = new ObjectSet();
    A->attach(B, nullptr);
    objectSet->add(A);
    objectSet->add(C);
    EXPECT_EQ(2, objectSet->size());
    delete objectSet;
    EXPECT_STREQ(nullptr, A->getId());
    EXPECT_STREQ(nullptr, C->getId());
}

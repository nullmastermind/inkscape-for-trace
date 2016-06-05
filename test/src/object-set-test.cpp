#include <gtest/gtest.h>
#include "object-set.h"

class ObjectSetTest: public testing::Test {
public:
    ObjectSetTest() {
        A = new Object("A");
        B = new Object("B");
        C = new Object("C");
        D = new Object("D");
        E = new Object("E");
        F = new Object("F");
        G = new Object("G");
        H = new Object("H");
        X = new Object("X");
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
    Object* A;
    Object* B;
    Object* C;
    Object* D;
    Object* E;
    Object* F;
    Object* G;
    Object* H;
    Object* X;
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
    Object* Q = new Object("Q");
    set.add(Q);
    EXPECT_TRUE(set.contains(Q));
    EXPECT_EQ(1, set.size());
    delete Q;
    EXPECT_EQ(0, set.size());
}

TEST_F(ObjectSetTest, BasicDescendants) {
    A->addChild(B);
    B->addChild(C);
    A->addChild(D);
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
    A->addChild(B);
    A->addChild(C);
    A->addChild(X);
    B->addChild(D);
    B->addChild(E);
    C->addChild(F);
    C->addChild(G);
    C->addChild(H);
    set.add(A);
    bool resultF = set.remove(F);
    EXPECT_TRUE(resultF);
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
    A->addChild(B);
    A->addChild(C);
    A->addChild(X);
    B->addChild(D);
    B->addChild(E);
    C->addChild(F);
    C->addChild(G);
    C->addChild(H);
    bool removeH = set.remove(H);
    EXPECT_FALSE(removeH);
    set.add(A);
    bool removeX = set.remove(X);
    EXPECT_TRUE(removeX);
    EXPECT_EQ(2, set.size());
    EXPECT_TRUE(set.contains(B));
    EXPECT_TRUE(set.contains(C));
    EXPECT_FALSE(set.contains(X));
    EXPECT_FALSE(set.contains(A));
    bool removeX2 = set.remove(X);
    EXPECT_FALSE(removeX2);
    EXPECT_EQ(2, set.size());
    bool removeA = set.remove(A);
    EXPECT_FALSE(removeA);
    EXPECT_EQ(2, set.size());
    bool removeC = set.remove(C);
    EXPECT_TRUE(removeC);
    EXPECT_EQ(1, set.size());
    EXPECT_TRUE(set.contains(B));
    EXPECT_FALSE(set.contains(C));
}

TEST_F(ObjectSetTest, TwoSets) {
    Object* Q = new Object("Q");
    A->addChild(B);
    A->addChild(Q);
    set.add(A);
    set2.add(A);
    EXPECT_EQ(1, set.size());
    EXPECT_EQ(1, set2.size());
    set.remove(B);
    EXPECT_EQ(1, set.size());
    EXPECT_TRUE(set.contains(Q));
    EXPECT_EQ(1, set2.size());
    EXPECT_TRUE(set2.contains(A));
    delete Q;
    EXPECT_EQ(0, set.size());
    EXPECT_EQ(1, set2.size());
    EXPECT_TRUE(set2.contains(A));
}

TEST_F(ObjectSetTest, SetRemoving) {
    ObjectSet *objectSet = new ObjectSet();
    A->addChild(B);
    objectSet->add(A);
    objectSet->add(C);
    EXPECT_EQ(2, objectSet->size());
    delete objectSet;
    EXPECT_EQ("A", A->getName());
    EXPECT_EQ("C", C->getName());
}

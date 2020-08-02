// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Test utilities from src/util
 */
/*
 * Authors:
 *   Thomas Holder
 *
 * Copyright (C) 2020 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "gtest/gtest.h"
#include "util/longest-common-suffix.h"

TEST(UtilTest, NearestCommonAncestor)
{
    // TODO change name of implementation to `nearest_common_ancestor`
    // TODO remove `pred` argument
#define nearest_common_ancestor(a, b, c) \
    Inkscape::Algorithms::longest_common_suffix(a, b, c, [](Node const &lhs, Node const &rhs) { return &lhs == &rhs; })

    // simple node with a parent
    struct Node
    {
        Node const *parent;
        Node(Node const *p) : parent(p){};
        Node(Node const &other) = delete;
    };

    // iterator which traverses towards the root node
    struct iter
    {
        Node const *node;
        iter(Node const &n) : node(&n) {}
        bool operator==(iter const &rhs) const { return node == rhs.node; }
        bool operator!=(iter const &rhs) const { return node != rhs.node; }
        iter &operator++()
        {
            node = node->parent;
            return *this;
        }

        // TODO remove, the implementation should not require this
        Node const &operator*() const { return *node; }
    };

    // construct a tree
    auto const node0 = Node(nullptr);
    auto const node1 = Node(&node0);
    auto const node2 = Node(&node1);
    auto const node3a = Node(&node2);
    auto const node4a = Node(&node3a);
    auto const node5a = Node(&node4a);
    auto const node3b = Node(&node2);
    auto const node4b = Node(&node3b);
    auto const node5b = Node(&node4b);

    // start at each node from 5a to 0 (first argument)
    ASSERT_EQ(nearest_common_ancestor(iter(node5a), iter(node5b), iter(node0)), iter(node2));
    ASSERT_EQ(nearest_common_ancestor(iter(node4a), iter(node5b), iter(node0)), iter(node2));
    ASSERT_EQ(nearest_common_ancestor(iter(node3a), iter(node5b), iter(node0)), iter(node2));
    ASSERT_EQ(nearest_common_ancestor(iter(node2), iter(node5b), iter(node0)), iter(node2));
    ASSERT_EQ(nearest_common_ancestor(iter(node1), iter(node5b), iter(node0)), iter(node1));
    ASSERT_EQ(nearest_common_ancestor(iter(node0), iter(node5b), iter(node0)), iter(node0));

    // start at each node from 5b to 0 (second argument)
    ASSERT_EQ(nearest_common_ancestor(iter(node5a), iter(node5b), iter(node0)), iter(node2));
    ASSERT_EQ(nearest_common_ancestor(iter(node5a), iter(node4b), iter(node0)), iter(node2));
    ASSERT_EQ(nearest_common_ancestor(iter(node5a), iter(node3b), iter(node0)), iter(node2));
    ASSERT_EQ(nearest_common_ancestor(iter(node5a), iter(node2), iter(node0)), iter(node2));
    ASSERT_EQ(nearest_common_ancestor(iter(node5a), iter(node1), iter(node0)), iter(node1));
    ASSERT_EQ(nearest_common_ancestor(iter(node5a), iter(node0), iter(node0)), iter(node0));

    // identity (special case in implementation)
    ASSERT_EQ(nearest_common_ancestor(iter(node5a), iter(node5a), iter(node0)), iter(node5a));

    // identical parents (special case in implementation)
    ASSERT_EQ(nearest_common_ancestor(iter(node3a), iter(node3b), iter(node0)), iter(node2));
}

// vim: filetype=cpp:expandtab:shiftwidth=4:softtabstop=4:fileencoding=utf-8:textwidth=99 :

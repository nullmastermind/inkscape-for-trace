// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests migrated from cxxtest
 *
 * Authors:
 *   Adrian Boguszewski
 *
 * Copyright (C) 2018 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtest/gtest.h>
#include <doc-per-case-test.h>
#include <src/object/sp-root.h>
#include <src/object/sp-path.h>

using namespace Inkscape;
using namespace Inkscape::XML;

class ObjectTest: public DocPerCaseTest {
public:
    ObjectTest() {
        // Sample document
        // svg:svg
        // svg:defs
        // svg:path
        // svg:linearGradient
        // svg:stop
        // svg:filter
        // svg:feGaussianBlur (feel free to implement for other filters)
        // svg:clipPath
        // svg:rect
        // svg:g
        // svg:use
        // svg:circle
        // svg:ellipse
        // svg:text
        // svg:polygon
        // svg:polyline
        // svg:image
        // svg:line
        char const *docString =
                "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n"
                "<!-- just a comment -->\n"
                "<title id=\"title\">SVG test</title>\n"
                "<defs>\n"
                "  <path id=\"P\" d=\"M -21,-4 -5,0 -18,12 -3,4 -4,21 0,5 12,17 4,2 21,3 5,-1 17,-12 2,-4 3,-21 -1,-5 -12,-18 -4,-3z\"/>\n"
                "  <linearGradient id=\"LG\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"0%\">\n"
                "    <stop offset=\"0%\" style=\"stop-color:#ffff00;stop-opacity:1\"/>\n"
                "    <stop offset=\"100%\" style=\"stop-color:red;stop-opacity:1\"/>\n"
                "  </linearGradient>\n"
                "  <clipPath id=\"clip\" clipPathUnits=\"userSpaceOnUse\">\n"
                "    <rect x=\"10\" y=\"10\" width=\"100\" height=\"100\"/>\n"
                "  </clipPath>\n"
                "  <filter style=\"color-interpolation-filters:sRGB\" id=\"filter\" x=\"-0.15\" width=\"1.34\" y=\"0\" height=\"1\">\n"
                "    <feGaussianBlur stdDeviation=\"4.26\"/>\n"
                "  </filter>\n"
                "</defs>\n"

                "<g id=\"G\" transform=\"skewX(10.5) translate(9,5)\">\n"
                "  <use id=\"U\" xlink:href=\"#P\" opacity=\"0.5\" fill=\"#1dace3\" transform=\"rotate(4)\"/>\n"
                "  <circle id=\"C\" cx=\"45.5\" cy=\"67\" r=\"23\" fill=\"#000\"/>\n"
                "  <ellipse id=\"E\" cx=\"200\" cy=\"70\" rx=\"85\" ry=\"55\" fill=\"url(#LG)\"/>\n"
                "  <text id=\"T\" fill=\"#fff\" style=\"font-size:45;font-family:Verdana\" x=\"150\" y=\"86\">TEST</text>\n"
                "  <polygon id=\"PG\" points=\"60,20 100,40 100,80 60,100 20,80 20,40\" clip-path=\"url(#clip)\" filter=\"url(#filter)\"/>\n"
                "  <polyline id=\"PL\" points=\"0,40 40,40 40,80 80,80 80,120 120,120 120,160\" style=\"fill:none;stroke:red;stroke-width:4\"/>\n"
                "  <image id=\"I\" xlink:href=\"data:image/svg+xml;base64,PHN2ZyBoZWlnaHQ9IjE4MCIgd2lkdGg9IjUwMCI+PHBhdGggZD0iTTAsNDAgNDAsNDAgNDAs" // this is one line
                "ODAgODAsODAgODAsMTIwIDEyMCwxMjAgMTIwLDE2MCIgc3R5bGU9ImZpbGw6d2hpdGU7c3Ryb2tlOnJlZDtzdHJva2Utd2lkdGg6NCIvPjwvc3ZnPgo=\"/>\n"
                "  <line id=\"L\" x1=\"20\" y1=\"100\" x2=\"100\" y2=\"20\" stroke=\"black\" stroke-width=\"2\"/>\n"
                "</g>\n"
                "</svg>\n";
        doc = SPDocument::createNewDocFromMem(docString, static_cast<int>(strlen(docString)), false);
    }

    ~ObjectTest() {
        doc->doUnref();
    }

    SPDocument *doc;
};

TEST_F(ObjectTest, Clones) {
    ASSERT_TRUE(doc != nullptr);
    ASSERT_TRUE(doc->getRoot() != nullptr);

    SPRoot *root = doc->getRoot();
    ASSERT_TRUE(root->getRepr() != nullptr);
    ASSERT_TRUE(root->hasChildren());

    SPPath *path = dynamic_cast<SPPath *>(doc->getObjectById("P"));
    ASSERT_TRUE(path != nullptr);

    Node *node = path->getRepr();
    ASSERT_TRUE(node != nullptr);

    Document *xml_doc = node->document();
    ASSERT_TRUE(xml_doc != nullptr);

    Node *parent = node->parent();
    ASSERT_TRUE(parent != nullptr);

    const size_t num_clones = 1000;
    std::string href = std::string("#") + std::string(path->getId());
    std::vector<Node *> clones(num_clones, nullptr);

    // Create num_clones clones of this path and stick them in the document
    for (size_t i = 0; i < num_clones; ++i) {
        Node *clone = xml_doc->createElement("svg:use");
        Inkscape::GC::release(clone);
        clone->setAttribute("xlink:href", href.c_str());
        parent->addChild(clone, node);
        clones[i] = clone;
    }

    // Remove those clones
    for (size_t i = 0; i < num_clones; ++i) {
        parent->removeChild(clones[i]);
    }
}

TEST_F(ObjectTest, Grouping) {
    ASSERT_TRUE(doc != nullptr);
    ASSERT_TRUE(doc->getRoot() != nullptr);

    SPRoot *root = doc->getRoot();
    ASSERT_TRUE(root->getRepr() != nullptr);
    ASSERT_TRUE(root->hasChildren());

    SPGroup *group = dynamic_cast<SPGroup *>(doc->getObjectById("G"));

    ASSERT_TRUE(group != nullptr);

    Node *node = group->getRepr();
    ASSERT_TRUE(node != nullptr);

    Document *xml_doc = node->document();
    ASSERT_TRUE(xml_doc != nullptr);

    const size_t num_elements = 1000;

    Node *new_group = xml_doc->createElement("svg:g");
    Inkscape::GC::release(new_group);
    node->addChild(new_group, nullptr);

    std::vector<Node *> elements(num_elements, nullptr);

    for (size_t i = 0; i < num_elements; ++i) {
        Node *circle = xml_doc->createElement("svg:circle");
        Inkscape::GC::release(circle);
        circle->setAttribute("cx", "2048");
        circle->setAttribute("cy", "1024");
        circle->setAttribute("r", "1.5");
        new_group->addChild(circle, nullptr);
        elements[i] = circle;
    }

    SPGroup *n_group = dynamic_cast<SPGroup *>(group->get_child_by_repr(new_group));
    ASSERT_TRUE(n_group != nullptr);

    std::vector<SPItem*> ch;
    sp_item_group_ungroup(n_group, ch, false);

    // Remove those elements
    for (size_t i = 0; i < num_elements; ++i) {
        elements[i]->parent()->removeChild(elements[i]);
    }

}

TEST_F(ObjectTest, Objects) {
    ASSERT_TRUE(doc != nullptr);
    ASSERT_TRUE(doc->getRoot() != nullptr);

    SPRoot *root = doc->getRoot();
    ASSERT_TRUE(root->getRepr() != nullptr);
    ASSERT_TRUE(root->hasChildren());

    SPPath *path = dynamic_cast<SPPath *>(doc->getObjectById("P"));
    ASSERT_TRUE(path != nullptr);

    // Test parent behavior
    SPObject *child = root->firstChild();
    ASSERT_TRUE(child != nullptr);

    EXPECT_EQ(root, child->parent);
    EXPECT_EQ(doc, child->document);
    EXPECT_TRUE(root->isAncestorOf(child));

    // Test list behavior
    SPObject *next = child->getNext();
    SPObject *prev = next;
    EXPECT_EQ(child, next->getPrev());

    prev = next;
    next = next->getNext();
    while (next != nullptr) {
        // Walk the list
        EXPECT_EQ(prev, next->getPrev());
        prev = next;
        next = next->getNext();
    }

    // Test hrefcount
    EXPECT_TRUE(path->isReferenced());
}
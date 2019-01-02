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
#include <src/object/sp-gradient.h>
#include <src/attributes.h>
#include <2geom/transforms.h>
#include <src/xml/node.h>
#include <src/xml/simple-document.h>
#include <src/svg/svg.h>

using namespace Inkscape;
using namespace Inkscape::XML;

class SPGradientTest: public DocPerCaseTest {
public:
    SPGradientTest() {
        DocPerCaseTest::SetUpTestCase();
        gr = new SPGradient();
    }

    ~SPGradientTest() override {
        delete gr;
        DocPerCaseTest::TearDownTestCase();
    }

    SPGradient *gr;
};

TEST_F(SPGradientTest, Init) {
    ASSERT_TRUE(gr != nullptr);
    EXPECT_TRUE(gr->gradientTransform.isIdentity());
    EXPECT_TRUE(Geom::are_near(Geom::identity(), gr->gradientTransform));
}

TEST_F(SPGradientTest, SetGradientTransform) {
    SP_OBJECT(gr)->document = _doc;

    SP_OBJECT(gr)->setKeyValue(SP_ATTR_GRADIENTTRANSFORM, "translate(5, 8)");
    EXPECT_TRUE(Geom::are_near(Geom::Affine(Geom::Translate(5.0, 8.0)), gr->gradientTransform));

    SP_OBJECT(gr)->setKeyValue(SP_ATTR_GRADIENTTRANSFORM, "");
    EXPECT_TRUE(Geom::are_near(Geom::identity(), gr->gradientTransform));

    SP_OBJECT(gr)->setKeyValue(SP_ATTR_GRADIENTTRANSFORM, "rotate(90)");
    EXPECT_TRUE(Geom::are_near(Geom::Affine(Geom::Rotate::from_degrees(90.0)), gr->gradientTransform));
}

TEST_F(SPGradientTest, Write) {
    SP_OBJECT(gr)->document = _doc;

    SP_OBJECT(gr)->setKeyValue(SP_ATTR_GRADIENTTRANSFORM, "matrix(0, 1, -1, 0, 0, 0)");
    Document *xml_doc = _doc->getReprDoc();

    ASSERT_TRUE(xml_doc != nullptr);

    Node *repr = xml_doc->createElement("svg:radialGradient");
    SP_OBJECT(gr)->updateRepr(xml_doc, repr, SP_OBJECT_WRITE_ALL);

    gchar const *tr = repr->attribute("gradientTransform");
    Geom::Affine svd;
    bool const valid = sp_svg_transform_read(tr, &svd);

    EXPECT_TRUE(valid);
    EXPECT_TRUE(Geom::are_near(Geom::Affine(Geom::Rotate::from_degrees(90.0)), svd));
}

TEST_F(SPGradientTest, GetG2dGetGs2dSetGs2) {
    SP_OBJECT(gr)->document = _doc;

    Geom::Affine grXform(2, 1,
                         1, 3,
                         4, 6);
    gr->gradientTransform = grXform;

    Geom::Rect unit_rect(Geom::Point(0, 0), Geom::Point(1, 1));
    {
        Geom::Affine g2d(gr->get_g2d_matrix(Geom::identity(), unit_rect));
        Geom::Affine gs2d(gr->get_gs2d_matrix(Geom::identity(), unit_rect));
        EXPECT_TRUE(Geom::are_near(Geom::identity(), g2d));
        EXPECT_TRUE(Geom::are_near(gs2d, gr->gradientTransform * g2d, 1e-12));

        gr->set_gs2d_matrix(Geom::identity(), unit_rect, gs2d);
        EXPECT_TRUE(Geom::are_near(gr->gradientTransform, grXform, 1e-12));
    }

    gr->gradientTransform = grXform;
    Geom::Affine funny(2, 3,
                       4, 5,
                       6, 7);
    {
        Geom::Affine g2d(gr->get_g2d_matrix(funny, unit_rect));
        Geom::Affine gs2d(gr->get_gs2d_matrix(funny, unit_rect));
        EXPECT_TRUE(Geom::are_near(funny, g2d));
        EXPECT_TRUE(Geom::are_near(gs2d, gr->gradientTransform * g2d, 1e-12));

        gr->set_gs2d_matrix(funny, unit_rect, gs2d);
        EXPECT_TRUE(Geom::are_near(gr->gradientTransform, grXform, 1e-12));
    }

    gr->gradientTransform = grXform;
    Geom::Rect larger_rect(Geom::Point(5, 6), Geom::Point(8, 10));
    {
        Geom::Affine g2d(gr->get_g2d_matrix(funny, larger_rect));
        Geom::Affine gs2d(gr->get_gs2d_matrix(funny, larger_rect));
        EXPECT_TRUE(Geom::are_near(Geom::Affine(3, 0,
                                                0, 4,
                                                5, 6) * funny, g2d ));
        EXPECT_TRUE(Geom::are_near(gs2d, gr->gradientTransform * g2d, 1e-12));

        gr->set_gs2d_matrix(funny, larger_rect, gs2d);
        EXPECT_TRUE(Geom::are_near(gr->gradientTransform, grXform, 1e-12));

        SP_OBJECT(gr)->setKeyValue( SP_ATTR_GRADIENTUNITS, "userSpaceOnUse");
        Geom::Affine user_g2d(gr->get_g2d_matrix(funny, larger_rect));
        Geom::Affine user_gs2d(gr->get_gs2d_matrix(funny, larger_rect));
        EXPECT_TRUE(Geom::are_near(funny, user_g2d));
        EXPECT_TRUE(Geom::are_near(user_gs2d, gr->gradientTransform * user_g2d, 1e-12));
    }
}

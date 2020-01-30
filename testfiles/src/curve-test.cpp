// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Curve test
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2020 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include <gtest/gtest.h>

#include "display/curve.h"
#include <2geom/curves.h>
#include <2geom/path.h>
#include <2geom/pathvector.h>

class CurveTest : public ::testing::Test {
  public:
    Geom::Path path1;
    Geom::Path path2;
    Geom::Path path3;
    Geom::Path path4;

  protected:
    CurveTest()
        : path4(Geom::Point(3, 5)) // Just a moveto
    {
        // Closed path
        path1.append(Geom::LineSegment(Geom::Point(0, 0), Geom::Point(1, 0)));
        path1.append(Geom::LineSegment(Geom::Point(1, 0), Geom::Point(1, 1)));
        path1.close();
        // Closed path (ClosingSegment is zero length)
        path2.append(Geom::LineSegment(Geom::Point(2, 0), Geom::Point(3, 0)));
        path2.append(Geom::CubicBezier(Geom::Point(3, 0), Geom::Point(2, 1), Geom::Point(1, 1), Geom::Point(2, 0)));
        path2.close();
        // Open path
        path3.setStitching(true);
        path3.append(Geom::EllipticalArc(Geom::Point(4, 0), 1, 2, M_PI, false, false, Geom::Point(5, 1)));
        path3.append(Geom::LineSegment(Geom::Point(5, 1), Geom::Point(5, 2)));
        path3.append(Geom::LineSegment(Geom::Point(6, 4), Geom::Point(2, 4)));
    }
};

TEST_F(CurveTest, testGetSegmentCount)
{
    { // Zero segments
        Geom::PathVector pv;
        SPCurve curve(pv);
        ASSERT_EQ(curve.get_segment_count(), 0u);
    }
    { // Zero segments
        Geom::PathVector pv;
        pv.push_back(Geom::Path());
        SPCurve curve(pv);
        ASSERT_EQ(curve.get_segment_count(), 0u);
    }
    { // Individual paths
        Geom::PathVector pv((Geom::Path()));
        pv[0] = path1;
        ASSERT_EQ(SPCurve(pv).get_segment_count(), 3u);
        pv[0] = path2;
        ASSERT_EQ(SPCurve(pv).get_segment_count(), 2u);
        pv[0] = path3;
        ASSERT_EQ(SPCurve(pv).get_segment_count(), 4u);
        pv[0] = path4;
        ASSERT_EQ(SPCurve(pv).get_segment_count(), 0u);
        pv[0].close();
        ASSERT_EQ(SPCurve(pv).get_segment_count(), 0u);
    }
    { // Combination
        Geom::PathVector pv;
        pv.push_back(path1);
        pv.push_back(path2);
        pv.push_back(path3);
        pv.push_back(path4);
        SPCurve curve(pv);
        ASSERT_EQ(curve.get_segment_count(), 9u);
    }
}

TEST_F(CurveTest, testNodesInPathForZeroSegments)
{
    { // Zero segments
        Geom::PathVector pv;
        SPCurve curve(pv);
        ASSERT_EQ(curve.nodes_in_path(), 0u);
    }
    { // Zero segments
        Geom::PathVector pv;
        pv.push_back(Geom::Path());
        SPCurve curve(pv);
        ASSERT_EQ(curve.nodes_in_path(), 1u);
    }
}

TEST_F(CurveTest, testNodesInPathForIndividualPaths)
{
    Geom::PathVector pv((Geom::Path()));
    pv[0] = path1;
    ASSERT_EQ(SPCurve(pv).nodes_in_path(), 3u);
    pv[0] = path2;
    ASSERT_EQ(SPCurve(pv).nodes_in_path(), 2u); // zero length closing segments do not increase the nodecount.
    pv[0] = path3;
    ASSERT_EQ(SPCurve(pv).nodes_in_path(), 5u);
    pv[0] = path4;
    ASSERT_EQ(SPCurve(pv).nodes_in_path(), 1u);
}

TEST_F(CurveTest, testNodesInPathForNakedMoveToClosedPath)
{
    Geom::PathVector pv((Geom::Path()));
    pv[0] = path4; // just a MoveTo
    pv[0].close();
    ASSERT_EQ(SPCurve(pv).nodes_in_path(), 1u);
}

/*
TEST_F(CurveTest, testNodesInPathForPathsCombination)
{
    Geom::PathVector pv;
    pv.push_back(path1);
    pv.push_back(path2);
    pv.push_back(path3);
    pv.push_back(path4);
    SPCurve curve(pv);
    ASSERT_EQ(curve.nodes_in_path(), 12u);
}
*/

TEST_F(CurveTest, testIsEmpty)
{
    ASSERT_TRUE(SPCurve(Geom::PathVector()).is_empty());
    ASSERT_FALSE(SPCurve(path1).is_empty());
    ASSERT_FALSE(SPCurve(path2).is_empty());
    ASSERT_FALSE(SPCurve(path3).is_empty());
    ASSERT_FALSE(SPCurve(path4).is_empty());
}

TEST_F(CurveTest, testIsClosed)
{
    ASSERT_FALSE(SPCurve(Geom::PathVector()).is_closed());
    Geom::PathVector pv((Geom::Path()));
    ASSERT_FALSE(SPCurve(pv).is_closed());
    pv[0].close();
    ASSERT_TRUE(SPCurve(pv).is_closed());
    ASSERT_TRUE(SPCurve(path1).is_closed());
    ASSERT_TRUE(SPCurve(path2).is_closed());
    ASSERT_FALSE(SPCurve(path3).is_closed());
    ASSERT_FALSE(SPCurve(path4).is_closed());
}

/*
TEST_F(CurveTest, testLastFirstSegment)
{
    Geom::PathVector pv(path4);
    ASSERT_EQ(SPCurve(pv).first_segment(), (void *)0);
    ASSERT_EQ(SPCurve(pv).last_segment(), (void *)0);
    pv[0].close();
    ASSERT_NE(SPCurve(pv).first_segment(), (void *)0);
    ASSERT_NE(SPCurve(pv).last_segment(), (void *)0);
}
*/

TEST_F(CurveTest, testLastFirstPath)
{
    Geom::PathVector pv;
    ASSERT_EQ(SPCurve(pv).first_path(), (void *)0);
    ASSERT_EQ(SPCurve(pv).last_path(), (void *)0);
    pv.push_back(path1);
    ASSERT_EQ(*SPCurve(pv).first_path(), pv[0]);
    ASSERT_EQ(*SPCurve(pv).last_path(), pv[0]);
    pv.push_back(path2);
    ASSERT_EQ(*SPCurve(pv).first_path(), pv[0]);
    ASSERT_EQ(*SPCurve(pv).last_path(), pv[1]);
    pv.push_back(path3);
    ASSERT_EQ(*SPCurve(pv).first_path(), pv[0]);
    ASSERT_EQ(*SPCurve(pv).last_path(), pv[2]);
    pv.push_back(path4);
    ASSERT_EQ(*SPCurve(pv).first_path(), pv[0]);
    ASSERT_EQ(*SPCurve(pv).last_path(), pv[3]);
}

TEST_F(CurveTest, testFirstPoint)
{
    ASSERT_EQ(*(SPCurve(path1).first_point()), Geom::Point(0, 0));
    ASSERT_EQ(*(SPCurve(path2).first_point()), Geom::Point(2, 0));
    ASSERT_EQ(*(SPCurve(path3).first_point()), Geom::Point(4, 0));
    ASSERT_EQ(*(SPCurve(path4).first_point()), Geom::Point(3, 5));
    Geom::PathVector pv;
    ASSERT_FALSE(SPCurve(pv).first_point());
    pv.push_back(path1);
    pv.push_back(path2);
    pv.push_back(path3);
    ASSERT_EQ(*(SPCurve(pv).first_point()), Geom::Point(0, 0));
    pv.insert(pv.begin(), path4);
    ASSERT_EQ(*(SPCurve(pv).first_point()), Geom::Point(3, 5));
}

/*
TEST_F(CurveTest, testLastPoint)
{
    ASSERT_EQ(*(SPCurve(path1).last_point()), Geom::Point(0, 0));
    ASSERT_EQ(*(SPCurve(path2).last_point()), Geom::Point(2, 0));
    ASSERT_EQ(*(SPCurve(path3).last_point()), Geom::Point(8, 4));
    ASSERT_EQ(*(SPCurve(path4).last_point()), Geom::Point(3, 5));
    Geom::PathVector pv;
    ASSERT_FALSE(SPCurve(pv).last_point());
    pv.push_back(path1);
    pv.push_back(path2);
    pv.push_back(path3);
    ASSERT_EQ(*(SPCurve(pv).last_point()), Geom::Point(8, 4));
    pv.push_back(path4);
    ASSERT_EQ(*(SPCurve(pv).last_point()), Geom::Point(3, 5));
}
*/

TEST_F(CurveTest, testSecondPoint)
{
    ASSERT_EQ(*(SPCurve(path1).second_point()), Geom::Point(1, 0));
    ASSERT_EQ(*(SPCurve(path2).second_point()), Geom::Point(3, 0));
    ASSERT_EQ(*(SPCurve(path3).second_point()), Geom::Point(5, 1));
    ASSERT_EQ(*(SPCurve(path4).second_point()), Geom::Point(3, 5));
    Geom::PathVector pv;
    pv.push_back(path1);
    pv.push_back(path2);
    pv.push_back(path3);
    ASSERT_EQ(*(SPCurve(pv).second_point()), Geom::Point(1, 0));
    pv.insert(pv.begin(), path4);
    ASSERT_EQ(*SPCurve(pv).second_point(), Geom::Point(0, 0));
}

/*
TEST_F(CurveTest, testPenultimatePoint)
{
    ASSERT_EQ(*(SPCurve(Geom::PathVector(path1)).penultimate_point()), Geom::Point(1, 1));
    ASSERT_EQ(*(SPCurve(Geom::PathVector(path2)).penultimate_point()), Geom::Point(3, 0));
    ASSERT_EQ(*(SPCurve(Geom::PathVector(path3)).penultimate_point()), Geom::Point(6, 4));
    ASSERT_EQ(*(SPCurve(Geom::PathVector(path4)).penultimate_point()), Geom::Point(3, 5));
    Geom::PathVector pv;
    pv.push_back(path1);
    pv.push_back(path2);
    pv.push_back(path3);
    ASSERT_EQ(*(SPCurve(pv).penultimate_point()), Geom::Point(6, 4));
    pv.push_back(path4);
    ASSERT_EQ(*(SPCurve(pv).penultimate_point()), Geom::Point(8, 4));
}
*/

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

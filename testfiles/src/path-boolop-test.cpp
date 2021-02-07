// SPDX-License-Identifier: GPL-2.0-or-later
#include <gtest/gtest.h>
#include <src/path/path-boolop.h>
#include <src/svg/svg.h>
#include <2geom/svg-path-writer.h>

class PathBoolopTest : public ::testing::Test
{
    public:
        std::string rectangle_bigger =  "M 0,0 L 0,2 L 2,2 L 2,0 z";
        std::string rectangle_smaller = "M 0.5,0.5 L 0.5,1.5 L 1.5,1.5 L 1.5,0.5 z";
        std::string rectangle_outside =  "M 0,1.5 L 0.5,1.5 L 0.5,2.5 L 0,2.5 z";
        std::string rectangle_outside_union = "M 0,0 L 0,1.5 L 0,2 L 0,2.5 L 0.5,2.5 L 0.5,2 L 2,2 L 2,0 L 0,0 z";
        Geom::PathVector pvRectangleBigger;
        Geom::PathVector pvRectangleSmaller;
        Geom::PathVector pvRectangleOutside;
        Geom::PathVector pvTargetUnion;
        Geom::PathVector pvEmpty;
        PathBoolopTest() {
            pvRectangleBigger = sp_svg_read_pathv(rectangle_bigger.c_str());
            pvRectangleSmaller = sp_svg_read_pathv(rectangle_smaller.c_str());
            pvRectangleOutside = sp_svg_read_pathv(rectangle_outside.c_str());
            pvTargetUnion = sp_svg_read_pathv(rectangle_outside_union.c_str());
            pvEmpty = sp_svg_read_pathv("");
        }
        void comparePaths(Geom::PathVector result, Geom::PathVector target){
            Geom::SVGPathWriter wr;
            wr.feed(result);
            std::string resultD = wr.str();
            wr.clear();
            wr.feed(target);
            std::string targetD = wr.str();
            EXPECT_EQ(resultD, targetD);
            EXPECT_EQ(result, target);
        }
};

TEST_F(PathBoolopTest, UnionOutside){
    // test that the union of two objects where one is outside the other results in a new larger shape
    Geom::PathVector pvRectangleUnion  = sp_pathvector_boolop(pvRectangleBigger, pvRectangleOutside, bool_op_union, fill_oddEven, fill_oddEven);
    comparePaths(pvRectangleUnion, pvTargetUnion);
}

TEST_F(PathBoolopTest, UnionOutsideSwap){
    // test that the union of two objects where one is outside the other results in a new larger shape, even when the order is reversed
    Geom::PathVector pvRectangleUnion  = sp_pathvector_boolop(pvRectangleOutside, pvRectangleBigger, bool_op_union, fill_oddEven, fill_oddEven);
    comparePaths(pvRectangleUnion, pvTargetUnion);
}

TEST_F(PathBoolopTest, UnionInside){
    // test that the union of two objects where one is completely inside the other is the larger shape
    Geom::PathVector pvRectangleUnion  = sp_pathvector_boolop(pvRectangleBigger, pvRectangleSmaller, bool_op_union, fill_oddEven, fill_oddEven);
    comparePaths(pvRectangleUnion, pvRectangleBigger);
}

TEST_F(PathBoolopTest, UnionInsideSwap){
    // test that the union of two objects where one is completely inside the other is the larger shape, even when the order is swapped
    Geom::PathVector pvRectangleUnion  = sp_pathvector_boolop(pvRectangleSmaller, pvRectangleBigger, bool_op_union, fill_oddEven, fill_oddEven);
    comparePaths(pvRectangleUnion, pvRectangleBigger);
}

TEST_F(PathBoolopTest, IntersectionInside){
    // test that the intersection of two objects where one is completely inside the other is the smaller shape
    Geom::PathVector pvRectangleIntersection  = sp_pathvector_boolop(pvRectangleBigger, pvRectangleSmaller, bool_op_inters, fill_oddEven, fill_oddEven);
    comparePaths(pvRectangleIntersection, pvRectangleSmaller);
}

TEST_F(PathBoolopTest, DifferenceInside){
    // test that the difference of two objects where one is completely inside the other is an empty path
    Geom::PathVector pvRectangleDifference  = sp_pathvector_boolop(pvRectangleBigger, pvRectangleSmaller, bool_op_diff, fill_oddEven, fill_oddEven);
    comparePaths(pvRectangleDifference, pvEmpty);
}

TEST_F(PathBoolopTest, DifferenceOutside){
    // test that the difference of two objects where one is completely outside the other is multiple shapes
    Geom::PathVector pvRectangleDifference = sp_pathvector_boolop(pvRectangleSmaller, pvRectangleBigger, bool_op_diff, fill_oddEven, fill_oddEven);
    std::string bothPaths = rectangle_bigger + rectangle_smaller;
    Geom::PathVector pvRectangleSmallerReversed = pvRectangleSmaller;
    Geom::PathVector pvBothPaths = pvRectangleBigger;
    
    pvRectangleSmallerReversed.reverse();
    for(Geom::Path _path:pvRectangleSmallerReversed){
        pvBothPaths.push_back(_path);
    }

    comparePaths(pvRectangleDifference, pvBothPaths);
}

//
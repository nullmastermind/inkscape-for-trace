// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Tests for Style internal classes
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2020 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include <gtest/gtest.h>
#include <src/style-internal.h>

TEST(StyleInternalTest, testSPIDashArrayInequality)
{
	SPIDashArray array;
	array.read("0 1 2 3");
	SPIDashArray subsetArray;
	subsetArray.read("0 1");
	
	ASSERT_FALSE(array == subsetArray);
	ASSERT_FALSE(subsetArray == array);
}

TEST(StyleInternalTest, testSPIDashArrayEquality)
{
	SPIDashArray anArray;
	anArray.read("0 1 2 3");
	SPIDashArray sameArray;
	sameArray.read("0 1 2 3");
	
	ASSERT_TRUE(anArray == sameArray);
	ASSERT_TRUE(sameArray == anArray);
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

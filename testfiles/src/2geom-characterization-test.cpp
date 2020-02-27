// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * 2Geom Lib characterization tests
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2020 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include <gtest/gtest.h>

#include <2geom/path.h>

TEST(Characterization2Geom, retrievingBackElementOfAnEmptyClosedPathFails)
{
    Geom::Path path(Geom::Point(3, 5));
    path.close();
    ASSERT_TRUE(path.closed());
    ASSERT_EQ(path.size_closed(), 0u);
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

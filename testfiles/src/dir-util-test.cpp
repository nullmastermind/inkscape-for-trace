// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for dir utils.
 *
 * Author:
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2015 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "gtest/gtest.h"

#include <glib.h>

#include "io/dir-util.h"

namespace {


TEST(DirUtilTest, Base)
{
    char const* cases[][3] = {
#if defined(WIN32) || defined(__WIN32__)
        {"\\foo\\bar", "\\foo", "bar"},
        {"\\foo\\barney", "\\foo\\bar", "\\foo\\barney"},
        {"\\foo\\bar\\baz", "\\foo\\", "bar\\baz"},
        {"\\foo\\bar\\baz", "\\", "foo\\bar\\baz"},
        {"\\foo\\bar\\baz", "\\foo\\qux", "\\foo\\bar\\baz"},
#else
        {"/foo/bar", "/foo", "bar"},
        {"/foo/barney", "/foo/bar", "/foo/barney"},
        {"/foo/bar/baz", "/foo/", "bar/baz"},
        {"/foo/bar/baz", "/", "foo/bar/baz"},
        {"/foo/bar/baz", "/foo/qux", "/foo/bar/baz"},
#endif
    };

    for (auto & i : cases)
    {
        if ( i[0] && i[1] ) { // std::string can't use null.
            std::string  result = sp_relative_path_from_path( i[0], i[1] );
            ASSERT_FALSE( result.empty() );
            if ( !result.empty() )
            {
                ASSERT_EQ( std::string(i[2]), result );
            }
        }
    }
}

} // namespace

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

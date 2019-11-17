// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Test CSSOStringStream and SVGOStringStream
 */
/*
 * Authors:
 *   Thomas Holder
 *
 * Copyright (C) 2019 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "2geom/point.h"
#include "svg/css-ostringstream.h"
#include "svg/stringstream.h"

#include "gtest/gtest.h"
#include <glibmm/ustring.h>

template <typename S, typename T>
static void assert_tostring_eq(T value, const char *expected)
{
    S os;

    // default of /options/svgoutput/numericprecision
    os.precision(8);

    os << value;
    ASSERT_EQ(os.str(), expected);
}

#define TEST_STRING "Hello & <World>"

template <typename S>
void test_tostring()
{
    assert_tostring_eq<S, char>('A', "A");
    assert_tostring_eq<S, signed char>('A', "A");
    assert_tostring_eq<S, unsigned char>('A', "A");

    assert_tostring_eq<S, short>(0x7FFF, "32767");
    assert_tostring_eq<S, short>(-30000, "-30000");
    assert_tostring_eq<S, unsigned short>(0xFFFFu, "65535");
    assert_tostring_eq<S, int>(0x7FFFFFFF, "2147483647");
    assert_tostring_eq<S, int>(-2000000000, "-2000000000");
    assert_tostring_eq<S, unsigned int>(0xFFFFFFFFu, "4294967295");

    // long is 32bit on Windows, 64bit on Linux
    assert_tostring_eq<S, long>(0x7FFFFFFFL, "2147483647");
    assert_tostring_eq<S, long>(-2000000000L, "-2000000000");
    assert_tostring_eq<S, unsigned long>(0xFFFFFFFFuL, "4294967295");

    assert_tostring_eq<S>((char const *)TEST_STRING, TEST_STRING);
    assert_tostring_eq<S>((signed char const *)TEST_STRING, TEST_STRING);
    assert_tostring_eq<S>((unsigned char const *)TEST_STRING, TEST_STRING);
    assert_tostring_eq<S, std::string>(TEST_STRING, TEST_STRING);
    assert_tostring_eq<S, Glib::ustring>(TEST_STRING, TEST_STRING);
}

TEST(CSSOStringStreamTest, tostring)
{
    using S = Inkscape::CSSOStringStream;

    test_tostring<S>();

    // float has 6 significant digits
    assert_tostring_eq<S, float>(0.0, "0");
    assert_tostring_eq<S, float>(4.5, "4.5");
    assert_tostring_eq<S, float>(-4.0, "-4");
    assert_tostring_eq<S, float>(0.001, "0.001");
    assert_tostring_eq<S, float>(0.00123456, "0.00123456");
    assert_tostring_eq<S, float>(-0.00123456, "-0.00123456");
    assert_tostring_eq<S, float>(-1234560.0, "-1234560");

    // double has 15 significant digits
    assert_tostring_eq<S, double>(0.0, "0");
    assert_tostring_eq<S, double>(4.5, "4.5");
    assert_tostring_eq<S, double>(-4.0, "-4");
    assert_tostring_eq<S, double>(0.001, "0.001");

    // 9 significant digits
    assert_tostring_eq<S, double>(1.23456789, "1.23456789");
    assert_tostring_eq<S, double>(-1.23456789, "-1.23456789");
    assert_tostring_eq<S, double>(12345678.9, "12345678.9");
    assert_tostring_eq<S, double>(-12345678.9, "-12345678.9");

    assert_tostring_eq<S, double>(1.234e-12, "0");
    assert_tostring_eq<S, double>(3e9, "3000000000");
    assert_tostring_eq<S, double>(-3.5e9, "-3500000000");
}

TEST(SVGOStringStreamTest, tostring)
{
    using S = Inkscape::SVGOStringStream;

    test_tostring<S>();

    assert_tostring_eq<S>(Geom::Point(12, 3.4), "12,3.4");

    // float has 6 significant digits
    assert_tostring_eq<S, float>(0.0, "0");
    assert_tostring_eq<S, float>(4.5, "4.5");
    assert_tostring_eq<S, float>(-4.0, "-4");
    assert_tostring_eq<S, float>(0.001, "0.001");
    assert_tostring_eq<S, float>(0.00123456, "0.00123456");
    assert_tostring_eq<S, float>(-0.00123456, "-0.00123456");
    assert_tostring_eq<S, float>(-1234560.0, "-1234560");

    // double has 15 significant digits
    assert_tostring_eq<S, double>(0.0, "0");
    assert_tostring_eq<S, double>(4.5, "4.5");
    assert_tostring_eq<S, double>(-4.0, "-4");
    assert_tostring_eq<S, double>(0.001, "0.001");

    // 8 significant digits
    assert_tostring_eq<S, double>(1.23456789, "1.2345679");
    assert_tostring_eq<S, double>(-1.23456789, "-1.2345679");
    assert_tostring_eq<S, double>(12345678.9, "12345679");
    assert_tostring_eq<S, double>(-12345678.9, "-12345679");

    assert_tostring_eq<S, double>(1.234e-12, "1.234e-12");
    assert_tostring_eq<S, double>(3e9, "3e+09");
    assert_tostring_eq<S, double>(-3.5e9, "-3.5e+09");
}

template <typename S>
void test_concat()
{
    S s;
    s << "hello, ";
    s << -53.5;
    ASSERT_EQ(s.str(), std::string("hello, -53.5"));
}

TEST(CSSOStringStreamTest, concat)
{ //
    test_concat<Inkscape::CSSOStringStream>();
}

TEST(SVGOStringStreamTest, concat)
{ //
    test_concat<Inkscape::SVGOStringStream>();
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

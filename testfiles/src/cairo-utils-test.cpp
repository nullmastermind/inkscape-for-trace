// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Tests for classes like Pixbuf from cairo-utils
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2020 Authors
 *
 * Released under GNU GPL version 2 or later, read the file 'COPYING' for more information
 */

#include <gtest/gtest.h>
#include <src/display/cairo-utils.h>
#include <src/inkscape.h>


class PixbufTest : public ::testing::Test {
  public:
    static std::string base64of(const std::string &s)
    {
        gchar *encoded = g_base64_encode(reinterpret_cast<guchar const *>(s.c_str()), s.size());
        std::string r(encoded);
        g_free(encoded);
        return r;
    }

  protected:
    void SetUp() override
    {
        // setup hidden dependency
        Inkscape::Application::create(false);
    }
};

TEST_F(PixbufTest, creatingFromSvgBufferWithoutViewboxOrWidthAndHeightReturnsNull)
{
    std::string svg_buffer(
        "<svg><path d=\"M 71.527648,186.14229 A 740.48715,740.48715 0 0 0 696.31258,625.8041 Z\"/></svg>");
    double default_dpi = 96.0;
    std::string filename_with_svg_extension("malformed.svg");

    ASSERT_EQ(Inkscape::Pixbuf::create_from_buffer(svg_buffer, default_dpi, filename_with_svg_extension), nullptr);
}

TEST_F(PixbufTest, creatingFromSvgUriWithoutViewboxOrWidthAndHeightReturnsNull)
{
    std::string uri_data = "image/svg+xml;base64," + base64of("<svg><path d=\"M 71.527648,186.14229 A 740.48715,740.48715 0 0 0 696.31258,625.8041 Z\"/></svg>");
    double default_dpi = 96.0;

    ASSERT_EQ(Inkscape::Pixbuf::create_from_data_uri(uri_data.c_str(), default_dpi), nullptr);
}
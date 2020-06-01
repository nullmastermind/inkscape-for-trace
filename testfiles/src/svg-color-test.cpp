// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Test for SVG colors
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2010 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "svg/svg-color.h"

#include <cstdlib>
#include <gtest/gtest.h>

#include "preferences.h"
#include "svg/svg-icc-color.h"

static void check_rgb24(unsigned const rgb24)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    char css[8];

    prefs->setBool("/options/svgoutput/usenamedcolors", false);
    sp_svg_write_color(css, sizeof(css), rgb24 << 8);
    ASSERT_EQ(sp_svg_read_color(css, 0xff), rgb24 << 8);

    prefs->setBool("/options/svgoutput/usenamedcolors", true);
    sp_svg_write_color(css, sizeof(css), rgb24 << 8);
    ASSERT_EQ(sp_svg_read_color(css, 0xff), rgb24 << 8);
}

TEST(SvgColorTest, testWrite)
{
    unsigned const components[] = {0, 0x80, 0xff, 0xc0, 0x77};
    unsigned const nc = G_N_ELEMENTS(components);
    for (unsigned i = nc * nc * nc; i--;) {
        unsigned tmp = i;
        unsigned rgb24 = 0;
        for (unsigned c = 0; c < 3; ++c) {
            unsigned const component = components[tmp % nc];
            rgb24 = (rgb24 << 8) | component;
            tmp /= nc;
        }
        ASSERT_TRUE(tmp == 0);
        check_rgb24(rgb24);
    }

    /* And a few completely random ones. */
    for (unsigned i = 500; i--;) { /* Arbitrary number of iterations. */
        unsigned const rgb24 = (std::rand() >> 4) & 0xffffff;
        check_rgb24(rgb24);
    }
}

TEST(SvgColorTest, testReadColor)
{
    gchar const *val[] = {"#f0f", "#ff00ff", "rgb(255,0,255)", "fuchsia"};
    size_t const n = sizeof(val) / sizeof(*val);
    for (size_t i = 0; i < n; i++) {
        gchar const *end = 0;
        guint32 result = sp_svg_read_color(val[i], &end, 0x3);
        ASSERT_EQ(result, 0xff00ff00);
        ASSERT_LT(val[i], end);
    }
}

TEST(SvgColorTest, testIccColor)
{
    struct
    {
        unsigned numEntries;
        bool shouldPass;
        char const *name;
        char const *str;
    } cases[] = {
        {1, true, "named", "icc-color(named, 3)"},
        {0, false, "", "foodle"},
        {1, true, "a", "icc-color(a, 3)"},
        {4, true, "named", "icc-color(named, 3, 0, 0.1, 2.5)"},
        {0, false, "", "icc-color(named, 3"},
        {0, false, "", "icc-color(space named, 3)"},
        {0, false, "", "icc-color(tab\tnamed, 3)"},
        {0, false, "", "icc-color(0name, 3)"},
        {0, false, "", "icc-color(-name, 3)"},
        {1, true, "positive", "icc-color(positive, +3)"},
        {1, true, "negative", "icc-color(negative, -3)"},
        {1, true, "positive", "icc-color(positive, +0.1)"},
        {1, true, "negative", "icc-color(negative, -0.1)"},
        {0, false, "", "icc-color(named, value)"},
        {1, true, "hyphen-name", "icc-color(hyphen-name, 1)"},
        {1, true, "under_name", "icc-color(under_name, 1)"},
    };

    for (size_t i = 0; i < G_N_ELEMENTS(cases); i++) {
        SVGICCColor tmp;
        char const *str = cases[i].str;
        char const *result = nullptr;

        bool parseRet = sp_svg_read_icc_color(str, &result, &tmp);
        ASSERT_EQ(parseRet, cases[i].shouldPass) << str;
        ASSERT_EQ(tmp.colors.size(), cases[i].numEntries) << str;
        if (cases[i].shouldPass) {
            ASSERT_STRNE(str, result);
            ASSERT_EQ(tmp.colorProfile, cases[i].name) << str;
        } else {
            ASSERT_STREQ(str, result);
            ASSERT_TRUE(tmp.colorProfile.empty());
        }
    }
}

// vim: filetype=cpp:expandtab:shiftwidth=4:softtabstop=4:fileencoding=utf-8:textwidth=99 :

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * SPGlyphKerning test
 *//*
 *
 * Authors:
 *   Cosmin Dancu
 *
 * Copyright (C) 2020 Authors
 *
 * Released under GNU GPL version 2 or later, read the file 'COPYING' for more information
 */

#include <gtest/gtest.h>
#include <src/object/sp-glyph-kerning.h>

TEST(SPGlyphKerningTest, EmptyGlyphNamesDoNotContainAnything) {
	GlyphNames empty_glyph_names(nullptr);
    ASSERT_FALSE(empty_glyph_names.contains("foo"));
}

TEST(SPGlyphKerningTest, GlyphNamesContainEachName) {
	GlyphNames glyph_names("name1 name2");
    ASSERT_TRUE(glyph_names.contains("name1"));
	ASSERT_TRUE(glyph_names.contains("name2"));
}

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * SPGroup test
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2020 Authors
 *
 * Released under GNU GPL version 2 or later, read the file 'COPYING' for more information
 */

#include <gtest/gtest.h>
#include <src/document.h>
#include <src/inkscape.h>
#include <src/live_effects/effect.h>
#include <src/object/sp-lpe-item.h>

using namespace Inkscape;
using namespace Inkscape::LivePathEffect;

class SPGroupTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        // setup hidden dependency
        Application::create(false);
    }
};

TEST_F(SPGroupTest, applyingPowerClipEffectToGroupWithoutClipIsIgnored)
{
    std::string svg("\
<svg width='100' height='100'>\
    <g id='group1'>\
        <rect id='rect1' width='100' height='50' />\
        <rect id='rect2' y='50' width='100' height='50' />\
    </g>\
</svg>");

    SPDocument *doc = SPDocument::createNewDocFromMem(svg.c_str(), svg.size(), true);

    auto group = dynamic_cast<SPGroup *>(doc->getObjectById("group1"));
    Effect::createAndApply(POWERCLIP, doc, group);

    ASSERT_FALSE(group->hasPathEffect());
}

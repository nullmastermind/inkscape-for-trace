// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * LPE Boolean operation test
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
#include <src/live_effects/lpe-bool.h>
#include <src/object/sp-ellipse.h>
#include <src/object/sp-lpe-item.h>



using namespace Inkscape;
using namespace Inkscape::LivePathEffect;

class LPEBoolTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        // setup hidden dependency
        Application::create(false);
    }
};

TEST_F(LPEBoolTest, canBeApplyedToNonSiblingPaths)
{
    std::string svg("\
<svg width='100' height='100'\
  xmlns:sodipodi='http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd'\
  xmlns:inkscape='http://www.inkscape.org/namespaces/inkscape'>\
  <defs>\
    <inkscape:path-effect\
      id='path-effect1'\
      effect='bool_op'\
      operation='diff'\
      operand-path='#circle1'\
      lpeversion='1'\
      hide-linked='true' />\
  </defs>\
  <path id='rect1'\
    inkscape:path-effect='#path-effect1'\
    sodipodi:type='rect'\
    width='100' height='100' fill='#ff0000' />\
  <g id='group1'>\
    <circle id='circle1'\
      r='40' cy='50' cx='50' fill='#ffffff' style='display:inline'/>\
  </g>\
</svg>");

    SPDocument *doc = SPDocument::createNewDocFromMem(svg.c_str(), svg.size(), true);
    doc->ensureUpToDate();

    auto lpe_item = dynamic_cast<SPLPEItem *>(doc->getObjectById("rect1"));
    ASSERT_TRUE(lpe_item != nullptr);

    auto lpe_bool_op_effect = dynamic_cast<LPEBool *>(lpe_item->getPathEffectOfType(EffectType::BOOL_OP));
    ASSERT_TRUE(lpe_bool_op_effect != nullptr);

    auto operand_path = lpe_bool_op_effect->getParameter("operand-path")->param_getSVGValue();
    auto circle = dynamic_cast<SPGenericEllipse *>(doc->getObjectById(operand_path.substr(1)));
    ASSERT_TRUE(circle != nullptr);
}
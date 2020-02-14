// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * SVG Extension test
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2020 Authors
 *
 * Released under GNU GPL version 2 or later, read the file 'COPYING' for more information
 */

#include <gtest/gtest.h>

#include <src/extension/db.h>
#include <src/extension/input.h>
#include <src/extension/internal/svg.h>
#include <src/inkscape.h>

#include <glib/gstdio.h>

using namespace Inkscape;
using namespace Inkscape::Extension;
using namespace Inkscape::Extension::Internal;

class SvgExtensionTest : public ::testing::Test {
  public:
    static std::string create_file(const std::string &filename, const std::string &content)
    {
        std::stringstream path_builder;
        path_builder << "SvgExtensionTest_" << _files.size() << "_" << filename;
        std::string path = path_builder.str();
        GError *error = nullptr;
        if (!g_file_set_contents(path.c_str(), content.c_str(), content.size(), &error)) {
            std::stringstream msg;
            msg << "SvgExtensionTest::create_file failed: GError(" << error->domain << ", " << error->code << ", "
                << error->message << ")";
            g_error_free(error);
            throw std::runtime_error(msg.str());
        }
        _files.insert(path);
        return path;
    }

    static std::set<std::string> _files;

  protected:
    void SetUp() override
    {
        // setup hidden dependency
        Application::create(false);
    }

    static void TearDownTestCase()
    {
        for (auto file : _files) {
            if (g_remove(file.c_str())) {
                std::cout << "SvgExtensionTest was unable to remove file: " << file << std::endl;
            }
        }
    }
};

std::set<std::string> SvgExtensionTest::_files;

TEST_F(SvgExtensionTest, openingAsLinkInImageASizelessSvgFileReturnsNull)
{
    std::string sizeless_svg_file =
        create_file("sizeless.svg",
                    "<svg><path d=\"M 71.527648,186.14229 A 740.48715,740.48715 0 0 0 696.31258,625.8041 Z\"/></svg>");
    
    Svg::init();
    Input *svg_input_extension(dynamic_cast<Input *>(db.get(SP_MODULE_KEY_INPUT_SVG))); 
    
    Preferences *prefs = Preferences::get();
    prefs->setBool("/options/onimport", true);
    prefs->setBool("/dialogs/import/ask_svg", false);
    prefs->setString("/dialogs/import/import_mode_svg", "link");

    ASSERT_EQ(svg_input_extension->open(sizeless_svg_file.c_str()), nullptr);
}
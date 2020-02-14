// SPDX-License-Identifier: GPL-2.0-or-later OR MIT
/**
 * @file
 * Test that svgz (= compressed SVG) import/drag-and-drop
 * is working: https://gitlab.com/inkscape/inkscape/issues/906 .
 *
 */
/*
 * Authors:
 *      Shlomi Fish
 *
 * Copyright (C) 2020 Authors
 */

#include "doc-per-case-test.h"
#include <glibmm.h>

#include "extension/db.h"
#include "extension/find_extension_by_mime.h"
#include "extension/internal/svgz.h"
#include "path-prefix.h"
#include "preferences.h"

#include "gtest/gtest.h"


class SvgzImportTest : public DocPerCaseTest {
  public:
    SvgzImportTest() {}
    void TestBody()
    {
        ASSERT_TRUE(_doc != nullptr);
        ASSERT_TRUE(_doc->getRoot() != nullptr);
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setBool("/dialogs/import/ask_svg", true);
        prefs->setBool("/options/onimport", true);
        auto ext = Inkscape::Extension::find_by_mime("image/svg+xml-compressed");
        ext->set_gui(true);
        auto fn = Glib::build_filename(INKSCAPE_EXAMPLESDIR, "tiger.svgz");
        auto imod = dynamic_cast<Inkscape::Extension::Input *>(ext);
        auto svg_mod = (new Inkscape::Extension::Internal::Svg);
        ASSERT_TRUE(svg_mod->open(imod, fn.c_str()) != nullptr);
    }
    ~SvgzImportTest() override {}
};

TEST_F(SvgzImportTest, Eq)
{
    SvgzImportTest foo;
    foo.TestBody();
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

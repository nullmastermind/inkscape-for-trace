// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Test fixture with SPDocument per entire test case.
 *
 * Author:
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2015 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "doc-per-case-test.h"

#include "inkscape.h"

std::unique_ptr<SPDocument> DocPerCaseTest::_doc = nullptr;

DocPerCaseTest::DocPerCaseTest() :
    ::testing::Test()
{
}

void DocPerCaseTest::SetUpTestCase()
{
    if ( !Inkscape::Application::exists() )
    {
        // Create the global inkscape object.
        Inkscape::Application::create(false);
    }

    _doc.reset(SPDocument::createNewDoc( NULL, TRUE, true ));
    ASSERT_TRUE(bool(_doc));
}

void DocPerCaseTest::TearDownTestCase()
{
    _doc.reset();
}

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

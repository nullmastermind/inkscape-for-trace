// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit test main.
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

#include "inkgc/gc-core.h"
#include "inkscape.h"

#include <giomm/init.h>

int main(int argc, char **argv) {

    // setup general environment
#if !GLIB_CHECK_VERSION(2,36,0)
    g_type_init();
#endif

    // If possible, unit tests shouldn't require a GUI session
    // since this won't generally be available in auto-builders

    Gio::init();

    Inkscape::GC::init();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
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

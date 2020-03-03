// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Inkview - An SVG file viewer.
 */
/*
 * Authors:
 *   Tavmjong Bah
 *
 * Copyright (C) 2018 Authors
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 * Read the file 'COPYING' for more information.
 *
 */

#ifdef _WIN32
#include <windows.h> // SetConsoleOutputCP
#include <fcntl.h> // _O_BINARY
#endif

#include "inkview-application.h"

int main(int argc, char *argv[])
{
#ifdef _WIN32
    // temporarily switch console encoding to UTF8 while Inkview runs
    // as everything else is a mess and it seems to work just fine
    const unsigned int initial_cp = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);
    fflush(stdout); // empty buffer, just to be safe (see warning in documentation for _setmode)
    _setmode(_fileno(stdout), _O_BINARY); // binary mode seems required for this to work properly
#endif

    auto application = InkviewApplication::create();
    int ret = application->run(argc, argv);

#ifdef _WIN32
    // switch back to initial console encoding
    SetConsoleOutputCP(initial_cp);
#endif

    return ret;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

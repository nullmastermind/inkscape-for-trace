// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Consolidates version info for Inkscape,
 * its various dependencies and the OS we're running on
 *//*
 * Authors:
 *   Patrick Storz <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2021 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_VERSION_INFO_H
#define SEEN_INKSCAPE_VERSION_INFO_H

namespace Inkscape {

    std::string inkscape_version();
    std::string os_version();
    std::string debug_info();

} // namespace Inkscape

#endif // SEEN_INKSCAPE_VERSION_INFO_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace .0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99:
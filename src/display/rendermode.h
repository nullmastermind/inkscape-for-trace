// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
/*
 * RenderMode enumeration.
 *
 * Trivially public domain.
 */

#ifndef SEEN_INKSCAPE_DISPLAY_RENDERMODE_H
#define SEEN_INKSCAPE_DISPLAY_RENDERMODE_H

namespace Inkscape {

enum class RenderMode {
    NORMAL,
    OUTLINE,
    NO_FILTERS,
    VISIBLE_HAIRLINES,
    OUTLINE_OVERLAY,
    size
};

enum class SplitMode {
    NORMAL,
    SPLIT,
    XRAY,
    size
};

enum class SplitDirection {
    NONE,
    NORTH,
    EAST,
    SOUTH,
    WEST,
    HORIZONTAL, // Only used when hovering
    VERTICAL    // Only used when hovering
};

enum class ColorMode {
    NORMAL,
    GRAYSCALE,
    PRINT_COLORS_PREVIEW
};

} // Namespace Inkscape

#endif

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

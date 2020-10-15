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

enum RenderMode {
    RENDERMODE_NORMAL,
    RENDERMODE_OUTLINE,
    RENDERMODE_NO_FILTERS,
    RENDERMODE_VISIBLE_HAIRLINES
};
const size_t RENDERMODE_SIZE = 4;

enum SplitMode {
    SPLITMODE_NORMAL,
    SPLITMODE_SPLIT,
    SPLITMODE_XRAY
};
const size_t SPLITMODE_SIZE = 3;

enum SplitDirection {
    SPLITDIRECTION_NONE,
    SPLITDIRECTION_NORTH,
    SPLITDIRECTION_EAST,
    SPLITDIRECTION_SOUTH,
    SPLITDIRECTION_WEST,
    SPLITDIRECTION_HORIZONTAL, // Only used when hovering
    SPLITDIRECTION_VERTICAL    // Only used when hovering
};

enum ColorMode {
    COLORMODE_NORMAL,
    COLORMODE_GRAYSCALE,
    COLORMODE_PRINT_COLORS_PREVIEW
};
const size_t COLORMODE_SIZE = 3;

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

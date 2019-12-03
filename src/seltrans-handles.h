// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_SELTRANS_HANDLES_H
#define SEEN_SP_SELTRANS_HANDLES_H

/*
 * Seltrans knots
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/forward.h>
#include <gdk/gdk.h>

#include "enums.h"
#include "verbs.h"

typedef unsigned int guint32;

namespace Inkscape {
    class SelTrans;
}

// Colours are RRGGBBAA:      FILL,       OVER&DRAG,  STROKE,     OVER&DRAG
guint32 const DEF_COLOR[] = { 0x000000ff, 0x00ff6600, 0x000000ff, 0x000000ff };
guint32 const CEN_COLOR[] = { 0x00000000, 0x00000000, 0x000000ff, 0xff0000b0 };

enum SPSelTransType {
    HANDLE_STRETCH,
    HANDLE_SCALE,
    HANDLE_SKEW,
    HANDLE_ROTATE,
    HANDLE_CENTER,
    HANDLE_ALIGN,
    HANDLE_CENTER_ALIGN
};

// Which handle does what in the alignment (clicking)
const int AlignVerb[18] = {
    // Left Click
    SP_VERB_ALIGN_VERTICAL_TOP,
    SP_VERB_ALIGN_HORIZONTAL_RIGHT,
    SP_VERB_ALIGN_VERTICAL_BOTTOM,
    SP_VERB_ALIGN_HORIZONTAL_LEFT,
    SP_VERB_ALIGN_VERTICAL_CENTER,
    SP_VERB_ALIGN_BOTH_TOP_LEFT,
    SP_VERB_ALIGN_BOTH_TOP_RIGHT,
    SP_VERB_ALIGN_BOTH_BOTTOM_RIGHT,
    SP_VERB_ALIGN_BOTH_BOTTOM_LEFT,
    // Shift Click
    SP_VERB_ALIGN_VERTICAL_BOTTOM_TO_ANCHOR,
    SP_VERB_ALIGN_HORIZONTAL_LEFT_TO_ANCHOR,
    SP_VERB_ALIGN_VERTICAL_TOP_TO_ANCHOR,
    SP_VERB_ALIGN_HORIZONTAL_RIGHT_TO_ANCHOR,
    SP_VERB_ALIGN_HORIZONTAL_CENTER,
    SP_VERB_ALIGN_BOTH_BOTTOM_RIGHT_TO_ANCHOR,
    SP_VERB_ALIGN_BOTH_BOTTOM_LEFT_TO_ANCHOR,
    SP_VERB_ALIGN_BOTH_TOP_LEFT_TO_ANCHOR,
    SP_VERB_ALIGN_BOTH_TOP_RIGHT_TO_ANCHOR,
};
// Ofset from the index in the handle list to the index in the verb list.
const int AlignHandleToVerb = -13;
// Offset for moving from Left click to Shift Click
const int AlignShiftVerb = 9;

struct SPSelTransTypeInfo {
        guint32 const *color;
        char const *tip;
};
// One per handle type in order
extern SPSelTransTypeInfo const handtypes[7];

struct SPSelTransHandle;

struct SPSelTransHandle {
        SPSelTransType type;
	SPAnchorType anchor;
	GdkCursorType cursor;
	unsigned int control;
	gdouble x, y;
};
// These are 4 * each handle type + 1 for center
int const NUMHANDS = 26;
extern SPSelTransHandle const hands[NUMHANDS];

#endif // SEEN_SP_SELTRANS_HANDLES_H

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

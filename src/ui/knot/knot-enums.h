// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_KNOT_ENUMS_H
#define SEEN_KNOT_ENUMS_H

/**
 * @file
 * Some enums used by SPKnot and by related types \& functions. 
 */
/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

enum SPKnotStateType {
    SP_KNOT_STATE_NORMAL,
    SP_KNOT_STATE_MOUSEOVER,
    SP_KNOT_STATE_DRAGGING,
    SP_KNOT_STATE_SELECTED,
    SP_KNOT_STATE_HIDDEN
};

#define SP_KNOT_VISIBLE_STATES 4

enum {
    SP_KNOT_VISIBLE = 1 << 0,
    SP_KNOT_MOUSEOVER = 1 << 1,
    SP_KNOT_DRAGGING = 1 << 2,
    SP_KNOT_GRABBED = 1 << 3,
    SP_KNOT_SELECTED = 1 << 4
};


#endif /* !SEEN_KNOT_ENUMS_H */

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

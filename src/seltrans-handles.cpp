// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2014 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "seltrans-handles.h"

#ifdef __cplusplus
#undef N_
#define N_(x) x
#endif

SPSelTransHandle const hands[] = {
    // clang-format off
    //center handle will be 0 so we can reference it quickly.
    { HANDLE_CENTER,        SP_ANCHOR_CENTER,  GDK_CROSSHAIR,            12,    0.5,  0.5 },
    //handle-type           anchor-nudge       cursor                    image  x     y
    { HANDLE_SCALE,         SP_ANCHOR_SE,      GDK_TOP_LEFT_CORNER,      0,     0,    1   },
    { HANDLE_STRETCH,       SP_ANCHOR_S,       GDK_TOP_SIDE,             3,     0.5,  1   },
    { HANDLE_SCALE,         SP_ANCHOR_SW,      GDK_TOP_RIGHT_CORNER,     1,     1,    1   },
    { HANDLE_STRETCH,       SP_ANCHOR_W,       GDK_RIGHT_SIDE,           2,     1,    0.5 },
    { HANDLE_SCALE,         SP_ANCHOR_NW,      GDK_BOTTOM_RIGHT_CORNER,  0,     1,    0   },
    { HANDLE_STRETCH,       SP_ANCHOR_N,       GDK_BOTTOM_SIDE,          3,     0.5,  0   },
    { HANDLE_SCALE,         SP_ANCHOR_NE,      GDK_BOTTOM_LEFT_CORNER,   1,     0,    0   },
    { HANDLE_STRETCH,       SP_ANCHOR_E,       GDK_LEFT_SIDE,            2,     0,    0.5 },
    { HANDLE_ROTATE,        SP_ANCHOR_SE,      GDK_EXCHANGE,             4,     0,    1   },
    { HANDLE_SKEW,          SP_ANCHOR_S,       GDK_SB_H_DOUBLE_ARROW,    8,     0.5,  1   },
    { HANDLE_ROTATE,        SP_ANCHOR_SW,      GDK_EXCHANGE,             5,     1,    1   },
    { HANDLE_SKEW,          SP_ANCHOR_W,       GDK_SB_V_DOUBLE_ARROW,    9,     1,    0.5 },
    { HANDLE_ROTATE,        SP_ANCHOR_NW,      GDK_EXCHANGE,             6,     1,    0   },
    { HANDLE_SKEW,          SP_ANCHOR_N,       GDK_SB_H_DOUBLE_ARROW,    10,    0.5,  0   },
    { HANDLE_ROTATE,        SP_ANCHOR_NE,      GDK_EXCHANGE,             7,     0,    0   },
    { HANDLE_SKEW,          SP_ANCHOR_E,       GDK_SB_V_DOUBLE_ARROW,    11,    0,    0.5 },

    { HANDLE_SIDE_ALIGN,    SP_ANCHOR_S,       GDK_TOP_SIDE,             13,    0.5,  1   },
    { HANDLE_SIDE_ALIGN,    SP_ANCHOR_W,       GDK_RIGHT_SIDE,           14,    1,    0.5 },
    { HANDLE_SIDE_ALIGN,    SP_ANCHOR_N,       GDK_BOTTOM_SIDE,          15,    0.5,  0   },
    { HANDLE_SIDE_ALIGN,    SP_ANCHOR_E,       GDK_LEFT_SIDE,            16,    0,    0.5 },
    { HANDLE_CENTER_ALIGN,  SP_ANCHOR_CENTER,  GDK_CROSSHAIR,            17,    0.5,  0.5 },
    { HANDLE_CORNER_ALIGN,  SP_ANCHOR_SE,      GDK_TOP_LEFT_CORNER,      18,    0,    1   },
    { HANDLE_CORNER_ALIGN,  SP_ANCHOR_SW,      GDK_TOP_RIGHT_CORNER,     19,    1,    1   },
    { HANDLE_CORNER_ALIGN,  SP_ANCHOR_NW,      GDK_BOTTOM_RIGHT_CORNER,  20,    1,    0   },
    { HANDLE_CORNER_ALIGN,  SP_ANCHOR_NE,      GDK_BOTTOM_LEFT_CORNER,   21,    0,    0   },
    // clang-format on
};

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

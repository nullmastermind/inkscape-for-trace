// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2017 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SP_CURSOR_H
#define SP_CURSOR_H

typedef unsigned int guint32;
typedef struct _GdkPixbuf GdkPixbuf;
typedef struct _GdkCursor GdkCursor;
typedef struct _GdkColor GdkColor;

GdkCursor* sp_cursor_from_xpm(char const *const *xpm, guint32 fill=0, guint32 stroke=0);

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

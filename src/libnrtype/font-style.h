// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_LIBNRTYPE_FONT_STYLE_H
#define SEEN_LIBNRTYPE_FONT_STYLE_H

#include <2geom/affine.h>
#include <livarot/LivarotDefs.h>

// structure that holds data describing how to render glyphs of a font

// Different raster styles.
struct font_style {
    Geom::Affine  transform; // the ctm. contains the font-size
    bool          vertical;  // should be rendered vertically or not? 
		                // good font support would take the glyph alternates for vertical mode, when present
    double        stroke_width; // if 0, the glyph is filled; otherwise stroked
    JoinType      stroke_join;
    ButtType      stroke_cap;
    float         stroke_miter_limit;
    int           nbDash;
    double        dash_offset;
    double*       dashes;
};


#endif /* !SEEN_LIBNRTYPE_FONT_STYLE_H */

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

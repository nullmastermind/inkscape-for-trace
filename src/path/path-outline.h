// SPDX-License-Identifier: GPL-2.0-or-later

/** @file
 *
 * Two related object to path operations:
 *
 * 1. Find a path that includes fill, stroke, and markers. Useful for finding a visual bounding box.
 * 2. Take a set of objects and find an identical visual representation using only paths.
 *
 * Copyright (C) 2020 Tavmjong Bah
 * Copyright (C) 2018 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_PATH_OUTLINE_H
#define SEEN_PATH_OUTLINE_H

class SPDesktop;
class SPItem;

namespace Geom {
  class PathVector;
}

namespace Inkscape {
namespace XML {
  class Node;
}
}

/**
 * Find an outline that represents an item.
 */
Geom::PathVector* item_to_outline (SPItem const *item, bool exclude_markers = false);

/**
 * Replace item by path objects (a.k.a. stroke to path).
 */
Inkscape::XML::Node* item_to_paths(SPItem *item, bool legacy = false, SPItem *context = nullptr);

/**
 * Replace selected items by path objects (a.k.a. stroke to >path).
 * TODO: remove desktop dependency.
 */
void selection_to_paths (SPDesktop *desktop, bool legacy = false);


#endif // SEEN_PATH_OUTLINE_H

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

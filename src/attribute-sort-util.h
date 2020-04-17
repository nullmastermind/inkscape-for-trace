// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors:
 * see git history
 * Tavmjong Bah
 *
 * Copyright (C) 2016 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef __SP_ATTRIBUTE_SORT_UTIL_H__
#define __SP_ATTRIBUTE_SORT_UTIL_H__

#include "xml/node.h"

using Inkscape::XML::Node;

/**
 * Utility functions for sorting attributes.
 */

/**
 * Sort attributes and CSS properties by name.
 */
void sp_attribute_sort_tree(Node& repr);

#endif /* __SP_ATTRIBUTE_SORT_UTIL_H__ */

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

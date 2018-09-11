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

#include <glibmm/ustring.h>
#include "xml/sp-css-attr.h"

using Inkscape::XML::Node;

/**
 * Utility functions for sorting attributes.
 */

/**
 * Sort attributes by name.
 */
void sp_attribute_sort_tree(Node *repr);

/**
 * Recursively sort.
 * repr: the root node in a document or any other node.
 */
void sp_attribute_sort_recursive(Node *repr);

/**
 * Sort one element (attributes).
 */
void sp_attribute_sort_element(Node *repr);

/**
 * Sort style properties for one element.
 */
void sp_attribute_sort_style(Node *repr);

/**
 * Sort style_property for a style string
 */
Glib::ustring sp_attribute_sort_style(Node *repr, gchar const *string);

/**
 * Sort style properties for one CSS.
 */
void sp_attribute_sort_style(Node* repr, SPCSSAttr *css);

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

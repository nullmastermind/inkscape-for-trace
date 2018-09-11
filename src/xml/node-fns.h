// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Helper functions for XML nodes
 *//*
 * Authors:
 *   see git history
 *   Unknown author
 *   Krzysztof Kosiński <tweenk.pl@gmail.com> (documentation)
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_XML_NODE_FNS_H
#define SEEN_XML_NODE_FNS_H

#include "xml/node.h"

namespace Inkscape {
namespace XML {

bool id_permitted(Node const *node);

//@{
/**
 * @brief Get the next node in sibling order
 * @param node The origin node
 * @return The next node in sibling order
 * @relates Inkscape::XML::Node
 */
inline Node *next_node(Node *node) {
    return ( node ? node->next() : nullptr );
}
inline Node const *next_node(Node const *node) {
    return ( node ? node->next() : nullptr );
}
//@}

//@{
/**
 * @brief Get the previous node in sibling order
 *
 * This method, unlike Node::next(), is a linear search over the children of @c node's parent.
 * The return value is NULL when the node has no parent or is first in the sibling order.
 *
 * @param node The origin node
 * @return The previous node in sibling order, or NULL
 * @relates Inkscape::XML::Node
 */
Node *previous_node(Node *node);
inline Node const *previous_node(Node const *node) {
    return previous_node(const_cast<Node *>(node));
}
//@}

//@{
/**
 * @brief Get the node's parent
 * @param node The origin node
 * @return The node's parent
 * @relates Inkscape::XML::Node
 */
inline Node *parent_node(Node *node) {
    return ( node ? node->parent() : nullptr );
}
inline Node const *parent_node(Node const *node) {
    return ( node ? node->parent() : nullptr );
}
//@}

}
}

#endif /* !SEEN_XML_NODE_FNS_H */
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

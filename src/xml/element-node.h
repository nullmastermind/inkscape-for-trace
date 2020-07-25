// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Element node implementation
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Copyright 2004-2005 MenTaLguY <mental@rydia.net>
 * 
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_XML_ELEMENT_NODE_H
#define SEEN_INKSCAPE_XML_ELEMENT_NODE_H

#include "xml/simple-node.h"

namespace Inkscape {

namespace XML {

/**
 * @brief Element node, e.g. &lt;group /&gt;
 */
class ElementNode : public SimpleNode {
public:
    ElementNode(int code, Document *doc)
    : SimpleNode(code, doc) {}
    ElementNode(ElementNode const &other, Document *doc)
    : SimpleNode(other, doc) {}

    Inkscape::XML::NodeType type() const override { return Inkscape::XML::NodeType::ELEMENT_NODE; }

protected:
    SimpleNode *_duplicate(Document* doc) const override { return new ElementNode(*this, doc); }
};

}

}

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

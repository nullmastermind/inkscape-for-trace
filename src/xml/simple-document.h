// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Inkscape::XML::SimpleDocument - generic XML document implementation
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Copyright 2004-2005 MenTaLguY <mental@rydia.net>
 * 
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_XML_SIMPLE_DOCUMENT_H
#define SEEN_INKSCAPE_XML_SIMPLE_DOCUMENT_H

#include "xml/document.h"
#include "xml/simple-node.h"
#include "xml/node-observer.h"
#include "xml/log-builder.h"

namespace Inkscape {

namespace XML {

class SimpleDocument : public SimpleNode,
                       public Document,
                       public NodeObserver
{
public:
    explicit SimpleDocument()
    : SimpleNode(g_quark_from_static_string("xml"), this),
      _in_transaction(false) {}

    NodeType type() const override { return Inkscape::XML::NodeType::DOCUMENT_NODE; }

    bool inTransaction() override { return _in_transaction; }

    void beginTransaction() override;
    void rollback() override;
    void commit() override;
    Inkscape::XML::Event *commitUndoable() override;

    Node *createElement(char const *name) override;
    Node *createTextNode(char const *content) override;
    Node *createTextNode(char const *content, bool const is_CData) override;
    Node *createComment(char const *content) override;
    Node *createPI(char const *target, char const *content) override;

    void notifyChildAdded(Node &parent, Node &child, Node *prev) override;

    void notifyChildRemoved(Node &parent, Node &child, Node *prev) override;

    void notifyChildOrderChanged(Node &parent, Node &child,
                                 Node *old_prev, Node *new_prev) override;

    void notifyContentChanged(Node &node,
                              Util::ptr_shared old_content,
                              Util::ptr_shared new_content) override;

    void notifyAttributeChanged(Node &node, GQuark name,
                                Util::ptr_shared old_value,
                                Util::ptr_shared new_value) override;

    void notifyElementNameChanged(Node& node, GQuark old_name, GQuark new_name) override;

protected:
    SimpleDocument(SimpleDocument const &doc)
    : Node(), SimpleNode(doc), Document(), NodeObserver(),
      _in_transaction(false)
      {}

    SimpleNode *_duplicate(Document* /*doc*/) const override
    {
        return new SimpleDocument(*this);
    }
    NodeObserver *logger() override { return this; }

private:
    bool _in_transaction;
    LogBuilder _log_builder;
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

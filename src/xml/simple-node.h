// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * GC-managed XML node implementation
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_XML_NODE_H
#error  You have included xml/simple-node.h in your document, which is an implementation.  Chances are that you want xml/node.h.  Please fix that.
#endif

#ifndef SEEN_INKSCAPE_XML_SIMPLE_NODE_H
#define SEEN_INKSCAPE_XML_SIMPLE_NODE_H

#include <cassert>
#include <iostream>
#include <vector>

#include "xml/node.h"
#include "xml/attribute-record.h"
#include "xml/composite-node-observer.h"

namespace Inkscape {

namespace XML {

/**
 * @brief Default implementation of the XML node stored in memory.
 *
 * @see Inkscape::XML::Node
 */
class SimpleNode
: virtual public Node, public Inkscape::GC::Managed<>
{
public:
    char const *name() const override;
    int code() const override { return _name; }
    void setCodeUnsafe(int code) override;

    Document *document() override { return _document; }
    Document const *document() const override {
        return const_cast<SimpleNode *>(this)->document();
    }

    Node *duplicate(Document* doc) const override { return _duplicate(doc); }

    Node *root() override;
    Node const *root() const override {
        return const_cast<SimpleNode *>(this)->root();
    }

    Node *parent() override { return _parent; }
    Node const *parent() const override { return _parent; }

    Node *next() override { return _next; }
    Node const *next() const override { return _next; }
    Node *prev() override { return _prev; }
    Node const *prev() const override { return _prev; }

    Node *firstChild() override { return _first_child; }
    Node const *firstChild() const override { return _first_child; }
    Node *lastChild() override { return _last_child; }
    Node const *lastChild() const override { return _last_child; }

    unsigned childCount() const override { return _child_count; }
    Node *nthChild(unsigned index) override;
    Node const *nthChild(unsigned index) const override {
        return const_cast<SimpleNode *>(this)->nthChild(index);
    }

    void addChild(Node *child, Node *ref) override;
    void appendChild(Node *child) override {
        SimpleNode::addChild(child, _last_child);
    }
    void removeChild(Node *child) override;
    void changeOrder(Node *child, Node *ref) override;

    unsigned position() const override;
    void setPosition(int pos) override;

    char const *attribute(char const *key) const override;
    bool matchAttributeName(char const *partial_name) const override;

    char const *content() const override;
    void setContent(char const *value) override;

    void cleanOriginal(Node *src, gchar const *key) override;
    bool equal(Node const *other, bool recursive) override;
    void mergeFrom(Node const *src, char const *key, bool extension = false, bool clean = false) override;

    const AttributeVector & attributeList() const override {
        return _attributes;
    }

    void synthesizeEvents(NodeEventVector const *vector, void *data) override;
    void synthesizeEvents(NodeObserver &observer) override;

    void addListener(NodeEventVector const *vector, void *data) override {
        assert(vector != NULL);
        _observers.addListener(*vector, data);
    }
    void addObserver(NodeObserver &observer) override {
        _observers.add(observer);
    }
    void removeListenerByData(void *data) override {
        _observers.removeListenerByData(data);
    }
    void removeObserver(NodeObserver &observer) override {
        _observers.remove(observer);
    }

    void addSubtreeObserver(NodeObserver &observer) override {
        _subtree_observers.add(observer);
    }
    void removeSubtreeObserver(NodeObserver &observer) override {
        _subtree_observers.remove(observer);
    }

    void recursivePrintTree(unsigned level = 0) override;

protected:
    SimpleNode(int code, Document *document);
    SimpleNode(SimpleNode const &repr, Document *document);

    virtual SimpleNode *_duplicate(Document *doc) const=0;
    void setAttributeImpl(char const *key, char const *value) override;

private:
    void operator=(Node const &); // no assign

    void _setParent(SimpleNode *parent);
    unsigned _childPosition(SimpleNode const &child) const;

    SimpleNode *_parent;
    SimpleNode *_next;
    SimpleNode *_prev;
    Document *_document;
    mutable unsigned _cached_position;

    int _name;

    AttributeVector _attributes;

    Inkscape::Util::ptr_shared _content;

    unsigned _child_count;
    mutable bool _cached_positions_valid;
    SimpleNode *_first_child;
    SimpleNode *_last_child;

    CompositeNodeObserver _observers;
    CompositeNodeObserver _subtree_observers;
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

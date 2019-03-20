// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Garbage collected XML document implementation.
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Copyright 2005 MenTaLguY <mental@rydia.net>
 * 
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glib.h> // g_assert()

#include "xml/simple-document.h"
#include "xml/event-fns.h"
#include "xml/element-node.h"
#include "xml/text-node.h"
#include "xml/comment-node.h"
#include "xml/pi-node.h"

namespace Inkscape {

namespace XML {

void SimpleDocument::beginTransaction() {
    g_assert(!_in_transaction);
    _in_transaction = true;
}

void SimpleDocument::rollback() {
    g_assert(_in_transaction);
    _in_transaction = false;
    Event *log = _log_builder.detach();
    sp_repr_undo_log(log);
    sp_repr_free_log(log);
}

void SimpleDocument::commit() {
    g_assert(_in_transaction);
    _in_transaction = false;
    _log_builder.discard();
}

Inkscape::XML::Event *SimpleDocument::commitUndoable() {
    g_assert(_in_transaction);
    _in_transaction = false;
    return _log_builder.detach();
}

Node *SimpleDocument::createElement(char const *name) {
    return new ElementNode(g_quark_from_string(name), this);
}

Node *SimpleDocument::createTextNode(char const *content) {
    return new TextNode(Util::share_string(content), this);
}

Node *SimpleDocument::createTextNode(char const *content, bool const is_CData) {
    return new TextNode(Util::share_string(content), this, is_CData);
}

Node *SimpleDocument::createComment(char const *content) {
    return new CommentNode(Util::share_string(content), this);
}

Node *SimpleDocument::createPI(char const *target, char const *content) {
    return new PINode(g_quark_from_string(target), Util::share_string(content), this);
}

void SimpleDocument::notifyChildAdded(Node &parent,
                                      Node &child,
                                      Node *prev)
{
    if (_in_transaction) {
        _log_builder.addChild(parent, child, prev);
    }
}

void SimpleDocument::notifyChildRemoved(Node &parent,
                                        Node &child,
                                        Node *prev)
{
    if (_in_transaction) {
        _log_builder.removeChild(parent, child, prev);
    }
}

void SimpleDocument::notifyChildOrderChanged(Node &parent,
                                             Node &child,
                                             Node *old_prev,
                                             Node *new_prev)
{
    if (_in_transaction) {
        _log_builder.setChildOrder(parent, child, old_prev, new_prev);
    }
}

void SimpleDocument::notifyContentChanged(Node &node,
                                          Util::ptr_shared old_content,
                                          Util::ptr_shared new_content)
{
    if (_in_transaction) {
        _log_builder.setContent(node, old_content, new_content);
    }
}

void SimpleDocument::notifyAttributeChanged(Node &node,
                                            GQuark name,
                                            Util::ptr_shared old_value,
                                            Util::ptr_shared new_value)
{
    if (_in_transaction) {
        _log_builder.setAttribute(node, name, old_value, new_value);
    }
}

void SimpleDocument::notifyElementNameChanged(Node& node, GQuark old_name, GQuark new_name)
{
    if (_in_transaction) {
        _log_builder.setElementName(node, old_name, new_name);
    }
}

} // end namespace XML
} // end namespace Inkscape

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

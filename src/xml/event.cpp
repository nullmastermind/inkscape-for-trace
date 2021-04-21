// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Repr transaction logging
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   MenTaLguY  <mental@rydia.net>
 *
 * Copyright (C) 2004-2005 MenTaLguY
 * Copyright (C) 1999-2003 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 * g++ port Copyright (C) 2003 Nathan Hurst
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glib.h> // g_assert()
#include <cstdio>

#include "event.h"
#include "event-fns.h"
#include "xml/document.h"
#include "xml/node-observer.h"
#include "debug/event-tracker.h"
#include "debug/simple-event.h"

int Inkscape::XML::Event::_next_serial=0;

void
sp_repr_begin_transaction (Inkscape::XML::Document *doc)
{
    using Inkscape::Debug::SimpleEvent;
    using Inkscape::Debug::EventTracker;
    using Inkscape::Debug::Event;

    EventTracker<SimpleEvent<Event::XML> > tracker("begin-transaction");

    g_assert(doc != nullptr);
    doc->beginTransaction();
}

void
sp_repr_rollback (Inkscape::XML::Document *doc)
{
    using Inkscape::Debug::SimpleEvent;
    using Inkscape::Debug::EventTracker;
    using Inkscape::Debug::Event;

    EventTracker<SimpleEvent<Event::XML> > tracker("rollback");

    g_assert(doc != nullptr);
    doc->rollback();
}

void
sp_repr_commit (Inkscape::XML::Document *doc)
{
    using Inkscape::Debug::SimpleEvent;
    using Inkscape::Debug::EventTracker;
    using Inkscape::Debug::Event;

    EventTracker<SimpleEvent<Event::XML> > tracker("commit");

    g_assert(doc != nullptr);
    doc->commit();
}

Inkscape::XML::Event *
sp_repr_commit_undoable (Inkscape::XML::Document *doc)
{
    using Inkscape::Debug::SimpleEvent;
    using Inkscape::Debug::EventTracker;
    using Inkscape::Debug::Event;

    EventTracker<SimpleEvent<Event::XML> > tracker("commit");

    g_assert(doc != nullptr);
    return doc->commitUndoable();
}

namespace {

class LogPerformer : public Inkscape::XML::NodeObserver {
public:
    typedef Inkscape::XML::Node Node;

    static LogPerformer &instance() {
        static LogPerformer singleton;
        return singleton;
    }

    void notifyChildAdded(Node &parent, Node &child, Node *ref) override {
        parent.addChild(&child, ref);
    }

    void notifyChildRemoved(Node &parent, Node &child, Node */*old_ref*/) override {
        parent.removeChild(&child);
    }

    void notifyChildOrderChanged(Node &parent, Node &child,
                         Node */*old_ref*/, Node *new_ref) override
    {
        parent.changeOrder(&child, new_ref);
    }

    void notifyAttributeChanged(Node &node, GQuark name,
                        Inkscape::Util::ptr_shared /*old_value*/,
                    Inkscape::Util::ptr_shared new_value) override
    {
        node.setAttribute(g_quark_to_string(name), new_value);
    }

    void notifyContentChanged(Node &node,
                      Inkscape::Util::ptr_shared /*old_value*/,
                  Inkscape::Util::ptr_shared new_value) override
    {
        node.setContent(new_value);
    }

    void notifyElementNameChanged(Node& node, GQuark /*old_value*/, GQuark new_value) override
    {
        node.setCodeUnsafe(new_value);
    }
};

}

void Inkscape::XML::undo_log_to_observer(
    Inkscape::XML::Event const *log,
    Inkscape::XML::NodeObserver &observer
) {
    for ( Event const *action = log ; action ; action = action->next ) {
        action->undoOne(observer);
    }
}

void sp_repr_undo_log (Inkscape::XML::Event *log)
{
    using Inkscape::Debug::SimpleEvent;
    using Inkscape::Debug::EventTracker;
    using Inkscape::Debug::Event;

    EventTracker<SimpleEvent<Event::XML> > tracker("undo-log");

    if (log) {
        if (log->repr) {
            g_assert(!log->repr->document()->inTransaction());
        }
    }

    Inkscape::XML::undo_log_to_observer(log, LogPerformer::instance());
}

void Inkscape::XML::EventAdd::_undoOne(
    Inkscape::XML::NodeObserver &observer
) const {
    observer.notifyChildRemoved(*this->repr, *this->child, this->ref);
}

void Inkscape::XML::EventDel::_undoOne(
    Inkscape::XML::NodeObserver &observer
) const {
    observer.notifyChildAdded(*this->repr, *this->child, this->ref);
}

void Inkscape::XML::EventChgAttr::_undoOne(
    Inkscape::XML::NodeObserver &observer
) const {
    observer.notifyAttributeChanged(*this->repr, this->key, this->newval, this->oldval);
}

void Inkscape::XML::EventChgContent::_undoOne(
    Inkscape::XML::NodeObserver &observer
) const {
    observer.notifyContentChanged(*this->repr, this->newval, this->oldval);
}

void Inkscape::XML::EventChgOrder::_undoOne(
    Inkscape::XML::NodeObserver &observer
) const {
    observer.notifyChildOrderChanged(*this->repr, *this->child, this->newref, this->oldref);
}

void Inkscape::XML::EventChgElementName::_undoOne(
    Inkscape::XML::NodeObserver& observer
) const {
    observer.notifyElementNameChanged(*this->repr, this->new_name, this->old_name);
}

void Inkscape::XML::replay_log_to_observer(
    Inkscape::XML::Event const *log,
    Inkscape::XML::NodeObserver &observer
) {
    std::vector<Inkscape::XML::Event const *> r;
    while (log) {
        r.push_back(log);
        log = log->next;
    }
    for ( auto reversed = r.rbegin(); reversed != r.rend(); ++reversed ) {
        (*reversed)->replayOne(observer);
    }
}

void
sp_repr_replay_log (Inkscape::XML::Event *log)
{
    using Inkscape::Debug::SimpleEvent;
    using Inkscape::Debug::EventTracker;
    using Inkscape::Debug::Event;

    EventTracker<SimpleEvent<Event::XML> > tracker("replay-log");

    if (log) {
        if (log->repr->document()) {
            g_assert(!log->repr->document()->inTransaction());
        }
    }

    Inkscape::XML::replay_log_to_observer(log, LogPerformer::instance());
}

void Inkscape::XML::EventAdd::_replayOne(
    Inkscape::XML::NodeObserver &observer
) const {
    observer.notifyChildAdded(*this->repr, *this->child, this->ref);
}

void Inkscape::XML::EventDel::_replayOne(
    Inkscape::XML::NodeObserver &observer
) const {
    observer.notifyChildRemoved(*this->repr, *this->child, this->ref);
}

void Inkscape::XML::EventChgAttr::_replayOne(
    Inkscape::XML::NodeObserver &observer
) const {
    observer.notifyAttributeChanged(*this->repr, this->key, this->oldval, this->newval);
}

void Inkscape::XML::EventChgContent::_replayOne(
    Inkscape::XML::NodeObserver &observer
) const {
    observer.notifyContentChanged(*this->repr, this->oldval, this->newval);
}

void Inkscape::XML::EventChgOrder::_replayOne(
    Inkscape::XML::NodeObserver &observer
) const {
    observer.notifyChildOrderChanged(*this->repr, *this->child, this->oldref, this->newref);
}

void Inkscape::XML::EventChgElementName::_replayOne(
    Inkscape::XML::NodeObserver &observer
) const {
    observer.notifyElementNameChanged(*this->repr, this->old_name, this->new_name);
}

Inkscape::XML::Event *
sp_repr_coalesce_log (Inkscape::XML::Event *a, Inkscape::XML::Event *b)
{
    Inkscape::XML::Event *action;
    Inkscape::XML::Event **prev_ptr;

    if (!b) return a;
    if (!a) return b;

    /* find the earliest action in the second log */
    /* (also noting the pointer that references it, so we can
     *  replace it later) */
    prev_ptr = &b;
    for ( action = b ; action->next ; action = action->next ) {
        prev_ptr = &action->next;
    }

    /* add the first log after it */
    action->next = a;

    /* optimize the result */
    *prev_ptr = action->optimizeOne();

    return b;
}

void
sp_repr_free_log (Inkscape::XML::Event *log)
{
    while (log) {
        Inkscape::XML::Event *action;
        action = log;
        log = action->next;
        delete action;
    }
}

namespace {

template <typename T> struct ActionRelations;

template <>
struct ActionRelations<Inkscape::XML::EventAdd> {
    typedef Inkscape::XML::EventDel Opposite;
};

template <>
struct ActionRelations<Inkscape::XML::EventDel> {
    typedef Inkscape::XML::EventAdd Opposite;
};

template <typename A>
Inkscape::XML::Event *cancel_add_or_remove(A *action) {
    typedef typename ActionRelations<A>::Opposite Opposite;
    Opposite *opposite=dynamic_cast<Opposite *>(action->next);

    bool OK = false;
    if (opposite){
        if (opposite->repr == action->repr &&
            opposite->child == action->child &&
            opposite->ref == action->ref ) {
            OK = true;
        }
    }
    if (OK){
        Inkscape::XML::Event *remaining=opposite->next;

        delete opposite;
        delete action;

        return remaining;
    } else {
        return action;
    }
}
}

Inkscape::XML::Event *Inkscape::XML::EventAdd::_optimizeOne() {
    return cancel_add_or_remove(this);
}

Inkscape::XML::Event *Inkscape::XML::EventDel::_optimizeOne() {
    return cancel_add_or_remove(this);
}

Inkscape::XML::Event *Inkscape::XML::EventChgAttr::_optimizeOne() {
    Inkscape::XML::EventChgAttr *chg_attr=dynamic_cast<Inkscape::XML::EventChgAttr *>(this->next);

    /* consecutive chgattrs on the same key can be combined */
    if ( chg_attr) {
        if ( chg_attr->repr == this->repr &&
             chg_attr->key == this->key )
        {
            /* replace our oldval with the prior action's */
            this->oldval = chg_attr->oldval;

            /* discard the prior action */
            this->next = chg_attr->next;
            delete chg_attr;
        }
    }

    return this;
}

Inkscape::XML::Event *Inkscape::XML::EventChgContent::_optimizeOne() {
    Inkscape::XML::EventChgContent *chg_content=dynamic_cast<Inkscape::XML::EventChgContent *>(this->next);

    /* consecutive content changes can be combined */
    if (chg_content) {
        if (chg_content->repr == this->repr ) {
            /* replace our oldval with the prior action's */
            this->oldval = chg_content->oldval;

            /* get rid of the prior action*/
            this->next = chg_content->next;
            delete chg_content;
        }
    }

    return this;
}

Inkscape::XML::Event *Inkscape::XML::EventChgOrder::_optimizeOne() {
    Inkscape::XML::EventChgOrder *chg_order=dynamic_cast<Inkscape::XML::EventChgOrder *>(this->next);

    /* consecutive chgorders for the same child may be combined or
     * canceled out */
    bool OK = false;
    if (chg_order) {
        if (chg_order->repr == this->repr &&
            chg_order->child == this->child ){
            OK = true;
        }
    }
    if (OK)    {
        if ( chg_order->oldref == this->newref ) {
            /* cancel them out */
            Inkscape::XML::Event *after=chg_order->next;

            delete chg_order;
            delete this;

            return after;
        } else {
            /* combine them */
            this->oldref = chg_order->oldref;

            /* get rid of the other one */
            this->next = chg_order->next;
            delete chg_order;

            return this;
        }
    } else {
        return this;
    }
}

Inkscape::XML::Event* Inkscape::XML::EventChgElementName::_optimizeOne() {
    auto next_chg_element_name = dynamic_cast<Inkscape::XML::EventChgElementName*>(this->next);
    if (next_chg_element_name && next_chg_element_name->repr == this->repr) {
        // Combine name changes to the same element.
        this->old_name = next_chg_element_name->old_name;
        this->next = next_chg_element_name->next;
        delete next_chg_element_name;
    }
    return this;
}

namespace {

class LogPrinter : public Inkscape::XML::NodeObserver {
public:
    typedef Inkscape::XML::Node Node;

    static LogPrinter &instance() {
        static LogPrinter singleton;
        return singleton;
    }

    static Glib::ustring node_to_string(Node const &node) {
        Glib::ustring result;
        char const *type_name=nullptr;
        switch (node.type()) {
        case Inkscape::XML::NodeType::DOCUMENT_NODE:
            type_name = "Document";
            break;
        case Inkscape::XML::NodeType::ELEMENT_NODE:
            type_name = "Element";
            break;
        case Inkscape::XML::NodeType::TEXT_NODE:
            type_name = "Text";
            break;
        case Inkscape::XML::NodeType::COMMENT_NODE:
            type_name = "Comment";
            break;
        default:
            g_assert_not_reached();
        }
        char buffer[40];
        result.append("#<");
        result.append(type_name);
        result.append(":");
        snprintf(buffer, 40, "0x%p", &node);
        result.append(buffer);
        result.append(">");

        return result;
    }

    static Glib::ustring ref_to_string(Node *ref) {
        if (ref) {
            return node_to_string(*ref);
        } else {
            return "beginning";
        }
    }

    void notifyChildAdded(Node &parent, Node &child, Node *ref) override {
        g_warning("Event: Added %s to %s after %s", node_to_string(parent).c_str(), node_to_string(child).c_str(), ref_to_string(ref).c_str());
    }

    void notifyChildRemoved(Node &parent, Node &child, Node */*ref*/) override {
        g_warning("Event: Removed %s from %s", node_to_string(parent).c_str(), node_to_string(child).c_str());
    }

    void notifyChildOrderChanged(Node &parent, Node &child,
                                 Node */*old_ref*/, Node *new_ref) override
    {
        g_warning("Event: Moved %s after %s in %s", node_to_string(child).c_str(), ref_to_string(new_ref).c_str(), node_to_string(parent).c_str());
    }

    void notifyAttributeChanged(Node &node, GQuark name,
                                Inkscape::Util::ptr_shared /*old_value*/,
                    Inkscape::Util::ptr_shared new_value) override
    {
        if (new_value) {
            g_warning("Event: Set attribute %s to \"%s\" on %s", g_quark_to_string(name), new_value.pointer(), node_to_string(node).c_str());
        } else {
            g_warning("Event: Unset attribute %s on %s", g_quark_to_string(name), node_to_string(node).c_str());
        }
    }

    void notifyContentChanged(Node &node,
                      Inkscape::Util::ptr_shared /*old_value*/,
                  Inkscape::Util::ptr_shared new_value) override
    {
        if (new_value) {
            g_warning("Event: Set content of %s to \"%s\"", node_to_string(node).c_str(), new_value.pointer());
        } else {
            g_warning("Event: Unset content of %s", node_to_string(node).c_str());
        }
    }

    void notifyElementNameChanged(Node& node, GQuark old_value, GQuark new_value) override
    {
        g_warning("Event: Changed name of %s from %s to %s\n",
                node_to_string(node).c_str(), g_quark_to_string(old_value), g_quark_to_string(new_value));
    }
};

}

void sp_repr_debug_print_log(Inkscape::XML::Event const *log) {
    Inkscape::XML::replay_log_to_observer(log, LogPrinter::instance());
}


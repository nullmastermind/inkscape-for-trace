// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::GC::Anchored - base class for anchored GC-managed objects
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2004 MenTaLguY
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <typeinfo>
#include "gc-anchored.h"
#include "debug/event-tracker.h"
#include "debug/simple-event.h"
#include "debug/demangle.h"
#include "util/format.h"

namespace Inkscape {

namespace GC {

namespace {

typedef Debug::SimpleEvent<Debug::Event::REFCOUNT> RefCountEvent;

class BaseAnchorEvent : public RefCountEvent {
public:
    BaseAnchorEvent(Anchored const *object, int bias,
                    char const *name)
    : RefCountEvent(name)
    {
        _addProperty("base", Util::format("%p", Core::base(const_cast<Anchored *>(object))).pointer());
        _addProperty("pointer", Util::format("%p", object).pointer());
        _addProperty("class", Debug::demangle(typeid(*object).name()));
        _addProperty("new-refcount", object->_anchored_refcount() + bias);
    }
};

class AnchorEvent : public BaseAnchorEvent {
public:
    AnchorEvent(Anchored const *object)
    : BaseAnchorEvent(object, 1, "gc-anchor")
    {}
};

class ReleaseEvent : public BaseAnchorEvent {
public:
    ReleaseEvent(Anchored const *object)
    : BaseAnchorEvent(object, -1, "gc-release")
    {}
};

}

Anchored::Anchor *Anchored::_new_anchor() const {
    return new Anchor(this);
}

void Anchored::_free_anchor(Anchored::Anchor *anchor) const {
    delete anchor;
}

void Anchored::anchor() const {
    Debug::EventTracker<AnchorEvent> tracker(this);
    if (!_anchor) {
        _anchor = _new_anchor();
    }
    _anchor->refcount++;
}

void Anchored::release() const {
    Debug::EventTracker<ReleaseEvent> tracker(this);
    g_return_if_fail(_anchor);
    if (!--_anchor->refcount) {
        _free_anchor(_anchor);
        _anchor = nullptr;
    }
}

}

}

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

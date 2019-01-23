// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::Debug::Event - event for debug tracing
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2005 MenTaLguY
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_DEBUG_EVENT_H
#define SEEN_INKSCAPE_DEBUG_EVENT_H

#include <utility>
#include "util/share.h"

namespace Inkscape {

namespace Debug {

class Event {
public:
    virtual ~Event() = default;

    enum Category {
        CORE=0,
        XML,
        SPOBJECT,
        DOCUMENT,
        REFCOUNT,
        EXTENSION,
        FINALIZERS,
        INTERACTION,
        CONFIGURATION,
        OTHER
    };
    enum { N_CATEGORIES=OTHER+1 };

    struct PropertyPair {
    public:
        PropertyPair() = default;
        PropertyPair(char const *n, Util::ptr_shared v)
        : name(n), value(v) {}
        PropertyPair(char const *n, char const *v)
        : name(n),
          value(Util::share_string(v)) {}

        char const *name;
        Util::ptr_shared value;
    };

    static Category category() { return OTHER; }

    // To reduce allocations, we assume the name here is always allocated statically and will never
    // need to be deallocated.  It would be nice to be able to assert that during the creation of
    // the Event though.
    virtual char const *name() const=0;
    virtual unsigned propertyCount() const=0;
    virtual PropertyPair property(unsigned property) const=0;

    virtual void generateChildEvents() const=0;
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

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Inkscape::GC::Alloc - GC-aware STL allocator
 *//*
 * Authors:
 * see git history
 * MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_GC_ALLOC_H
#define SEEN_INKSCAPE_GC_ALLOC_H

#include <cstddef>
#include "inkgc/gc-core.h"

namespace Inkscape {

namespace GC {

template <typename T, CollectionPolicy collect>
class Alloc {
public:
    typedef T value_type;
    typedef T *pointer;
    typedef std::size_t size_type;

    template <typename U>
    struct rebind { typedef Alloc<U, collect> other; };

    Alloc() = default;
    template <typename U> Alloc(Alloc<U, collect> const &) {}

    pointer allocate(size_type count, void const * =nullptr) {
        return static_cast<pointer>(::operator new(count * sizeof(T), SCANNED, collect));
    }

    void deallocate(pointer p, size_type) { ::operator delete(p, GC); }
};

// allocators with the same collection policy are interchangeable

template <typename T1, typename T2,
          CollectionPolicy collect1, CollectionPolicy collect2>
bool operator==(Alloc<T1, collect1> const &, Alloc<T2, collect2> const &) {
    return collect1 == collect2;
}

template <typename T1, typename T2,
          CollectionPolicy collect1, CollectionPolicy collect2>
bool operator!=(Alloc<T1, collect1> const &, Alloc<T2, collect2> const &) {
    return collect1 != collect2;
}

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

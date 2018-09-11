// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors:
 * see git history
 * MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_GC_SOFT_PTR_H
#define SEEN_INKSCAPE_GC_SOFT_PTR_H

#include "inkgc/gc-core.h"

namespace Inkscape {

namespace GC {

/**
 * A class for pointers which can be automatically cleared to break
 * finalization cycles.
 */
template <typename T>
class soft_ptr {
public:
    soft_ptr(T *pointer=nullptr) : _pointer(pointer) {
        _register();
    }

    operator T *() const { return static_cast<T *>(_pointer); }
    T &operator*() const { return *static_cast<T *>(_pointer); }
    T *operator->() const { return static_cast<T *>(_pointer); } 
    T &operator[](int i) const { return static_cast<T *>(_pointer)[i]; }

    soft_ptr &operator=(T *pointer) {
        _pointer = pointer;
        return *this;
    }

    // default copy

private:
    void _register() {
        void *base=Core::base(this);
        if (base) {
            Core::general_register_disappearing_link(&_pointer, base);
        }
    }

    void *_pointer;
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

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::Debug::SysVHeap - malloc() statistics via System V interface
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2005 MenTaLguY
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_DEBUG_SYSV_HEAP_H
#define SEEN_INKSCAPE_DEBUG_SYSV_HEAP_H

#include "debug/heap.h"

namespace Inkscape {
namespace Debug {

class SysVHeap : public Heap {
public:
    SysVHeap() = default;
    
    int features() const override;

    char const *name() const override {
        return "standard malloc()";
    }
    Stats stats() const override;
    void force_collect() override {}
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

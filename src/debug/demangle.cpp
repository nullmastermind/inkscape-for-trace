// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::Debug::demangle - demangle C++ symbol names
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2006 MenTaLguY
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <boost/core/demangle.hpp>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include "debug/demangle.h"

namespace Inkscape {

namespace Debug {

namespace {

struct string_less_than {
    bool operator()(char const *a, char const *b) const {
        return ( strcmp(a, b) < 0 );
    }
};

typedef std::map<char const *, std::shared_ptr<std::string>, string_less_than> MangleCache;
MangleCache mangle_cache;

}

std::shared_ptr<std::string> demangle(char const *name) {
    MangleCache::iterator found=mangle_cache.find(name);
    if ( found != mangle_cache.end() ) {
        return (*found).second;
    }

    std::string result = boost::core::demangle(name);
    return mangle_cache[name] = std::make_shared<std::string>(result);
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

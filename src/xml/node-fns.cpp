// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifdef HAVE_CONFIG_H
#endif

#include <map>
#include <cstring>
#include <string>
#include <glib.h> // g_assert()

#include "xml/node-iterators.h"
#include "node-fns.h"

namespace Inkscape {
namespace XML {

/* the id_permitted stuff is a temporary-ish hack */

namespace {

bool id_permitted_internal(GQuark qname) {
    char const *qname_s=g_quark_to_string(qname);
    return !strncmp("svg:", qname_s, 4) || !strncmp("sodipodi:", qname_s, 9) ||
           !strncmp("inkscape:", qname_s, 9);
}


bool id_permitted_internal_memoized(GQuark qname) {
    typedef std::map<GQuark, bool> IdPermittedMap;
    static IdPermittedMap id_permitted_names;

    IdPermittedMap::iterator found;
    found = id_permitted_names.find(qname);
    if ( found != id_permitted_names.end() ) {
        return found->second;
    } else {
        bool permitted=id_permitted_internal(qname);
        id_permitted_names[qname] = permitted;
        return permitted;
    }
}

}

bool id_permitted(Node const *node) {
    g_return_val_if_fail(node != nullptr, false);

    if ( node->type() != NodeType::ELEMENT_NODE ) {
        return false;
    }

    return id_permitted_internal_memoized((GQuark)node->code());
}

struct node_matches {
    node_matches(Node const &n) : node(n) {}
    bool operator()(Node const &other) { return &other == &node; }
    Node const &node;
};

// documentation moved to header
Node *previous_node(Node *node) {
    return node->prev();
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

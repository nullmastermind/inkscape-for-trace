/*
 * Multiindex container for selection
 *
 * Authors:
 *   Adrian Boguszewski
 *
 * Copyright (C) 2016 Adrian Boguszewski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <sigc++/sigc++.h>
#include "object-set.h"

bool ObjectSet::add(SPObject* object) {
    // any ancestor is in the set - do nothing
    if (_anyAncestorIsInSet(object)) {
        return false;
    }

    // very nice function, but needs refinement
    // check if there is mutual ancestor for some elements, which can replace all of them in the set
//    object = _getMutualAncestor(object);

    // remove all descendants from the set
    _removeDescendantsFromSet(object);

    _add(object);
    return true;
}

bool ObjectSet::remove(SPObject* object) {
    // object is the top of subtree
    if (contains(object)) {
        _remove(object);
        return true;
    }

    // any ancestor of object is in the set
    if (_anyAncestorIsInSet(object)) {
        _removeAncestorsFromSet(object);
        return true;
    }

    // no object nor any parent in the set
    return false;
}

bool ObjectSet::contains(SPObject* object) {
    return container.get<hashed>().find(object) != container.get<hashed>().end();
}

void ObjectSet::clear() {
    for (auto object: container) {
        _remove(object);
    }
}

int ObjectSet::size() {
    return container.size();
}

bool ObjectSet::_anyAncestorIsInSet(SPObject *object) {
    SPObject* o = object;
    while (o != nullptr) {
        if (contains(o)) {
            return true;
        }
        o = o->parent;
    }

    return false;
}

void ObjectSet::_removeDescendantsFromSet(SPObject *object) {
    for (auto& child: object->childList(false)) {
        if (contains(child)) {
            _remove(child);
            // there is certainly no children of this child in the set
            continue;
        }

        _removeDescendantsFromSet(child);
    }
}

void ObjectSet::_remove(SPObject *object) {
    releaseConnections[object].disconnect();
    releaseConnections.erase(object);
    container.get<hashed>().erase(object);
}

void ObjectSet::_add(SPObject *object) {
    releaseConnections[object] = object->connectRelease(sigc::hide_return(sigc::mem_fun(*this, &ObjectSet::remove)));
    container.push_back(object);
}

SPObject *ObjectSet::_getMutualAncestor(SPObject *object) {
    SPObject *o = object;

    bool flag = true;
    while (o->parent != nullptr) {
        for (auto &child: o->parent->childList(false)) {
            if(child != o && !contains(child)) {
                flag = false;
                break;
            }
        }
        if (!flag) {
            break;
        }
        o = o->parent;
    }
    return o;
}

void ObjectSet::_removeAncestorsFromSet(SPObject *object) {
    SPObject* o = object;
    while (o->parent != nullptr) {
        for (auto &child: o->parent->childList(false)) {
            if (child != o) {
                _add(child);
            }
        }
        if (contains(o->parent)) {
            _remove(o->parent);
            break;
        }
        o = o->parent;
    }
}

ObjectSet::~ObjectSet() {
    clear();
}

multi_index_container::iterator ObjectSet::begin() {
    return container.begin();
}

multi_index_container::iterator ObjectSet::end() {
    return container.end();
}

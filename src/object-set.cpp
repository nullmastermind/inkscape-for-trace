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

#include "object-set.h"

bool ObjectSet::add(Object* object) {
    // any ancestor is in the set - do nothing
    if (_anyAncestorIsInSet(object)) {
        return false;
    }

    // check if there is mutual ancestor for some elements, which can replace all of them in the set
    Object* o = _getMutualAncestor(object);

    // remove all descendants from the set
    _removeDescendantsFromSet(o);

    _add(o);
    return true;
}

bool ObjectSet::remove(Object* object) {
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

bool ObjectSet::contains(Object* object) {
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

bool ObjectSet::_anyAncestorIsInSet(Object *object) {
    Object* o = object;
    while (o != nullptr) {
        if (contains(o)) {
            return true;
        }
        o = o->getParent();
    }

    return false;
}

void ObjectSet::_removeDescendantsFromSet(Object *object) {
    for (auto& child: object->getChildren()) {
        if (contains(&child)) {
            _remove(&child);
            // there is certainly no children of this child in the set
            continue;
        }

        _removeDescendantsFromSet(&child);
    }
}

void ObjectSet::_remove(Object *object) {
    releaseConnections[object].disconnect();
    releaseConnections.erase(object);
    container.get<hashed>().erase(object);
}

void ObjectSet::_add(Object *object) {
    releaseConnections[object] = object->connectRelease(sigc::mem_fun(*this, &ObjectSet::remove));
    container.push_back(object);
}

Object *ObjectSet::_getMutualAncestor(Object *object) {
    Object *o = object;

    bool flag = true;
    while (o->getParent() != nullptr) {
        for (auto &child: o->getParent()->getChildren()) {
            if(&child != o && !contains(&child)) {
                flag = false;
                break;
            }
        }
        if (!flag) {
            break;
        }
        o = o->getParent();
    }
    return o;
}

void ObjectSet::_removeAncestorsFromSet(Object *object) {
    Object* o = object;
    while (o->getParent() != nullptr) {
        for (auto &child: o->getParent()->getChildren()) {
            if (&child != o) {
                _add(&child);
            }
        }
        if (contains(o->getParent())) {
            _remove(o->getParent());
            break;
        }
        o = o->getParent();
    }
}

ObjectSet::~ObjectSet() {
    clear();
}

/*
 * Temporary file
 *
 * Authors:
 *   Adrian Boguszewski
 *
 * Copyright (C) 2016 Adrian Boguszewski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "object.h"

Object::Object(std::string name) : name(name), parent(NULL) { }

Object::~Object() {
    // only for this prototype
    // call destructor on every child
    children.clear_and_dispose(delete_disposer());
    release_signal.emit(this);
}

const std::string& Object::getName() const {
    return name;
}

Object *Object::getParent()  {
    return parent;
}

bool Object::isDescendantOf(Object *o) {
    Object* p = parent;
    while(p != NULL) {
        if (p == o) {
            return true;
        }
        p = p->parent;
    }
    return false;
}

void Object::addChild(Object* o) {
    o->parent = this;
    children.push_back(*o);
}

sigc::connection Object::connectRelease(sigc::slot<bool, Object*> slot) {
    return release_signal.connect(slot);
}


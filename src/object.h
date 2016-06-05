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

#ifndef INKSCAPE_PROTOTYPE_OBJECT_H
#define INKSCAPE_PROTOTYPE_OBJECT_H

#include <string>
#include <sigc++/signal.h>
#include <sigc++/connection.h>
#include <boost/intrusive/list.hpp>

// this causes some warning, but it's only for this prototype
class Object;
struct delete_disposer
{
    void operator()(Object *delete_this) {
        delete delete_this;
    }
};

class Object {
private:
    std::string name;
    Object* parent;

    typedef boost::intrusive::list_member_hook<
            boost::intrusive::link_mode<
                    boost::intrusive::auto_unlink
            >> list_hook;
    list_hook child_hook;

    typedef boost::intrusive::list<
            Object,
            boost::intrusive::constant_time_size<false>,
            boost::intrusive::member_hook<
                    Object,
                    list_hook,
                    &Object::child_hook
            >> list;
    list children;

    sigc::signal<bool, Object*> release_signal;

public:
    Object(std::string name);
    virtual ~Object();

    list &getChildren() {
        return children;
    }

    const std::string &getName() const;
    Object * getParent() ;

    sigc::connection connectRelease(sigc::slot<bool, Object*> slot);
    void addChild(Object* o);
    bool isDescendantOf(Object* o);

    bool operator==(Object o) {
        return name == o.name;
    }

    bool operator!=(Object o) {
        return name != o.name;
    }
};

#endif //INKSCAPE_PROTOTYPE_OBJECT_H

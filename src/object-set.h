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

#ifndef INKSCAPE_PROTOTYPE_OBJECTSET_H
#define INKSCAPE_PROTOTYPE_OBJECTSET_H

#include <string>
#include <unordered_map>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <sigc++/connection.h>
#include "sp-object.h"

struct hashed{};

typedef boost::multi_index_container<
        SPObject*,
        boost::multi_index::indexed_by<
                boost::multi_index::sequenced<>,
                boost::multi_index::hashed_unique<
                        boost::multi_index::tag<hashed>,
                        boost::multi_index::identity<SPObject*>>
        >> multi_index_container;

class ObjectSet {
public:
    ObjectSet() {};
    ~ObjectSet();
    bool add(SPObject* object);
    bool remove(SPObject* object);
    bool contains(SPObject* object);
    void clear();
    int size();

private:
    void _add(SPObject* object);
    void _remove(SPObject* object);
    bool _anyAncestorIsInSet(SPObject *object);
    void _removeDescendantsFromSet(SPObject *object);
    void _removeAncestorsFromSet(SPObject *object);
    SPObject *_getMutualAncestor(SPObject *object);

    multi_index_container container;
    std::unordered_map<SPObject*, sigc::connection> releaseConnections;
};


#endif //INKSCAPE_PROTOTYPE_OBJECTSET_H

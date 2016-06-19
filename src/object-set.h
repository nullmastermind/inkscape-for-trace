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
#include <boost/multi_index/random_access_index.hpp>
#include <boost/range/sub_range.hpp>
#include <boost/range/any_range.hpp>
#include <sigc++/connection.h>
#include "sp-object.h"
#include "sp-item.h"

class SPBox3D;
class Persp3D;

struct hashed{};
struct random_access{};

typedef boost::multi_index_container<
        SPObject*,
        boost::multi_index::indexed_by<
                boost::multi_index::sequenced<>,
                boost::multi_index::random_access<
                        boost::multi_index::tag<random_access>>,
                boost::multi_index::hashed_unique<
                        boost::multi_index::tag<hashed>,
                        boost::multi_index::identity<SPObject*>>
        >> multi_index_container;

typedef boost::any_range<
        SPObject*,
        boost::random_access_traversal_tag,
        SPObject* const&,
        std::ptrdiff_t> SPObjectRange;

typedef boost::any_range<
        SPItem*,
        boost::random_access_traversal_tag,
        SPItem* const&,
        std::ptrdiff_t> SPItemRange;

class ObjectSet {
public:
    enum CompareSize {HORIZONTAL, VERTICAL, AREA};

    ObjectSet() {};
    virtual ~ObjectSet();

    /**
     * Add an SPObject to the set of selected objects.
     *
     * @param obj the SPObject to add
     */
    bool add(SPObject* object);

    /**  Add items from an STL iterator range to the selection.
     *  \param from the begin iterator
     *  \param to the end iterator
     */
    void add(const std::vector<SPItem*>::iterator& from, const std::vector<SPItem*>::iterator& to);

    /**
     * Removes an item from the set of selected objects.
     *
     * It is ok to call this method for an unselected item.
     *
     * @param item the item to unselect
     *
     * @return is success
     */
    bool remove(SPObject* object);

    /**
     * Returns true if the given object is selected.
     */
    bool includes(SPObject *object);

    /**
     * Set the selection to a single specific object.
     *
     * @param obj the object to select
     */
    void set(SPObject *object);

    /**
     * Unselects all selected objects.
     */
    void clear();

    /**
     * Returns size of the selection.
     */
    int size();

    /**
     * Returns true if no items are selected.
     */
    bool isEmpty();

    /**
     * Removes an item if selected, adds otherwise.
     *
     * @param item the item to unselect
     */
    void toggle(SPObject *obj);

    /**
     * Returns a single selected object.
     *
     * @return NULL unless exactly one object is selected
     */
    SPObject *single();

    /**
     * Returns a single selected item.
     *
     * @return NULL unless exactly one object is selected
     */
    SPItem *singleItem();

    /**
     * Returns the smallest item from this selection.
     */
    SPItem *smallestItem(CompareSize compare);

    /**
     * Returns the largest item from this selection.
     */
    SPItem *largestItem(CompareSize compare);

    /** Returns the list of selected objects. */
    SPObjectRange range();

    /** Returns the list of selected SPItems. */
    std::vector<SPItem*> itemList();

    /**
     * Selects exactly the specified objects.
     *
     * @param objs the objects to select
     */
    void setList(const std::vector<SPItem *> &objs);

    /**
     * Adds the specified objects to selection, without deselecting first.
     *
     * @param objs the objects to select
     */
    void addList(std::vector<SPItem*> const &objs);

    /** Returns the bounding rectangle of the selection. */
    Geom::OptRect bounds(SPItem::BBoxType type) const;
    Geom::OptRect visualBounds() const;
    Geom::OptRect geometricBounds() const;

    /**
     * Returns either the visual or geometric bounding rectangle of the selection, based on the
     * preferences specified for the selector tool
     */
    Geom::OptRect preferredBounds() const;

    /* Returns the bounding rectangle of the selectionin document coordinates.*/
    Geom::OptRect documentBounds(SPItem::BBoxType type) const;

    /**
     * Returns the rotation/skew center of the selection.
     */
    boost::optional<Geom::Point> center() const;

    /** Returns a list of all perspectives which have a 3D box in the current selection.
       (these may also be nested in groups) */
    std::list<Persp3D *> const perspList();

    /**
     * Returns a list of all 3D boxes in the current selection which are associated to @c
     * persp. If @c pers is @c NULL, return all selected boxes.
     */
    std::list<SPBox3D *> const box3DList(Persp3D *persp = NULL);

protected:
    virtual void _connectSignals(SPObject* object) {};
    virtual void _releaseSignals(SPObject* object) {};
    virtual void _emitSignals() {};
    void _add(SPObject* object);
    void _clear();
    void _remove(SPObject* object);
    bool _anyAncestorIsInSet(SPObject *object);
    void _removeDescendantsFromSet(SPObject *object);
    void _removeAncestorsFromSet(SPObject *object);
    SPItem *_sizeistItem(bool sml, CompareSize compare);
    SPObject *_getMutualAncestor(SPObject *object);
    virtual void _add_3D_boxes_recursively(SPObject *obj);
    virtual void _remove_3D_boxes_recursively(SPObject *obj);

    multi_index_container container;
    std::list<SPBox3D *> _3dboxes;
    std::unordered_map<SPObject*, sigc::connection> releaseConnections;

};


#endif //INKSCAPE_PROTOTYPE_OBJECTSET_H

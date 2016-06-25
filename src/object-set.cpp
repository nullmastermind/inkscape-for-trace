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
#include <glib.h>
#include "object-set.h"
#include "box3d.h"
#include "persp3d.h"
#include "preferences.h"

namespace Inkscape {

bool ObjectSet::add(SPObject* object) {
    g_return_val_if_fail(object != NULL, false);
    g_return_val_if_fail(SP_IS_OBJECT(object), false);


    // any ancestor is in the set - do nothing
    if (_anyAncestorIsInSet(object)) {
        return false;
    }

    // very nice function, but changes selection behavior (probably needs new selection option to deal with it)
    // check if there is mutual ancestor for some elements, which can replace all of them in the set
//    object = _getMutualAncestor(object);

    // remove all descendants from the set
    _removeDescendantsFromSet(object);

    _add(object);
    _emitSignals();
    return true;
}

bool ObjectSet::remove(SPObject* object) {
    g_return_val_if_fail(object != NULL, false);
    g_return_val_if_fail(SP_IS_OBJECT(object), false);

    // object is the top of subtree
    if (includes(object)) {
        _remove(object);
        _emitSignals();
        return true;
    }

    // any ancestor of object is in the set
    if (_anyAncestorIsInSet(object)) {
        _removeAncestorsFromSet(object);
        _emitSignals();
        return true;
    }

    // no object nor any parent in the set
    return false;
}

bool ObjectSet::includes(SPObject *object) {
    g_return_val_if_fail(object != NULL, false);
    g_return_val_if_fail(SP_IS_OBJECT(object), false);

    return container.get<hashed>().find(object) != container.get<hashed>().end();
}

void ObjectSet::clear() {
    _clear();
    _emitSignals();
}

int ObjectSet::size() {
    return container.size();
}

bool ObjectSet::_anyAncestorIsInSet(SPObject *object) {
    SPObject* o = object;
    while (o != nullptr) {
        if (includes(o)) {
            return true;
        }
        o = o->parent;
    }

    return false;
}

void ObjectSet::_removeDescendantsFromSet(SPObject *object) {
    for (auto& child: object->childList(false)) {
        if (includes(child)) {
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
    _remove_3D_boxes_recursively(object);
    _releaseSignals(object);
}

void ObjectSet::_add(SPObject *object) {
    releaseConnections[object] = object->connectRelease(sigc::hide_return(sigc::mem_fun(*this, &ObjectSet::remove)));
    container.push_back(object);
    _add_3D_boxes_recursively(object);
    _connectSignals(object);
}

void ObjectSet::_clear() {
    for (auto object: container) {
        _remove(object);
    }
}

SPObject *ObjectSet::_getMutualAncestor(SPObject *object) {
    SPObject *o = object;

    bool flag = true;
    while (o->parent != nullptr) {
        for (auto &child: o->parent->childList(false)) {
            if(child != o && !includes(child)) {
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
        if (includes(o->parent)) {
            _remove(o->parent);
            break;
        }
        o = o->parent;
    }
}

ObjectSet::~ObjectSet() {
    _clear();
}

void ObjectSet::toggle(SPObject *obj) {
    if (includes(obj)) {
        remove(obj);
    } else {
        add(obj);
    }
}

bool ObjectSet::isEmpty() {
    return container.size() == 0;
}


SPObject *ObjectSet::single() {
    if (container.size() == 1) {
        return *container.begin();
    }

    return nullptr;
}

SPItem *ObjectSet::singleItem() {
    if (container.size() == 1) {
        SPObject* obj = *container.begin();
        if (SP_IS_ITEM(obj)) {
            return SP_ITEM(obj);
        }
    }

    return nullptr;
}

SPItem *ObjectSet::smallestItem(CompareSize compare) {
    return _sizeistItem(true, compare);
}

SPItem *ObjectSet::largestItem(CompareSize compare) {
    return _sizeistItem(false, compare);
}

SPItem *ObjectSet::_sizeistItem(bool sml, CompareSize compare) {
    std::vector<SPItem*> const items = const_cast<ObjectSet *>(this)->items();
    gdouble max = sml ? 1e18 : 0;
    SPItem *ist = NULL;

    for ( std::vector<SPItem*>::const_iterator i=items.begin();i!=items.end(); ++i) {
        Geom::OptRect obox = SP_ITEM(*i)->desktopPreferredBounds();
        if (!obox || obox.empty()) {
            continue;
        }

        Geom::Rect bbox = *obox;

        gdouble size = compare == AREA ? bbox.area() :
                       (compare == VERTICAL ? bbox.width() : bbox.height());
        size = sml ? size : size * -1;
        if (size < max) {
            max = size;
            ist = SP_ITEM(*i);
        }
    }

    return ist;
}

SPObjectRange ObjectSet::objects() {
    return SPObjectRange(container.get<random_access>().begin(), container.get<random_access>().end());
}
std::vector<SPItem*> ObjectSet::items() {
    std::vector<SPObject *> tmp = std::vector<SPObject *>(container.begin(), container.end());
    std::vector<SPItem*> result;
    std::remove_if(tmp.begin(), tmp.end(), [](SPObject* o){return !SP_IS_ITEM(o);});
    std::transform(tmp.begin(), tmp.end(), std::back_inserter(result), [](SPObject* o){return SP_ITEM(o);});
    return result;
}

std::vector<XML::Node*> ObjectSet::xmlNodes() {
    std::vector<SPItem*> list = items();
    std::vector<XML::Node*> result;
    std::transform(list.begin(), list.end(), std::back_inserter(result), [](SPItem* item) { return item->getRepr(); });
    return result;
}

Inkscape::XML::Node *ObjectSet::singleRepr() {
    SPObject *obj = single();
    return obj ? obj->getRepr() : nullptr;
}

void ObjectSet::set(SPObject *object) {
    _clear();
    _add(object);
    // can't emit signal here due to boolean argument in Selection
//    _emitSignals();
}

void ObjectSet::setList(const std::vector<SPItem *> &objs) {
    _clear();
    addList(objs);
}

void ObjectSet::addList(const std::vector<SPItem *> &objs) {
    for (std::vector<SPItem*>::const_iterator iter = objs.begin(); iter != objs.end(); ++iter) {
        SPObject *obj = *iter;
        if (!includes(obj)) {
            add(obj);
        }
    }
}

void ObjectSet::add(const std::vector<SPItem*>::iterator& from, const std::vector<SPItem*>::iterator& to) {
    for(auto it = from; it != to; ++it) {
        _add(*it);
    }
}


Geom::OptRect ObjectSet::bounds(SPItem::BBoxType type) const
{
    return (type == SPItem::GEOMETRIC_BBOX) ?
           geometricBounds() : visualBounds();
}

Geom::OptRect ObjectSet::geometricBounds() const
{
    std::vector<SPItem*> const items = const_cast<ObjectSet *>(this)->items();

    Geom::OptRect bbox;
    for ( std::vector<SPItem*>::const_iterator iter=items.begin();iter!=items.end(); ++iter) {
        bbox.unionWith(SP_ITEM(*iter)->desktopGeometricBounds());
    }
    return bbox;
}

Geom::OptRect ObjectSet::visualBounds() const
{
    std::vector<SPItem*> const items = const_cast<ObjectSet *>(this)->items();

    Geom::OptRect bbox;
    for ( std::vector<SPItem*>::const_iterator iter=items.begin();iter!=items.end(); ++iter) {
        bbox.unionWith(SP_ITEM(*iter)->desktopVisualBounds());
    }
    return bbox;
}

Geom::OptRect ObjectSet::preferredBounds() const
{
    if (Inkscape::Preferences::get()->getInt("/tools/bounding_box") == 0) {
        return bounds(SPItem::VISUAL_BBOX);
    } else {
        return bounds(SPItem::GEOMETRIC_BBOX);
    }
}

Geom::OptRect ObjectSet::documentBounds(SPItem::BBoxType type) const
{
    Geom::OptRect bbox;
    std::vector<SPItem*> const items = const_cast<ObjectSet *>(this)->items();
    if (items.empty()) return bbox;

    for ( std::vector<SPItem*>::const_iterator iter=items.begin();iter!=items.end(); ++iter) {
        SPItem *item = SP_ITEM(*iter);
        bbox |= item->documentBounds(type);
    }

    return bbox;
}

// If we have a selection of multiple items, then the center of the first item
// will be returned; this is also the case in SelTrans::centerRequest()
boost::optional<Geom::Point> ObjectSet::center() const {
    std::vector<SPItem*> const items = const_cast<ObjectSet *>(this)->items();
    if (!items.empty()) {
        SPItem *first = items.back(); // from the first item in selection
        if (first->isCenterSet()) { // only if set explicitly
            return first->getCenter();
        }
    }
    Geom::OptRect bbox = preferredBounds();
    if (bbox) {
        return bbox->midpoint();
    } else {
        return boost::optional<Geom::Point>();
    }
}


std::list<Persp3D *> const ObjectSet::perspList() {
    std::list<Persp3D *> pl;
    for (std::list<SPBox3D *>::iterator i = _3dboxes.begin(); i != _3dboxes.end(); ++i) {
        Persp3D *persp = box3d_get_perspective(*i);
        if (std::find(pl.begin(), pl.end(), persp) == pl.end())
            pl.push_back(persp);
    }
    return pl;
}

std::list<SPBox3D *> const ObjectSet::box3DList(Persp3D *persp) {
    std::list<SPBox3D *> boxes;
    if (persp) {
        for (std::list<SPBox3D *>::iterator i = _3dboxes.begin(); i != _3dboxes.end(); ++i) {
            SPBox3D *box = *i;
            if (persp == box3d_get_perspective(box)) {
                boxes.push_back(box);
            }
        }
    } else {
        boxes = _3dboxes;
    }
    return boxes;
}

void ObjectSet::_add_3D_boxes_recursively(SPObject *obj) {
    std::list<SPBox3D *> boxes = box3d_extract_boxes(obj);

    for (std::list<SPBox3D *>::iterator i = boxes.begin(); i != boxes.end(); ++i) {
        SPBox3D *box = *i;
        _3dboxes.push_back(box);
    }
}

void ObjectSet::_remove_3D_boxes_recursively(SPObject *obj) {
    std::list<SPBox3D *> boxes = box3d_extract_boxes(obj);

    for (std::list<SPBox3D *>::iterator i = boxes.begin(); i != boxes.end(); ++i) {
        SPBox3D *box = *i;
        std::list<SPBox3D *>::iterator b = std::find(_3dboxes.begin(), _3dboxes.end(), box);
        if (b == _3dboxes.end()) {
            g_print ("Warning! Trying to remove unselected box from selection.\n");
            return;
        }
        _3dboxes.erase(b);
    }
}

} // namespace Inkscape

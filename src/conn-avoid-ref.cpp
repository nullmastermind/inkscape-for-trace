// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A class for handling shape interaction with libavoid.
 *
 * Authors:
 *   Michael Wybrow <mjwybrow@users.sourceforge.net>
 *   Abhishek Sharma
 *
 * Copyright (C) 2005 Michael Wybrow
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include <cstring>
#include <string>
#include <iostream>

#include "2geom/convex-hull.h"
#include "2geom/line.h"

#include "conn-avoid-ref.h"
#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "inkscape.h"
#include "verbs.h"

#include "display/curve.h"

#include "3rdparty/adaptagrams/libavoid/router.h"
#include "3rdparty/adaptagrams/libavoid/shape.h"

#include "object/sp-namedview.h"
#include "object/sp-shape.h"

#include "svg/stringstream.h"

#include "xml/node.h"

using Inkscape::DocumentUndo;

using Avoid::Router;

static Avoid::Polygon avoid_item_poly(SPItem const *item);


SPAvoidRef::SPAvoidRef(SPItem *spitem)
    : shapeRef(nullptr)
    , item(spitem)
    , setting(false)
    , new_setting(false)
    , _transformed_connection()
{
}


SPAvoidRef::~SPAvoidRef()
{
    _transformed_connection.disconnect();

    // If the document is being destroyed then the router instance
    // and the ShapeRefs will have been destroyed with it.
    Router *router = item->document->getRouter();

    if (shapeRef && router) {
        router->deleteShape(shapeRef);
    }
    shapeRef = nullptr;
}


void SPAvoidRef::setAvoid(char const *value)
{
    // Don't keep avoidance information for cloned objects.
    if ( !item->cloned ) {
        new_setting = false;
        if (value && (strcmp(value, "true") == 0)) {
            new_setting = true;
        }
    }
}

void SPAvoidRef::handleSettingChange()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == nullptr) {
        return;
    }
    if (desktop->getDocument() != item->document) {
        // We don't want to go any further if the active desktop's document
        // isn't the same as the document that this item is part of.  This
        // case can happen if a new document is loaded from the file chooser
        // or via the recent file menu.  In this case, we can end up here
        // as a result of a ensureUpToDate performed on a
        // document not yet attached to the active desktop.
        return;
    }

    if (new_setting == setting) {
        // Don't need to make any changes
        return;
    }
    setting = new_setting;

    Router *router = item->document->getRouter();

    _transformed_connection.disconnect();
    if (new_setting) {
        Avoid::Polygon poly = avoid_item_poly(item);
        if (poly.size() > 0) {
            _transformed_connection = item->connectTransformed(
                    sigc::ptr_fun(&avoid_item_move));

            char const *id = item->getAttribute("id");
            g_assert(id != nullptr);

            // Get a unique ID for the item.
            GQuark itemID = g_quark_from_string(id);

            shapeRef = new Avoid::ShapeRef(router, poly, itemID);
        }
    }
    else if (shapeRef)
    {
        router->deleteShape(shapeRef);
        shapeRef = nullptr;
    }
}


std::vector<SPItem *> SPAvoidRef::getAttachedShapes(const unsigned int type)
{
    std::vector<SPItem *> list;

    Avoid::IntList shapes;
    GQuark shapeId = g_quark_from_string(item->getId());
    item->document->getRouter()->attachedShapes(shapes, shapeId, type);

    Avoid::IntList::iterator finish = shapes.end();
    for (Avoid::IntList::iterator i = shapes.begin(); i != finish; ++i) {
        const gchar *connId = g_quark_to_string(*i);
        SPObject *obj = item->document->getObjectById(connId);
        if (obj == nullptr) {
            g_warning("getAttachedShapes: Object with id=\"%s\" is not "
                    "found. Skipping.", connId);
            continue;
        }
        SPItem *shapeItem = SP_ITEM(obj);
        list.push_back(shapeItem);
    }
    return list;
}


std::vector<SPItem *> SPAvoidRef::getAttachedConnectors(const unsigned int type)
{
    std::vector<SPItem *> list;

    Avoid::IntList conns;
    GQuark shapeId = g_quark_from_string(item->getId());
    item->document->getRouter()->attachedConns(conns, shapeId, type);

    Avoid::IntList::iterator finish = conns.end();
    for (Avoid::IntList::iterator i = conns.begin(); i != finish; ++i) {
        const gchar *connId = g_quark_to_string(*i);
        SPObject *obj = item->document->getObjectById(connId);
        if (obj == nullptr) {
            g_warning("getAttachedConnectors: Object with id=\"%s\" is not "
                    "found. Skipping.", connId);
            continue;
        }
        SPItem *connItem = SP_ITEM(obj);
        list.push_back(connItem);
    }
    return list;
}

Geom::Point SPAvoidRef::getConnectionPointPos()
{
    g_assert(item);
    // the center is all we are interested in now; we used to care
    // about non-center points, but that's moot.
    Geom::OptRect bbox = item->documentVisualBounds();
    return (bbox) ? bbox->midpoint() : Geom::Point(0, 0);
}

static std::vector<Geom::Point> approxCurveWithPoints(SPCurve *curve)
{
    // The number of segments to use for not straight curves approximation
    const unsigned NUM_SEGS = 4;
    
    const Geom::PathVector& curve_pv = curve->get_pathvector();
   
    // The structure to hold the output
    std::vector<Geom::Point> poly_points;

    // Iterate over all curves, adding the endpoints for linear curves and
    // sampling the other curves
    double seg_size = 1.0 / NUM_SEGS;
    double at;
    at = 0;
    Geom::PathVector::const_iterator pit = curve_pv.begin();
    while (pit != curve_pv.end())
    {
        Geom::Path::const_iterator cit = pit->begin();
        while (cit != pit->end())
        {
            if (cit == pit->begin())
            {
                poly_points.push_back(cit->initialPoint());
            }

            if (dynamic_cast<Geom::CubicBezier const*>(&*cit))
            {
                at += seg_size;
                if (at <= 1.0 )
                    poly_points.push_back(cit->pointAt(at));
                else
                {
                    at = 0.0;
                    ++cit;
                }
            }
            else
            {
                poly_points.push_back(cit->finalPoint());
                ++cit;
            }
        }
        ++pit;
    }
    return poly_points;
}

static std::vector<Geom::Point> approxItemWithPoints(SPItem const *item, const Geom::Affine& item_transform)
{
    // The structure to hold the output
    std::vector<Geom::Point> poly_points;
    SPCurve *item_curve = nullptr;

    if (SP_IS_GROUP(item))
    {
        SPGroup* group = SP_GROUP(item);
        // consider all first-order children
        std::vector<SPItem*> itemlist = sp_item_group_item_list(group);
        for (auto child_item : itemlist) {
            std::vector<Geom::Point> child_points = approxItemWithPoints(child_item, item_transform * child_item->transform);
            poly_points.insert(poly_points.end(), child_points.begin(), child_points.end());
        }
    }
    else if (SP_IS_SHAPE(item))
    {
        SP_SHAPE(item)->set_shape();
        item_curve = SP_SHAPE(item)->getCurve();
        // make sure it has an associated curve
        if (item_curve)
        {
            // apply transformations (up to common ancestor)
            item_curve->transform(item_transform);
        }
    } else {
        auto bbox = item->documentPreferredBounds();
        if (bbox) {
            item_curve = SPCurve::new_from_rect(*bbox);
        }
    }

    if (item_curve) {
        std::vector<Geom::Point> curve_points = approxCurveWithPoints(item_curve);
        poly_points.insert(poly_points.end(), curve_points.begin(), curve_points.end());
        item_curve->unref();
    }

    return poly_points;
}
static Avoid::Polygon avoid_item_poly(SPItem const *item)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    g_assert(desktop != nullptr);
    double spacing = desktop->namedview->connector_spacing;

    Geom::Affine itd_mat = item->i2doc_affine();
    std::vector<Geom::Point> hull_points;
    hull_points = approxItemWithPoints(item, itd_mat);

    // create convex hull from all sampled points
    Geom::ConvexHull hull(hull_points);

    // enlarge path by "desktop->namedview->connector_spacing"
    // store expanded convex hull in Avoid::Polygn
    Avoid::Polygon poly;
    if (hull.empty()) {
        return poly;
    }

    Geom::Line hull_edge(hull.back(), hull.front());
    Geom::Line prev_parallel_hull_edge;
    prev_parallel_hull_edge.setOrigin(hull_edge.origin()+hull_edge.versor().ccw()*spacing);
    prev_parallel_hull_edge.setVector(hull_edge.versor());
    int hull_size = hull.size();
    for (int i = 0; i < hull_size; ++i)
    {
        if (i + 1 == hull_size) {
            hull_edge.setPoints(hull.back(), hull.front());
        } else {
            hull_edge.setPoints(hull[i], hull[i + 1]);
        }
        Geom::Line parallel_hull_edge;
        parallel_hull_edge.setOrigin(hull_edge.origin()+hull_edge.versor().ccw()*spacing);
        parallel_hull_edge.setVector(hull_edge.versor());

        // determine the intersection point
        try {
            Geom::OptCrossing int_pt = Geom::intersection(parallel_hull_edge, prev_parallel_hull_edge);
            if (int_pt)
            {
                Avoid::Point avoid_pt((parallel_hull_edge.origin()+parallel_hull_edge.versor()*int_pt->ta)[Geom::X],
                                        (parallel_hull_edge.origin()+parallel_hull_edge.versor()*int_pt->ta)[Geom::Y]);
                poly.ps.push_back(avoid_pt);
            }
            else
            {
                // something went wrong...
                std::cout<<"conn-avoid-ref.cpp: avoid_item_poly: Geom:intersection failed."<<std::endl;
            }
        }
        catch (Geom::InfiniteSolutions const &e) {
            // the parallel_hull_edge and prev_parallel_hull_edge lie on top of each other, hence infinite crossings
            g_message("conn-avoid-ref.cpp: trying to get crossings of identical lines");
        }
        prev_parallel_hull_edge = parallel_hull_edge;
    }

    return poly;
}


std::vector<SPItem *> get_avoided_items(std::vector<SPItem *> &list, SPObject *from, SPDesktop *desktop,
        bool initialised)
{
    for (auto& child: from->children) {
        if (SP_IS_ITEM(&child) &&
            !desktop->isLayer(SP_ITEM(&child)) &&
            !SP_ITEM(&child)->isLocked() &&
            !desktop->itemIsHidden(SP_ITEM(&child)) &&
            (!initialised || SP_ITEM(&child)->getAvoidRef().shapeRef)
            )
        {
            list.push_back(SP_ITEM(&child));
        }

        if (SP_IS_ITEM(&child) && desktop->isLayer(SP_ITEM(&child))) {
            list = get_avoided_items(list, &child, desktop, initialised);
        }
    }

    return list;
}


void avoid_item_move(Geom::Affine const */*mp*/, SPItem *moved_item)
{
    Avoid::ShapeRef *shapeRef = moved_item->getAvoidRef().shapeRef;
    g_assert(shapeRef);

    Router *router = moved_item->document->getRouter();
    Avoid::Polygon poly = avoid_item_poly(moved_item);
    if (!poly.empty()) {
        router->moveShape(shapeRef, poly);
    }
}


void init_avoided_shape_geometry(SPDesktop *desktop)
{
    // Don't count this as changes to the document,
    // it is basically just late initialisation.
    SPDocument *document = desktop->getDocument();
    DocumentUndo::ScopedInsensitive _no_undo(document);

    bool initialised = false;
    std::vector<SPItem *> tmp;
    std::vector<SPItem *> items = get_avoided_items(tmp, desktop->currentRoot(), desktop,
            initialised);

    for (auto item : items) {
        item->getAvoidRef().handleSettingChange();
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

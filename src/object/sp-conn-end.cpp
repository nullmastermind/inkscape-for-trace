// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "sp-conn-end.h"

#include <cstring>
#include <string>
#include <limits>

#include "bad-uri-exception.h"
#include "display/curve.h"
#include "xml/repr.h"
#include "sp-path.h"
#include "uri.h"
#include "document.h"
#include "sp-item-group.h"
#include "2geom/path-intersection.h"


static void change_endpts(SPCurve *const curve, double const endPos[2]);

SPConnEnd::SPConnEnd(SPObject *const owner)
    : ref(owner)
    , sub_ref(owner)
    , href(nullptr)
    , sub_href(nullptr)
    // Default to center connection endpoint
    , _changed_connection()
    , _delete_connection()
    , _transformed_connection()
    , _group_connection()
{
}

static SPObject const *get_nearest_common_ancestor(SPObject const *const obj, SPItem const *const objs[2])
{
    SPObject const *anc_sofar = obj;
    for (unsigned i = 0; i < 2; ++i) {
        if ( objs[i] != nullptr ) {
            anc_sofar = anc_sofar->nearestCommonAncestor(objs[i]);
        }
    }
    return anc_sofar;
}


static bool try_get_intersect_point_with_item_recursive(Geom::PathVector& conn_pv, SPItem* item,
        const Geom::Affine& item_transform, double& intersect_pos)
{
    double initial_pos = intersect_pos;
    // if this is a group...
    if (SP_IS_GROUP(item)) {
        SPGroup* group = SP_GROUP(item);

        // consider all first-order children
        double child_pos = 0.0;
        std::vector<SPItem*> g = sp_item_group_item_list(group);
        for (auto child_item : g) {
            try_get_intersect_point_with_item_recursive(conn_pv, child_item,
                    item_transform * child_item->transform, child_pos);
            if (intersect_pos < child_pos)
                intersect_pos = child_pos;
        }
        return intersect_pos != initial_pos;
    }

    // if this is not a shape, nothing to be done
    auto shape = dynamic_cast<SPShape const *>(item);
    if (!shape)
        return false;

    // make sure it has an associated curve
    auto item_curve = SPCurve::copy(shape->curve());
    if (!item_curve) return false;

    // apply transformations (up to common ancestor)
    item_curve->transform(item_transform);

    const Geom::PathVector& curve_pv = item_curve->get_pathvector();
    Geom::CrossingSet cross = crossings(conn_pv, curve_pv);
    // iterate over all Crossings
    //TODO: check correctness of the following code: inner loop uses loop variable
    //      with a name identical to the loop variable of the outer loop. Then rename.
    for (const auto & cr : cross) {
        for (const auto & cr_pt : cr) {
            if ( intersect_pos < cr_pt.ta)
                intersect_pos = cr_pt.ta;
        }
    }

    return intersect_pos != initial_pos;
}


// This function returns the outermost intersection point between the path (a connector)
// and the item given.  If the item is a group, then the component items are considered.
// The transforms given should be to a common ancestor of both the path and item.
//
static bool try_get_intersect_point_with_item(SPPath* conn, SPItem* item,
        const Geom::Affine& item_transform, const Geom::Affine& conn_transform,
        const bool at_start, double& intersect_pos)
{
    // Copy the curve and apply transformations up to common ancestor.
    auto conn_curve = conn->curve()->copy();
    conn_curve->transform(conn_transform);

    Geom::PathVector conn_pv = conn_curve->get_pathvector();

    // If this is not the starting point, use Geom::Path::reverse() to reverse the path
    if (!at_start) {
        // connectors are actually a single path, so consider the first element from a Geom::PathVector
        conn_pv[0] = conn_pv[0].reversed();
    }

    // We start with the intersection point at the beginning of the path
    intersect_pos = 0.0;

    // Find the intersection.
    bool result = try_get_intersect_point_with_item_recursive(conn_pv, item, item_transform, intersect_pos);

    if (!result) {
        // No intersection point has been found (why?)
        // just default to connector end
        intersect_pos = 0;
    }
    // If not at the starting point, recompute position with respect to original path
    if (!at_start) {
        intersect_pos = conn_pv[0].size() - intersect_pos;
    }

    return result;
}


static void sp_conn_get_route_and_redraw(SPPath *const path, const bool updatePathRepr = true)
{
    // Get the new route around obstacles.
    bool rerouted = path->connEndPair.reroutePathFromLibavoid();
    if (!rerouted) {
        return;
    }

    SPItem *h2attItem[2] = {nullptr};
    path->connEndPair.getAttachedItems(h2attItem);

    SPObject const *const ancestor = get_nearest_common_ancestor(path, h2attItem);
    Geom::Affine const path2anc(i2anc_affine(path, ancestor));

    // Set sensible values in case there the connector ends are not
    // attached to any shapes.
    Geom::PathVector conn_pv = path->curve()->get_pathvector();
    double endPos[2] = { 0.0, static_cast<double>(conn_pv[0].size()) };

    for (unsigned h = 0; h < 2; ++h) {
        // Assume center point for all
        if (h2attItem[h]) {
            Geom::Affine h2i2anc = i2anc_affine(h2attItem[h], ancestor);
            try_get_intersect_point_with_item(path, h2attItem[h], h2i2anc, path2anc,
                        (h == 0), endPos[h]);
        }
    }
    change_endpts(path->curve(), endPos);
    if (updatePathRepr) {
        path->updateRepr();
        path->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    }
}


static void sp_conn_end_shape_modified(SPObject */*moved_item*/, int /*flags*/, SPPath *const path)
{
    if (path->connEndPair.isAutoRoutingConn()) {
        path->connEndPair.tellLibavoidNewEndpoints();
    }
}


void sp_conn_reroute_path(SPPath *const path)
{
    if (path->connEndPair.isAutoRoutingConn()) {
        path->connEndPair.tellLibavoidNewEndpoints();
    }
}


void sp_conn_reroute_path_immediate(SPPath *const path)
{
    if (path->connEndPair.isAutoRoutingConn()) {
        bool processTransaction = true;
        path->connEndPair.tellLibavoidNewEndpoints(processTransaction);
    }
    // Don't update the path repr or else connector dragging is slowed by
    // constant update of values to the xml editor, and each step is also
    // needlessly remembered by undo/redo.
    bool const updatePathRepr = false;
    sp_conn_get_route_and_redraw(path, updatePathRepr);
}

void sp_conn_redraw_path(SPPath *const path)
{
    sp_conn_get_route_and_redraw(path);
}


static void change_endpts(SPCurve *const curve, double const endPos[2])
{
    // Use Geom::Path::portion to cut the curve at the end positions
    if (endPos[0] > endPos[1]) {
        // Path is "negative", reset the curve and return
        curve->reset();
        return;
    }
    const Geom::Path& old_path = curve->get_pathvector()[0];
    Geom::PathVector new_path_vector;
    new_path_vector.push_back(old_path.portion(endPos[0], endPos[1]));
    curve->set_pathvector(new_path_vector);
}

static void sp_conn_end_deleted(SPObject *, SPObject *const owner, unsigned const handle_ix)
{
    char const * const attrs[] = {
        "inkscape:connection-start", "inkscape:connection-end"};
    owner->removeAttribute(attrs[handle_ix]);

    char const * const point_attrs[] = {
        "inkscape:connection-start-point", "inkscape:connection-end-point"};
    owner->removeAttribute(point_attrs[handle_ix]);
    /* I believe this will trigger sp_conn_end_href_changed. */
}

void sp_conn_end_detach(SPObject *const owner, unsigned const handle_ix)
{
    sp_conn_end_deleted(nullptr, owner, handle_ix);
}

void SPConnEnd::setAttacherHref(gchar const *value)
{
    if (g_strcmp0(value, href) != 0) {
        g_free(href);
        href = g_strdup(value);
        if (!ref.try_attach(value)) {
            g_free(href);
            href = nullptr;
        }
    }
}

void SPConnEnd::setAttacherSubHref(gchar const *value)
{
    // TODO This could check the URI object is actually a sub-object
    // of the set href. It should be done here and in setAttacherHref
    if (g_strcmp0(value, sub_href) != 0) {
        g_free(sub_href);
        sub_href = g_strdup(value);
        if (!sub_ref.try_attach(value)) {
            g_free(sub_href);
            sub_href = nullptr;
        }
    }
}



void sp_conn_end_href_changed(SPObject */*old_ref*/, SPObject */*ref*/,
        SPConnEnd *connEndPtr, SPPath *const path, unsigned const handle_ix)
{
    g_return_if_fail(connEndPtr != nullptr);
    SPConnEnd &connEnd = *connEndPtr;
    connEnd._delete_connection.disconnect();
    connEnd._transformed_connection.disconnect();
    connEnd._group_connection.disconnect();

    if (connEnd.href) {
        SPObject *refobj = connEnd.ref.getObject();
        if (refobj) {
            connEnd._delete_connection
                = refobj->connectDelete(sigc::bind(sigc::ptr_fun(&sp_conn_end_deleted),
                                                   path, handle_ix));
            // This allows the connector tool to dive into a group's children
            // And connect to their children's centers.
            SPObject *parent = refobj->parent;
            if (SP_IS_GROUP(parent) && ! SP_IS_LAYER(parent)) {
                connEnd._group_connection
                    = SP_ITEM(parent)->connectModified(sigc::bind(sigc::ptr_fun(&sp_conn_end_shape_modified),
                                                                 path));
            }
            connEnd._transformed_connection
                = SP_ITEM(refobj)->connectModified(sigc::bind(sigc::ptr_fun(&sp_conn_end_shape_modified),
                                                                 path));
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

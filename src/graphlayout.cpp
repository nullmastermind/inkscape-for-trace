// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Interface between Inkscape code (SPItem) and graphlayout functions.
 */
/*
 * Authors:
 *   Tim Dwyer <Tim.Dwyer@infotech.monash.edu.au>
 *   Abhishek Sharma
 *
 * Copyright (C) 2005 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <algorithm>
#include <cstring>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <valarray>
#include <vector>

#include <2geom/transforms.h>

#include "conn-avoid-ref.h"
#include "desktop.h"
#include "graphlayout.h"
#include "inkscape.h"

#include "3rdparty/adaptagrams/libavoid/router.h"

#include "3rdparty/adaptagrams/libcola/cola.h"
#include "3rdparty/adaptagrams/libcola/connected_components.h"

#include "object/sp-item-transform.h"
#include "object/sp-namedview.h"
#include "object/sp-path.h"
#include "style.h"

using namespace cola;
using namespace vpsc;

/**
 * Returns true if item is a connector
 */
bool isConnector(SPItem const * const item) {
    SPPath * path = nullptr;
    if (SP_IS_PATH(item)) {
        path = SP_PATH(item);
    }
    return path && path->connEndPair.isAutoRoutingConn();
}

struct CheckProgress: TestConvergence {
    CheckProgress(double d, unsigned i, std::list<SPItem*> & selected, Rectangles & rs,
            std::map<std::string, unsigned> & nodelookup)
        : TestConvergence(d, i)
        , selected(selected)
        , rs(rs)
        , nodelookup(nodelookup)
    {}
    bool operator()(const double new_stress, std::valarray<double> & X, std::valarray<double> & Y) override {
        /* This is where, if we wanted to animate the layout, we would need to update
         * the positions of all objects and redraw the canvas and maybe sleep a bit
         cout << "stress="<<new_stress<<endl;
        cout << "x[0]="<<rs[0]->getMinX()<<endl;
        for (std::list<SPItem *>::iterator it(selected.begin());
            it != selected.end();
            ++it)
        {
            SPItem *u=*it;
            if(!isConnector(u)) {
                Rectangle* r=rs[nodelookup[u->id]];
                Geom::Rect const item_box(sp_item_bbox_desktop(u));
                Geom::Point const curr(item_box.midpoint());
                Geom::Point const dest(r->getCentreX(),r->getCentreY());
                u->move_rel(Geom::Translate(dest - curr));
            }
        }
        */
        return TestConvergence::operator()(new_stress, X, Y);
    }
    std::list<SPItem*> & selected;
    Rectangles & rs;
    std::map<std::string, unsigned> & nodelookup;
};

/**
 * Scans the items list and places those items that are
 * not connectors in filtered
 */
void filterConnectors(std::vector<SPItem*> const & items, std::list<SPItem*> & filtered) {
    for (SPItem * item: items) {
        if (!isConnector(item)) {
            filtered.push_back(item);
        }
    }
}

/**
 * Takes a list of inkscape items, extracts the graph defined by
 * connectors between them, and uses graph layout techniques to find
 * a nice layout
 */
void graphlayout(std::vector<SPItem*> const & items) {
    if (items.empty()) return;

    std::list<SPItem*> selected;
    filterConnectors(items, selected);

    if (selected.size() < 2) return;

    // add the connector spacing to the size of node bounding boxes
    // so that connectors can always be routed between shapes
    SPDesktop * desktop = SP_ACTIVE_DESKTOP;
    double spacing = 0;
    if (desktop) spacing = desktop->namedview->connector_spacing + 0.1;

    std::map<std::string, unsigned> nodelookup;
    Rectangles rs;
    std::vector<Edge> es;
    for (SPItem * item: selected) {
        Geom::OptRect const item_box = item->desktopVisualBounds();
        if (item_box) {
            Geom::Point ll(item_box->min());
            Geom::Point ur(item_box->max());
            nodelookup[item->getId()] = rs.size();
            rs.push_back(new Rectangle(ll[0] - spacing, ur[0] + spacing,
                    ll[1] - spacing, ur[1] + spacing));
        } else {
            // I'm not actually sure if it's possible for something with a
            // NULL item-box to be attached to a connector in which case we
            // should never get to here... but if such a null box can occur it's
            // probably pretty safe to simply ignore
            //fprintf(stderr, "NULL item_box found in graphlayout, ignoring!\n");
        }
    }

    Inkscape::Preferences * prefs = Inkscape::Preferences::get();
    CompoundConstraints constraints;
    double ideal_connector_length = prefs->getDouble("/tools/connector/length", 100.0);
    double directed_edge_height_modifier = 1.0;

    bool directed       = prefs->getBool("/tools/connector/directedlayout");
    bool avoid_overlaps = prefs->getBool("/tools/connector/avoidoverlaplayout");

    for (SPItem * item: selected) {
        std::map<std::string, unsigned>::iterator i_iter=nodelookup.find(item->getId());
        if (i_iter == nodelookup.end()) continue;
        unsigned u = i_iter->second;
        std::vector<SPItem*> nlist = item->getAvoidRef().getAttachedConnectors(Avoid::runningFrom);
        std::list<SPItem*> connectors;

        connectors.insert(connectors.end(), nlist.begin(), nlist.end());

        for (SPItem * conn: connectors) {
            SPItem * iv;
            SPItem * items[2];
            assert(isConnector(conn));
            SP_PATH(conn)->connEndPair.getAttachedItems(items);
            if (items[0] == item) {
                iv = items[1];
            } else {
                iv = items[0];
            }

            if (iv == nullptr) {
                // The connector is not attached to anything at the
                // other end so we should just ignore it.
                continue;
            }

            // If iv not in nodelookup we again treat the connector
            // as disconnected and continue
            std::map<std::string, unsigned>::iterator v_pair = nodelookup.find(iv->getId());
            if (v_pair != nodelookup.end()) {
                unsigned v = v_pair->second;
                //cout << "Edge: (" << u <<","<<v<<")"<<endl;
                es.emplace_back(u, v);
                if (conn->style->marker_end.set) {
                    if (directed && strcmp(conn->style->marker_end.value(), "none")) {
                        constraints.push_back(new SeparationConstraint(YDIM, v, u,
                                ideal_connector_length * directed_edge_height_modifier));
                    }
                }
            }
        }
    }
    EdgeLengths elengths(es.size(), 1);
    std::vector<Component*> cs;
    connectedComponents(rs, es, cs);
    for (Component * c: cs) {
        if (c->edges.size() < 2) continue;
        CheckProgress test(0.0001, 100, selected, rs, nodelookup);
        ConstrainedMajorizationLayout alg(c->rects, c->edges, nullptr, ideal_connector_length, elengths, &test);
        if (avoid_overlaps) alg.setAvoidOverlaps();
        alg.setConstraints(&constraints);
        alg.run();
    }
    separateComponents(cs);

    for (SPItem * item: selected) {
        if (!isConnector(item)) {
            std::map<std::string, unsigned>::iterator i = nodelookup.find(item->getId());
            if (i != nodelookup.end()) {
                Rectangle * r = rs[i->second];
                Geom::OptRect item_box = item->desktopVisualBounds();
                if (item_box) {
                    Geom::Point const curr(item_box->midpoint());
                    Geom::Point const dest(r->getCentreX(),r->getCentreY());
                    item->move_rel(Geom::Translate(dest - curr));
                }
            }
        }
    }
    for (CompoundConstraint * c: constraints) {
        delete c;
    }
    for (Rectangle * r: rs) {
        delete r;
    }
}
// vim: set cindent
// vim: ts=4 sw=4 et tw=0 wm=0

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

// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * Interface between Inkscape code (SPItem) and remove-overlaps function.
 */
/*
 * Authors:
 *   Tim Dwyer <tgdwyer@gmail.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2005 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <utility>

#include <2geom/transforms.h>

#include "removeoverlap.h"

#include "libvpsc/rectangle.h"

#include "object/sp-item.h"
#include "object/sp-item-transform.h"


using vpsc::Rectangle;

namespace {

struct Record {
    SPItem * item;
    Geom::Point midpoint;
    Rectangle * vspc_rect;

    Record() : item(nullptr), vspc_rect(nullptr) {}
    Record(SPItem * i, Geom::Point m, Rectangle * r)
        : item(i), midpoint(m), vspc_rect(r) {}
};

}

/**
* Takes a list of inkscape items and moves them as little as possible
* such that rectangular bounding boxes are separated by at least xGap
* horizontally and yGap vertically
*/
void removeoverlap(std::vector<SPItem*> const & items, double const xGap, double const yGap) {
    std::vector<SPItem*> selected = items;
    std::vector<Record> records;
    std::vector<Rectangle*> rs;

    Geom::Point const gap(xGap, yGap);
    for (SPItem * item: selected) {
        using Geom::X; using Geom::Y;
        Geom::OptRect item_box(item->desktopVisualBounds());
        if (item_box) {
            Geom::Point min(item_box->min() - .5 * gap);
            Geom::Point max(item_box->max() + .5 * gap);
            // A negative gap is allowed, but will lead to problems when the gap is larger than
            // the bounding box (in either X or Y direction, or both); min will have become max
            // now, which cannot be handled by Rectangle() which is called below. And how will
            // removeRectangleOverlap handle such a case?
            // That's why we will enforce some boundaries on min and max here:
            if (max[X] < min[X]) {
                min[X] = max[X] = (min[X] + max[X]) / 2.;
            }
            if (max[Y] < min[Y]) {
                min[Y] = max[Y] = (min[Y] + max[Y]) / 2.;
            }
            Rectangle * vspc_rect = new Rectangle(min[X], max[X], min[Y], max[Y]);
            records.emplace_back(item, item_box->midpoint(), vspc_rect);
            rs.push_back(vspc_rect);
        }
    }
    if (!rs.empty()) {
        removeoverlaps(rs);
    }
    for (Record & rec: records) {
        Geom::Point const curr = rec.midpoint;
        Geom::Point const dest(rec.vspc_rect->getCentreX(), rec.vspc_rect->getCentreY());
        rec.item->move_rel(Geom::Translate(dest - curr));
        delete rec.vspc_rect;
    }
}

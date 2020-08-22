// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_ITEM_GROUP_H
#define SEEN_CANVAS_ITEM_GROUP_H

/**
 * A CanvasItem that contains other CanvasItem's.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of SPCanvasGroup
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

//#include <2geom/rect.h>
#include <boost/intrusive/list.hpp>

#include "canvas-item.h"

namespace Inkscape {

class CanvasItemGroup : public CanvasItem {

public:
    CanvasItemGroup(CanvasItemGroup* group = nullptr);
    ~CanvasItemGroup() override;

    // Structure
    void add(CanvasItem *item);
    void remove(CanvasItem *item, bool Delete = true);

    void update(Geom::Affine const &affine) override;

    // Display
    void render(Inkscape::CanvasItemBuffer *buf) override;

    // Selection
    CanvasItem* pick_item(Geom::Point &p);

    CanvasItemList & get_items() { return items; } 

    // Properties
    void update_canvas_item_ctrl_sizes(int size_index);

protected:

private:
public:
    // TODO: Make private (used in canvas-item.cpp).
    CanvasItemList items; // Used to speed deletion.
};


} // namespace Inkscape

#endif // SEEN_CANVAS_ITEM_H

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

// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SNAP_TOOLBAR_H
#define SEEN_SNAP_TOOLBAR_H

/**
 * @file
 * Snapping toolbar
 *
 * @authors Inkscape authors, 2004-2019
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "toolbar.h"

enum SPAttributeEnum : unsigned;

namespace Inkscape {
namespace UI {
namespace Toolbar {
class SnapToolbar : public Toolbar {
private:
    bool _freeze;

    // Toolbar widgets
    Gtk::ToggleToolButton *_snap_global_item;
    Gtk::ToggleToolButton *_snap_from_bbox_corner_item;
    Gtk::ToggleToolButton *_snap_to_bbox_path_item;
    Gtk::ToggleToolButton *_snap_to_bbox_node_item;
    Gtk::ToggleToolButton *_snap_to_from_bbox_edge_midpoints_item;
    Gtk::ToggleToolButton *_snap_to_from_bbox_edge_centers_item;
    Gtk::ToggleToolButton *_snap_from_node_item;
    Gtk::ToggleToolButton *_snap_to_item_path_item;
    Gtk::ToggleToolButton *_snap_to_path_intersections_item;
    Gtk::ToggleToolButton *_snap_to_item_node_item;
    Gtk::ToggleToolButton *_snap_to_smooth_nodes_item;
    Gtk::ToggleToolButton *_snap_to_from_line_midpoints_item;
    Gtk::ToggleToolButton *_snap_from_others_item;
    Gtk::ToggleToolButton *_snap_to_from_object_centers_item;
    Gtk::ToggleToolButton *_snap_to_from_rotation_center_item;
    Gtk::ToggleToolButton *_snap_to_from_text_baseline_item;
    Gtk::ToggleToolButton *_snap_to_page_border_item;
    Gtk::ToggleToolButton *_snap_to_grids_item;
    Gtk::ToggleToolButton *_snap_to_guides_item;

    void on_snap_toggled(SPAttributeEnum attr);

protected:
    SnapToolbar(SPDesktop *desktop);

public:
    static GtkWidget * create(SPDesktop *desktop);
    static void update(SnapToolbar *tb);
};

}
}
}
#endif /* !SEEN_SNAP_TOOLBAR_H */

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

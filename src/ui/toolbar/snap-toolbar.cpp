// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file
 * Inkscape Snap toolbar
 *
 * @authors Inkscape Authors
 * Copyright (C) 1999-2019 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "snap-toolbar.h"

#include <glibmm/i18n.h>

#include "attributes.h"
#include "desktop.h"
#include "verbs.h"

#include "object/sp-namedview.h"

#include "ui/icon-names.h"

namespace Inkscape {
namespace UI {
namespace Toolbar {

SnapToolbar::SnapToolbar(SPDesktop *desktop)
    : Toolbar(desktop),
    _freeze(false)
{
    // Global snapping control
    {
        auto snap_global_verb = Inkscape::Verb::get(SP_VERB_TOGGLE_SNAPPING);
        _snap_global_item = add_toggle_button(snap_global_verb->get_name(),
                                              snap_global_verb->get_tip());
        _snap_global_item->set_icon_name(INKSCAPE_ICON("snap"));
        _snap_global_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SnapToolbar::on_snap_toggled),
                                                               SP_ATTR_INKSCAPE_SNAP_GLOBAL));
    }

    add_separator();

    // Snapping to bounding boxes
    {
        _snap_from_bbox_corner_item = add_toggle_button(_("Bounding box"),
                                                        _("Snap bounding boxes"));
        _snap_from_bbox_corner_item->set_icon_name(INKSCAPE_ICON("snap"));
        _snap_from_bbox_corner_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SnapToolbar::on_snap_toggled),
                                                                         SP_ATTR_INKSCAPE_SNAP_BBOX));
    }

    {
        _snap_to_bbox_path_item = add_toggle_button(_("Bounding box edges"),
                                                    _("Snap to edges of a bounding box"));
        _snap_to_bbox_path_item->set_icon_name(INKSCAPE_ICON("snap-bounding-box-edges"));
        _snap_to_bbox_path_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SnapToolbar::on_snap_toggled),
                                                                     SP_ATTR_INKSCAPE_SNAP_BBOX_EDGE));
    }

    {
        _snap_to_bbox_node_item = add_toggle_button(_("Bounding box corners"),
                                                    _("Snap bounding box corners"));
        _snap_to_bbox_node_item->set_icon_name(INKSCAPE_ICON("snap-bounding-box-corners"));
        _snap_to_bbox_node_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SnapToolbar::on_snap_toggled),
                                                                     SP_ATTR_INKSCAPE_SNAP_BBOX_CORNER));
    }

    {
        _snap_to_from_bbox_edge_midpoints_item = add_toggle_button(_("BBox Edge Midpoints"),
                                                                   _("Snap midpoints of bounding box edges"));
        _snap_to_from_bbox_edge_midpoints_item->set_icon_name(INKSCAPE_ICON("snap-bounding-box-midpoints"));
        _snap_to_from_bbox_edge_midpoints_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SnapToolbar::on_snap_toggled),
                                                                                    SP_ATTR_INKSCAPE_SNAP_BBOX_EDGE_MIDPOINT));
    }

    {
        _snap_to_from_bbox_edge_centers_item = add_toggle_button(_("BBox Centers"),
                                                                 _("Snapping centers of bounding boxes"));
        _snap_to_from_bbox_edge_centers_item->set_icon_name(INKSCAPE_ICON("snap-bounding-box-center"));
        _snap_to_from_bbox_edge_centers_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SnapToolbar::on_snap_toggled),
                                                                                  SP_ATTR_INKSCAPE_SNAP_BBOX_MIDPOINT));
    }

    add_separator();

    // Snapping to nodes, paths & handles
    {
        _snap_from_node_item = add_toggle_button(_("Nodes"),
                                                 _("Snap nodes, paths, and handles"));
        _snap_from_node_item->set_icon_name(INKSCAPE_ICON("snap"));
        _snap_from_node_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SnapToolbar::on_snap_toggled),
                                                                  SP_ATTR_INKSCAPE_SNAP_NODE));
    }

    {
        _snap_to_item_path_item = add_toggle_button(_("Paths"),
                                                    _("Snap to paths"));
        _snap_to_item_path_item->set_icon_name(INKSCAPE_ICON("snap-nodes-path"));
        _snap_to_item_path_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SnapToolbar::on_snap_toggled),
                                                                     SP_ATTR_INKSCAPE_SNAP_PATH));
    }

    {
        _snap_to_path_intersections_item = add_toggle_button(_("Path intersections"),
                                                             _("Snap to path intersections"));
        _snap_to_path_intersections_item->set_icon_name(INKSCAPE_ICON("snap-nodes-intersection"));
        _snap_to_path_intersections_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SnapToolbar::on_snap_toggled),
                                                                     SP_ATTR_INKSCAPE_SNAP_PATH_INTERSECTION));
    }

    {
        _snap_to_item_node_item = add_toggle_button(_("To nodes"),
                                                             _("Snap to cusp nodes, incl. rectangle corners"));
        _snap_to_item_node_item->set_icon_name(INKSCAPE_ICON("snap-nodes-cusp"));
        _snap_to_item_node_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SnapToolbar::on_snap_toggled),
                                                                     SP_ATTR_INKSCAPE_SNAP_NODE_CUSP));
    }

    {
        _snap_to_smooth_nodes_item = add_toggle_button(_("Smooth nodes"),
                                                       _("Snap smooth nodes, incl. quadrant points of ellipses"));
        _snap_to_smooth_nodes_item->set_icon_name(INKSCAPE_ICON("snap-nodes-smooth"));
        _snap_to_smooth_nodes_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SnapToolbar::on_snap_toggled),
                                                                        SP_ATTR_INKSCAPE_SNAP_NODE_SMOOTH));
    }

    {
        _snap_to_from_line_midpoints_item = add_toggle_button(_("Line Midpoints"),
                                                              _("Snap midpoints of line segments"));
        _snap_to_from_line_midpoints_item->set_icon_name(INKSCAPE_ICON("snap-nodes-midpoint"));
        _snap_to_from_line_midpoints_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SnapToolbar::on_snap_toggled),
                                                                        SP_ATTR_INKSCAPE_SNAP_LINE_MIDPOINT));
    }

    add_separator();

    {
        _snap_from_others_item = add_toggle_button(_("Others"),
                                                   _("Snap other points (centers, guide origins, gradient handles, etc.)"));
        _snap_from_others_item->set_icon_name(INKSCAPE_ICON("snap"));
        _snap_from_others_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SnapToolbar::on_snap_toggled),
                                                                        SP_ATTR_INKSCAPE_SNAP_OTHERS));
    }

    {
        _snap_to_from_object_centers_item = add_toggle_button(_("Object Centers"),
                                                   _("Snap centers of objects"));
        _snap_to_from_object_centers_item->set_icon_name(INKSCAPE_ICON("snap-nodes-center"));
        _snap_to_from_object_centers_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SnapToolbar::on_snap_toggled),
                                                                        SP_ATTR_INKSCAPE_SNAP_OBJECT_MIDPOINT));
    }

    {
        _snap_to_from_rotation_center_item = add_toggle_button(_("Rotation Centers"),
                                                               _("Snap an item's rotation center"));
        _snap_to_from_rotation_center_item->set_icon_name(INKSCAPE_ICON("snap-nodes-rotation-center"));
        _snap_to_from_rotation_center_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SnapToolbar::on_snap_toggled),
                                                                        SP_ATTR_INKSCAPE_SNAP_ROTATION_CENTER));
    }

    {
        _snap_to_from_text_baseline_item = add_toggle_button(_("Text baseline"),
                                                               _("Snap text anchors and baselines"));
        _snap_to_from_text_baseline_item->set_icon_name(INKSCAPE_ICON("snap-text-baseline"));
        _snap_to_from_text_baseline_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SnapToolbar::on_snap_toggled),
                                                                        SP_ATTR_INKSCAPE_SNAP_TEXT_BASELINE));
    }

    add_separator();

    {
        _snap_to_page_border_item = add_toggle_button(_("Page border"),
                                                      _("Snap to the page border"));
        _snap_to_page_border_item->set_icon_name(INKSCAPE_ICON("snap-page"));
        _snap_to_page_border_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SnapToolbar::on_snap_toggled),
                                                                        SP_ATTR_INKSCAPE_SNAP_PAGE_BORDER));
    }

    {
        _snap_to_grids_item = add_toggle_button(_("Grids"),
                                                _("Snap to grids"));
        _snap_to_grids_item->set_icon_name(INKSCAPE_ICON("grid-rectangular"));
        _snap_to_grids_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SnapToolbar::on_snap_toggled),
                                                                        SP_ATTR_INKSCAPE_SNAP_GRID));
    }

    {
        _snap_to_guides_item = add_toggle_button(_("Guides"),
                                                _("Snap guides"));
        _snap_to_guides_item->set_icon_name(INKSCAPE_ICON("guides"));
        _snap_to_guides_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SnapToolbar::on_snap_toggled),
                                                                        SP_ATTR_INKSCAPE_SNAP_GUIDE));
    }

    show_all();
}

GtkWidget *
SnapToolbar::create(SPDesktop *desktop)
{
    auto tb = Gtk::manage(new SnapToolbar(desktop));
    return GTK_WIDGET(tb->gobj());
}

void
SnapToolbar::update(SnapToolbar *tb)
{
    auto nv = tb->_desktop->getNamedView();

    if (nv == nullptr) {
        g_warning("Namedview cannot be retrieved (in updateSnapToolbox)!");
        return;
    }

    // The ..._set_active calls below will toggle the buttons, but this shouldn't lead to
    // changes in our document because we're only updating the UI;
    // Setting the "freeze" parameter to true will block the code in toggle_snap_callback()
    tb->_freeze = true;

    bool const c1 = nv->snap_manager.snapprefs.getSnapEnabledGlobally();
    tb->_snap_global_item->set_active(c1);

    bool const c2 = nv->snap_manager.snapprefs.isTargetSnappable(SNAPTARGET_BBOX_CATEGORY);
    tb->_snap_from_bbox_corner_item->set_active(c2);
    tb->_snap_from_bbox_corner_item->set_sensitive(c1);

    tb->_snap_to_bbox_path_item->set_active(nv->snap_manager.snapprefs.isSnapButtonEnabled(SNAPTARGET_BBOX_EDGE));
    tb->_snap_to_bbox_path_item->set_sensitive(c1 && c2);

    tb->_snap_to_bbox_node_item->set_active(nv->snap_manager.snapprefs.isSnapButtonEnabled(SNAPTARGET_BBOX_CORNER));
    tb->_snap_to_bbox_node_item->set_sensitive(c1 && c2);

    tb->_snap_to_from_bbox_edge_midpoints_item->set_active(nv->snap_manager.snapprefs.isSnapButtonEnabled(SNAPTARGET_BBOX_EDGE_MIDPOINT));
    tb->_snap_to_from_bbox_edge_midpoints_item->set_sensitive(c1 && c2);
    tb->_snap_to_from_bbox_edge_centers_item->set_active(nv->snap_manager.snapprefs.isSnapButtonEnabled(SNAPTARGET_BBOX_MIDPOINT));
    tb->_snap_to_from_bbox_edge_centers_item->set_sensitive(c1 && c2);

    bool const c3 = nv->snap_manager.snapprefs.isTargetSnappable(SNAPTARGET_NODE_CATEGORY);
    tb->_snap_from_node_item->set_active(c3);
    tb->_snap_from_node_item->set_sensitive(c1);

    tb->_snap_to_item_path_item->set_active(nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH));
    tb->_snap_to_item_path_item->set_sensitive(c1 && c3);
    tb->_snap_to_path_intersections_item->set_active(nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_INTERSECTION));
    tb->_snap_to_path_intersections_item->set_sensitive(c1 && c3);
    tb->_snap_to_item_node_item->set_active(nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_NODE_CUSP));
    tb->_snap_to_item_node_item->set_sensitive(c1 && c3);
    tb->_snap_to_smooth_nodes_item->set_active(nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_NODE_SMOOTH));
    tb->_snap_to_smooth_nodes_item->set_sensitive(c1 && c3);
    tb->_snap_to_from_line_midpoints_item->set_active(nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_LINE_MIDPOINT));
    tb->_snap_to_from_line_midpoints_item->set_sensitive(c1 && c3);

    bool const c5 = nv->snap_manager.snapprefs.isTargetSnappable(SNAPTARGET_OTHERS_CATEGORY);
    tb->_snap_from_others_item->set_active(c5);
    tb->_snap_from_others_item->set_sensitive(c1);
    tb->_snap_to_from_object_centers_item->set_active(nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_OBJECT_MIDPOINT));
    tb->_snap_to_from_object_centers_item->set_sensitive(c1 && c5);
    tb->_snap_to_from_rotation_center_item->set_active(nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_ROTATION_CENTER));
    tb->_snap_to_from_rotation_center_item->set_sensitive(c1 && c5);
    tb->_snap_to_from_text_baseline_item->set_active(nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_TEXT_BASELINE));
    tb->_snap_to_from_text_baseline_item->set_sensitive(c1 && c5);
    tb->_snap_to_page_border_item->set_active(nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PAGE_BORDER));
    tb->_snap_to_page_border_item->set_sensitive(c1);
    tb->_snap_to_grids_item->set_active(nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_GRID));
    tb->_snap_to_grids_item->set_sensitive(c1);
    tb->_snap_to_guides_item->set_active(nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_GUIDE));
    tb->_snap_to_guides_item->set_sensitive(c1);

    tb->_freeze = false;
}

void
SnapToolbar::on_snap_toggled(SPAttributeEnum attr)
{
    if(_freeze) return;

    auto dt = _desktop;
    auto nv = dt->getNamedView();

    if(!nv) {
        g_warning("No namedview specified in toggle-snap callback");
        return;
    }

    auto doc = nv->document;
    auto repr = nv->getRepr();

    if(!repr) {
        g_warning("This namedview doesn't have an XML representation attached!");
        return;
    }

    DocumentUndo::ScopedInsensitive _no_undo(doc);

    bool v = false;

    switch (attr) {
        case SP_ATTR_INKSCAPE_SNAP_GLOBAL:
            dt->toggleSnapGlobal();
            break;
        case SP_ATTR_INKSCAPE_SNAP_BBOX:
            v = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_BBOX_CATEGORY);
            sp_repr_set_boolean(repr, "inkscape:snap-bbox", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_BBOX_EDGE:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_EDGE);
            sp_repr_set_boolean(repr, "inkscape:bbox-paths", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_BBOX_CORNER:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_CORNER);
            sp_repr_set_boolean(repr, "inkscape:bbox-nodes", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_NODE:
            v = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_NODE_CATEGORY);
            sp_repr_set_boolean(repr, "inkscape:snap-nodes", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_PATH:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH);
            sp_repr_set_boolean(repr, "inkscape:object-paths", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_PATH_CLIP:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_CLIP);
            sp_repr_set_boolean(repr, "inkscape:snap-path-clip", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_PATH_MASK:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_MASK);
            sp_repr_set_boolean(repr, "inkscape:snap-path-mask", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_NODE_CUSP:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_NODE_CUSP);
            sp_repr_set_boolean(repr, "inkscape:object-nodes", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_NODE_SMOOTH:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_NODE_SMOOTH);
            sp_repr_set_boolean(repr, "inkscape:snap-smooth-nodes", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_PATH_INTERSECTION:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_INTERSECTION);
            sp_repr_set_boolean(repr, "inkscape:snap-intersection-paths", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_OTHERS:
            v = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_OTHERS_CATEGORY);
            sp_repr_set_boolean(repr, "inkscape:snap-others", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_ROTATION_CENTER:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_ROTATION_CENTER);
            sp_repr_set_boolean(repr, "inkscape:snap-center", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_GRID:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_GRID);
            sp_repr_set_boolean(repr, "inkscape:snap-grids", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_GUIDE:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_GUIDE);
            sp_repr_set_boolean(repr, "inkscape:snap-to-guides", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_PAGE_BORDER:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PAGE_BORDER);
            sp_repr_set_boolean(repr, "inkscape:snap-page", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_LINE_MIDPOINT:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_LINE_MIDPOINT);
            sp_repr_set_boolean(repr, "inkscape:snap-midpoints", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_OBJECT_MIDPOINT:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_OBJECT_MIDPOINT);
            sp_repr_set_boolean(repr, "inkscape:snap-object-midpoints", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_TEXT_BASELINE:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_TEXT_BASELINE);
            sp_repr_set_boolean(repr, "inkscape:snap-text-baseline", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_BBOX_EDGE_MIDPOINT:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_EDGE_MIDPOINT);
            sp_repr_set_boolean(repr, "inkscape:snap-bbox-edge-midpoints", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_BBOX_MIDPOINT:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_MIDPOINT);
            sp_repr_set_boolean(repr, "inkscape:snap-bbox-midpoints", !v);
            break;
        default:
            g_warning("toggle_snap_callback has been called with an ID for which no action has been defined");
            break;
    }

    doc->setModifiedSinceSave();
}

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

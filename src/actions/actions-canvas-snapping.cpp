// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for toggling snapping preferences. Tied to a particular document.
 *
 * Copyright (C) 2019 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include <iostream>

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>

#include "actions-canvas-snapping.h"
#include "inkscape-application.h"

#include "document.h"

#include "attributes.h"
#include "desktop.h"
#include "document-undo.h"

#include "object/sp-namedview.h"

// There are four snapping lists that must be connected:
// 1. The attribute name in NamedView: e.g. "inkscape:snap-bbox".
// 2. The SPAttr value:       e.g. SPAttr::INKSCAPE_SNAP_BBOX.
// 3. The Inkscape::SNAPTARGET value:  e.g. Inkscape::SNAPTARGET_BBOX_CATEGORY.
// 4. The Gio::Action name:            e.g. "snap-bbox"
// It seems we could simplify this somehow.

// This might work better as a class.

static void
canvas_snapping_toggle(SPDocument* document, const SPAttr option)
{
    Inkscape::XML::Node* repr = document->getReprNamedView();

    if (repr == nullptr) {
        std::cerr << "canvas_snapping_toggle: namedview XML repr missing!" << std::endl;
        return;
    }

    // This is a bit ackward.
    SPObject* obj = document->getObjectByRepr(repr);
    SPNamedView* nv = dynamic_cast<SPNamedView *> (obj);
    if (nv == nullptr) {
        std::cerr << "canvas_snapping_toggle: no namedview!" << std::endl;
        return;
    }

    // Disable undo
    Inkscape::DocumentUndo::ScopedInsensitive _no_undo(document);

    bool v = false;

    switch (option) {
        case SPAttr::INKSCAPE_SNAP_GLOBAL:
            v = nv->getSnapGlobal();
            nv->setSnapGlobal(!v); // Calls sp_repr_set_boolean
            break;

        // BBox
        case SPAttr::INKSCAPE_SNAP_BBOX:
            v = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_BBOX_CATEGORY);
            sp_repr_set_boolean(repr, "inkscape:snap-bbox", !v);
            break;

        case SPAttr::INKSCAPE_SNAP_BBOX_EDGE:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_EDGE);
            sp_repr_set_boolean(repr, "inkscape:bbox-paths", !v);
            break;

        case SPAttr::INKSCAPE_SNAP_BBOX_CORNER:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_CORNER);
            sp_repr_set_boolean(repr, "inkscape:bbox-nodes", !v);
            break;

        case SPAttr::INKSCAPE_SNAP_BBOX_EDGE_MIDPOINT:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_EDGE_MIDPOINT);
            sp_repr_set_boolean(repr, "inkscape:snap-bbox-edge-midpoints", !v);
            break;

        case SPAttr::INKSCAPE_SNAP_BBOX_MIDPOINT:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_MIDPOINT);
            sp_repr_set_boolean(repr, "inkscape:snap-bbox-midpoints", !v);
            break;

        // Nodes
        case SPAttr::INKSCAPE_SNAP_NODE:
            v = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_NODE_CATEGORY);
            sp_repr_set_boolean(repr, "inkscape:snap-nodes", !v);
            break;

        case SPAttr::INKSCAPE_SNAP_PATH:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH);
            sp_repr_set_boolean(repr, "inkscape:object-paths", !v);
            break;

        case SPAttr::INKSCAPE_SNAP_PATH_INTERSECTION:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_INTERSECTION);
            sp_repr_set_boolean(repr, "inkscape:snap-intersection-paths", !v);
            break;

        case SPAttr::INKSCAPE_SNAP_NODE_CUSP:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_NODE_CUSP);
            sp_repr_set_boolean(repr, "inkscape:object-nodes", !v);
            break;

        case SPAttr::INKSCAPE_SNAP_NODE_SMOOTH:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_NODE_SMOOTH);
            sp_repr_set_boolean(repr, "inkscape:snap-smooth-nodes", !v);
            break;


        case SPAttr::INKSCAPE_SNAP_LINE_MIDPOINT:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_LINE_MIDPOINT);
            sp_repr_set_boolean(repr, "inkscape:snap-midpoints", !v);
            break;

        // Others
        case SPAttr::INKSCAPE_SNAP_OTHERS:
            v = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_OTHERS_CATEGORY);
            sp_repr_set_boolean(repr, "inkscape:snap-others", !v);
            break;

        case SPAttr::INKSCAPE_SNAP_OBJECT_MIDPOINT:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_OBJECT_MIDPOINT);
            sp_repr_set_boolean(repr, "inkscape:snap-object-midpoints", !v);
            break;

        case SPAttr::INKSCAPE_SNAP_ROTATION_CENTER:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_ROTATION_CENTER);
            sp_repr_set_boolean(repr, "inkscape:snap-center", !v);
            break;

        case SPAttr::INKSCAPE_SNAP_TEXT_BASELINE:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_TEXT_BASELINE);
            sp_repr_set_boolean(repr, "inkscape:snap-text-baseline", !v);
            break;

        // Page/Grid/Guides
        case SPAttr::INKSCAPE_SNAP_PAGE_BORDER:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PAGE_BORDER);
            sp_repr_set_boolean(repr, "inkscape:snap-page", !v);
            break;

        case SPAttr::INKSCAPE_SNAP_GRID:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_GRID);
            sp_repr_set_boolean(repr, "inkscape:snap-grids", !v);
            break;

        case SPAttr::INKSCAPE_SNAP_GUIDE:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_GUIDE);
            sp_repr_set_boolean(repr, "inkscape:snap-to-guides", !v);
            break;

        // Not used in default snap toolbar
        case SPAttr::INKSCAPE_SNAP_PATH_CLIP:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_CLIP);
            sp_repr_set_boolean(repr, "inkscape:snap-path-clip", !v);
            break;
        case SPAttr::INKSCAPE_SNAP_PATH_MASK:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_MASK);
            sp_repr_set_boolean(repr, "inkscape:snap-path-mask", !v);
            break;

        default:
            std::cerr << "canvas_snapping_toggle: unhandled option!" << std::endl;
    }

    // Some actions depend on others... we need to update everything!
    set_actions_canvas_snapping(document);

    // The snapping preferences are stored in the document, and therefore toggling makes the document dirty.
    document->setModifiedSinceSave();

}

std::vector<std::vector<Glib::ustring>> raw_data_canvas_snapping =
{
    {"doc.snap-global-toggle",        N_("Snapping"),                          "Snap",  N_("Toggle snapping on/off.")                             },

    {"doc.snap-bbox",                 N_("Snap Bounding Boxes"),               "Snap",  N_("Toggle snapping to bounding boxes (global).")         },
    {"doc.snap-bbox-edge",            N_("Snap Bounding Box Edges"),           "Snap",  N_("Toggle snapping to bounding-box edges.")              },
    {"doc.snap-bbox-corner",          N_("Snap Bounding Box Corners"),         "Snap",  N_("Toggle snapping to bounding-box corners.")            },
    {"doc.snap-bbox-edge-midpoint",   N_("Snap Bounding Box Edge Midpoints"),  "Snap",  N_("Toggle snapping to bounding-box edge mid-points.")    },
    {"doc.snap-bbox-center",          N_("Snap Bounding Box Centers"),         "Snap",  N_("Toggle snapping to bounding-box centers.")            },

    {"doc.snap-node-category",        N_("Snap Nodes"),                        "Snap",  N_("Toggle snapping to nodes (global).")                  },
    {"doc.snap-path",                 N_("Snap Paths"),                        "Snap",  N_("Toggle snapping to paths.")                           },
    {"doc.snap-path-intersection",    N_("Snap Path Intersections"),           "Snap",  N_("Toggle snapping to path intersections.")              },
    {"doc.snap-node-cusp",            N_("Snap Cusp Nodes"),                   "Snap",  N_("Toggle snapping to cusp nodes, including rectangle corners.")},
    {"doc.snap-node-smooth",          N_("Snap Smoth Node"),                   "Snap",  N_("Toggle snapping to smooth nodes, including quadrant points of ellipses.")},
    {"doc.snap-line-midpoint",        N_("Snap Line Midpoints"),               "Snap",  N_("Toggle snapping to midpoints of lines.")              },

    {"doc.snap-others",               N_("Snap Others"),                       "Snap",  N_("Toggle snapping to misc. points (global).")           },
    {"doc.snap-object-midpoint",      N_("Snap Object Midpoint"),              "Snap",  N_("Toggle snapping to object midpoint.")                 },
    {"doc.snap-rotation-center",      N_("Snap Rotation Center"),              "Snap",  N_("Toggle snapping to object rotation center.")          },
    {"doc.snap-text-baseline",        N_("Snap Text Baselines"),               "Snap",  N_("Toggle snapping to text baseline and text anchors.")  },

    {"doc.snap-page-border",          N_("Snap Page Border"),                  "Snap",  N_("Toggle snapping to page border.")                     },
    {"doc.snap-grid",                 N_("Snap Grids"),                        "Snap",  N_("Toggle snapping to grids.")                           },
    {"doc.snap-guide",                N_("Snap Guide Lines"),                  "Snap",  N_("Toggle snapping to guides lines.")                    },

    {"doc.snap-path-mask",            N_("Snap Mask Paths"),                   "Snap",  N_("Toggle snapping to mask paths.")                      },
    {"doc.snap-path-clip",            N_("Snap Clip Paths"),                   "Snap",  N_("Toggle snapping to clip paths.")                      },
};

void
add_actions_canvas_snapping(SPDocument* document)
{
    Glib::RefPtr<Gio::SimpleActionGroup> map = document->getActionGroup();

    map->add_action_bool( "snap-global-toggle",      sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_GLOBAL));

    map->add_action_bool( "snap-bbox",               sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_BBOX));
    map->add_action_bool( "snap-bbox-edge",          sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_BBOX_EDGE));
    map->add_action_bool( "snap-bbox-corner",        sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_BBOX_CORNER));
    map->add_action_bool( "snap-bbox-edge-midpoint", sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_BBOX_EDGE_MIDPOINT));
    map->add_action_bool( "snap-bbox-center",        sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_BBOX_MIDPOINT));

    map->add_action_bool( "snap-node-category",      sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_NODE));
    map->add_action_bool( "snap-path",               sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_PATH));
    map->add_action_bool( "snap-path-intersection",  sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_PATH_INTERSECTION));
    map->add_action_bool( "snap-node-cusp",          sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_NODE_CUSP));
    map->add_action_bool( "snap-node-smooth",        sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_NODE_SMOOTH));
    map->add_action_bool( "snap-line-midpoint",      sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_LINE_MIDPOINT));

    map->add_action_bool( "snap-others",             sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_OTHERS));
    map->add_action_bool( "snap-object-midpoint",    sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_OBJECT_MIDPOINT));
    map->add_action_bool( "snap-rotation-center",    sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_ROTATION_CENTER));
    map->add_action_bool( "snap-text-baseline",      sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_TEXT_BASELINE));

    map->add_action_bool( "snap-page-border",        sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_PAGE_BORDER));
    map->add_action_bool( "snap-grid",               sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_GRID));
    map->add_action_bool( "snap-guide",              sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_GUIDE));

    // Not used in toolbar
    map->add_action_bool( "snap-path-mask",          sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_PATH_MASK));
    map->add_action_bool( "snap-path-clip",          sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_PATH_CLIP));

    // Check if there is already an application instance (GUI or non-GUI).
    auto app = InkscapeApplication::instance();
    if (!app) {
        std::cerr << "add_actions_canvas_snapping: no app!" << std::endl;
        return;
    }
    app->get_action_extra_data().add_data(raw_data_canvas_snapping);
}


void
set_actions_canvas_snapping_helper (Glib::RefPtr<Gio::SimpleActionGroup>& map, Glib::ustring action_name, bool state, bool enabled)
{
    // Glib::RefPtr<Gio::SimpleAction> saction = map->lookup_action(action_name); NOT POSSIBLE!

    // We can't enable/disable action directly! (Gio::Action can "get" enabled value but can not
    // "set" it! We need to cast to Gio::SimpleAction)
    Glib::RefPtr<Gio::Action> action = map->lookup_action(action_name);
    if (!action) {
        std::cerr << "set_actions_canvas_snapping_helper: action " << action_name << " missing!" << std::endl;
        return;
    }

    auto simple = Glib::RefPtr<Gio::SimpleAction>::cast_dynamic(action);
    if (!simple) {
        std::cerr << "set_actions_canvas_snapping_helper: action " << action_name << " not SimpleAction!" << std::endl;
        return;
    }

    simple->change_state(state);
    simple->set_enabled(enabled);
}

void
set_actions_canvas_snapping(SPDocument* document)
{
    Inkscape::XML::Node* repr = document->getReprNamedView();

    if (repr == nullptr) {
        std::cerr << "set_actions_canvas_snapping: namedview XML repr missing!" << std::endl;
        return;
    }

    // This is a bit ackward.
    SPObject* obj = document->getObjectByRepr(repr);
    SPNamedView* nv = dynamic_cast<SPNamedView *> (obj);

    if (nv == nullptr) {
        std::cerr << "set_actions_canvas_snapping: no namedview!" << std::endl;
        return;
    }

    Glib::RefPtr<Gio::SimpleActionGroup> map = document->getActionGroup();
    if (!map) {
        std::cerr << "set_actions_canvas_snapping: no ActionGroup!" << std::endl;
        return;
    }

    bool global = nv->snap_manager.snapprefs.getSnapEnabledGlobally();
    set_actions_canvas_snapping_helper(map, "snap-global-toggle", global, true); // Always enabled

    bool bbox = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_BBOX_CATEGORY);
    set_actions_canvas_snapping_helper(map, "snap-bbox", bbox, global);
    set_actions_canvas_snapping_helper(map, "snap-bbox-edge",          nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_EDGE),          global && bbox);
    set_actions_canvas_snapping_helper(map, "snap-bbox-corner",        nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_CORNER),        global && bbox);
    set_actions_canvas_snapping_helper(map, "snap-bbox-edge-midpoint", nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_EDGE_MIDPOINT), global && bbox);
    set_actions_canvas_snapping_helper(map, "snap-bbox-center",        nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_MIDPOINT),      global && bbox);

    bool node = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_NODE_CATEGORY);
    set_actions_canvas_snapping_helper(map, "snap-node-category", node, global);
    set_actions_canvas_snapping_helper(map, "snap-path",               nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH),               global && node);
    set_actions_canvas_snapping_helper(map, "snap-path-intersection",  nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_INTERSECTION),  global && node);
    set_actions_canvas_snapping_helper(map, "snap-node-cusp",          nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_NODE_CUSP),          global && node);
    set_actions_canvas_snapping_helper(map, "snap-node-smooth",        nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_NODE_SMOOTH),        global && node);
    set_actions_canvas_snapping_helper(map, "snap-line-midpoint",      nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_LINE_MIDPOINT),      global && node);

    bool other = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_OTHERS_CATEGORY);
    set_actions_canvas_snapping_helper(map, "snap-others", other, global);
    set_actions_canvas_snapping_helper(map, "snap-object-midpoint",    nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_OBJECT_MIDPOINT),    global && other);
    set_actions_canvas_snapping_helper(map, "snap-rotation-center",    nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_ROTATION_CENTER),    global && other);
    set_actions_canvas_snapping_helper(map, "snap-text-baseline",      nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_TEXT_BASELINE),      global && other);

    set_actions_canvas_snapping_helper(map, "snap-page-border",        nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PAGE_BORDER),        global);
    set_actions_canvas_snapping_helper(map, "snap-grid",               nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_GRID),               global);
    set_actions_canvas_snapping_helper(map, "snap-guide",              nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_GUIDE),              global);

    set_actions_canvas_snapping_helper(map, "snap-path-mask",          nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_CLIP),          global);
    set_actions_canvas_snapping_helper(map, "snap-path-clip",          nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_MASK),          global);
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

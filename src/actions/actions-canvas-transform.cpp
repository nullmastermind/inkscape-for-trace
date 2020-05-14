// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for transforming the canvas view. Tied to a particular InkscapeWindow.
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include <iostream>

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>

#include "actions-canvas-transform.h"
#include "inkscape-application.h"
#include "inkscape-window.h"
#include "desktop.h"

#include "display/sp-canvas.h"

#include "ui/tools-switch.h"  // TOOLS_FREEHAND_PEN, etc.
#include "ui/tools/freehand-base.h" // SP_DRAW_CONTEXT

enum {
    INK_CANVAS_ZOOM_IN,
    INK_CANVAS_ZOOM_OUT,
    INK_CANVAS_ZOOM_1_1,
    INK_CANVAS_ZOOM_1_2,
    INK_CANVAS_ZOOM_2_1,
    INK_CANVAS_ZOOM_SELECTION,
    INK_CANVAS_ZOOM_DRAWING,
    INK_CANVAS_ZOOM_PAGE,
    INK_CANVAS_ZOOM_PAGE_WIDTH,
    INK_CANVAS_ZOOM_PREV,
    INK_CANVAS_ZOOM_NEXT
};

static void
canvas_zoom_helper(SPDesktop* dt, const Geom::Point& midpoint, double zoom_factor)
{
    if (tools_isactive(dt, TOOLS_FREEHAND_PENCIL) ||
        tools_isactive(dt, TOOLS_FREEHAND_PEN   ) ) {

        // Zoom around end of unfinished path.
        boost::optional<Geom::Point> zoom_to =
            SP_DRAW_CONTEXT(dt->event_context)->red_curve_get_last_point();
        if (zoom_to) {
            dt->zoom_relative_keep_point(*zoom_to, zoom_factor);
            return;
        }
    }

    dt->zoom_relative_center_point(midpoint, zoom_factor);
}

void
canvas_transform(InkscapeWindow *win, const int& option)
{
    SPDesktop* dt = win->get_desktop();

    // The following might be better done elsewhere:

    // Get preference dependent parameters
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double zoom_inc =
        prefs->getDoubleLimited("/options/zoomincrement/value", M_SQRT2, 1.01, 10);

    // Get document dependent parameters
    Geom::Rect const canvas = dt->getCanvas()->getViewbox();  // Not SVG 'viewBox'!
    Geom::Point midpoint = dt->w2d(canvas.midpoint());        // Midpoint of drawing on canvas.

    switch (option) {
        case INK_CANVAS_ZOOM_IN:
            canvas_zoom_helper( dt, midpoint,       zoom_inc);
            break;

        case INK_CANVAS_ZOOM_OUT:
            canvas_zoom_helper( dt, midpoint, 1.0 / zoom_inc); // zoom_inc > 1
            break;

        case INK_CANVAS_ZOOM_1_1:
            dt->zoom_absolute_center_point( midpoint, 1.0 );
            break;

        case INK_CANVAS_ZOOM_1_2:
            dt->zoom_absolute_center_point( midpoint, 0.5 );
            break;

        case INK_CANVAS_ZOOM_2_1:
            dt->zoom_absolute_center_point( midpoint, 2.0 );
            break;

        case INK_CANVAS_ZOOM_SELECTION:
            dt->zoom_selection();
            break;

        case INK_CANVAS_ZOOM_DRAWING:
            dt->zoom_drawing();
            break;

        case INK_CANVAS_ZOOM_PAGE:
            dt->zoom_page();
            break;

        case INK_CANVAS_ZOOM_PAGE_WIDTH:
            dt->zoom_page_width();
            break;

        case INK_CANVAS_ZOOM_PREV:
            dt->prev_transform();
            break;

        case INK_CANVAS_ZOOM_NEXT:
            dt->next_transform(); // Is this only zoom?
            break;

        default:
            std::cerr << "canvas_zoom: unhandled action value!" << std::endl;
    }
}

std::vector<std::vector<Glib::ustring>> raw_data_canvas_transform =
{
    {"win.canvas-zoom-in",            "ZoomIn",                  "View",       N_("Zoom in")                                            },                  
    {"win.canvas-zoom-out",           "ZoomOut",                 "View",       N_("Zoom out")                                           },
    {"win.canvas-zoom-1-1",           "Zoom1:1",                 "View",       N_("Zoom to 1:1")                                        },
    {"win.canvas-zoom-1-2",           "Zoom1:2",                 "View",       N_("Zoom to 1:2")                                        },
    {"win.canvas-zoom-2-1",           "Zoom2:1",                 "View",       N_("Zoom to 2:1")                                        },
    {"win.canvas-zoom-selection",     "ZoomSelection",           "View",       N_("Zoom to fit selection in window")                    },
    {"win.canvas-zoom-drawing",       "ZoomDrawing",             "View",       N_("Zoom to fit drawing in window")                      },
    {"win.canvas-zoom-page",          "ZoomPage",                "View",       N_("Zoom to fit page in window")                         },
    {"win.canvas-zoom-page-width",    "ZoomPageWidth",           "View",       N_("Zoom to fit page width in window")                   },
    {"win.canvas-zoom-prev",          "ZoomPrev",                "View",       N_("Previous zoom (from the history of zooms)")          },
    {"win.canvas-zoom-next",          "ZoomNext",                "View",       N_("Next zoom (from the history of zooms)")              },
    {"win.canvas-zoom-center-page",   "ZoomCenterPage",          "View",       N_("Center page in window")                              },
};

void
add_actions_canvas_transform(InkscapeWindow* win)
{
    win->add_action( "canvas-zoom-in",         sigc::bind<InkscapeWindow*, int>(sigc::ptr_fun(&canvas_transform), win, INK_CANVAS_ZOOM_IN));
    win->add_action( "canvas-zoom-out",        sigc::bind<InkscapeWindow*, int>(sigc::ptr_fun(&canvas_transform), win, INK_CANVAS_ZOOM_OUT));
    win->add_action( "canvas-zoom-1-1",        sigc::bind<InkscapeWindow*, int>(sigc::ptr_fun(&canvas_transform), win, INK_CANVAS_ZOOM_1_1));
    win->add_action( "canvas-zoom-1-2",        sigc::bind<InkscapeWindow*, int>(sigc::ptr_fun(&canvas_transform), win, INK_CANVAS_ZOOM_1_2));
    win->add_action( "canvas-zoom-2-1",        sigc::bind<InkscapeWindow*, int>(sigc::ptr_fun(&canvas_transform), win, INK_CANVAS_ZOOM_2_1));
    win->add_action( "canvas-zoom-selection",  sigc::bind<InkscapeWindow*, int>(sigc::ptr_fun(&canvas_transform), win, INK_CANVAS_ZOOM_SELECTION));
    win->add_action( "canvas-zoom-drawing",    sigc::bind<InkscapeWindow*, int>(sigc::ptr_fun(&canvas_transform), win, INK_CANVAS_ZOOM_DRAWING));
    win->add_action( "canvas-zoom-page",       sigc::bind<InkscapeWindow*, int>(sigc::ptr_fun(&canvas_transform), win, INK_CANVAS_ZOOM_PAGE));
    win->add_action( "canvas-zoom-page-width", sigc::bind<InkscapeWindow*, int>(sigc::ptr_fun(&canvas_transform), win, INK_CANVAS_ZOOM_PAGE_WIDTH));
    win->add_action( "canvas-zoom-prev",       sigc::bind<InkscapeWindow*, int>(sigc::ptr_fun(&canvas_transform), win, INK_CANVAS_ZOOM_PREV));
    win->add_action( "canvas-zoom-next",       sigc::bind<InkscapeWindow*, int>(sigc::ptr_fun(&canvas_transform), win, INK_CANVAS_ZOOM_NEXT));

    auto app = &(ConcreteInkscapeApplication<Gtk::Application>::get_instance());
    app->get_action_extra_data().add_data(raw_data_canvas_transform);
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

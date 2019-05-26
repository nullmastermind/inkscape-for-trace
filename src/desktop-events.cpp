// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Event handlers for SPDesktop.
 */
/* Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 1999-2010 Others
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <map>
#include <string>

#include "desktop-events.h"

#include <gdkmm/display.h>
#include <gdkmm/seat.h>

#include <gtk/gtk.h>

#include <glibmm/i18n.h>

#include <2geom/line.h>
#include <2geom/angle.h>

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "message-context.h"
#include "preferences.h"
#include "snap.h"
#include "sp-cursor.h"
#include "verbs.h"

#include "display/canvas-grid.h"
#include "display/guideline.h"
#include "display/snap-indicator.h"
#include "display/sp-canvas.h"

#include "helper/action.h"

#include "ui/pixmaps/cursor-select.xpm"

#include "object/sp-guide.h"
#include "object/sp-namedview.h"
#include "object/sp-root.h"

#include "ui/dialog-events.h"
#include "ui/tools-switch.h"
#include "ui/dialog/guides.h"
#include "ui/tools/tool-base.h"

#include "widgets/desktop-widget.h"

#include "xml/repr.h"

using Inkscape::DocumentUndo;

static void snoop_extended(GdkEvent* event, SPDesktop *desktop);
static void init_extended();

/* Root item handler */

int sp_desktop_root_handler(SPCanvasItem */*item*/, GdkEvent *event, SPDesktop *desktop)
{
    static bool watch = false;
    static bool first = true;

    if ( first ) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        if ( prefs->getBool("/options/useextinput/value", true)
            && prefs->getBool("/options/switchonextinput/value") ) {
            watch = true;
            init_extended();
        }
        first = false;
    }
    if ( watch ) {
        snoop_extended(event, desktop);
    }

    return sp_event_context_root_handler(desktop->event_context, event);
}



static Geom::Point drag_origin;
static SPGuideDragType drag_type = SP_DRAG_NONE;
//static bool reset_drag_origin = false; // when Ctrl is pressed while dragging, this is used to trigger resetting of the
//                                       // drag origin to that location so that constrained movement is more intuitive

// Min distance from anchor to initiate rotation, measured in screenpixels
#define tol 40.0

gint sp_dt_guide_event(SPCanvasItem *item, GdkEvent *event, gpointer data)
{
    static bool moved = false;
    gint ret = FALSE;

    SPGuide *guide = SP_GUIDE(data);
    SPDesktop *desktop = static_cast<SPDesktop*>(g_object_get_data(G_OBJECT(item->canvas), "SPDesktop"));

    switch (event->type) {
    case GDK_2BUTTON_PRESS:
            if (event->button.button == 1) {
                drag_type = SP_DRAG_NONE;
                sp_event_context_discard_delayed_snap_event(desktop->event_context);
                sp_canvas_item_ungrab(item);
                Inkscape::UI::Dialogs::GuidelinePropertiesDialog::showDialog(guide, desktop);
                ret = TRUE;
            }
            break;
    case GDK_BUTTON_PRESS:
            if (event->button.button == 1) {
                Geom::Point const event_w(event->button.x, event->button.y);
                Geom::Point const event_dt(desktop->w2d(event_w));

                // Due to the tolerance allowed when grabbing a guide, event_dt will generally
                // be close to the guide but not just exactly on it. The drag origin calculated
                // here must be exactly on the guide line though, otherwise
                // small errors will occur once we snap, see
                // https://bugs.launchpad.net/inkscape/+bug/333762
                drag_origin = Geom::projection(event_dt, Geom::Line(guide->getPoint(), guide->angle()));

                if (event->button.state & GDK_SHIFT_MASK) {
                    // with shift we rotate the guide
                    drag_type = SP_DRAG_ROTATE;
                } else {
                    if (event->button.state & GDK_CONTROL_MASK) {
                        drag_type = SP_DRAG_MOVE_ORIGIN;
                    } else {
                        drag_type = SP_DRAG_TRANSLATE;
                    }
                }

                if (drag_type == SP_DRAG_ROTATE || drag_type == SP_DRAG_TRANSLATE) {
                    sp_canvas_item_grab(item,
                                        ( GDK_BUTTON_RELEASE_MASK  |
                                          GDK_BUTTON_PRESS_MASK    |
                                          GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK ),
                                        nullptr,
                                        event->button.time);
                }
                ret = TRUE;
            }
            break;
        case GDK_MOTION_NOTIFY:
            if (drag_type != SP_DRAG_NONE) {
                Geom::Point const motion_w(event->motion.x,
                                           event->motion.y);
                Geom::Point motion_dt(desktop->w2d(motion_w));

                sp_event_context_snap_delay_handler(desktop->event_context, (gpointer) item, data, (GdkEventMotion *)event, Inkscape::UI::Tools::DelayedSnapEvent::GUIDE_HANDLER);

                // This is for snapping while dragging existing guidelines. New guidelines,
                // which are dragged off the ruler, are being snapped in sp_dt_ruler_event
                SnapManager &m = desktop->namedview->snap_manager;
                m.setup(desktop, true, nullptr, nullptr, guide);
                if (drag_type == SP_DRAG_MOVE_ORIGIN) {
                    // If we snap in guideConstrainedSnap() below, then motion_dt will
                    // be forced to be on the guide. If we don't snap however, then
                    // the origin should still be constrained to the guide. So let's do
                    // that explicitly first:
                    Geom::Line line(guide->getPoint(), guide->angle());
                    Geom::Coord t = line.nearestTime(motion_dt);
                    motion_dt = line.pointAt(t);
                    if (!(event->motion.state & GDK_SHIFT_MASK)) {
                        m.guideConstrainedSnap(motion_dt, *guide);
                    }
                } else if (!((drag_type == SP_DRAG_ROTATE) && (event->motion.state & GDK_CONTROL_MASK))) {
                    // cannot use shift here to disable snapping, because we already use it for rotating the guide
                    Geom::Point temp;
                    if (drag_type == SP_DRAG_ROTATE) {
                        temp = guide->getPoint();
                        m.guideFreeSnap(motion_dt, temp, true, false);
                        guide->moveto(temp, false);
                    } else {
                        temp = guide->getNormal();
                        m.guideFreeSnap(motion_dt, temp, false, true);
                        guide->set_normal(temp, false);
                    }
                }
                m.unSetup();

                switch (drag_type) {
                    case SP_DRAG_TRANSLATE:
                    {
                        guide->moveto(motion_dt, false);
                        break;
                    }
                    case SP_DRAG_ROTATE:
                    {
                        Geom::Point pt = motion_dt - guide->getPoint();
                        Geom::Angle angle(pt);
                        if (event->motion.state & GDK_CONTROL_MASK) {
                            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
                            unsigned const snaps = abs(prefs->getInt("/options/rotationsnapsperpi/value", 12));
                            bool const relative_snaps = prefs->getBool("/options/relativeguiderotationsnap/value", false);
                            if (snaps) {
                                if (relative_snaps) {
                                    Geom::Angle orig_angle(guide->getNormal());
                                    Geom::Angle snap_angle = angle - orig_angle;
                                    double sections = floor(snap_angle.radians0() * snaps / M_PI + .5);
                                    angle = (M_PI / snaps) * sections + orig_angle.radians0();
                                } else {
                                    double sections = floor(angle.radians0() * snaps / M_PI + .5);
                                    angle = (M_PI / snaps) * sections;
                                }
                            }
                        }
                        guide->set_normal(Geom::Point::polar(angle).cw(), false);
                        break;
                    }
                    case SP_DRAG_MOVE_ORIGIN:
                    {
                        guide->moveto(motion_dt, false);
                        break;
                    }
                    case SP_DRAG_NONE:
                        assert(false);
                        break;
                }
                moved = true;
                desktop->set_coordinate_status(motion_dt);

                ret = TRUE;
            }
            break;
    case GDK_BUTTON_RELEASE:
            if (drag_type != SP_DRAG_NONE && event->button.button == 1) {
                sp_event_context_discard_delayed_snap_event(desktop->event_context);

                if (moved) {
                    Geom::Point const event_w(event->button.x,
                                              event->button.y);
                    Geom::Point event_dt(desktop->w2d(event_w));

                    SnapManager &m = desktop->namedview->snap_manager;
                    m.setup(desktop, true, nullptr, nullptr, guide);
                    if (drag_type == SP_DRAG_MOVE_ORIGIN) {
                        // If we snap in guideConstrainedSnap() below, then motion_dt will
                        // be forced to be on the guide. If we don't snap however, then
                        // the origin should still be constrained to the guide. So let's
                        // do that explicitly first:
                        Geom::Line line(guide->getPoint(), guide->angle());
                        Geom::Coord t = line.nearestTime(event_dt);
                        event_dt = line.pointAt(t);
                        if (!(event->button.state & GDK_SHIFT_MASK)) {
                            m.guideConstrainedSnap(event_dt, *guide);
                        }
                    } else if (!((drag_type == SP_DRAG_ROTATE) && (event->motion.state & GDK_CONTROL_MASK))) {
                        // cannot use shift here to disable snapping, because we already use it for rotating the guide
                        Geom::Point temp;
                        if (drag_type == SP_DRAG_ROTATE) {
                            temp = guide->getPoint();
                            m.guideFreeSnap(event_dt, temp, true, false);
                            guide->moveto(temp, false);
                        } else {
                            temp = guide->getNormal();
                            m.guideFreeSnap(event_dt, temp, false, true);
                            guide->set_normal(temp, false);
                        }
                    }
                    m.unSetup();

                    if (sp_canvas_world_pt_inside_window(item->canvas, event_w)) {
                        switch (drag_type) {
                            case SP_DRAG_TRANSLATE:
                            {
                                guide->moveto(event_dt, true);
                                break;
                            }
                            case SP_DRAG_ROTATE:
                            {
                                Geom::Point pt = event_dt - guide->getPoint();
                                Geom::Angle angle(pt);
                                if (event->motion.state & GDK_CONTROL_MASK) {
                                    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
                                    unsigned const snaps = abs(prefs->getInt("/options/rotationsnapsperpi/value", 12));
                                    bool const relative_snaps = prefs->getBool("/options/relativeguiderotationsnap/value", false);
                                    if (snaps) {
                                        if (relative_snaps) {
                                            Geom::Angle orig_angle(guide->getNormal());
                                            Geom::Angle snap_angle = angle - orig_angle;
                                            double sections = floor(snap_angle.radians0() * snaps / M_PI + .5);
                                            angle = (M_PI / snaps) * sections + orig_angle.radians0();
                                        } else {
                                            double sections = floor(angle.radians0() * snaps / M_PI + .5);
                                            angle = (M_PI / snaps) * sections;
                                        }
                                    }
                                }
                                guide->set_normal(Geom::Point::polar(angle).cw(), true);
                                break;
                            }
                            case SP_DRAG_MOVE_ORIGIN:
                            {
                                guide->moveto(event_dt, true);
                                break;
                            }
                            case SP_DRAG_NONE:
                                assert(false);
                                break;
                        }
                        DocumentUndo::done(desktop->getDocument(), SP_VERB_NONE,
                                         _("Move guide"));
                    } else {
                        /* Undo movement of any attached shapes. */
                        guide->moveto(guide->getPoint(), false);
                        guide->set_normal(guide->getNormal(), false);
                        sp_guide_remove(guide);

                        Glib::wrap(GTK_WIDGET(desktop->getCanvas()))->get_window()->set_cursor(desktop->event_context->cursor);

                        DocumentUndo::done(desktop->getDocument(), SP_VERB_NONE,
                                     _("Delete guide"));
                    }
                    moved = false;
                    desktop->set_coordinate_status(event_dt);
                }
                drag_type = SP_DRAG_NONE;
                sp_canvas_item_ungrab(item);
                ret=TRUE;
            }
            break;
    case GDK_ENTER_NOTIFY:
    {
            if (!guide->getLocked()) {
                sp_guideline_set_color(SP_GUIDELINE(item), guide->getHiColor());
            }

            // set move or rotate cursor
            Geom::Point const event_w(event->crossing.x, event->crossing.y);

            GdkDisplay *display = gdk_display_get_default();
            GdkCursorType cursor_type;

            if ((event->crossing.state & GDK_SHIFT_MASK) && (drag_type != SP_DRAG_MOVE_ORIGIN)) {
                cursor_type = GDK_EXCHANGE;
            } else {
                cursor_type = GDK_HAND1;
            }

            GdkCursor *guide_cursor = gdk_cursor_new_for_display(display, cursor_type);
            if(guide->getLocked()){
                guide_cursor = sp_cursor_from_xpm(cursor_select_xpm);
            }
            gdk_window_set_cursor(gtk_widget_get_window (GTK_WIDGET(desktop->getCanvas())), guide_cursor);
            g_object_unref(guide_cursor);

            char *guide_description = guide->description();
            desktop->guidesMessageContext()->setF(Inkscape::NORMAL_MESSAGE, _("<b>Guideline</b>: %s"), guide_description);
            g_free(guide_description);
            break;
    }
    case GDK_LEAVE_NOTIFY:
            sp_guideline_set_color(SP_GUIDELINE(item), guide->getColor());

            // restore event context's cursor
            Glib::wrap(GTK_WIDGET(desktop->getCanvas()))->get_window()->set_cursor(desktop->event_context->cursor);

            desktop->guidesMessageContext()->clear();
            break;
        case GDK_KEY_PRESS:
            switch (Inkscape::UI::Tools::get_latin_keyval (&event->key)) {
                case GDK_KEY_Delete:
                case GDK_KEY_KP_Delete:
                case GDK_KEY_BackSpace:
                {
                    SPDocument *doc = guide->document;
                    sp_guide_remove(guide);
                    DocumentUndo::done(doc, SP_VERB_NONE, _("Delete guide"));
                    ret = TRUE;
                    sp_event_context_discard_delayed_snap_event(desktop->event_context);
                    Glib::wrap(GTK_WIDGET(desktop->getCanvas()))->get_window()->set_cursor(desktop->event_context->cursor);
                    break;
                }
                case GDK_KEY_Shift_L:
                case GDK_KEY_Shift_R:
                    if (drag_type != SP_DRAG_MOVE_ORIGIN) {
                        GdkDisplay *display      = gdk_display_get_default();
                        GdkCursor  *guide_cursor = gdk_cursor_new_for_display(display, GDK_EXCHANGE);
                        gdk_window_set_cursor(gtk_widget_get_window (GTK_WIDGET(desktop->getCanvas())), guide_cursor);
                        g_object_unref(guide_cursor);
                        ret = TRUE;
                        break;
                    }

                default:
                    // do nothing;
                    break;
            }
            break;
        case GDK_KEY_RELEASE:
            switch (Inkscape::UI::Tools::get_latin_keyval (&event->key)) {
                case GDK_KEY_Shift_L:
                case GDK_KEY_Shift_R:
                {
                    GdkDisplay *display      = gdk_display_get_default();
                    GdkCursor  *guide_cursor = gdk_cursor_new_for_display(display, GDK_HAND1);
                    gdk_window_set_cursor(gtk_widget_get_window (GTK_WIDGET(desktop->getCanvas())), guide_cursor);
                    g_object_unref(guide_cursor);
                    break;
                }
                default:
                    // do nothing;
                    break;
            }
            break;
    default:
        break;
    }

    return ret;
}

//static std::map<GdkInputSource, std::string> switchMap;
static std::map<std::string, int> toolToUse;
static std::string lastName;
static GdkInputSource lastType = GDK_SOURCE_MOUSE;

static void init_extended()
{
    Glib::ustring avoidName("pad");
    auto display = Gdk::Display::get_default();
    auto seat = display->get_default_seat();
    auto const devices = seat->get_slaves(Gdk::SEAT_CAPABILITY_ALL);
    
    if ( !devices.empty() ) {
        for (auto const dev : devices) {
            auto const devName = dev->get_name();
            auto devSrc = dev->get_source();
            
            if ( !devName.empty()
                 && (avoidName != devName)
                 && (devSrc != Gdk::SOURCE_MOUSE) ) {
//                 g_message("Adding '%s' as [%d]", devName, devSrc);

                // Set the initial tool for the device
                switch ( devSrc ) {
                    case Gdk::SOURCE_PEN:
                        toolToUse[devName] = TOOLS_CALLIGRAPHIC;
                        break;
                    case Gdk::SOURCE_ERASER:
                        toolToUse[devName] = TOOLS_ERASER;
                        break;
                    case Gdk::SOURCE_CURSOR:
                        toolToUse[devName] = TOOLS_SELECT;
                        break;
                    default:
                        ; // do not add
                }
//            } else if (devName) {
//                 g_message("Skippn '%s' as [%d]", devName, devSrc);
            }
        }
    }
}


void snoop_extended(GdkEvent* event, SPDesktop *desktop)
{
    GdkInputSource source = GDK_SOURCE_MOUSE;
    std::string name;

    switch ( event->type ) {
        case GDK_MOTION_NOTIFY:
        {
            GdkEventMotion* event2 = reinterpret_cast<GdkEventMotion*>(event);
            if ( event2->device ) {
                source = gdk_device_get_source(event2->device);
                name = gdk_device_get_name(event2->device);
            }
        }
        break;

        case GDK_BUTTON_PRESS:
        case GDK_2BUTTON_PRESS:
        case GDK_3BUTTON_PRESS:
        case GDK_BUTTON_RELEASE:
        {
            GdkEventButton* event2 = reinterpret_cast<GdkEventButton*>(event);
            if ( event2->device ) {
                source = gdk_device_get_source(event2->device);
                name = gdk_device_get_name(event2->device);
            }
        }
        break;

        case GDK_SCROLL:
        {
            GdkEventScroll* event2 = reinterpret_cast<GdkEventScroll*>(event);
            if ( event2->device ) {
                source = gdk_device_get_source(event2->device);
                name = gdk_device_get_name(event2->device);
            }
        }
        break;

        case GDK_PROXIMITY_IN:
        case GDK_PROXIMITY_OUT:
        {
            GdkEventProximity* event2 = reinterpret_cast<GdkEventProximity*>(event);
            if ( event2->device ) {
                source = gdk_device_get_source(event2->device);
                name = gdk_device_get_name(event2->device);
            }
        }
        break;

        default:
            ;
    }

    if (!name.empty()) {
        if ( lastType != source || lastName != name ) {
            // The device switched. See if it is one we 'count'
            //g_message("Changed device %s -> %s", lastName.c_str(), name.c_str());
            std::map<std::string, int>::iterator it = toolToUse.find(lastName);
            if (it != toolToUse.end()) {
                // Save the tool currently selected for next time the input
                // device shows up.
                it->second = tools_active(desktop);
            }

            it = toolToUse.find(name);
            if (it != toolToUse.end() ) {
                tools_switch(desktop, it->second);
            }

            lastName = name;
            lastType = source;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :


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
#include "verbs.h"

#include "display/control/snap-indicator.h"
#include "display/control/canvas-item.h" // NOT USED!!
#include "display/control/canvas-item-guideline.h"

#include "helper/action.h"

#include "object/sp-guide.h"
#include "object/sp-namedview.h"
#include "object/sp-root.h"

#include "ui/cursor-utils.h"
#include "ui/dialog-events.h"
#include "ui/tools-switch.h"
#include "ui/dialog/guides.h"
#include "ui/tools/tool-base.h"
#include "ui/widget/canvas.h"  // Desktop hidden in g_object data.

#include "ui/event-debug.h"

#include "widgets/desktop-widget.h"

#include "xml/repr.h"

using Inkscape::DocumentUndo;

static void snoop_extended(GdkEvent* event, SPDesktop *desktop);
static void init_extended();

/* Root canvas item handler */

bool sp_desktop_root_handler(GdkEvent *event, SPDesktop *desktop)
{
#ifdef EVENT_DEBUG
    ui_dump_event(reinterpret_cast<GdkEvent *>(event), "sp_desktop_root_handler");
#endif
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

    return (bool)sp_event_context_root_handler(desktop->event_context, event);
}



static Geom::Point drag_origin;
static SPGuideDragType drag_type = SP_DRAG_NONE;
//static bool reset_drag_origin = false; // when Ctrl is pressed while dragging, this is used to trigger resetting of the
//                                       // drag origin to that location so that constrained movement is more intuitive

// Min distance from anchor to initiate rotation, measured in screenpixels
#define tol 40.0

bool sp_dt_guide_event(GdkEvent *event, Inkscape::CanvasItemGuideLine *guide_item, SPGuide *guide)
{
#ifdef EVENT_DEBUG
    ui_dump_event(reinterpret_cast<GdkEvent *>(event), "sp_dt_guide_event");
#endif

    static bool moved = false;
    bool ret = false;

    SPDesktop *desktop = guide_item->get_canvas()->get_desktop();
    if (!desktop) {
        std::cerr << "sp_dt_guide_event: No desktop!" << std::endl;
    }

    switch (event->type) {
        case GDK_2BUTTON_PRESS:
            if (event->button.button == 1) {
                drag_type = SP_DRAG_NONE;
                sp_event_context_discard_delayed_snap_event(desktop->event_context);
                guide_item->ungrab();
                Inkscape::UI::Dialogs::GuidelinePropertiesDialog::showDialog(guide, desktop);
                ret = true;
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
                    guide_item->grab((Gdk::BUTTON_RELEASE_MASK      |
                                      Gdk::BUTTON_PRESS_MASK        |
                                      Gdk::POINTER_MOTION_MASK      |
                                      Gdk::POINTER_MOTION_HINT_MASK ),
                                     nullptr);
                }
                ret = true;
            }
            break;

        case GDK_MOTION_NOTIFY:
            if (drag_type != SP_DRAG_NONE) {
                Geom::Point const motion_w(event->motion.x,
                                           event->motion.y);
                Geom::Point motion_dt(desktop->w2d(motion_w));

                sp_event_context_snap_delay_handler(
                    desktop->event_context, (void *) guide_item, (void *) guide, (GdkEventMotion *)event,
                    Inkscape::UI::Tools::DelayedSnapEvent::GUIDE_HANDLER);

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

                ret = true;
            }
            break;

        case GDK_BUTTON_RELEASE:
            if (drag_type != SP_DRAG_NONE && event->button.button == 1) {
                sp_event_context_discard_delayed_snap_event(desktop->event_context);

                if (moved) {
                    Geom::Point const event_w(event->button.x,
                                              event->button.y);
                    Geom::Point event_dt(desktop->w2d(event_w));
                    std::cout << "    event_w: " << event_w << ", event_dt: " << event_dt << std::endl;
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

                    if (guide_item->get_canvas()->world_point_inside_canvas(event_w)) {
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
                        desktop->getCanvas()->get_window()->set_cursor(desktop->event_context->cursor);

                        DocumentUndo::done(desktop->getDocument(), SP_VERB_NONE,
                                           _("Delete guide"));
                    }
                    moved = false;
                    desktop->set_coordinate_status(event_dt);
                }
                drag_type = SP_DRAG_NONE;
                guide_item->ungrab();
                ret = true;
            }
            break;

        case GDK_ENTER_NOTIFY:
        {
            if (!guide->getLocked()) {
                guide_item->set_stroke(guide->getHiColor());
            }

            // set move or rotate cursor
            Geom::Point const event_w(event->crossing.x, event->crossing.y);

            auto display = desktop->getCanvas()->get_display();
            auto window  = desktop->getCanvas()->get_window();

            if (guide->getLocked()) {
                Inkscape::load_svg_cursor(display, window, "select.svg");
            } else if ((event->crossing.state & GDK_SHIFT_MASK) && (drag_type != SP_DRAG_MOVE_ORIGIN)) {
                Inkscape::load_svg_cursor(display, window, "rotate.svg");
            } else {
                auto guide_cursor = Gdk::Cursor::create(display, "grab");
                window->set_cursor(guide_cursor);
            }

            char *guide_description = guide->description();
            desktop->guidesMessageContext()->setF(Inkscape::NORMAL_MESSAGE, _("<b>Guideline</b>: %s"), guide_description);
            g_free(guide_description);
            break;
        }

        case GDK_LEAVE_NOTIFY:

            guide_item->set_stroke(guide->getColor());

            // restore event context's cursor
            desktop->getCanvas()->get_window()->set_cursor(desktop->event_context->cursor);

            desktop->guidesMessageContext()->clear();
            break;

        case GDK_KEY_PRESS:
            switch (Inkscape::UI::Tools::get_latin_keyval (&event->key)) {
                case GDK_KEY_Delete:
                case GDK_KEY_KP_Delete:
                case GDK_KEY_BackSpace:
                {
                    SPDocument *doc = guide->document;
                    if (!guide->getLocked()) {
                        sp_guide_remove(guide);
                        DocumentUndo::done(doc, SP_VERB_NONE, _("Delete guide"));
                        ret = true;
                        sp_event_context_discard_delayed_snap_event(desktop->event_context);
                        desktop->getCanvas()->get_window()->set_cursor(desktop->event_context->cursor);
                    }
                    break;
                }
                case GDK_KEY_Shift_L:
                case GDK_KEY_Shift_R:
                    if (drag_type != SP_DRAG_MOVE_ORIGIN) {

                        auto display = desktop->getCanvas()->get_display();
                        auto window  = desktop->getCanvas()->get_window();

                        Inkscape::load_svg_cursor(display, window, "rotate.svg");
                        ret = true;
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
                    auto display = Gdk::Display::get_default();
                    auto guide_cursor = Gdk::Cursor::create(display, "grab");
                    desktop->getCanvas()->get_window()->set_cursor(guide_cursor);
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
        for (auto const &dev : devices) {
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


// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_UI_EVENT_DEBUG_H
#define SEEN_UI_EVENT_DEBUG_H

/**
 * @file
 * Dump event data.
 */
/*
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtk/gtk.h>
#include <glibmm/ustring.h>

// See: https://developer.gnome.org/gdk3/stable/gdk3-Events.html

inline void ui_dump_event (GdkEvent *event, Glib::ustring const &prefix, bool merge = true) {

    static GdkEventType old_type = GDK_NOTHING;
    static unsigned count = 0;

    // Doesn't usually help to dump a zillion motion notify events.
    ++count;
    if (merge && event->type == old_type && event->type == GDK_MOTION_NOTIFY) {
        if ( count == 1 ) {
            std::cout << prefix << "  ... ditto" << std::endl;
        }
        return;
    }
    count = 0;
    old_type = event->type;

    std::cout << prefix << ": ";

    switch (event->type) {

        case GDK_KEY_PRESS:
            std::cout << "GDK_KEY_PRESS: " << std::hex
                      << " hardware: " << event->key.hardware_keycode
                      << " state: "    << event->key.state
                      << " keyval: "   << event->key.keyval << std::endl;
            break;
        case GDK_KEY_RELEASE:
            std::cout << "GDK_KEY_RELEASE: " << event->key.hardware_keycode << std::endl;
            break;

        case GDK_BUTTON_PRESS:
            std::cout << "GDK_BUTTON_PRESS: " << event->button.button << std::endl;
            break;
        case GDK_2BUTTON_PRESS:
            std::cout << "GDK_2BUTTON_PRESS: " << event->button.button << std::endl;
            break;
        case GDK_3BUTTON_PRESS:
            std::cout << "GDK_3BUTTON_PRESS: " << event->button.button << std::endl;
            break;
        case GDK_BUTTON_RELEASE:
            std::cout << "GDK_BUTTON_RELEASE: " << event->button.button << std::endl;
            break;

        case GDK_SCROLL:
            std::cout << "GDK_SCROLL" << std::endl;
            break;

        case GDK_MOTION_NOTIFY:
            std::cout << "GDK_MOTION_NOTIFY" << std::endl;
            break;
        case GDK_ENTER_NOTIFY:
            std::cout << "GDK_ENTER_NOTIFY" << std::endl;
            break;
        case GDK_LEAVE_NOTIFY:
            std::cout << "GDK_LEAVE_NOTIFY" << std::endl;
            break;

        case GDK_TOUCH_BEGIN:
            std::cout << "GDK_TOUCH_BEGIN" << std::endl;
            break;
        case GDK_TOUCH_UPDATE:
            std::cout << "GDK_TOUCH_UPDATE" << std::endl;
            break;
        case GDK_TOUCH_END:
            std::cout << "GDK_TOUCH_END" << std::endl;
            break;
        case GDK_TOUCH_CANCEL:
            std::cout  << "GDK_TOUCH_CANCEL" << std::endl;
            break;
        case GDK_TOUCHPAD_SWIPE:
            std::cout << "GDK_TOUCHPAD_SWIPE" << std::endl;
            break;
        case GDK_TOUCHPAD_PINCH:
            std::cout << "GDK_TOUCHPAD_PINCH" << std::endl;
            break;
        case GDK_PAD_BUTTON_PRESS:
            std::cout << "GDK_PAD_BUTTON_PRESS" << std::endl;
            break;
        case GDK_PAD_BUTTON_RELEASE:
            std::cout << "GDK_PAD_BUTTON_RELEASE" << std::endl;
            break;
        case GDK_PAD_RING:
            std::cout << "GDK_PAD_RING" << std::endl;
            break;
        case GDK_PAD_STRIP:
            std::cout << "GDK_PAD_STRIP" << std::endl;
            break;
        case GDK_PAD_GROUP_MODE:
            std::cout << "GDK_PAD_GROUP_MODE" << std::endl;
            break;
        default:
            std::cout << "GDK event not recognized!" << std::endl;
            break;
    }
}

#endif // SEEN_UI_EVENT_DEBUG_H

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

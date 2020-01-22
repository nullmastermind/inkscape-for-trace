// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for window handling tied to the application and with GUI.
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include <iostream>

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <gtkmm.h>
#include <glibmm/i18n.h>

#include "actions-window.h"
#include "actions-helper.h"
#include "inkscape-application.h"
#include "inkscape-window.h"

#include "inkscape.h"             // Inkscape::Application
#include "helper/action-context.h"

// Actions for window handling (should be integrated with file dialog).

class InkscapeWindow;

// Open a window for current document
void
window_open(InkscapeApplication *app)
{
    SPDocument *document = app->get_active_document();
    if (document) {
        InkscapeWindow* window = app->get_active_window();
        if (window && window->get_document() && window->get_document()->getVirgin()) {
            // We have a window with an untouched template document, use this window.
            app->document_swap (window, document);
        } else {
            app->window_open(document);
        }
    } else {
        std::cerr << "window_open(): failed to find document!" << std::endl;
    }
}

void
window_close(InkscapeApplication *app)
{
    app->window_close_active();
}

std::vector<std::vector<Glib::ustring>> raw_data_window =
{
    {"window-open",               "WindowOpen",              "Window",     N_("Open a window for the active document. GUI only.")   },
    {"window-close",              "WindowClose",             "Window",     N_("Close the active window.")                           }
};

template <class T>
void
add_actions_window(ConcreteInkscapeApplication<T>* app)
{
    Glib::VariantType Bool(  Glib::VARIANT_TYPE_BOOL);
    Glib::VariantType Int(   Glib::VARIANT_TYPE_INT32);
    Glib::VariantType Double(Glib::VARIANT_TYPE_DOUBLE);
    Glib::VariantType String(Glib::VARIANT_TYPE_STRING);
    Glib::VariantType BString(Glib::VARIANT_TYPE_BYTESTRING);

    // Debian 9 has 2.50.0
#if GLIB_CHECK_VERSION(2, 52, 0)

    app->add_action(                "window-open",  sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&window_open),         app));
    app->add_action(                "window-close", sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&window_close),        app));
#else
    std::cerr << "add_actions: Some actions require Glibmm 2.52, compiled with: " << glib_major_version << "." << glib_minor_version << std::endl;
#endif

    app->get_action_extra_data().add_data(raw_data_window);
}


template void add_actions_window(ConcreteInkscapeApplication<Gio::Application>* app);
template void add_actions_window(ConcreteInkscapeApplication<Gtk::Application>* app);



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

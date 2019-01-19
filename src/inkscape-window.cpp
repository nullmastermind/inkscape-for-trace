// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Inkscape - An SVG editor.
 */
/*
 * Authors:
 *   Tavmjong Bah
 *
 * Copyright (C) 2018 Authors
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 * Read the file 'COPYING' for more information.
 *
 */


#include "inkscape-window.h"
#include "inkscape.h"   // SP_ACTIVE_DESKTOP
#include "enums.h"      // PREFS_WINDOW_GEOMETRY_NONE
#include "shortcuts.h"

#include "object/sp-namedview.h"  // TODO Remove need for this!

#include "ui/drag-and-drop.h"  // Move to canvas?
#include "ui/monitor.h" // get_monitor_geometry_at_point()

#include "ui/drag-and-drop.h"

#include "widgets/desktop-widget.h"

InkscapeWindow::InkscapeWindow(SPDocument* document)
    : _document(document)
{
    if (!_document) {
        std::cerr << "InkscapeWindow::InkscapeWindow: null document!" << std::endl;
        return;
    }

    // Register window with application.
    Glib::RefPtr<Gio::Application> gio_app = Gio::Application::get_default();
    Glib::RefPtr<Gtk::Application> app = Glib::RefPtr<Gtk::Application>::cast_dynamic(gio_app);
    if (app) {
        set_application(app);  // Same as Gtk::Application::add_window()
    } else {
        std::cerr << "InkscapeWindow::InkscapeWindow:: Didn't get app!" << std::endl;
    }

    set_resizable(true);

    sp_ui_drag_setup(this);

     // =============== Build interface ===============

    // Main box
    _mainbox = Gtk::manage(new Gtk::Box);
    _mainbox->set_name("DesktopMainBox");
    _mainbox->show();
    add(*_mainbox);

    // Menu bar

    // Desktop widget (=> MultiPaned)
    SPViewWidget* _vw = sp_desktop_widget_new(sp_document_namedview(document, nullptr));
    _desktop_widget = reinterpret_cast<SPDesktopWidget*>(_vw);
    gtk_container_add(GTK_CONTAINER(_mainbox->gobj()), GTK_WIDGET(_desktop_widget));
    gtk_widget_show(GTK_WIDGET(_desktop_widget));
    _desktop = _desktop_widget->desktop;

    // Pallet

    // Status bar

    // ================== Callbacks ==================
    signal_key_press_event().connect(sigc::mem_fun(*this, &InkscapeWindow::key_press));

    // =================== Actions ===================


    // ============ Stuff to be cleaned up ===========

    g_object_set_data(G_OBJECT(_desktop_widget), "window", this);
    _desktop_widget->window = this;

    set_data("desktop", _desktop);
    set_data("desktopwidget", _desktop_widget);

    signal_delete_event().connect(      sigc::mem_fun(*_desktop, &SPDesktop::onDeleteUI));
    signal_window_state_event().connect(sigc::mem_fun(*_desktop, &SPDesktop::onWindowStateEvent));
    signal_focus_in_event().connect(sigc::mem_fun(*_desktop_widget, &SPDesktopWidget::onFocusInEvent));

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int window_geometry = prefs->getInt("/options/savewindowgeometry/value", PREFS_WINDOW_GEOMETRY_NONE);
    if (window_geometry == PREFS_WINDOW_GEOMETRY_LAST) {
        gint pw = prefs->getInt("/desktop/geometry/width", -1);
        gint ph = prefs->getInt("/desktop/geometry/height", -1);
        gint px = prefs->getInt("/desktop/geometry/x", -1);
        gint py = prefs->getInt("/desktop/geometry/y", -1);
        gint full = prefs->getBool("/desktop/geometry/fullscreen");
        gint maxed = prefs->getBool("/desktop/geometry/maximized");
        if (pw>0 && ph>0) {
            Gdk::Rectangle monitor_geometry = Inkscape::UI::get_monitor_geometry_at_point(px, py);
            pw = std::min(pw, monitor_geometry.get_width());
            ph = std::min(ph, monitor_geometry.get_height());
            _desktop->setWindowSize(pw, ph);
            _desktop->setWindowPosition(Geom::Point(px, py));
        }
        if (maxed) {
            maximize();
        }
        if (full) {
            fullscreen();
        }
    }

    // ================ Window Options ==============

    show(); // Must show before resize!

    // Resize the window to match the document properties
    sp_namedview_window_from_document(_desktop);
    sp_namedview_update_layers_from_document(_desktop);

}

bool
InkscapeWindow::key_press(GdkEventKey* event)
{
    unsigned shortcut = sp_shortcut_get_for_event(event);
    return sp_shortcut_invoke (shortcut, _desktop);
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

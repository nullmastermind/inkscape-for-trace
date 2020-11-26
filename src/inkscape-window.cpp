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
#include "desktop-events.h" // Handle key events
#include "enums.h"      // PREFS_WINDOW_GEOMETRY_NONE

#include "inkscape-application.h"

#include "actions/actions-canvas-mode.h"
#include "actions/actions-canvas-transform.h"

#include "object/sp-namedview.h"  // TODO Remove need for this!

#include "ui/dialog/dialog-container.h"
#include "ui/dialog/dialog-window.h"
#include "ui/drag-and-drop.h"  // Move to canvas?
#include "ui/interface.h" // main menu, sp_ui_close_view()

#include "ui/monitor.h" // get_monitor_geometry_at_point()

#include "ui/desktop/menubar.h"

#include "ui/drag-and-drop.h"

#include "ui/event-debug.h"
#include "ui/shortcuts.h"

#include "widgets/desktop-widget.h"

using Inkscape::UI::Dialog::DialogContainer;
using Inkscape::UI::Dialog::DialogWindow;

InkscapeWindow::InkscapeWindow(SPDocument* document)
    : _document(document)
    , _app(nullptr)
{
    if (!_document) {
        std::cerr << "InkscapeWindow::InkscapeWindow: null document!" << std::endl;
        return;
    }

    _app = InkscapeApplication::instance();
    _app->gtk_app()->add_window(*this);

    set_resizable(true);

    insert_action_group("doc", document->getActionGroup());

    // =============== Build interface ===============

    // Main box
    _mainbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    _mainbox->set_name("DesktopMainBox");
    _mainbox->show();
    add(*_mainbox);

    // Desktop widget (=> MultiPaned)
    _desktop_widget = new SPDesktopWidget(_document);
    _desktop_widget->window = this;
    _desktop_widget->show();
    _desktop = _desktop_widget->desktop;

    // =================== Actions ===================
    // After canvas has been constructed.. move to canvas proper.
    add_actions_canvas_transform(this);    // Actions to transform canvas view.
    add_actions_canvas_mode(this);         // Actions to change canvas display mode.

    // ========== Drag and Drop of Documents =========
    ink_drag_setup(_desktop_widget);

    // Menu bar (must come after desktop widget creation as we need _desktop)
    // _menubar = build_menubar(_desktop);
    // _menubar->set_name("MenuBar");
    // _menubar->show_all();

    // Pallet

    // Status bar

    // _mainbox->pack_start(*_menubar, false, false);
    _mainbox->pack_start(*Gtk::manage(_desktop_widget), true, true);

    // ================== Callbacks ==================
    signal_delete_event().connect(      sigc::mem_fun(*_desktop, &SPDesktop::onDeleteUI));
    signal_window_state_event().connect(sigc::mem_fun(*_desktop, &SPDesktop::onWindowStateEvent));
    signal_focus_in_event().connect(    sigc::mem_fun(*_desktop_widget, &SPDesktopWidget::onFocusInEvent));


    // ================ Window Options ===============
    setup_view();

    // ========= Update text for Accellerators =======
    Inkscape::Shortcuts::getInstance().update_gui_text_recursive(this);
}

// Change a document, leaving desktop/view the same. (Eventually move all code here.)
void
InkscapeWindow::change_document(SPDocument* document)
{
    if (!_app) {
        std::cerr << "Inkscapewindow::change_document: app is nullptr!" << std::endl;
        return;
    }

    _document = document;
    _app->set_active_document(_document);

    setup_view();
    update_dialogs();
}

// Sets up the window and view according to user preferences and <namedview> of the just loaded document
void
InkscapeWindow::setup_view()
{
    // Make sure the GdkWindow is fully initialized before resizing/moving
    // (ensures the monitor it'll be shown on is known)
    realize();

    // Resize the window to match the document properties
    sp_namedview_window_from_document(_desktop); // This should probably be a member function here.

    // Must show before setting zoom and view! (crashes otherwise)
    //
    // Showing after resizing/moving allows the window manager to correct an invalid size/position of the window
    // TODO: This does *not* work when called from 'change_document()', i.e. when the window is already visible.
    //       This can result in off-screen windows! We previously worked around this by hiding and re-showing
    //       the window, but a call to hide() causes Inkscape to just exit since the migration to Gtk::Application
    show();
    
    // Show dialogs after the main window, otherwise dialogs may be associated as the main window of the program.
    _desktop->getContainer()->load_container_state();
    
    sp_namedview_zoom_and_view_from_document(_desktop);
    sp_namedview_update_layers_from_document(_desktop);

    SPNamedView *nv = _desktop->namedview;
    if (nv && nv->lockguides) {
        nv->lockGuides();
    }
}

/**
 * Return true if this is the Cmd-Q shortcut on macOS
 */
inline bool is_Cmd_Q(GdkEventKey *event)
{
#ifdef GDK_WINDOWING_QUARTZ
    return (event->keyval == 'q' && event->state == (GDK_MOD2_MASK | GDK_META_MASK));
#else
    return false;
#endif
}

bool
InkscapeWindow::on_key_press_event(GdkEventKey* event)
{
#ifdef EVENT_DEBUG
    ui_dump_event(reinterpret_cast<GdkEvent *>(event), "\nInkscapeWindow::on_key_press_event");
#endif

    // Key press and release events are normally sent first to Gtk::Window for processing as
    // accelerators and menomics before bubbling up from the "grab" or "focus" widget (unlike other
    // events which always bubble up). This would means that key combinations used for accelerators
    // won't reach the focus widget (and our tool event handlers). As we use single keys for
    // accelerators, we wouldn't even be able to type text! We can get around this by sending key
    // events first to the focus widget.
    //
    // See https://developer.gnome.org/gtk3/stable/chap-input-handling.html (Event Propogation)

    auto focus = get_focus();
    if (focus) {
        if (focus->event(reinterpret_cast<GdkEvent *>(event))) {
            return true;
        }
    }

    // Intercept Cmd-Q on macOS to not bypass confirmation dialog
    if (!is_Cmd_Q(event) && Gtk::Window::on_key_press_event(event)) {
        return true;
    }

    // Verbs get last crack at events.
    return Inkscape::Shortcuts::getInstance().invoke_verb(event, _desktop);
}

/**
 * If "dialogs on top" is activated in the preferences, set `parent` as the
 * new transient parent for all DialogWindow windows of the application.
 */
static void retransientize_dialogs(Gtk::Window &parent)
{
    assert(!dynamic_cast<DialogWindow *>(&parent));

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool window_above =
        prefs->getInt("/options/transientpolicy/value", PREFS_DIALOGS_WINDOWS_NORMAL) != PREFS_DIALOGS_WINDOWS_NONE;

    for (auto const &window : parent.get_application()->get_windows()) {
        if (auto dialog_window = dynamic_cast<DialogWindow *>(window)) {
            if (window_above) {
                dialog_window->set_transient_for(parent);
            } else {
                dialog_window->unset_transient_for();
            }
        }
    }
}

bool
InkscapeWindow::on_focus_in_event(GdkEventFocus* event)
{
    if (_app) {
        _app->set_active_window(this);
        _app->set_active_document(_document);
        _app->set_active_view(_desktop);
        _app->set_active_selection(_desktop->selection);
        _app->windows_update(_document);
        update_dialogs();
        retransientize_dialogs(*this);
    } else {
        std::cerr << "Inkscapewindow::on_focus_in_event: app is nullptr!" << std::endl;
    }

    return Gtk::ApplicationWindow::on_focus_in_event(event);
}

// Called when a window is closed via the 'X' in the window bar.
bool
InkscapeWindow::on_delete_event(GdkEventAny* event)
{
    if (_app) {
        _app->destroy_window(this);
    }
    return true;
};

void InkscapeWindow::on_selection_changed()
{
    if (_app) {
        _app->set_active_selection(_desktop->selection);
        update_dialogs();
    }
}

void InkscapeWindow::update_dialogs()
{
    std::vector<Gtk::Window *> windows = _app->gtk_app()->get_windows();
    for (auto const &window : windows) {
        DialogWindow *dialog_window = dynamic_cast<DialogWindow *>(window);
        if (dialog_window) {
            dialog_window->update_dialogs();
        }

        _desktop_widget->getContainer()->update_dialogs();
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

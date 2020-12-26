// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Author:
 *   Tavmjong Bah
 *
 * Rewrite of code originally in desktop-widget.cpp.
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


// The scrollbars, and canvas are tightly coupled so it makes sense to have a dedicated
// widget to handle their interactions. The buttons are along for the ride. I don't see
// how to add the buttons easily via a .ui file (which would allow the user to put any
// buttons they want in their place).

#include <glibmm/i18n.h>
#include <gtkmm/enums.h>
#include <gtkmm/label.h>

#include "canvas-grid.h"

#include "desktop.h"                 // Hopefully temp.
#include "desktop-events.h"          // Hopefully temp.

#include "display/control/canvas-item-drawing.h" // sticky

#include "ui/icon-loader.h"
#include "ui/widget/canvas.h"
#include "ui/widget/ink-ruler.h"

#include "widgets/desktop-widget.h"  // Hopefully temp.


namespace Inkscape {
namespace UI {
namespace Widget {

CanvasGrid::CanvasGrid(SPDesktopWidget *dtw)
{
    _dtw = dtw;
    set_name("CanvasGrid");

    // Canvas
    _canvas = Gtk::manage(new Inkscape::UI::Widget::Canvas());
    _canvas->set_hexpand(true);
    _canvas->set_vexpand(true);
    _canvas->set_can_focus(true);
    _canvas->signal_event().connect(sigc::mem_fun(*this, &CanvasGrid::SignalEvent)); // TEMP

    _canvas_overlay.add(*_canvas);
    _canvas_overlay.add_overlay(*_command_palette.get_base_widget());

    // Horizontal Scrollbar
    _hadj = Gtk::Adjustment::create(0.0, -4000.0, 4000.0, 10.0, 100.0, 4.0);
    _hadj->signal_value_changed().connect(sigc::mem_fun(_dtw, &SPDesktopWidget::on_adjustment_value_changed));
    _hscrollbar = Gtk::manage(new Gtk::Scrollbar(_hadj, Gtk::ORIENTATION_HORIZONTAL));
    _hscrollbar->set_name("CanvasScrollbar");
    _hscrollbar->set_hexpand(true);
    _hscrollbar->set_no_show_all();

    // Vertical Scrollbar
    _vadj = Gtk::Adjustment::create(0.0, -4000.0, 4000.0, 10.0, 100.0, 4.0);
    _vadj->signal_value_changed().connect(sigc::mem_fun(_dtw, &SPDesktopWidget::on_adjustment_value_changed));
    _vscrollbar = Gtk::manage(new Gtk::Scrollbar(_vadj, Gtk::ORIENTATION_VERTICAL));
    _vscrollbar->set_name("CanvasScrollbar");
    _vscrollbar->set_vexpand(true);
    _vscrollbar->set_no_show_all();

    // Horizontal Ruler
    _hruler = Gtk::manage(new Inkscape::UI::Widget::Ruler(Gtk::ORIENTATION_HORIZONTAL));
    _hruler->add_track_widget(*_canvas);
    _hruler->set_hexpand(true);
    // Tooltip/Unit set elsewhere.

    // For creating guides, etc.
    _hruler->signal_button_press_event().connect(
        sigc::bind(sigc::mem_fun(*_dtw, &SPDesktopWidget::on_ruler_box_button_press_event),   _hruler, true));
    _hruler->signal_button_release_event().connect(
        sigc::bind(sigc::mem_fun(*_dtw, &SPDesktopWidget::on_ruler_box_button_release_event), _hruler, true));
    _hruler->signal_motion_notify_event().connect(
        sigc::bind(sigc::mem_fun(*_dtw, &SPDesktopWidget::on_ruler_box_motion_notify_event),  _hruler, true));

    // Vertical Ruler
    _vruler = Gtk::manage(new Inkscape::UI::Widget::Ruler(Gtk::ORIENTATION_VERTICAL));
    _vruler->add_track_widget(*_canvas);
    _vruler->set_vexpand(true);
    // Tooltip/Unit set elsewhere.

    // For creating guides, etc.
    _vruler->signal_button_press_event().connect(
        sigc::bind(sigc::mem_fun(*_dtw, &SPDesktopWidget::on_ruler_box_button_press_event),   _vruler, false));
    _vruler->signal_button_release_event().connect(
        sigc::bind(sigc::mem_fun(*_dtw, &SPDesktopWidget::on_ruler_box_button_release_event), _vruler, false));
    _vruler->signal_motion_notify_event().connect(
        sigc::bind(sigc::mem_fun(*_dtw, &SPDesktopWidget::on_ruler_box_motion_notify_event),  _vruler, false));

    // Guide Lock
    auto image1 = Gtk::manage(new Gtk::Image("object-locked", Gtk::ICON_SIZE_MENU));
    _guide_lock = Gtk::manage(new Gtk::ToggleButton());
    _guide_lock->set_name("LockGuides");
    _guide_lock->add(*image1);
    _guide_lock->set_no_show_all();
    // To be replaced by Gio::Action:
    _guide_lock->signal_toggled().connect(sigc::mem_fun(_dtw, &SPDesktopWidget::update_guides_lock));
    _guide_lock->set_tooltip_text(_("Toggle lock of all guides in the document"));

    // CMS Adjust
    auto image2 = Gtk::manage(new Gtk::Image("color-management", Gtk::ICON_SIZE_MENU));
    _cms_adjust = Gtk::manage(new Gtk::ToggleButton());
    _cms_adjust->set_name("CMS_Adjust");
    _cms_adjust->add(*image2);
    // Can't access via C++ API, fixed in Gtk4.
    gtk_actionable_set_action_name( GTK_ACTIONABLE(_cms_adjust->gobj()), "win.canvas-color-manage");
    _cms_adjust->set_tooltip_text(_("Toggle color-managed display for this document window"));
    _cms_adjust->set_no_show_all();

    // Sticky Zoom
    auto image3 = Gtk::manage(sp_get_icon_image("zoom-original", Gtk::ICON_SIZE_MENU));
    _sticky_zoom = Gtk::manage(new Gtk::ToggleButton());
    _sticky_zoom->set_name("StickyZoom");
    _sticky_zoom->add(*image3);
    // To be replaced by Gio::Action:
    _sticky_zoom->signal_toggled().connect(sigc::mem_fun(_dtw, &SPDesktopWidget::sticky_zoom_toggled));
    _sticky_zoom->set_tooltip_text(_("Zoom drawing if window size changes"));
    _sticky_zoom->set_no_show_all();

    // Top row
    attach(*_guide_lock,    0, 0, 1, 1);
    attach(*_hruler,        1, 0, 1, 1);
    attach(*_sticky_zoom,   2, 0, 1, 1);

    // Middle row
    attach(*_vruler,        0, 1, 1, 1);
    attach(_canvas_overlay, 1, 1, 1, 1);
    attach(*_vscrollbar,    2, 1, 1, 1);

    // Bottom row
    attach(*_hscrollbar,    1, 2, 1, 1);
    attach(*_cms_adjust,    2, 2, 1, 1);

    // Update rulers on size change.
    signal_size_allocate().connect(sigc::mem_fun(*this, &CanvasGrid::OnSizeAllocate));

    show_all();
}

// _dt2r should be a member of _canvas.
// get_display_area should be a member of _canvas.
void
CanvasGrid::UpdateRulers()
{
    Geom::Rect viewbox = _dtw->desktop->get_display_area(true).bounds();
    // "true" means: Use integer values of the canvas for calculating the display area, similar
    // to the integer values used for positioning the grid lines. (see SPCanvas::scrollTo(), 
    // where ix and iy are rounded integer values; these values are stored in CanvasItemBuffer->rect,
    // and used for drawing the grid). By using the integer values here too, the ruler ticks 
    // will be perfectly aligned to the grid
    double _dt2r = _dtw->_dt2r;
    Geom::Point _ruler_origin = _dtw->_ruler_origin;

    double lower_x = _dt2r * (viewbox.left()  - _ruler_origin[Geom::X]);
    double upper_x = _dt2r * (viewbox.right() - _ruler_origin[Geom::X]);
    _hruler->set_range(lower_x, upper_x);

    double lower_y = _dt2r * (viewbox.bottom() - _ruler_origin[Geom::Y]);
    double upper_y = _dt2r * (viewbox.top()    - _ruler_origin[Geom::Y]);
    if (_dtw->desktop->is_yaxisdown()) {
        std::swap(lower_y, upper_y);
    }
    _vruler->set_range(lower_y, upper_y);
}

void
CanvasGrid::ShowScrollbars(bool state)
{
    _show_scrollbars = state;

    if (_show_scrollbars) {
        // Show scrollbars
        _hscrollbar->show();
        _vscrollbar->show();
        _cms_adjust->show();
        _cms_adjust->show_all_children();
        _sticky_zoom->show();
        _sticky_zoom->show_all_children();
    } else {
        // Hide scrollbars
        _hscrollbar->hide();
        _vscrollbar->hide();
        _cms_adjust->hide();
        _sticky_zoom->hide();
    }
}

void
CanvasGrid::ToggleScrollbars()
{
    _show_scrollbars = !_show_scrollbars;
    ShowScrollbars(_show_scrollbars);

    // Will be replaced by actions
    auto prefs = Inkscape::Preferences::get();
    prefs->setBool("/fullscreen/scrollbars/state", _show_scrollbars);
    prefs->setBool("/window/scrollbars/state", _show_scrollbars);
}

void
CanvasGrid::ShowRulers(bool state)
{
    // Sticky zoom button is always shown. We must adjust canvas when rulers are toggled or canvas
    // won't expand fully.
    _show_rulers = state;

    if (_show_rulers) {
        // Show rulers
        _hruler->show();
        _vruler->show();
        _guide_lock->show();
        _guide_lock->show_all_children();
        remove(_canvas_overlay);
        attach(_canvas_overlay, 1, 1, 1, 1);
    } else {
        // Hide rulers
        _hruler->hide();
        _vruler->hide();
        _guide_lock->hide();
        remove(_canvas_overlay);
        attach(_canvas_overlay, 1, 0, 1, 2);
    }
}

void
CanvasGrid::ToggleRulers()
{
    _show_rulers = !_show_rulers;
    ShowRulers(_show_rulers);

    // Will be replaced by actions
    auto prefs = Inkscape::Preferences::get();
    prefs->setBool("/fullscreen/rulers/state", _show_rulers);
    prefs->setBool("/window/rulers/state", _show_rulers);
}

void
CanvasGrid::ToggleCommandPalette() {
    _command_palette.toggle();
}

void
CanvasGrid::ShowCommandPalette(bool state)
{
    if (state) {
        _command_palette.open();
    }
    _command_palette.close();
}

// Update rulers on change of widget size, but only if allocation really changed.
void
CanvasGrid::OnSizeAllocate(Gtk::Allocation& allocation)
{
    if (!(_allocation == allocation)) { // No != function defined!
        _allocation = allocation;
        UpdateRulers();
    }
}

// This belong in Canvas class
bool
CanvasGrid::SignalEvent(GdkEvent *event)
{
    if (event->type == GDK_BUTTON_PRESS) {
        _canvas->grab_focus();
        _command_palette.close();
    }

    if (event->type == GDK_BUTTON_PRESS && event->button.button == 3) {
        if (event->button.state & GDK_SHIFT_MASK) {
            _dtw->desktop->getCanvasDrawing()->set_sticky(true);
        } else {
            _dtw->desktop->getCanvasDrawing()->set_sticky(false);
        }
    }

    {
        // The key press/release events need to be passed to desktop handler explicitly,
        // because otherwise the event contexts only receive key events when the mouse cursor
        // is over the canvas. This redirection is only done for key events and only if there's no
        // current item on the canvas, because item events and all mouse events are caught
        // and passed on by the canvas acetate (I think). --bb

        if ((event->type == GDK_KEY_PRESS || event->type == GDK_KEY_RELEASE)
            && !_canvas->get_current_canvas_item()) {
            return sp_desktop_root_handler (event, _dtw->desktop);
        }
    }
    return false;
}

// TODO Add actions so we can set shortcuts.
// * Sticky Zoom
// * CMS Adjust
// * Guide Lock


} // namespace Widget
} // namespace UI
} // namespace Inkscape

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

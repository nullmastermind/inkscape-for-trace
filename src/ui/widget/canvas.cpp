// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Author:
 *   Tavmjong Bah
 *
 * Rewrite of code originally in sp-canvas.cpp.
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <iostream>

#include <glibmm/i18n.h>

#include <2geom/rect.h>

#include "canvas.h"
#include "canvas-grid.h"

#include "color.h"          // Background color
#include "cms-system.h"     // Color correction
#include "inkscape.h"       // SP_ACTIVE_DESKTOP TEMP TEMP TEMP
#include "preferences.h"

#include "display/sp-canvas-item.h"  // Canvas group TEMP TEMP TEMP
#include "display/sp-canvas-group.h" // Canvas group TEMP TEMP TEMP
#include "display/canvas-arena.h"    // Change rendering mode
#include "display/cairo-utils.h"     // Checkerboard background.

/**
 * Helper function to update item and its children.
 *
 * NB! affine is parent2canvas.
 */
static void sp_canvas_item_invoke_update2(SPCanvasItem *item, Geom::Affine const &affine, unsigned int flags)
{
    // Apply the child item's transform
    Geom::Affine child_affine = item->xform * affine;

    // apply object flags to child flags
    int child_flags = flags & ~SP_CANVAS_UPDATE_REQUESTED;

    if (item->need_update) {
        child_flags |= SP_CANVAS_UPDATE_REQUESTED;
    }

    if (item->need_affine) {
        child_flags |= SP_CANVAS_UPDATE_AFFINE;
    }

    if (child_flags & (SP_CANVAS_UPDATE_REQUESTED | SP_CANVAS_UPDATE_AFFINE)) {
        if (SP_CANVAS_ITEM_GET_CLASS (item)->update) {
            SP_CANVAS_ITEM_GET_CLASS (item)->update(item, child_affine, child_flags);
        }
    }

    item->need_update = FALSE;
    item->need_affine = FALSE;
}

struct PaintRectSetup {
    Geom::IntRect canvas_rect;
    gint64 start_time;
    int max_pixels;
    Geom::Point mouse_loc;
};

namespace Inkscape {
namespace UI {
namespace Widget {


Canvas::Canvas()
{
  // Events
  add_events(Gdk::BUTTON_PRESS_MASK     |
             Gdk::BUTTON_RELEASE_MASK   |
             Gdk::ENTER_NOTIFY_MASK     |
             Gdk::LEAVE_NOTIFY_MASK     |
             Gdk::FOCUS_CHANGE_MASK     |
             Gdk::KEY_PRESS_MASK        |
             Gdk::KEY_RELEASE_MASK      |
             Gdk::POINTER_MOTION_MASK   |
             Gdk::SCROLL_MASK           |
             Gdk::SMOOTH_SCROLL_MASK    );

  // Give _pick_event an initial definition.
  _pick_event.type = GDK_LEAVE_NOTIFY;
  _pick_event.crossing.x = 0;
  _pick_event.crossing.y = 0;

  // Drawing
  _clean_region = Cairo::Region::create();

  _background = Cairo::SolidPattern::create_rgb(1.0, 1.0, 1.0);

  _root = SP_CANVAS_ITEM(g_object_new(SP_TYPE_CANVAS_GROUP, nullptr));
  SP_CANVAS_ITEM(_root)->canvas = this;
}

Canvas::~Canvas()
{
    _in_destruction = true;

    remove_idle();
}

/**
 * Is world point inside of canvas area?
 */
bool
Canvas::world_point_inside_canvas(Geom::Point const &world)
{
    Gtk::Allocation allocation = get_allocation();
    return ( (_x0 <= world.x()) && (world.x() < _x0 + allocation.get_width()) &&
             (_y0 <= world.y()) && (world.y() < _y0 + allocation.get_height()) );
}

/**
 * Translate point in canvas to world coordinates.
 */
Geom::Point
Canvas::canvas_to_world(Geom::Point const &point)
{
    return Geom::Point(point[Geom::X]+ _x0, point[Geom::Y] + _y0);
}

/**
 * Return the area shown in the canvas in world coordinates.
 */
Geom::Rect
Canvas::get_area_world()
{
    // This is called by desktop.cpp before Canvas::on_draw() is called (and on_realize()) so
    // we don't rely on stored allocation value.
    Gtk::Allocation allocation = get_allocation();
    return Geom::Rect::from_xywh(_x0, _y0, allocation.get_width(), allocation.get_height());
}

/**
 * Return the area showin the canvas in world coordinates, rounded to integer values.
 */
Geom::IntRect
Canvas::get_area_world_int()
{
    Gtk::Allocation allocation = get_allocation();
    return Geom::IntRect::from_xywh(_x0, _y0, allocation.get_width(), allocation.get_height());
}

/**
 * Invalidate drawing and redraw during idle.
 */
void
Canvas::redraw_all()
{
    if (_in_destruction) {
        std::cerr << "Canvas::redraw_all: Called after canvas destroyed!" << std::endl;
        return;
    }
    _clean_region->intersect(Cairo::Region::create()); // Empty region (i.e. everything is dirty).
    add_idle();
}

/**
 * Redraw the given area during idle.
 */
void
Canvas::redraw_area(int x0, int y0, int x1, int y1)
{
    if (_in_destruction) {
        std::cerr << "Canvas::redraw_area: Called after canvas destroyed!" << std::endl;
        return;
    }

    if (x0 >= x1 || y0 >= y1) {
        return;
    }

    Cairo::RectangleInt crect = { x0, y0, x1-x0, y1-y0 };
    _clean_region->subtract(crect);
    add_idle();
}

/**
 * Immediate redraw of areas needing update.
 */
void
Canvas::redraw_now()
{
    if (_need_update) {
        do_update();
    }
}

/**
 * Redraw after updating canvas items.
 */
void
Canvas::request_update()
{
    _need_update = true;
    add_idle();
}

/**
 * This is the first function called (after constructor).
 * Scroll window so drawing point 'c' is at upper left corner of canvas.
 * Complete redraw if 'clear' is true.
 */
void
Canvas::scroll_to(Geom::Point const &c, bool clear)
{
    int old_x0 = _x0;
    int old_y0 = _y0;

    // This is the only place the _x0 and _y0 are set!
    _x0 = (int) round(c[Geom::X]); // cx might be negative, so (int)(cx + 0.5) will not do!
    _y0 = (int) round(c[Geom::Y]);
    _window_origin = c; // Double value

    if (!_backing_store) {
        // We haven't drawn anything yet!
        return;
    }

    int dx = _x0 - old_x0;
    int dy = _y0 - old_y0;

    if (dx == 0 && dy == 0) {
        return; // No scroll... do nothing.
    }

    // See if there is any overlap between canvas before and after scrolling.
    Geom::IntRect old_area = Geom::IntRect::from_xywh(old_x0, old_y0, _allocation.get_width(), _allocation.get_height());
    Geom::IntRect new_area = old_area + Geom::IntPoint(dx, dy);
    bool overlap = new_area.intersects(old_area);

    if (clear || !overlap) {
        redraw_all();
        return; // Check if this is OK
    }

    // Copy backing store
    shift_content(Geom::IntPoint(dx, dy), _backing_store);
    if (_split_mode != Inkscape::SPLITMODE_NORMAL) {
        shift_content(Geom::IntPoint(dx, dy), _outline_store);
    }

    // Mark surface to redraw (everything outside clean region).
    Cairo::RectangleInt crect = { _x0, _y0, _allocation.get_width(), _allocation.get_height() };
    _clean_region->intersect(crect); // Shouldn't the clean region be reset and then this added?

    // Scroll without zoom: redraw only newly exposed areas.
    if (get_realized()) {
        auto window = get_window();
        window->scroll(-dx, -dy); // Triggers of newly exposed region.
    }

    auto grid = dynamic_cast<Inkscape::UI::Widget::CanvasGrid *>(get_parent());
    if (grid) {
        grid->UpdateRulers();
    }
}

/**
 * Return the root canvas group.
 */
SPCanvasGroup*
Canvas::get_canvas_item_root()
{
    return SP_CANVAS_GROUP(_root);
}

/**
 * Set canvas backbround color (display only).
 */
void
Canvas::set_background_color(guint32 rgba)
{
    double r = SP_RGBA32_R_F(rgba);
    double g = SP_RGBA32_G_F(rgba);
    double b = SP_RGBA32_B_F(rgba);
    _background = Cairo::SolidPattern::create_rgb(r, g, b);
    _background_is_checkerboard = false;
    redraw_all();
}

/**
 * Set canvas background to a checkerboard pattern.
 */
void
Canvas::set_background_checkerboard(guint32 rgba)
{
    auto pattern = ink_cairo_pattern_create_checkerboard(rgba);
    _background = Cairo::RefPtr<Cairo::Pattern>(new Cairo::Pattern(pattern));
    _background_is_checkerboard = true;
    redraw_all();
}

void
Canvas::set_render_mode(Inkscape::RenderMode mode)
{
    if (_render_mode != mode) {
        _render_mode = mode;
        redraw_all();
    }
}

void
Canvas::set_color_mode(Inkscape::ColorMode mode)
{
    if (_color_mode != mode) {
        _color_mode = mode;
        redraw_all();
    }
}

void
Canvas::set_split_mode(Inkscape::SplitMode mode)
{
    if (_split_mode != mode) {
        _split_mode = mode;
        redraw_all();
    }
}

void
Canvas::set_split_direction(Inkscape::SplitDirection dir)
{
    if (_split_direction != dir) {
        _split_direction = dir;
        redraw_all();
    }
}

void
Canvas::forced_redraws_start(int count, bool reset)
{
    _forced_redraw_limit = count;
    if (reset) {
        _forced_redraw_count = 0;
    }
}

/**
 * Clear current, grabbed, and focused items.
 */
void
Canvas::canvas_item_clear(SPCanvasItem *item)
{
    if (item == _current_item) {
        _current_item = nullptr;
        _need_repick = true;
    }

    if (item == _current_item_new) {
        _current_item_new = nullptr;
        _need_repick = true;
    }

    if (item == _grabbed_item) {
        _grabbed_item = nullptr;
        auto const display = Gdk::Display::get_default();
        auto const seat    = display->get_default_seat();
        seat->ungrab();
    }

    if (item == _focused_item) {
        _focused_item = nullptr;
    }

}

// ============== Protected Functions ==============

void
Canvas::get_preferred_width_vfunc (int& minimum_width,  int& natural_width) const
{
    minimum_width = natural_width = 256;
} 	

void
Canvas::get_preferred_height_vfunc (int& minimum_height, int& natural_height) const
{
    minimum_height = natural_height = 256;
}

// ******* Event handlers ******
bool
Canvas::on_scroll_event(GdkEventScroll *scroll_event)
{
    // Scroll canvas and in Select Tool, cycle selection through objets under cursor.
    return emit_event(reinterpret_cast<GdkEvent *>(scroll_event));
}

// Our own function that combines press and release.
bool
Canvas::on_button_event(GdkEventButton *button_event)
{
    // Dispatch normally regardless of the event's window if an item
    // has a pointer grab in effect.
    auto window = get_window();
    if (!_grabbed_item && window->gobj() != button_event->window) {
        return false;
    }

    int mask = 0;
    switch (button_event->button) {
        case 1:  mask = GDK_BUTTON1_MASK; break;
        case 2:  mask = GDK_BUTTON1_MASK; break;
        case 3:  mask = GDK_BUTTON1_MASK; break;
        case 4:  mask = GDK_BUTTON1_MASK; break;
        case 5:  mask = GDK_BUTTON1_MASK; break;
        default: mask = 0;  // Buttons can range at least to 9 but mask defined only to 5.
    }

    bool retval = false;
    switch (button_event->type) {
        case GDK_BUTTON_PRESS:
        case GDK_2BUTTON_PRESS:
        case GDK_3BUTTON_PRESS:
            // Pick the current item as if the button were not press and then process event.
            _state = button_event->state;
            pick_current_item(reinterpret_cast<GdkEvent *>(button_event));
            _state ^= mask;
            retval = emit_event(reinterpret_cast<GdkEvent *>(button_event));
            break;

        case GDK_BUTTON_RELEASE:
            // Process the event as if the button were pressed, then repick after the button has
            // been released.
            _state = button_event->state;
            retval = emit_event(reinterpret_cast<GdkEvent *>(button_event));
            button_event->state ^= mask;
            _state = button_event->state;
            pick_current_item(reinterpret_cast<GdkEvent *>(button_event));
            button_event->state ^= mask;
            break;

        default:
            std::cerr << "Canvas::on_button_event: illegal event type!" << std::endl;
    }

    return retval;
}

bool
Canvas::on_button_press_event(GdkEventButton *button_event)
{
    return on_button_event(button_event);
}

bool
Canvas::on_button_release_event(GdkEventButton *button_event)
{
    return on_button_event(button_event);
}

bool
Canvas::on_enter_notify_event(GdkEventCrossing *crossing_event)
{
    auto window = get_window();
    if (window->gobj() != crossing_event->window) {
        std::cout << "  WHOOPS... this does really happen" << std::endl;
        return false;
    }
    _state = crossing_event->state;
    return pick_current_item(reinterpret_cast<GdkEvent *>(crossing_event));
}

bool
Canvas::on_leave_notify_event(GdkEventCrossing *crossing_event)
{
    auto window = get_window();
    if (window->gobj() != crossing_event->window) {
        std::cout << "  WHOOPS... this does really happen" << std::endl;
        return false;
    }
    _state = crossing_event->state;
    return pick_current_item(reinterpret_cast<GdkEvent *>(crossing_event));
}

bool
Canvas::on_focus_in_event(GdkEventFocus *focus_event)
{
    grab_focus();
    if  (_focused_item) {
        return emit_event(reinterpret_cast<GdkEvent *>(focus_event));
    }
    return false;
}

bool
Canvas::on_focus_out_event(GdkEventFocus *focus_event)
{
    if  (_focused_item) {
        return emit_event(reinterpret_cast<GdkEvent *>(focus_event));
    }
    return false;
}

// Actually, key events never reach here.
bool
Canvas::on_key_press_event(GdkEventKey *key_event)
{
    return emit_event(reinterpret_cast<GdkEvent *>(key_event));
}

// Actually, key events never reach here.
bool
Canvas::on_key_release_event(GdkEventKey *key_event)
{
    return emit_event(reinterpret_cast<GdkEvent *>(key_event));
}

bool
Canvas::on_motion_notify_event(GdkEventMotion *motion_event)
{
    _state = motion_event->state;
    pick_current_item(reinterpret_cast<GdkEvent *>(motion_event));
    bool status = emit_event(reinterpret_cast<GdkEvent *>(motion_event));
    return status;
}


bool
Canvas::on_draw(const::Cairo::RefPtr<::Cairo::Context>& cr)
{
    // This function should be the only place _allocation is redefined (except in on_realize())!
    Gtk::Allocation allocation = get_allocation();
    int device_scale = get_scale_factor();

    // This is the only place we should initialize _backing_store! (Elsewhere, it's recreated.)
    if (!_backing_store || !_outline_store) {
        _allocation = allocation;
        _device_scale = device_scale;

        // Gdk::Window::create_similar_image_surface() creates a Cairo::Surface of type SURFACE_IMAGE_TYPE.
        // This is not the same as a Cairo::ImageSurface.. at least it can't be cast to it. So we can't use
        // that handy function (it sets device_scale automatically).
        _backing_store =
            Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32,
                                        _allocation.get_width()  * _device_scale,
                                        _allocation.get_height() * _device_scale);
        cairo_surface_set_device_scale(_backing_store->cobj(), _device_scale, _device_scale); // No C++ API!

        _outline_store =
            Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32,
                                        _allocation.get_width()  * _device_scale,
                                        _allocation.get_height() * _device_scale);
        cairo_surface_set_device_scale(_outline_store->cobj(), _device_scale, _device_scale); // No C++ API!

        _split_point = Geom::Point(_allocation.get_width()/2, _allocation.get_height()/2);
    }

    if (!(_allocation == allocation) || _device_scale != device_scale) { // "!=" for allocation not defined!
        _allocation = allocation;
        _device_scale = device_scale;

        // Create new stores and copy/shift contents.
        shift_content(Geom::IntPoint(0, 0), _backing_store);
        if (_split_mode != Inkscape::SPLITMODE_NORMAL) {
            shift_content(Geom::IntPoint(0, 0), _outline_store);
        }

        // Clip the clean region to the new allocation
        Cairo::RectangleInt clip = { _x0, _y0, _allocation.get_width(), _allocation.get_height() };
        _clean_region->intersect(clip);
    }

    // Blit from the backing store, without regard for the clean region.
    // This is the only place the widget content is drawn!
    cr->set_source(_backing_store, 0, 0);
    cr->paint();

    if (_split_mode != Inkscape::SPLITMODE_NORMAL) {
        cr->save();
        cr->set_source(_outline_store, 0, 0);
        add_clippath(cr);
        cr->paint();
        cr->restore();
    }

    // static int i = 0;
    // ++i;
    // std::string file = "on_draw_" + std::to_string(i) + ".png";
    // _backing_store->write_to_png(file);

    // This whole section is just to determine if we call add_idle!
    auto dirty_region = Cairo::Region::create();

    std::vector<Cairo::Rectangle> clip_rectangles;
    cr->copy_clip_rectangle_list(clip_rectangles);
    for (auto & rectangle : clip_rectangles) {
        Geom::Rect dr = Geom::Rect::from_xywh(rectangle.x + _x0,
                                              rectangle.y + _y0,
                                              rectangle.width,
                                              rectangle.height);
        // "rectangle" is floating point, we must convert to integer. We round outward as it's
        // better to have a larger dirty region to avoid artifacts.
        Geom::IntRect ir = dr.roundOutwards();
        Cairo::RectangleInt irect = { ir.left(), ir.top(), ir.width(), ir.height() };
        dirty_region->do_union(irect);
    }

    dirty_region->subtract(_clean_region);
    
    if (!dirty_region->empty()) {
        add_idle();
    }

    return true;
}

void
Canvas::add_idle()
{
    if (_in_destruction) {
        std::cerr << "Canvas::add_idle: Called after canvas destroyed!" << std::endl;
        return;
    }

    if (get_realized()) {
        _idle_connection = Glib::signal_idle().connect(sigc::mem_fun(*this, &Canvas::on_idle), Glib::PRIORITY_LOW);
    }
}

// Probably not needed.
void
Canvas::remove_idle()
{
    _idle_connection.disconnect();
}

bool
Canvas::on_idle()
{
    if (_in_destruction) {
        std::cerr << "Canvas::on_idle: Called after canvas destroyed!" << std::endl;
        return false; // Disconnect
    }

    bool done = do_update();
    int n_rects = _clean_region->get_num_rectangles();

    // If we've drawn everything then we should have just one clean rectangle, covering the entire canvas.
    if (n_rects == 0) {
        std::cerr << "Canvas::on_idle: clean region is empty!" << std::endl;
    } else {
        auto clean_rect = _clean_region->get_extents();
    }

    if (n_rects > 1) {
        done = false;
    }

    return !done;
}

// Return true if done.
bool
Canvas::do_update()
{
    if (!_root) {
        // Canvas destroyed?
        return true;
    }

    if (_drawing_disabled) {
        return true;
    }

    if (_need_update) {
        sp_canvas_item_invoke_update2(_root, Geom::identity(), 0);
        _need_update = false;
    }

    if (get_is_drawable()) {
        // We're mapped and visible.
        return paint();
    }

    // Pick current item:
    while (_need_repick) {
        _need_repick = false;
        pick_current_item(&_pick_event);
    }

    return true; // FIXME
}

bool
Canvas::paint()
{
    Cairo::RectangleInt crect = { _x0, _y0, _allocation.get_width(), _allocation.get_height() };
    auto draw_region = Cairo::Region::create(crect);
    draw_region->subtract(_clean_region);

    int n_rects = draw_region->get_num_rectangles();

    for (int i = 0; i < n_rects; ++i) {
        auto rect = draw_region->get_rectangle(i);
        if (!paint_rect(rect)) {
            // Aborted
            return false;
        };
    }

    return true;
}

bool
Canvas::paint_rect(Cairo::RectangleInt& rect)
{
    // Find window rectangle in 'world coordinates'.
    Geom::IntRect canvas_rect = Geom::IntRect::from_xywh(_x0, _y0, _allocation.get_width(), _allocation.get_height());
    Geom::IntRect paint_rect = Geom::IntRect::from_xywh(rect.x, rect.y, rect.width, rect.height);
    Geom::OptIntRect area = paint_rect & canvas_rect;

    // Don't stop idle process if empty.
    if (!area || area->hasZeroArea()) {
        return true;
    }

    // Get cursor position
    auto const display = Gdk::Display::get_default();
    auto const seat    = display->get_default_seat();
    auto const device  = seat->get_pointer();

    int x = 0;
    int y = 0;
    Gdk::ModifierType mask;
    auto window = get_window();
    if (window) {
        window->get_device_position(device, x, y, mask);
    }

    PaintRectSetup setup;
    setup.canvas_rect = canvas_rect;
    setup.mouse_loc = Geom::Point(_x0 + x, _y0 + y);
    setup.start_time = g_get_monotonic_time();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    unsigned tile_multiplier = prefs->getIntLimited("/options/rendering/tile-multiplier", 16, 1, 512);
    if (_render_mode != Inkscape::RENDERMODE_OUTLINE) {
        // Can't be too small or large gradient will be rerendered too many times!
        setup.max_pixels = 65536 * tile_multiplier;
    } else {
        // Paths only. 1M is catched buffer and we need four channels.
        setup.max_pixels = 262144;
    }

    return paint_rect_internal(&setup, paint_rect);
}

bool
Canvas::paint_rect_internal(PaintRectSetup const *setup, Geom::IntRect const &this_rect)
{
    gint64 now = g_get_monotonic_time();
    gint64 elapsed = now - setup->start_time;

    // Allow only very fast buffers to be run together;
    // as soon as the total redraw time exceeds 1ms, cancel;
    // this returns control to the idle loop and allows Inkscape to process user input
    // (potentially interrupting the redraw); as soon as Inkscape has some more idle time,
    if (elapsed > 1000) {
        // Interrupting redraw isn't always good.
        // For example, when you drag one node of a big path, only the buffer containing
        // the mouse cursor will be redrawn again and again, and the rest of the path
        // will remain stale because Inkscape never has enough idle time to redraw all
        // of the screen. To work around this, such operations set a forced_redraw_limit > 0.
        // If this limit is set, and if we have aborted redraw more times than is allowed,
        // interrupting is blocked and we're forced to redraw full screen once
        // (after which we can again interrupt forced_redraw_limit times).
        if (_forced_redraw_limit < 0 ||
            _forced_redraw_count < _forced_redraw_limit) {

            if (_forced_redraw_limit != -1) {
                _forced_redraw_count++;
            }
            return false;
        }
        _forced_redraw_count = 0;
    }

    // Find optimal buffer dimension
    int bw = this_rect.width();
    int bh = this_rect.height();

    if (bw < 1 || bh < 1) {
        // Nothing to draw!
        return false; // Don't idle stop process if area is empty.
    }

    if (bw * bh < setup->max_pixels) {
        // We are small enough!

        // TODO Find better solution
        SPDesktop *desktop = SP_ACTIVE_DESKTOP;
        SPCanvasArena *arena = SP_CANVAS_ARENA(desktop->drawing);

        arena->drawing.setRenderMode(_render_mode);
        paint_single_buffer(this_rect, setup->canvas_rect, _backing_store);

        if (_split_mode != Inkscape::SPLITMODE_NORMAL) {
            arena->drawing.setRenderMode(Inkscape::RENDERMODE_OUTLINE);
            paint_single_buffer(this_rect, setup->canvas_rect, _outline_store);
        }

        Cairo::RectangleInt crect = { this_rect.left(), this_rect.top(), this_rect.width(), this_rect.height() };
        _clean_region->do_union( crect );

        queue_draw_area(this_rect.left() - _x0, this_rect.top() - _y0, this_rect.width(), this_rect.height());

        return true;
    }

    /*
     * Determine redraw strategy:
     *
     * bw < bh (strips mode): Draw horizontal strips starting from cursor position.
     *                        Seems to be faster for drawing many smaller objects zoomed out.
     *
     * bw > hb (chunks mode): Splits across the larger dimension of the rectangle, painting
     *                        in almost square chunks (from the cursor.
     *                        Seems to be faster for drawing a few blurred objects across the entire screen.
     *                        Seems to be somewhat psycologically faster.
     *
     * Default is for strips mode.
     */

    static int TILE_SIZE = 16;
    Geom::IntRect lo, hi;

    if (bw < bh || bh < 2 * TILE_SIZE) {
        int mid = this_rect[Geom::X].middle();

        lo = Geom::IntRect(this_rect.left(), this_rect.top(), mid,               this_rect.bottom());
        hi = Geom::IntRect(mid,              this_rect.top(), this_rect.right(), this_rect.bottom());

        if (setup->mouse_loc[Geom::X] < mid) {
            // Always paint towards the mouse first
            return paint_rect_internal(setup, lo)
                && paint_rect_internal(setup, hi);
        } else {
            return paint_rect_internal(setup, hi)
                && paint_rect_internal(setup, lo);
        }
    } else {
        int mid = this_rect[Geom::Y].middle();

        lo = Geom::IntRect(this_rect.left(), this_rect.top(), this_rect.right(), mid                );
        hi = Geom::IntRect(this_rect.left(), mid,             this_rect.right(), this_rect.bottom());

        if (setup->mouse_loc[Geom::Y] < mid) {
            // Always paint towards the mouse first
            return paint_rect_internal(setup, lo)
                && paint_rect_internal(setup, hi);
        } else {
            return paint_rect_internal(setup, hi)
                && paint_rect_internal(setup, lo);
        }
    }
}

void
Canvas::paint_single_buffer(Geom::IntRect const &paint_rect, Geom::IntRect const &canvas_rect,
                            Cairo::RefPtr<Cairo::ImageSurface> &store)
{
    if (!store) {
        std::cerr << "Canvas::paint_single_buffer: store not created!" << std::endl;
        return;
        // Maybe store not created!
    }

    SPCanvasBuf buf;
    buf.buf = nullptr;
    buf.buf_rowstride = 0;
    buf.rect = paint_rect;
    buf.canvas_rect = canvas_rect;
    buf.device_scale = _device_scale;
    buf.is_empty = true;

    // Make sure the following code does not go outside of store's data
    assert(store->get_format() == Cairo::FORMAT_ARGB32);
    assert(paint_rect.left()   - _x0 >= 0);
    assert(paint_rect.top()    - _y0 >= 0);
    assert(paint_rect.right()  - _x0 <= store->get_width());
    assert(paint_rect.bottom() - _y0 <= store->get_height());

    // Create temporary surface that draws directly to store.
    store->flush();

    // static int i = 0;
    // ++i;
    // std::string file = "paint_single_buffer0_" + std::to_string(i) + ".png";
    // store->write_to_png(file);

    // Create temporary surface that draws directly to store.
    unsigned char *data = store->get_data();
    int stride = store->get_stride();

    // Check we are using the correct device scale.
    double x_scale = 1.0;
    double y_scale = 1.0;
    cairo_surface_get_device_scale(store->cobj(), &x_scale, &y_scale); // No C++ API!
    assert (_device_scale == (int)x_scale);
    assert (_device_scale == (int)y_scale);

    // Move to the correct row.
    data += stride * (paint_rect.top() - _y0) * (int)y_scale;
    // Move to the correct column.
    data += 4 * (paint_rect.left() - _x0) * (int)x_scale;
    auto imgs = Cairo::ImageSurface::create(data, Cairo::FORMAT_ARGB32,
                                            paint_rect.width()  * _device_scale,
                                            paint_rect.height() * _device_scale,
                                            stride);
    cairo_surface_set_device_scale(imgs->cobj(), _device_scale, _device_scale); // No C++ API!

    auto cr = Cairo::Context::create(imgs);

    // Paint background
    cr->save();
    cr->translate(-paint_rect.left(), -paint_rect.top());
    cr->set_operator(Cairo::OPERATOR_SOURCE);
    cr->set_source(_background);
    cr->paint();
    cr->restore();
    buf.ct = cr->cobj();

    // Render drawing on top of background.
    if (_root->visible) {
        SP_CANVAS_ITEM_GET_CLASS(_root)->render(_root, &buf);
    }

#if defined(HAVE_LIBLCMS2)
    if (_cms_active) {
        cmsHTRANSFORM transf = nullptr;
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        bool fromDisplay = prefs->getBool( "/options/displayprofile/from_display");
        if ( fromDisplay ) {
            transf = Inkscape::CMSSystem::getDisplayPer(_cms_key);
        } else {
            transf = Inkscape::CMSSystem::getDisplayTransform();
        }

        if (transf) {
            imgs->flush();
            unsigned char *px = imgs->get_data();
            int stride = imgs->get_stride();
            for (int i=0; i<paint_rect.height(); ++i) {
                unsigned char *row = px + i*stride;
                Inkscape::CMSSystem::doTransform(transf, row, row, paint_rect.width());
            }
            imgs->mark_dirty();
        }
    }
#endif // defined(HAVE_LIBLCMS2)

    store->mark_dirty();

    // Uncomment to see how Inkscape paints to rectangles on canvas.
    // cr->save();
    // cr->move_to (0.5,                    0.5);
    // cr->line_to (0.5,                    paint_rect.height()-0.5);
    // cr->line_to (paint_rect.width()-0.5, paint_rect.height()-0.5);
    // cr->line_to (paint_rect.width()-0.5, 0.5);
    // cr->close_path();
    // cr->set_source_rgba(0.0, 0.0, 0.5, 1.0);
    // cr->stroke();
    // cr->restore();

    Cairo::RectangleInt crect = { paint_rect.left(), paint_rect.top(), paint_rect.width(), paint_rect.height() };
    _clean_region->do_union( crect );

    queue_draw_area(paint_rect.left() - _x0, paint_rect.top() - _y0, paint_rect.width(), paint_rect.height());
}


// Shift backing store (when canvas scrolled or size changed).
void
Canvas::shift_content(Geom::IntPoint shift, Cairo::RefPtr<Cairo::ImageSurface> &store)
{
    Cairo::RefPtr<::Cairo::ImageSurface> new_store =
        Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32,
                                    _allocation.get_width()  * _device_scale,
                                    _allocation.get_height() * _device_scale);

    cairo_surface_set_device_scale(new_store->cobj(), _device_scale, _device_scale); // No C++ API!

    // Copy the old store contents to new backing store.
    auto cr = Cairo::Context::create(new_store);

    // Paint background
    cr->set_operator(Cairo::Operator::OPERATOR_SOURCE);
    cr->set_source(_background);
    cr->paint();

    // Copy old background
    cr->translate(-shift.x(), -shift.y());
    cr->set_source(store, 0, 0);
    cr->paint();

    store = new_store;

    // static int i = 0;
    // ++i;
    // std::string file = "shift_content_" + std::to_string(i) + ".png";
    // _store->write_to_png(file);
}


// Sets clip path for Split and X-Ray modes.
void
Canvas::add_clippath(const Cairo::RefPtr<Cairo::Context>& cr) {

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double radius = prefs->getIntLimited("/options/rendering/xray-radius", 100, 1, 1500);

    double width  = _allocation.get_width();
    double height = _allocation.get_height();
    double sx     = _split_point.x();
    double sy     = _split_point.y();

    if (_split_mode == Inkscape::SPLITMODE_SPLIT) {
        switch (_split_direction) {
            case Inkscape::SPLITDIRECTION_NORTH:
                cr->rectangle(0,   0, width,               sy);
                break;
            case Inkscape::SPLITDIRECTION_SOUTH:
                cr->rectangle(0,  sy, width,      height - sy);
                break;
            case Inkscape::SPLITDIRECTION_WEST:
                cr->rectangle(0,   0,         sx, height     );
                break;
            case Inkscape::SPLITDIRECTION_EAST:
                cr->rectangle(sx,  0, width - sx, height     );
                break;
        }
    } else {
        cr->arc(sx, sy, radius, 0, 2 * M_PI);
    }

    cr->clip();
}

// This routine reacts to events from the canvas. It's main purpose is to find the canvas item
// closest to the cursor where the event occured and then send the event (sometimes modified) to
// that item. The event then bubbles up the canvas item tree until an object handles it. If the
// widget is redrawn, this routine may be called again for the same event.
//
// Canvas items register their interest by connecting to the "event" signal.
// Example in desktop.cpp:
//   g_signal_connect (G_OBJECT (acetate), "event", G_CALLBACK (sp_desktop_root_handler), this);
bool
Canvas::pick_current_item(GdkEvent *event)
{
    int button_down = 0;
    if (_all_enter_events == false) {
        // Only set true in connector-tool.cpp.

        // If a button is down, we'll perform enter and leave events on the
        // current item, but not enter on any other item.  This is more or
        // less like X pointer grabbing for canvas items.
        button_down = _state & (GDK_BUTTON1_MASK |
                                GDK_BUTTON2_MASK |
                                GDK_BUTTON3_MASK |
                                GDK_BUTTON4_MASK |
                                GDK_BUTTON5_MASK);
        if (!button_down) _left_grabbed_item = false;
    }
        
    // Save the event in the canvas.  This is used to synthesize enter and
    // leave events in case the current item changes.  It is also used to
    // re-pick the current item if the current one gets deleted.  Also,
    // synthesize an enter event.
    if (event != &_pick_event) {
        if (event->type == GDK_MOTION_NOTIFY || event->type == GDK_BUTTON_RELEASE) {
            // Convert to GDK_ENTER_NOTIFY

            // These fields have the same offsets in both types of events.
            _pick_event.crossing.type       = GDK_ENTER_NOTIFY;
            _pick_event.crossing.window     = event->motion.window;
            _pick_event.crossing.send_event = event->motion.send_event;
            _pick_event.crossing.subwindow  = nullptr;
            _pick_event.crossing.x          = event->motion.x;
            _pick_event.crossing.y          = event->motion.y;
            _pick_event.crossing.mode       = GDK_CROSSING_NORMAL;
            _pick_event.crossing.detail     = GDK_NOTIFY_NONLINEAR;
            _pick_event.crossing.focus      = false;
            _pick_event.crossing.state      = event->motion.state;

            // These fields don't have the same offsets in both types of events.
            if (event->type == GDK_MOTION_NOTIFY) {
                _pick_event.crossing.x_root = event->motion.x_root;
                _pick_event.crossing.y_root = event->motion.y_root;
            } else {
                _pick_event.crossing.x_root = event->button.x_root;
                _pick_event.crossing.y_root = event->button.y_root;
            }
        } else {
            _pick_event = *event;
        }
    }

    if (_in_repick) {
        // Don't do anything else if this is a recursive call.
        return false;
    }

    // Find new item
    _current_item_new = nullptr; // Not used in call to sp_canvas_item_invoke_point
    if (_pick_event.type != GDK_LEAVE_NOTIFY && _root->visible) {
        // Leave notify means there is no current item.

        // Find closest item.
        if (_root->visible) {
            double x = 0.0;
            double y = 0.0;

            if (_pick_event.type == GDK_ENTER_NOTIFY) {
                x = _pick_event.crossing.x;
                y = _pick_event.crossing.y;
            } else {
                x = _pick_event.motion.x;
                y = _pick_event.motion.y;
            }

            // Convert to world coordinates.
            x += _x0;
            y += _y0;

            sp_canvas_item_invoke_point (_root, Geom::Point(x, y), &_current_item_new);
        }
    }

    if ((_current_item_new == _current_item) && !_left_grabbed_item) {
        // Current item did not change!
        return false;
    }

    // Synthesize events for old and new current items.
    bool retval = false;
    if ( (_current_item_new != _current_item) &&
         _current_item != nullptr             &&
         !_left_grabbed_item                   ) {
        GdkEvent new_event;
        new_event = _pick_event;
        new_event.type = GDK_LEAVE_NOTIFY;
        new_event.crossing.detail = GDK_NOTIFY_ANCESTOR;
        new_event.crossing.subwindow = nullptr;
        _in_repick = true;
        retval = emit_event(&new_event);
        _in_repick = false;
    }

    if (_all_enter_events == false) {
        // new_current_item may have been set to nullptr during the call to emitEvent() above.
        if ((_current_item_new != _current_item) && button_down) {
            _left_grabbed_item = true;
            return retval;
        }
    }

    // Handle the rest of cases
    _left_grabbed_item = false;
    _current_item = _current_item_new;

    if (_current_item != nullptr) {
        GdkEvent new_event;
        new_event = _pick_event;
        new_event.type = GDK_ENTER_NOTIFY;
        new_event.crossing.detail = GDK_NOTIFY_ANCESTOR;
        new_event.crossing.subwindow = nullptr;
        retval = emit_event(&new_event);
    }

    return retval;
}

bool
Canvas::emit_event(GdkEvent *event)
{
    int mask = 0;
    if (_grabbed_item) {
        switch (event->type) {
        case GDK_ENTER_NOTIFY:
            mask = GDK_ENTER_NOTIFY_MASK;
            break;
        case GDK_LEAVE_NOTIFY:
            mask = GDK_LEAVE_NOTIFY_MASK;
            break;
        case GDK_MOTION_NOTIFY:
            mask = GDK_POINTER_MOTION_MASK;
            break;
        case GDK_BUTTON_PRESS:
        case GDK_2BUTTON_PRESS:
        case GDK_3BUTTON_PRESS:
            mask = GDK_BUTTON_PRESS_MASK;
            break;
        case GDK_BUTTON_RELEASE:
            mask = GDK_BUTTON_RELEASE_MASK;
            break;
        case GDK_KEY_PRESS:
            mask = GDK_KEY_PRESS_MASK;
            break;
        case GDK_KEY_RELEASE:
            mask = GDK_KEY_RELEASE_MASK;
            break;
        case GDK_SCROLL:
            mask = GDK_SCROLL_MASK;
            mask |= GDK_SMOOTH_SCROLL_MASK;
            break;
        default:
            mask = 0;
            break;
        }

        if (!(mask & _grabbed_event_mask)) {
            return false;
        }
    }

    // Convert to world coordinates. We have two different cases due to
    // different event structures.
    GdkEvent *event_copy = gdk_event_copy(event);
    switch (event_copy->type) {
        case GDK_LEAVE_NOTIFY:
            event_copy->crossing.x += _x0;
            event_copy->crossing.y += _y0;
            break;
        case GDK_MOTION_NOTIFY:
        case GDK_BUTTON_PRESS:
        case GDK_2BUTTON_PRESS:
        case GDK_3BUTTON_PRESS:
        case GDK_BUTTON_RELEASE:
            event_copy->motion.x += _x0;
            event_copy->motion.y += _y0;
            break;
        default:
            break;
    }

    // Block undo/redo while anything is dragged.
    if (event->type == GDK_BUTTON_PRESS && event->button.button == 1) {
        _is_dragging = true;
    } else if (event->type == GDK_BUTTON_RELEASE) {
        _is_dragging = false;
    }

    // Choose where to send event.
    SPCanvasItem *item = _current_item;

    if (_grabbed_item && !is_descendant(_current_item, _grabbed_item)) {
        item = _grabbed_item;
    }

    if (_focused_item &&
        ((event->type == GDK_KEY_PRESS) ||
         (event->type == GDK_KEY_RELEASE) ||
         (event->type == GDK_FOCUS_CHANGE))) {
        item = _focused_item;
    }

    // Propogate the event up the item hierarchy until handled.
    gint finished = false; // Can't be bool or "stack smashing detected"!
    while (item && !finished) {
        g_object_ref (item);
        g_signal_emit (G_OBJECT (item), item_signals[ITEM_EVENT], 0, event_copy, &finished);
        SPCanvasItem *parent = item->parent;
        g_object_unref (item);
        item = parent;
    }

    gdk_event_free(event_copy);

    return finished;
}


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

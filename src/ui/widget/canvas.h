// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INKSCAPE_UI_WIDGET_CANVAS_H
#define INKSCAPE_UI_WIDGET_CANVAS_H
/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtkmm.h>

#include <2geom/rect.h>
#include <2geom/int-rect.h>

#include "preferences.h" // Update canvas_item_ctrl sizes.

#include "display/rendermode.h"

class SPDesktop;

struct PaintRectSetup;


namespace Inkscape {

class CanvasItem;
class CanvasItemGroup;
class Drawing;

namespace UI {
namespace Widget {

/**
 * A Gtk::DrawingArea widget for Inkscape's canvas.
 */ 
class Canvas : public Gtk::DrawingArea
{
public:

    Canvas();
    ~Canvas() override;

    // Structure TODO: Remove desktop dependency.
    void set_desktop(SPDesktop *desktop) { _desktop = desktop; }
    SPDesktop *get_desktop() { return _desktop; }

    // Geometry
    bool world_point_inside_canvas(Geom::Point const &world); // desktop-events.cpp
    Geom::Point canvas_to_world(Geom::Point const &window);
    Geom::Rect get_area_world();
    Geom::IntRect get_area_world_int(); // Shouldn't really need this, only used for rulers.
    void set_affine(Geom::Affine const &affine);
    Geom::Affine get_affine() { return _affine; }

    // Drawing
    void set_drawing(Inkscape::Drawing *drawing) { _drawing = drawing; }
    void redraw_all();                                // Draw entire surface during idle.
    void redraw_area(int x0, int y0, int x1, int y1); // Draw specified area during idle.
    void redraw_area(Geom::Rect& area);
    void redraw_now();                                // Draw areas needing update immediately.
    void request_update();                            // Draw after updating canvas items.
    void scroll_to(Geom::Point const &c, bool clear);

    void set_background_color(guint32 rgba);
    void set_background_checkerboard(guint32 rgba = 0xC4C4C4FF);

    void set_drawing_disabled(bool disable) { _drawing_disabled = disable; } // Disable during path ops, etc.
    bool is_dragging() {return _is_dragging; }                // selection-chemistry.cpp

    //  Rendering modes
    void set_render_mode(Inkscape::RenderMode mode);
    void set_color_mode(Inkscape::ColorMode   mode);
    void set_split_mode(Inkscape::SplitMode   mode);
    void set_split_direction(Inkscape::SplitDirection dir);

#if defined(HAVE_LIBLCMS2)
    void set_cms_key(std::string key) {
        _cms_key = key;
        _cms_active = !key.empty();
    }
    std::string get_cms_key() { return _cms_key; }
    void set_cms_active(bool active) { _cms_active = active; }
    bool get_cms_active() { return _cms_active; }
#endif

    Cairo::RefPtr<Cairo::ImageSurface> get_backing_store() { return _backing_store; } // Background rotation preview

    // For a GTK bug (see SelectedStyle::on_opacity_changed()).
    void forced_redraws_start(int count, bool reset = true);
    void forced_redraws_stop() { _forced_redraw_limit = -1; }

    // Canvas Items
    CanvasItemGroup *get_canvas_item_root() { return _canvas_item_root; }

    Inkscape::CanvasItem *get_current_canvas_item() { return _current_canvas_item; }
    void                  set_current_canvas_item(Inkscape::CanvasItem *item) {
        _current_canvas_item = item; }
    Inkscape::CanvasItem *get_grabbed_canvas_item() { return _grabbed_canvas_item; }
    void                  set_grabbed_canvas_item(Inkscape::CanvasItem *item, Gdk::EventMask mask) {
        _grabbed_canvas_item = item;
        _grabbed_event_mask = mask;
    }
    
    void           set_need_repick(bool repick = true) { _need_repick = repick; }
    void           canvas_item_clear(Inkscape::CanvasItem *item);

    // Events
    void           set_all_enter_events(bool on) { _all_enter_events = on; }

protected:

    void get_preferred_width_vfunc( int& minimum_width,  int& natural_width ) const override;
    void get_preferred_height_vfunc(int& minimum_height, int& natural_height) const override;

    // Event handlers
    bool on_scroll_event(        GdkEventScroll   *scroll_event)   override;
    bool on_button_event(        GdkEventButton   *button_event);
    bool on_button_press_event(  GdkEventButton   *button_event)   override;
    bool on_button_release_event(GdkEventButton   *button_event)   override;
    bool on_enter_notify_event(  GdkEventCrossing *crossing_event) override;
    bool on_leave_notify_event(  GdkEventCrossing *crossing_event) override;
    bool on_focus_in_event(      GdkEventFocus    *focus_event )   override;
    bool on_focus_out_event(     GdkEventFocus    *focus_event )   override;
    bool on_key_press_event(     GdkEventKey      *key_event   )   override;
    bool on_key_release_event(   GdkEventKey      *key_event   )   override;
    bool on_motion_notify_event( GdkEventMotion   *motion_event)   override;

    // Painting
    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;

    void update_canvas_item_ctrl_sizes(int size_index);

private:

    // ======== Functions =======
    void add_idle();
    void remove_idle(); // Not needed?
    bool on_idle();

    // Painting

    // In order they are called in painting.
    bool do_update();
    bool paint();
    bool paint_rect(Cairo::RectangleInt& rect);
    bool paint_rect_internal(PaintRectSetup const *setup, Geom::IntRect const &this_rect);
    void paint_single_buffer(Geom::IntRect const &paint_rect, Geom::IntRect const &canvas_rect,
                             Cairo::RefPtr<Cairo::ImageSurface> &store);

    void shift_content(Geom::IntPoint shift, Cairo::RefPtr<Cairo::ImageSurface> &store);
    void add_clippath(const Cairo::RefPtr<Cairo::Context>& cr);
    void set_cursor();

    // Events
    bool pick_current_item(GdkEvent *event);
    bool emit_event(GdkEvent *event);

    // ==== Signal callbacks ====
    sigc::connection _idle_connection;  // Probably not needed (automatically disconnects).

    // ====== Data members =======

    // Structure
    SPDesktop * _desktop = nullptr;

    // Geometry
    int _x0 = 0;                     ///< World coordinate of the leftmost pixels of window.
    int _y0 = 0;                     ///< World coordinate of the topmost pixels of window.
    Geom::Point _window_origin;      ///< World coordinate of the upper-leftmost pixel of window.
    Geom::Affine _affine;            // Only used for canvas items at moment.

    int _device_scale = 1;           ///< Scale for high DPI montiors. Probably should be double.
    Gtk::Allocation _allocation;     ///< Canvas allocation, save so we know when it changes.

    // Event handling/item picking
    GdkEvent _pick_event;                       ///< Event used to find currently selected item.
    bool     _need_repick           = true;     ///< ?
    bool     _in_repick             = false;    ///< Used internally by pick_current_item().
    bool     _left_grabbed_item     = false;    ///< ?
    bool     _all_enter_events      = false;    ///< Keep all enter events. Only set true in connector-tool.cpp.
    bool     _is_dragging           = false;    ///< Used in selection-chemistry to block undo/redo.
    int      _state                 = 0;        ///< Last know modifier state (SHIFT, CTRL, etc.).

    Inkscape::CanvasItem *_current_canvas_item     = nullptr;  ///< Item containing cursor, nullptr if none.
    Inkscape::CanvasItem *_current_canvas_item_new = nullptr;  ///< Item to become _current_item, nullptr if none.
    Inkscape::CanvasItem *_grabbed_canvas_item     = nullptr;  ///< Item that holds a pointer grab; nullptr if none.

    Gdk::EventMask _grabbed_event_mask = (Gdk::EventMask)0;

    // Drawing
    bool _drawing_disabled = false;  ///< Disable drawing during critical operations
    bool _need_update = true; // Set true so setting CanvasItem bounds are calculated at least once.
    CanvasItemGroup *_canvas_item_root = nullptr;
    Inkscape::Drawing *_drawing = nullptr;

    Inkscape::RenderMode _render_mode = Inkscape::RENDERMODE_NORMAL;
    Inkscape::SplitMode  _split_mode  = Inkscape::SPLITMODE_NORMAL;
    Geom::Point _split_position;
    Inkscape::SplitDirection _split_direction   = Inkscape::SPLITDIRECTION_EAST;
    Inkscape::SplitDirection _hover_direction   = Inkscape::SPLITDIRECTION_NONE;
    bool _split_dragging = false;
    Geom::Point _split_drag_start;
    Inkscape::ColorMode  _color_mode  = Inkscape::COLORMODE_NORMAL;

#if defined(HAVE_LIBLCMS2)
    std::string _cms_key;
    bool _cms_active = false;
#endif

    // For a GTK bug (see SelectedStyle::on_opacity_changed()).
    int _forced_redraw_limit = -1;
    int _forced_redraw_count =  0;

    // Some objects (e.g. grids) when destroyed will request redraws. We need to block them when canvas
    // is destructed. (Windows are destroyed before documents as a document may have several windows.
    // Changes to documents should not be triggering changes to closed windows. This fix is a hack.)
    bool _in_destruction = false;

    // ======= CAIRO ======= ... Keep in one place

    /// Image surface storing the content of the widget.
    Cairo::RefPtr<Cairo::ImageSurface> _backing_store; ///< The canvas image content. We draw to this then blit.
    Cairo::RefPtr<Cairo::ImageSurface> _outline_store; ///< The outline image if we are in split/x-ray mode.

    Cairo::RefPtr<Cairo::Pattern> _background;         ///< The background of the image.
    bool _background_is_checkerboard = false;
    
    Cairo::RefPtr<Cairo::Region> _clean_region;        ///< Area of widget that has up-to-date content.


    // Used to update CanvasItemCtrl's when size changed.
    class CanvasPrefObserver : public Inkscape::Preferences::Observer {
    public:
        CanvasPrefObserver(Inkscape::UI::Widget::Canvas *canvas, Glib::ustring const &path)
            : Inkscape::Preferences::Observer(path)
            , _canvas(canvas)
        {
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            prefs->addObserver(*this);
        }
        ~CanvasPrefObserver() override = default;
    private:
        void notify(Inkscape::Preferences::Entry const &entry) override
        {
            if (entry.getEntryName() == "value") {
                int size = entry.getIntLimited(3, 1, 7);
            _canvas->update_canvas_item_ctrl_sizes(size);
            }
        }
        Inkscape::UI::Widget::Canvas *_canvas = nullptr;
    };

    CanvasPrefObserver _size_observer;
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape


#endif // INKSCAPE_UI_WIDGET_CANVAS_H

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

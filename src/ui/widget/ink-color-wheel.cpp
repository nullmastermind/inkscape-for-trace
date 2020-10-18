// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Color wheel widget. Outer ring for Hue. Inner triangle for Saturation and Value.
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include "ink-color-wheel.h"

// A point with a color value.
class color_point {
public:
    color_point() : x(0), y(0), r(0), g(0), b(0) {};
    color_point(double x, double y, double r, double g, double b) : x(x), y(y), r(r), g(g), b(b) {};
    color_point(double x, double y, guint32 color) : x(x), y(y),
                                                     r(((color & 0xff0000) >> 16)/255.0),
                                                     g(((color &   0xff00) >>  8)/255.0),
                                                     b(((color &     0xff)      )/255.0) {};
    guint32 get_color() { return (int(r*255) << 16 | int(g*255) << 8 | int(b*255)); };
    double x;
    double y;
    double r;
    double g;
    double b;
};

inline double lerp(const double& v0, const double& v1, const double& t0, const double&t1, const double& t) {
    double s = 0;
    if (t0 != t1) {
        s = (t - t0)/(t1 - t0);
    }
    return (1.0 - s) * v0 + s * v1;
}

inline color_point lerp(const color_point& v0, const color_point& v1, const double &t0, const double &t1, const double& t) {
    double x = lerp(v0.x, v1.x, t0, t1, t);
    double y = lerp(v0.y, v1.y, t0, t1, t);
    double r = lerp(v0.r, v1.r, t0, t1, t);
    double g = lerp(v0.g, v1.g, t0, t1, t);
    double b = lerp(v0.b, v1.b, t0, t1, t);
    return (color_point(x, y, r, g, b));
}

inline double clamp(const double& value, const double& min, const double& max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// h, s, and v in range 0 to 1. Returns rgb value useful for use in Cairo.
guint32 hsv_to_rgb(double h, double s, double v) {

    if (h < 0.0 || h > 1.0 ||
        s < 0.0 || s > 1.0 ||
        v < 0.0 || v > 1.0) {
        std::cerr << "ColorWheel: hsv_to_rgb: input out of bounds: (0-1)"
                  << " h: " << h << " s: " << s << " v: " << v << std::endl;
        return 0x0;
    }

    double r = v;
    double g = v;
    double b = v;

    if (s != 0.0) {
        if (h == 1.0) h = 0.0;
        h *= 6.0;

        double f = h - (int)h;
        double p = v * (1.0 - s);
        double q = v * (1.0 - s * f);
        double t = v * (1.0 - s * (1.0 - f));

        switch ((int)h) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: r = v; g = p; b = q; break;
            default: g_assert_not_reached();
        }
    }
    guint32 rgb = (((int)floor (r*255 + 0.5) << 16) |
                   ((int)floor (g*255 + 0.5) <<  8) |
                   ((int)floor (b*255 + 0.5)      ));
    return rgb;
}

double luminance(guint32 color)
{
    double r(((color & 0xff0000) >> 16)/255.0);
    double g(((color &   0xff00) >>  8)/255.0);
    double b(((color &     0xff)      )/255.0);
    return (r * 0.2125 + g * 0.7154 + b * 0.0721);
}

namespace Inkscape {
namespace UI {
namespace Widget {

ColorWheel::ColorWheel()
    : _hue(0.0)
    , _saturation(1.0)
    , _value(1.0)
    , _ring_width(0.2)
    , _mode(DragMode::NONE)
    , _focus_on_ring(true)
{
    set_name("ColorWheel");
    add_events(Gdk::BUTTON_PRESS_MASK   |
               Gdk::BUTTON_RELEASE_MASK |
               Gdk::BUTTON_MOTION_MASK  |
               Gdk::KEY_PRESS_MASK      );
    set_can_focus();
}

void
ColorWheel::set_rgb(const double& r, const double&g, const double&b, bool override_hue)
{
    double Min = std::min({r, g, b});
    double Max = std::max({r, g, b});
    _value = Max;
    if (Min == Max) {
        if (override_hue) {
            _hue = 0.0;
        }
    } else {
        if (Max == r) {
            _hue = ((g-b)/(Max-Min)    )/6.0;
        } else if (Max == g) {
            _hue = ((b-r)/(Max-Min) + 2)/6.0;
        } else {
            _hue = ((r-g)/(Max-Min) + 4)/6.0;
        }
        if (_hue < 0.0) {
            _hue += 1.0;
        }
    }
    if (Max == 0) {
        _saturation = 0;
    } else {
        _saturation = (Max - Min)/Max;
    }
}

void
ColorWheel::get_rgb(double& r, double& g, double& b)
{
    guint32 color = get_rgb();
    r = ((color & 0xff0000) >> 16)/255.0;
    g = ((color & 0x00ff00) >>  8)/255.0;
    b = ((color & 0x0000ff)      )/255.0;
}

guint32
ColorWheel::get_rgb()
{
    return hsv_to_rgb(_hue, _saturation, _value);
}

/**
  * Paints padding for an edge of the triangle,
  * using the (vertically) closest point.
  *
  * @param p0 A corner of the triangle. Not the same corner as p1
  * @param p1 A corner of the triangle. Not the same corner as p0
  * @param padding The height of the padding
  * @param pad_upwards True if padding is above the line
  * @param buffer Array that the triangle is painted to
  * @param height Height of buffer
  * @param stride Stride of buffer
*/
void
draw_vertical_padding(color_point p0, color_point p1, int padding, bool pad_upwards,
                      guint32 *buffer, int height, int stride);

bool
ColorWheel::on_draw(const::Cairo::RefPtr<::Cairo::Context>& cr) {
    Gtk::Allocation allocation = get_allocation();
    const int width  = allocation.get_width();
    const int height = allocation.get_height();

    const int stride = Cairo::ImageSurface::format_stride_for_width(Cairo::FORMAT_RGB24, width);

    int cx = width/2;
    int cy = height/2;

    int focus_line_width;
    int focus_padding;
    get_style_property("focus-line-width", focus_line_width);
    get_style_property("focus-padding",    focus_padding);

    // Paint ring
    guint32* buffer_ring = g_new (guint32, height * stride / 4);
    double r_max = std::min( width, height)/2.0 - 2 * (focus_line_width + focus_padding);
    double r_min = r_max * (1.0 - _ring_width);
    double r2_max = (r_max+2) * (r_max+2); // Must expand a bit to avoid edge effects.
    double r2_min = (r_min-2) * (r_min-2); // Must shrink a bit to avoid edge effects.

    for (int i = 0; i < height; ++i) {
        guint32* p = buffer_ring + i * width;
        double dy = (cy - i);
        for (int j = 0; j < width; ++j) {
            double dx = (j - cx);
            double r2 = dx * dx + dy * dy;
            if (r2 < r2_min || r2 > r2_max) {
                *p++ = 0; // Save calculation time.
            } else {
                double angle = atan2 (dy, dx);
                if (angle < 0.0) {
                    angle += 2.0 * M_PI;
                }
                double hue = angle/(2.0 * M_PI);

                *p++ = hsv_to_rgb(hue, 1.0, 1.0);
            }
        }
    }

    Cairo::RefPtr<::Cairo::ImageSurface> source_ring =
        ::Cairo::ImageSurface::create((unsigned char *)buffer_ring,
                                      Cairo::FORMAT_RGB24,
                                      width, height, stride);

    cr->set_antialias(Cairo::ANTIALIAS_SUBPIXEL);

    // Paint line on ring in source (so it gets clipped by stroke).
    double l = 0.0;
    guint32 color_on_ring = hsv_to_rgb(_hue, 1.0, 1.0);
    if (luminance(color_on_ring) < 0.5) l = 1.0;

    Cairo::RefPtr<::Cairo::Context> cr_source_ring = ::Cairo::Context::create(source_ring);
    cr_source_ring->set_source_rgb(l, l, l);

    cr_source_ring->move_to (cx, cy);
    cr_source_ring->line_to (cx + cos(_hue * M_PI * 2.0) * r_max+1,
                             cy - sin(_hue * M_PI * 2.0) * r_max+1);
    cr_source_ring->stroke();

    // Paint with ring surface, clipping to ring.
    cr->save();
    cr->set_source(source_ring, 0, 0);
    cr->set_line_width (r_max - r_min);
    cr->begin_new_path();
    cr->arc(cx, cy, (r_max + r_min)/2.0, 0, 2.0 * M_PI);
    cr->stroke();
    cr->restore();

    g_free(buffer_ring);

    // Draw focus
    if (has_focus() && _focus_on_ring) {
        Glib::RefPtr<Gtk::StyleContext> style_context = get_style_context();
        style_context->render_focus(cr, 0, 0, width, height);
    }
  
    // Paint triangle.
    /* The triangle is painted by first finding color points on the
     * edges of the triangle at the same y value via linearly
     * interpolating between corner values, and then interpolating along
     * x between the those edge points. The interpolation is in sRGB
     * space which leads to a complicated mapping between x/y and
     * saturation/value. This was probably done to remove the need to
     * convert between HSV and RGB for each pixel.
     * Black corner: v = 0, s = 1
     * White corner: v = 1, s = 0
     * Color corner; v = 1, s = 1
     */
    const int padding = 3; // Avoid edge artifacts.
    double x0, y0, x1, y1, x2, y2;
    triangle_corners(x0, y0, x1, y1, x2, y2);
    guint32 color0 = hsv_to_rgb(_hue, 1.0, 1.0);
    guint32 color1 = hsv_to_rgb(_hue, 1.0, 0.0);
    guint32 color2 = hsv_to_rgb(_hue, 0.0, 1.0);

    color_point p0 (x0, y0, color0);
    color_point p1 (x1, y1, color1);
    color_point p2 (x2, y2, color2);

    // Reorder so we paint from top down.
    if (p1.y > p2.y) {
        std::swap(p1, p2);
    }

    if (p0.y > p2.y) {
        std::swap(p0, p2);
    }

    if (p0.y > p1.y) {
        std::swap(p0, p1);
    }

    guint32* buffer_triangle = g_new (guint32, height * stride / 4);

    for (int y = 0; y < height; ++y) {
        guint32 *p = buffer_triangle + y * (stride / 4);

        if (p0.y <= y+padding && y-padding < p2.y) {

            // Get values on side at position y.
            color_point side0;
            double y_inter = clamp(y, p0.y, p2.y);
            if (y < p1.y) {
                side0 = lerp(p0, p1, p0.y, p1.y, y_inter);
            } else {
                side0 = lerp(p1, p2, p1.y, p2.y, y_inter);
            }
            color_point side1 = lerp(p0, p2, p0.y, p2.y, y_inter);

            // side0 should be on left
            if (side0.x > side1.x) {
                std::swap (side0, side1);
            }

            int x_start = std::max(0, int(side0.x));
            int x_end   = std::min(int(side1.x), width);

            for (int x = 0; x < width; ++x) {
                if (x <= x_start) {
                    *p++ = side0.get_color();
                } else if (x < x_end) {
                    *p++ = lerp(side0, side1, side0.x, side1.x, x).get_color();
                } else {
                    *p++ = side1.get_color();
                }
            }
        }
    }

    // add vertical padding to each side separately
    color_point temp_point = lerp(p0, p1, p0.x, p1.x, (p0.x + p1.x) / 2.0);
    bool pad_upwards = is_in_triangle(temp_point.x, temp_point.y + 1);
    draw_vertical_padding(p0, p1, padding, pad_upwards, buffer_triangle, height, stride / 4);

    temp_point = lerp(p0, p2, p0.x, p2.x, (p0.x + p2.x) / 2.0);
    pad_upwards = is_in_triangle(temp_point.x, temp_point.y + 1);
    draw_vertical_padding(p0, p2, padding, pad_upwards, buffer_triangle, height, stride / 4);

    temp_point = lerp(p1, p2, p1.x, p2.x, (p1.x + p2.x) / 2.0);
    pad_upwards = is_in_triangle(temp_point.x, temp_point.y + 1);
    draw_vertical_padding(p1, p2, padding, pad_upwards, buffer_triangle, height, stride / 4);

    Cairo::RefPtr<::Cairo::ImageSurface> source_triangle =
        ::Cairo::ImageSurface::create((unsigned char *)buffer_triangle,
                                      Cairo::FORMAT_RGB24,
                                      width, height, stride);

    // Paint with triangle surface, clipping to triangle.
    cr->save();
    cr->set_source(source_triangle, 0, 0);
    cr->move_to(p0.x, p0.y);
    cr->line_to(p1.x, p1.y);
    cr->line_to(p2.x, p2.y);
    cr->close_path();
    cr->fill();
    cr->restore();

    g_free(buffer_triangle);

    // Draw marker
    double mx = x1 + (x2-x1) * _value + (x0-x2) * _saturation * _value;
    double my = y1 + (y2-y1) * _value + (y0-y2) * _saturation * _value;

    double a = 0.0;
    guint32 color_at_marker = get_rgb();
    if (luminance(color_at_marker) < 0.5) a = 1.0;

    cr->set_source_rgb(a, a, a);
    cr->begin_new_path();
    cr->arc(mx, my, 4, 0, 2 * M_PI);
    cr->stroke();

    // Draw focus
    if (has_focus() && !_focus_on_ring) {
        Glib::RefPtr<Gtk::StyleContext> style_context = get_style_context();
        style_context->render_focus(cr, mx-4, my-4, 8, 8); // This doesn't seem to work.
        cr->set_line_width(0.5);
        cr->set_source_rgb(1-a, 1-a, 1-a);
        cr->begin_new_path();
        cr->arc(mx, my, 7, 0, 2 * M_PI);
        cr->stroke();
    }

    return true;
}

void
draw_vertical_padding(color_point p0, color_point p1, int padding, bool pad_upwards,
                      guint32 *buffer, int height, int stride)
{
    // skip if horizontal padding is more accurate, e.g. if the edge is vertical
    double gradient = (p1.y - p0.y) / (p1.x - p0.x);
    if (std::abs(gradient) > 1.0) {
        return;
    }

    double min_y = std::min(p0.y, p1.y);
    double max_y = std::max(p0.y, p1.y);

    double min_x = std::min(p0.x, p1.x);
    double max_x = std::max(p0.x, p1.x);

    // go through every point on the line
    for (int y = min_y; y <= max_y; ++y) {
        double start_x = lerp(p0, p1, p0.y, p1.y, clamp(y, min_y, max_y)).x;
        double end_x = lerp(p0, p1, p0.y, p1.y, clamp(y + 1, min_y, max_y)).x;
        if (start_x > end_x) {
            std::swap(start_x, end_x);
        }

        guint32 *p = buffer + y * stride;
        p += static_cast<int>(start_x);
        for (int x = start_x; x <= end_x; ++x) {
            // get the color at this point on the line
            color_point point = lerp(p0, p1, p0.x, p1.x, clamp(x, min_x, max_x));
            // paint the padding vertically above or below this point
            for (int offset = 0; offset <= padding; ++offset) {
                if (pad_upwards && (point.y - offset) >= 0) {
                    *(p - (offset * stride)) = point.get_color();
                } else if (!pad_upwards && (point.y + offset) < height) {
                    *(p + (offset * stride)) = point.get_color();
                }
            }
            ++p;
        }
    }
}

// Find triangle corners given hue and radius.
void
ColorWheel::triangle_corners(double &x0, double &y0,
                             double &x1, double &y1,
                             double &x2, double &y2)
{
    Gtk::Allocation allocation = get_allocation();
    const int width  = allocation.get_width();
    const int height = allocation.get_height();

    int cx = width/2;
    int cy = height/2;

    int focus_line_width;
    int focus_padding;
    get_style_property("focus-line-width", focus_line_width);
    get_style_property("focus-padding",    focus_padding);

    double r_max = std::min( width, height)/2.0 - 2 * (focus_line_width + focus_padding);
    double r_min = r_max * (1.0 - _ring_width);

    double angle = _hue * 2.0 * M_PI;

    x0 = cx + cos(angle) * r_min;
    y0 = cy - sin(angle) * r_min;
    x1 = cx + cos(angle + 2.0 * M_PI / 3.0) * r_min;
    y1 = cy - sin(angle + 2.0 * M_PI / 3.0) * r_min;
    x2 = cx + cos(angle + 4.0 * M_PI / 3.0) * r_min;
    y2 = cy - sin(angle + 4.0 * M_PI / 3.0) * r_min;
}

void
ColorWheel::set_from_xy(const double& x, const double& y)
{
    Gtk::Allocation allocation = get_allocation();
    const int width  = allocation.get_width();
    const int height = allocation.get_height();
    double cx = width/2.0;
    double cy = height/2.0;
    double r = std::min(cx, cy) * (1 - _ring_width);

    // We calculate RGB value under the cursor by rotating the cursor
    // and triangle by the hue value and looking at position in the
    // now right pointing triangle.
    double angle = _hue  * 2 * M_PI;
    double Sin = sin(angle);
    double Cos = cos(angle);
    double xp =  ((x-cx) * Cos - (y-cy) * Sin) / r;
    double yp =  ((x-cx) * Sin + (y-cy) * Cos) / r;

    double xt = lerp(0.0, 1.0, -0.5, 1.0, xp);
    xt = clamp(xt, 0, 1);

    double dy = (1-xt) * cos(M_PI/6.0);
    double yt = lerp(0.0, 1.0, -dy, dy, yp);
    yt = clamp(yt, 0, 1);

    color_point c0(0, 0, yt, yt, yt);              // Grey point along base.
    color_point c1(0, 0, hsv_to_rgb(_hue, 1, 1));  // Hue point at apex
    color_point c = lerp(c0, c1, 0, 1, xt);

    set_rgb(c.r, c.g, c.b, false); // Don't override previous hue.
}

bool
ColorWheel::is_in_ring(const double& x, const double& y)
{
    Gtk::Allocation allocation = get_allocation();
    const int width  = allocation.get_width();
    const int height = allocation.get_height();

    int cx = width/2;
    int cy = height/2;

    int focus_line_width;
    int focus_padding;
    get_style_property("focus-line-width", focus_line_width);
    get_style_property("focus-padding",    focus_padding);

    double r_max = std::min( width, height)/2.0 - 2 * (focus_line_width + focus_padding);
    double r_min = r_max * (1.0 - _ring_width);
    double r2_max = r_max * r_max;
    double r2_min = r_min * r_min;

    double dx = x - cx;
    double dy = y - cy;
    double r2 = dx * dx + dy * dy;
  
    return (r2_min < r2 && r2 < r2_max);
}

bool
ColorWheel::is_in_triangle(const double& x, const double& y)
{
    double x0, y0, x1, y1, x2, y2;
    triangle_corners(x0, y0, x1, y1, x2, y2);

    double det = (x2-x1) * (y0-y1) - (y2-y1) * (x0-x1);
    double s = ((x -x1) * (y0-y1) - (y -y1) * (x0-x1)) / det; 
    double t = ((x2-x1) * (y -y1) - (y2-y1) * (x -x1)) / det; 

    return (s >= 0.0 && t >= 0.0 && s+t <= 1.0);
}

bool
ColorWheel::on_focus(Gtk::DirectionType direction)
{
    // In forward direction, focus passes from no focus to ring focus to triangle focus to no focus.
    if (!has_focus()) {
        _focus_on_ring = (direction == Gtk::DIR_TAB_FORWARD);
        grab_focus();
        return true;
    }

    // Already have focus
    bool keep_focus = false;

    switch (direction) {
        case Gtk::DIR_UP:
        case Gtk::DIR_LEFT:
        case Gtk::DIR_TAB_BACKWARD:
            if (!_focus_on_ring) {
                _focus_on_ring = true;
                keep_focus = true;
            }
            break;

        case Gtk::DIR_DOWN:
        case Gtk::DIR_RIGHT:
        case Gtk::DIR_TAB_FORWARD:
            if (_focus_on_ring) {
                _focus_on_ring = false;
                keep_focus = true;
            }
            break;
    }

    queue_draw();  // Update focus indicators.

    return keep_focus;
}

void
ColorWheel::update_triangle_color(const double x, const double y)
{
    set_from_xy(x, y);
    _signal_color_changed.emit();
    queue_draw();
}

void
ColorWheel::update_ring_color(const double x, const double y)
{
    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();
    double cx = width / 2.0;
    double cy = height / 2.0;
    double angle = -atan2(y - cy, x - cx);

    if (angle < 0)
        angle += 2.0 * M_PI;
    _hue = angle / (2.0 * M_PI);

    queue_draw();
    _signal_color_changed.emit();
}

bool
ColorWheel::on_button_press_event(GdkEventButton* event)
{
    // Seat is automatically grabbed.
    double x = event->x;
    double y = event->y;

    if (is_in_ring(x, y) ) {
        _mode = DragMode::HUE;
        grab_focus();
        _focus_on_ring = true;
        update_ring_color(x, y);
        return true;
    }

    if (is_in_triangle(x, y)) {
        _mode = DragMode::SATURATION_VALUE;
        grab_focus();
        _focus_on_ring = false;
        update_triangle_color(x, y);
        return true;
    }

    return false;
}

bool
ColorWheel::on_button_release_event(GdkEventButton* event)
{
    _mode = DragMode::NONE;
    return true;
}


bool
ColorWheel::on_motion_notify_event(GdkEventMotion* event)
{
    double x = event->x;
    double y = event->y;

    if (_mode == DragMode::HUE) {
        update_ring_color(x, y);
        return true;
    }

    if (_mode == DragMode::SATURATION_VALUE) {
        update_triangle_color(x, y);
        return true;
    }

    return false;
}

bool
ColorWheel::on_key_press_event(GdkEventKey* key_event)
{
    bool consumed = false;

    unsigned int key = 0;
    gdk_keymap_translate_keyboard_state( Gdk::Display::get_default()->get_keymap(),
                                         key_event->hardware_keycode,
                                         (GdkModifierType)key_event->state,
                                         0, &key, nullptr, nullptr, nullptr );

    double x0, y0, x1, y1, x2, y2;
    triangle_corners(x0, y0, x1, y1, x2, y2);

    // Marker position
    double mx = x1 + (x2-x1) * _value + (x0-x2) * _saturation * _value;
    double my = y1 + (y2-y1) * _value + (y0-y2) * _saturation * _value;


    const double delta_hue = 2.0/360.0;

    switch (key) {

        case GDK_KEY_Up:
        case GDK_KEY_KP_Up:
            if (_focus_on_ring) {
                _hue += delta_hue;
            } else {
                my -= 1.0;
                set_from_xy(mx, my);
            }
            consumed = true;
            break;

        case GDK_KEY_Down:
        case GDK_KEY_KP_Down:
            if (_focus_on_ring) {
                _hue -= delta_hue;
            } else {
                my += 1.0;
                set_from_xy(mx, my);
            }
            consumed = true;
            break;

        case GDK_KEY_Left:
        case GDK_KEY_KP_Left:
            if (_focus_on_ring) {
                _hue += delta_hue;
            } else {
                mx -= 1.0;
                set_from_xy(mx, my);
            }
            consumed = true;
            break;

        case GDK_KEY_Right:
        case GDK_KEY_KP_Right:
            if (_focus_on_ring) {
                _hue -= delta_hue;
            } else {
                mx += 1.0;
                set_from_xy(mx, my);
            }
            consumed = true;
            break;

    }

    if (consumed) {
        if (_hue >= 1.0) _hue -= 1.0;
        if (_hue <  0.0) _hue += 1.0;
        _signal_color_changed.emit();
        queue_draw();
    }

    return consumed;
}

sigc::signal<void>
ColorWheel::signal_color_changed()
{
    return _signal_color_changed;
}

} // Namespace Inkscape
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

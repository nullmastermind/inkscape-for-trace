// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Ruler widget. Indicates horizontal or vertical position of a cursor in a specified widget.
 *
 * Copyright (C) 2019 Tavmjong Bah
 *
 * Rewrite of the 'C' ruler code which came originally from Gimp.
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include "ink-ruler.h"

#include <iostream>
#include <cmath>

#include "util/units.h"

struct SPRulerMetric
{
  gdouble ruler_scale[16];
  gint    subdivide[5];
};

// Ruler metric for general use.
static SPRulerMetric const ruler_metric_general = {
  { 1, 2, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000 },
  { 1, 5, 10, 50, 100 }
};

// Ruler metric for inch scales.
static SPRulerMetric const ruler_metric_inches = {
  { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768 },
  { 1, 2, 4, 8, 16 }
};

// Half width of pointer triangle.
static double half_width = 5.0;

namespace Inkscape {
namespace UI {
namespace Widget {

Ruler::Ruler(Gtk::Orientation orientation)
    : _orientation(orientation)
    , _backing_store(nullptr)
    , _lower(0)
    , _upper(1000)
    , _max_size(1000)
    , _unit(nullptr)
    , _backing_store_valid(false)
    , _rect()
    , _position(0)
{
    set_name("InkRuler");

    set_events(Gdk::POINTER_MOTION_MASK);

    signal_motion_notify_event().connect(sigc::mem_fun(*this, &Ruler::draw_marker_callback));
}

// Set display unit for ruler.
void
Ruler::set_unit(Inkscape::Util::Unit const *unit)
{
    if (_unit != unit) {
        _unit = unit;

        _backing_store_valid = false;
        queue_draw();
    }
}

// Set range for ruler, update ticks.
void
Ruler::set_range(const double& lower, const double& upper)
{
    if (_lower != lower || _upper != upper) {

        _lower = lower;
        _upper = upper;
        _max_size = _upper - _lower;
        if (_max_size == 0) {
            _max_size = 1;
        }

        _backing_store_valid = false;
        queue_draw();
    }
}

// Add a widget (i.e. canvas) to monitor. Note, we don't worry about removing this signal as
// our ruler is tied tightly to the canvas, if one is destroyed, so is the other.
void
Ruler::add_track_widget(Gtk::Widget& widget)
{
    widget.signal_motion_notify_event().connect(sigc::mem_fun(*this, &Ruler::draw_marker_callback), false); // false => connect first
}


// Draws marker in response to motion events from canvas.  Position is defined in ruler pixel
// coordinates. The routine assumes that the ruler is the same width (height) as the canvas. If
// not, one could use Gtk::Widget::translate_coordinates() to convert the coordinates.
bool
Ruler::draw_marker_callback(GdkEventMotion* motion_event)
{
    double position = 0;
    if (_orientation == Gtk::ORIENTATION_HORIZONTAL) {
        position = motion_event->x;
    } else {
        position = motion_event->y;
    }

    if (position != _position) {

        _position = position;

        // Find region to repaint (old and new marker positions).
        Cairo::RectangleInt new_rect = marker_rect();
        Cairo::RefPtr<Cairo::Region> region = Cairo::Region::create(new_rect);
        region->do_union(_rect);

        // Queue repaint
        queue_draw_region(region);

        _rect = new_rect;
    }

    return false;
}


// Find smallest dimension of ruler based on font size.
void
Ruler::size_request (Gtk::Requisition& requisition) const
{
    // Get border size
    Glib::RefPtr<Gtk::StyleContext> style_context = get_style_context();
    Gtk::Border border = style_context->get_border(get_state_flags());

    // Get font size
    Pango::FontDescription font = style_context->get_font(get_state_flags());
    int font_size = font.get_size();
    if (!font.get_size_is_absolute()) {
        font_size /= Pango::SCALE;
    }

    int size = 2 + font_size * 2.0; // Room for labels and ticks
 
    int width = border.get_left() + border.get_right();
    int height = border.get_top() + border.get_bottom();

    if (_orientation == Gtk::ORIENTATION_HORIZONTAL) {
        width += 1;
        height += size;
    } else {
        width += size;
        height += 1;
    }

    // Only valid for orientation in question (smallest dimension)!
    requisition.width = width;
    requisition.height = height;
}

void
Ruler::get_preferred_width_vfunc (int& minimum_width,  int& natural_width) const
{
    Gtk::Requisition requisition;
    size_request(requisition);
    minimum_width = natural_width = requisition.width;
} 	

void
Ruler::get_preferred_height_vfunc (int& minimum_height, int& natural_height) const
{
    Gtk::Requisition requisition;
    size_request(requisition);
    minimum_height = natural_height = requisition.height;
}

// Update backing store when scale changes.
// Note: in principle, there should not be a border (ruler ends should match canvas ends). If there
// is a border, we calculate tick position ignoring border width at ends of ruler but move the
// ticks and position marker inside the border.
bool
Ruler::draw_scale(const::Cairo::RefPtr<::Cairo::Context>& cr_in)
{

    // Get style information
    Glib::RefPtr<Gtk::StyleContext> style_context = get_style_context();
    Gtk::Border border = style_context->get_border(get_state_flags());
    Gdk::RGBA foreground = style_context->get_color(get_state_flags());

    Pango::FontDescription font = style_context->get_font(get_state_flags());
    int font_size = font.get_size();
    if (!font.get_size_is_absolute()) {
        font_size /= Pango::SCALE;
    }

    Gtk::Allocation allocation = get_allocation();
    int awidth  = allocation.get_width();
    int aheight = allocation.get_height();

    // if (allocation.get_x() != 0 || allocation.get_y() != 0) {
    //     std::cerr << "Ruler::draw_scale: maybe we do have to handle allocation x and y! "
    //               << " x: " << allocation.get_x() << " y: " << allocation.get_y() << std::endl;
    // }

    // Create backing store (need surface_in to get scale factor correct).
    Cairo::RefPtr<Cairo::Surface> surface_in = cr_in->get_target();
    _backing_store = Cairo::Surface::create(surface_in, Cairo::CONTENT_COLOR_ALPHA, awidth, aheight);

    // Get context
    Cairo::RefPtr<::Cairo::Context> cr = ::Cairo::Context::create(_backing_store);
    style_context->render_background(cr, 0, 0, awidth, aheight);

    cr->set_line_width(1.0);
    Gdk::Cairo::set_source_rgba(cr, foreground);

    // Ruler size (only smallest dimension used later).
    int rwidth  = awidth  - (border.get_left() + border.get_right());
    int rheight = aheight - (border.get_top()  + border.get_bottom());

    // Draw bottom/right line of ruler
    if (_orientation == Gtk::ORIENTATION_HORIZONTAL) {
        cr->rectangle(  0, aheight - border.get_bottom() - 1, awidth, 1);
    } else {
        cr->rectangle(  awidth - border.get_left() - 1, 0, 1, aheight);
        std::swap(awidth, aheight);
        std::swap(rwidth, rheight);
    }
    cr->fill();

    // From here on, awidth is the longest dimension of the ruler, rheight is the shortest.

    // Figure out scale. Largest ticks must be far enough apart to fit largest text in vertical ruler.
    // We actually require twice the distance.
    unsigned int scale = std::ceil (_max_size); // Largest number
    Glib::ustring scale_text = std::to_string(scale);
    unsigned int digits = scale_text.length() + 1; // Add one for negative sign.
    unsigned int minimum = digits * font_size * 2;

    double pixels_per_unit = awidth/_max_size; // pixel per distance

    SPRulerMetric ruler_metric = ruler_metric_general;
    if (_unit == Inkscape::Util::unit_table.getUnit("in")) {
        ruler_metric = ruler_metric_inches;
    }

    unsigned scale_index;
    for (scale_index = 0; scale_index < G_N_ELEMENTS (ruler_metric.ruler_scale)-1; ++scale_index) {
        if (ruler_metric.ruler_scale[scale_index] * std::abs (pixels_per_unit) > minimum) break;
    }

    // Now we find out what is the subdivide index for the closest ticks we can draw
    unsigned divide_index;
    for (divide_index = 0; divide_index < G_N_ELEMENTS (ruler_metric.subdivide)-1; ++divide_index) {
        if (ruler_metric.ruler_scale[scale_index] * std::abs (pixels_per_unit) < 5 * ruler_metric.subdivide[divide_index+1]) break;
    }

    // We'll loop over all ticks.
    double pixels_per_tick = pixels_per_unit *
        ruler_metric.ruler_scale[scale_index] / ruler_metric.subdivide[divide_index];

    double units_per_tick = pixels_per_tick/pixels_per_unit;
    double ticks_per_unit = 1.0/units_per_tick;

    // Find first and last ticks
    int start = 0;
    int end = 0;
    if (_lower < _upper) {
        start = std::floor (_lower * ticks_per_unit);
        end   = std::ceil  (_upper * ticks_per_unit);
    } else {
        start = std::floor (_upper * ticks_per_unit);
        end   = std::ceil  (_lower * ticks_per_unit);
    }

    // std::cout << " start: " << start
    //           << " end: " << end
    //           << " pixels_per_unit: " << pixels_per_unit
    //           << " pixels_per_tick: " << pixels_per_tick
    //           << std::endl;

    // Loop over all ticks
    for (int i = start; i < end+1; ++i) {

        // Position of tick (add 0.5 to center tick on pixel).
        double position = std::floor(i*pixels_per_tick - _lower*pixels_per_unit) + 0.5;

        // Height of tick
        int height = rheight;
        for (int j = divide_index; j > 0; --j) {
            if (i%ruler_metric.subdivide[j] == 0) break;
            height = height/2 + 1;
        }

        // Draw text for major ticks.
        if (i%ruler_metric.subdivide[divide_index] == 0) {

            int label_value = std::round(i*units_per_tick);
            Glib::ustring label = std::to_string(label_value);

            Glib::RefPtr<Pango::Layout> layout = create_pango_layout("");
            layout->set_font_description(font);

            if (_orientation == Gtk::ORIENTATION_HORIZONTAL) {
                layout->set_text(label);
                cr->move_to (position+2, border.get_top());
                layout->show_in_cairo_context(cr);
            } else {
                cr->move_to (border.get_left(), position);
                int n = 0;
                for (char const &c : label) {
                    std::string s(1, c);
                    layout->set_text(s);
                    int text_width;
                    int text_height;
                    layout->get_pixel_size(text_width, text_height);
                    cr->move_to(border.get_left() + (aheight-text_width)/2.0 - 1,
                                position + n*0.7*text_height - 1);
                    layout->show_in_cairo_context(cr);
                    ++n;
                }
                // Glyphs are not centered in vertical text... should specify fixed width numbers.
                // Glib::RefPtr<Pango::Context> context = layout->get_context();
                // if (_orientation == Gtk::ORIENTATION_VERTICAL) {
                //     context->set_base_gravity(Pango::GRAVITY_EAST);
                //     context->set_gravity_hint(Pango::GRAVITY_HINT_STRONG);
                //     cr->move_to(...)
                //     cr->save();
                //     cr->rotate(M_PI_2);
                //     layout->show_in_cairo_context(cr);
                //     cr->restore();
                // }
            }
        }

        // Draw ticks
        if (_orientation == Gtk::ORIENTATION_HORIZONTAL) {
            cr->move_to(position, rheight + border.get_top() - height);
            cr->line_to(position, rheight + border.get_top());
        } else {
            cr->move_to(rheight + border.get_left() - height, position);
            cr->line_to(rheight + border.get_left(),          position);
        }
        cr->stroke();
    }

    _backing_store_valid = true;

    return true;
}

// Draw position marker, we use doubles here.
void
Ruler::draw_marker(const Cairo::RefPtr<::Cairo::Context>& cr)
{

    // Get style information
    Glib::RefPtr<Gtk::StyleContext> style_context = get_style_context();
    Gtk::Border border = style_context->get_border(get_state_flags());
    Gdk::RGBA foreground = style_context->get_color(get_state_flags());

    Gtk::Allocation allocation = get_allocation();
    const int awidth  = allocation.get_width();
    const int aheight = allocation.get_height();

    // Temp (to verify our redraw rectangle encloses position marker).
    // Cairo::RectangleInt rect = marker_rect();
    // cr->set_source_rgb(0, 1.0, 0);
    // cr->rectangle (rect.x, rect.y, rect.width, rect.height);
    // cr->fill();

    Gdk::Cairo::set_source_rgba(cr, foreground);
    if (_orientation == Gtk::ORIENTATION_HORIZONTAL) {
        double offset = aheight - border.get_bottom();
        cr->move_to(_position,              offset);
        cr->line_to(_position - half_width, offset - half_width);
        cr->line_to(_position + half_width, offset - half_width);
        cr->close_path();
     } else {
        double offset = awidth - border.get_right();
        cr->move_to(offset,              _position);
        cr->line_to(offset - half_width, _position - half_width);
        cr->line_to(offset - half_width, _position + half_width);
        cr->close_path();
    }
    cr->fill();
}

// This is a pixel aligned integer rectangle that encloses the position marker. Used to define the
// redraw area.
Cairo::RectangleInt
Ruler::marker_rect()
{
    // Get border size
    Glib::RefPtr<Gtk::StyleContext> style_context = get_style_context();
    Gtk::Border border = style_context->get_border(get_state_flags());

    Gtk::Allocation allocation = get_allocation();
    const int awidth  = allocation.get_width();
    const int aheight = allocation.get_height();

    int rwidth  = awidth  - border.get_left() - border.get_right();
    int rheight = aheight - border.get_top()  - border.get_bottom();

    Cairo::RectangleInt rect;
    rect.x = 0;
    rect.y = 0;
    rect.width = 0;
    rect.height = 0;

    // Find size of rectangle to enclose triangle.
    if (_orientation == Gtk::ORIENTATION_HORIZONTAL) {
        rect.x = std::floor(_position - half_width);
        rect.y = std::floor(border.get_top() + rheight - half_width);
        rect.width  = std::ceil(half_width * 2.0 + 1);
        rect.height = std::ceil(half_width);
    } else {
        rect.x = std::floor(border.get_left() + rwidth - half_width);
        rect.y = std::floor(_position - half_width);
        rect.width  = std::ceil(half_width);
        rect.height = std::ceil(half_width * 2.0 + 1);
    }

    return rect;
}

// Draw the ruler using the tick backing store.
bool
Ruler::on_draw(const::Cairo::RefPtr<::Cairo::Context>& cr) {

    if (!_backing_store_valid) {
        draw_scale (cr);
    }

    cr->set_source (_backing_store, 0, 0);
    cr->paint();

    draw_marker (cr);

    return true;
}

// Update ruler on style change (font-size, etc.)
void
Ruler::on_style_updated() {

    Gtk::DrawingArea::on_style_updated();

    _backing_store_valid = false; // If font-size changed we need to regenerate store.

    queue_resize();
    queue_draw();
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

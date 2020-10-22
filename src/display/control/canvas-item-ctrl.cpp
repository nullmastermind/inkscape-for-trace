// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * A class to represent a control node.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of SPCtrl
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/transforms.h>

#include "canvas-item-ctrl.h"

#include "preferences.h"         // Default size. 
#include "display/cairo-utils.h" // argb32_from_rgba()

#include "ui/widget/canvas.h"

namespace Inkscape {

CanvasItemCtrl::~CanvasItemCtrl()
{
    delete[] _cache;
}

/**
 * Create an null control node.
 */
CanvasItemCtrl::CanvasItemCtrl(CanvasItemGroup *group)
    : CanvasItem(group)
{
    _name = "CanvasItemCtrl:Null";
    _pickable = true; // Everybody gets events from this class!
}

/**
 * Create a control ctrl. Shape auto-set by type.
 */
CanvasItemCtrl::CanvasItemCtrl(CanvasItemGroup *group, Inkscape::CanvasItemCtrlType type)
    : CanvasItem(group)
    , _type(type)
{
    _name = "CanvasItemCtrl:Type_" + std::to_string(_type);
    _pickable = true; // Everybody gets events from this class!

    // Use _type to set default values:
    set_shape_default();
    set_size_default();
}

/**
 * Create a control ctrl. Point is in document coordinates.
 */
CanvasItemCtrl::CanvasItemCtrl(CanvasItemGroup *group, Inkscape::CanvasItemCtrlType type, Geom::Point const &p)
    : CanvasItemCtrl(group, type)
{
    _position = p;
}


/**
 * Create a control ctrl.
 */
CanvasItemCtrl::CanvasItemCtrl(CanvasItemGroup *group, Inkscape::CanvasItemCtrlShape shape)
    : CanvasItem(group)
    , _shape(shape)
    , _type(CANVAS_ITEM_CTRL_TYPE_DEFAULT)
{
    _name = "CanvasItemCtrl:Shape_" + std::to_string(_shape);
    _pickable = true; // Everybody gets events from this class!
}


/**
 * Create a control ctrl. Point is in document coordinates.
 */
CanvasItemCtrl::CanvasItemCtrl(CanvasItemGroup *group, Inkscape::CanvasItemCtrlShape shape, Geom::Point const &p)
    : CanvasItemCtrl(group, shape)
{
    _position = p;
    request_update();
}

/**
 * Set the postion. Point is in document coordinates.
 */
void CanvasItemCtrl::set_position(Geom::Point const &position)
{
    // std::cout << "CanvasItemCtrl::set_ctrl: " << _name << ": " << position << std::endl;
    if (_position != position) {
        _position = position;
        request_update();
    }
}

/**
 * Returns distance between point in canvas units and position of ctrl.
 */
double CanvasItemCtrl::closest_distance_to(Geom::Point const &p)
{
    // TODO: Different criteria for different shapes.
    Geom::Point position = _position * _affine;
    return Geom::distance(p, position);
}

/**
 * If tolerance is zero, returns true if point p (in canvas units) is inside bounding box,
 * else returns true if p (in canvas units) is within tolerance (canvas units) distance of ctrl.
 * The latter assumes ctrl center anchored.
 */
bool CanvasItemCtrl::contains(Geom::Point const &p, double tolerance)
{
    // TODO: Different criteria for different shapes.
    if (tolerance == 0) {
        return _bounds.interiorContains(p);
    } else {
        return closest_distance_to(p) <= tolerance;
    }
}

/**
 * Update and redraw control ctrl.
 */
void CanvasItemCtrl::update(Geom::Affine const &affine)
{
    if (_affine == affine && !_need_update) {
        // Nothing to do.
        return;
    }

    // Queue redraw of old area (erase previous content).
    _canvas->redraw_area(_bounds);

    // Get new bounds
    _affine = affine;

    // We must be pixel aligned, width and height are always odd.
    _bounds = Geom::Rect::from_xywh(-(_width/2.0 - 0.5), -(_height/2.0 - 0.5), _width, _height);

    // Adjust for anchor
    int w_half = _width/2;
    int h_half = _height/2;
    int dx = 0;
    int dy = 0;

    switch (_shape) {
        case CANVAS_ITEM_CTRL_SHAPE_DARROW:
        case CANVAS_ITEM_CTRL_SHAPE_SARROW:
        case CANVAS_ITEM_CTRL_SHAPE_CARROW:
        case CANVAS_ITEM_CTRL_SHAPE_SALIGN:
        case CANVAS_ITEM_CTRL_SHAPE_CALIGN:
        {

            _angle = _anchor * M_PI/4.0 + std::atan2(_affine[1], _affine[0]);

            double half = _width/2.0;

            dx = - (half + 2) * cos(_angle); // Add a bit to prevent tip from overlapping due to rounding errors.
            dy = - (half + 2) * sin(_angle);

            switch (_shape) {

                case CANVAS_ITEM_CTRL_SHAPE_CARROW:
                    _angle += 5 * M_PI/4.0;
                    break;

                case CANVAS_ITEM_CTRL_SHAPE_SARROW:
                    _angle += M_PI/2.0;
                    break;

                case CANVAS_ITEM_CTRL_SHAPE_SALIGN:
                    dx = - (half/2 + 2) * cos(_angle);
                    dy = - (half/2 + 2) * sin(_angle);
                    _angle -= M_PI/2.0;
                    break;

                case CANVAS_ITEM_CTRL_SHAPE_CALIGN:
                    _angle -= M_PI/4.0;
                    dx = (half/2 + 2) * ( sin(_angle) - cos(_angle));
                    dy = (half/2 + 2) * (-sin(_angle) - cos(_angle));
                    break;

                default:
                    break;
            }

            _built = false; // Angle may have change, must rebuild!

            break;
        }

        case CANVAS_ITEM_CTRL_SHAPE_PIVOT:
        case CANVAS_ITEM_CTRL_SHAPE_MALIGN:

            _angle = std::atan2(_affine[1], _affine[0]);

            _built = false; // Angle may have change, must rebuild!

            break;

        default:

            switch (_anchor) {
                case SP_ANCHOR_N:
                case SP_ANCHOR_CENTER:
                case SP_ANCHOR_S:
                    break;

                case SP_ANCHOR_NW:
                case SP_ANCHOR_W:
                case SP_ANCHOR_SW:
                    dx = w_half;
                    break;

                case SP_ANCHOR_NE:
                case SP_ANCHOR_E:
                case SP_ANCHOR_SE:
                    dx = -w_half;
                    break;
            }

            switch (_anchor) {
                case SP_ANCHOR_W:
                case SP_ANCHOR_CENTER:
                case SP_ANCHOR_E:
                    break;

                case SP_ANCHOR_NW:
                case SP_ANCHOR_N:
                case SP_ANCHOR_NE:
                    dy = h_half;
                    break;

                case SP_ANCHOR_SW:
                case SP_ANCHOR_S:
                case SP_ANCHOR_SE:
                    dy = -h_half;
                    break;
            }
            break;
    }

    _bounds *= Geom::Translate(Geom::IntPoint(dx, dy));

    // Position must also be integer.
    Geom::Point position = _position * _affine;
    Geom::IntPoint iposition(position.x(), position.y());

    _bounds *= Geom::Translate(iposition);

    // Queue redraw of new area
    _canvas->redraw_area(_bounds);

    _need_update = false;
}


static inline guint32 compose_xor(guint32 bg, guint32 fg, guint32 a)
{
    guint32 c = bg * (255-a) + (((bg ^ ~fg) + (bg >> 2) - (bg > 127 ? 63 : 0)) & 255) * a;
    return (c + 127) / 255;
}

/**
 * Render ctrl to screen via Cairo.
 */
void CanvasItemCtrl::render(Inkscape::CanvasItemBuffer *buf)
{
    if (!buf) {
        std::cerr << "CanvasItemCtrl::Render: No buffer!" << std::endl;
         return;
    }

    if (!_bounds.intersects(buf->rect)) {
        return; // Control not inside buffer rectangle.
    }

    if (!_visible) {
        return; // Hidden.
    }

    if (!_built) {
        build_cache(buf->device_scale);
    }

    Geom::Point c = _bounds.min() - buf->rect.min();
    int x = c.x(); // Must be pixel aligned.
    int y = c.y();

    buf->cr->save();

    if (_mode == CANVAS_ITEM_CTRL_MODE_XOR) {
        // This code works regardless of source type.

        // 1. Copy the affected part of output to a temporary surface

        // Size in device pixels. Does not set device scale.
        int width  = _width  * buf->device_scale;
        int height = _height * buf->device_scale;
        auto work = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, width, height);
        cairo_surface_set_device_scale(work->cobj(), buf->device_scale, buf->device_scale); // No C++ API!

        auto cr = Cairo::Context::create(work);
        cr->translate(-_bounds.left(), -_bounds.top());
        cr->set_source(buf->cr->get_target(), buf->rect.left(), buf->rect.top());
        cr->paint();
        // static int a = 0;
        // std::string name0 = "ctrl0_" + _name + "_" + std::to_string(a++) + ".png";
        // work->write_to_png(name0);

        // 2. Composite the control on a temporary surface
        work->flush();
        int strideb = work->get_stride();
        unsigned char *pxb = work->get_data();
        guint32 *p = _cache;
        for (int i = 0; i < height; ++i) {
            guint32 *pb = reinterpret_cast<guint32*>(pxb + i*strideb);
            for (int j = 0; j < width; ++j) {
                guint32 cc = *p++;
                guint32 ac = cc & 0xff;
                if (ac == 0 && cc != 0) {
                    *pb++ = argb32_from_rgba(cc | 0x000000ff);
                } else {
                    EXTRACT_ARGB32(*pb, ab,rb,gb,bb)
                    guint32 ro = compose_xor(rb, (cc & 0xff000000) >> 24, ac);
                    guint32 go = compose_xor(gb, (cc & 0x00ff0000) >> 16, ac);
                    guint32 bo = compose_xor(bb, (cc & 0x0000ff00) >>  8, ac);
                    ASSEMBLE_ARGB32(px, ab,ro,go,bo)
                    *pb++ = px;
                }
            }
        }
        work->mark_dirty();
        // std::string name1 = "ctrl1_" + _name + "_" + std::to_string(a) + ".png";
        // work->write_to_png(name1);

        // 3. Replace the affected part of output with contents of temporary surface
        buf->cr->set_source(work, x, y);

    } else {
        // Create surface with cached bitmap and set it as source.
        auto cache = Cairo::ImageSurface::create(reinterpret_cast<unsigned char*>(_cache),
                                                 Cairo::FORMAT_ARGB32, _width, _height, _width*4);
        cairo_surface_set_device_scale(cache->cobj(), buf->device_scale, buf->device_scale); // No C++ API!
        cache->mark_dirty();
        buf->cr->save();
        buf->cr->set_source(cache, x, y);
    }

    buf->cr->rectangle(x, y, _width, _height);
    buf->cr->clip();
    buf->cr->set_operator(Cairo::OPERATOR_SOURCE);
    buf->cr->paint();   
    buf->cr->restore();
}

void CanvasItemCtrl::set_fill(guint32 rgba)
{
    if (_fill != rgba) {
        _fill = rgba;
        _built = false;
        _canvas->redraw_area(_bounds);
    }
}

void CanvasItemCtrl::set_stroke(guint32 rgba)
{
    if (_stroke != rgba) {
        _stroke = rgba;
        _built = false;
        _canvas->redraw_area(_bounds);
    }
}

void CanvasItemCtrl::set_shape(int shape)
{
    if (_shape != shape) {
        _shape = Inkscape::CanvasItemCtrlShape(shape); // Fixme
        _built = false;
        request_update(); // Geometry could change
    }
}

void CanvasItemCtrl::set_shape_default()
{
    switch (_type) {
        case CANVAS_ITEM_CTRL_TYPE_ADJ_HANDLE:
            _shape = CANVAS_ITEM_CTRL_SHAPE_DARROW;
            break;

        case CANVAS_ITEM_CTRL_TYPE_ADJ_SKEW:
            _shape = CANVAS_ITEM_CTRL_SHAPE_SARROW;
            break;

        case CANVAS_ITEM_CTRL_TYPE_ADJ_ROTATE:
            _shape = CANVAS_ITEM_CTRL_SHAPE_CARROW;
            break;

        case CANVAS_ITEM_CTRL_TYPE_ADJ_CENTER:
            _shape = CANVAS_ITEM_CTRL_SHAPE_PIVOT;
            break;

        case CANVAS_ITEM_CTRL_TYPE_ADJ_SALIGN:
            _shape = CANVAS_ITEM_CTRL_SHAPE_SALIGN;
            break;

        case CANVAS_ITEM_CTRL_TYPE_ADJ_CALIGN:
            _shape = CANVAS_ITEM_CTRL_SHAPE_CALIGN;
            break;

        case CANVAS_ITEM_CTRL_TYPE_ADJ_MALIGN:
            _shape = CANVAS_ITEM_CTRL_SHAPE_MALIGN;
            break;

        case CANVAS_ITEM_CTRL_TYPE_NODE_AUTO:
        case CANVAS_ITEM_CTRL_TYPE_ROTATE:
            _shape = CANVAS_ITEM_CTRL_SHAPE_CIRCLE;
            break;

        case CANVAS_ITEM_CTRL_TYPE_CENTER:
            _shape = CANVAS_ITEM_CTRL_SHAPE_PLUS;
            break;

        case CANVAS_ITEM_CTRL_TYPE_SHAPER:
        case CANVAS_ITEM_CTRL_TYPE_LPE:
        case CANVAS_ITEM_CTRL_TYPE_NODE_CUSP:
            _shape = CANVAS_ITEM_CTRL_SHAPE_DIAMOND;
            break;

        case CANVAS_ITEM_CTRL_TYPE_POINT:
            _shape = CANVAS_ITEM_CTRL_SHAPE_CROSS;
            break;

        default:
            _shape = CANVAS_ITEM_CTRL_SHAPE_SQUARE;
    }
}

void CanvasItemCtrl::set_mode(int mode)
{
    if (_mode != mode) {
        _mode = Inkscape::CanvasItemCtrlMode(mode); // Fixme
        _built = false;
        request_update();
    }
}

void CanvasItemCtrl::set_pixbuf(GdkPixbuf *pixbuf)
{
    if (_pixbuf != pixbuf) {
        _pixbuf = pixbuf;
        _width = gdk_pixbuf_get_width(pixbuf);
        _height = gdk_pixbuf_get_height(pixbuf);
        _built = false;
        request_update();
    }
}

// Nominally width == height == size except possibly for pixmaps.
void CanvasItemCtrl::set_size(int size)
{
    if (_pixbuf) {
        // std::cerr << "CanvasItemCtrl::set_size: Attempting to set size on pixbuf control!" << std::endl;
        return;
    }
    if (_width != size + _extra || _height != size + _extra) {
        _width  = size + _extra;
        _height = size + _extra;
        _built = false;
        request_update(); // Geometry change
    }
}

void CanvasItemCtrl::set_size_via_index(int size_index)
{
    // Size must always be an odd number to center on pixel.

    if (size_index < 1 || size_index > 15) {
        std::cerr << "CanvasItemCtrl::set_size_via_index: size_index out of range!" << std::endl;
        size_index = 3;
    }

    int size = 0;
    switch (_type) {
        case CANVAS_ITEM_CTRL_TYPE_ADJ_HANDLE:
        case CANVAS_ITEM_CTRL_TYPE_ADJ_SKEW:
            size = size_index * 2 + 7;
            break;

        case CANVAS_ITEM_CTRL_TYPE_ADJ_ROTATE:
        case CANVAS_ITEM_CTRL_TYPE_ADJ_CENTER:
            size = size_index * 2 + 9; // 2 larger than HANDLE/SKEW
            break;

        case CANVAS_ITEM_CTRL_TYPE_ADJ_SALIGN:
        case CANVAS_ITEM_CTRL_TYPE_ADJ_CALIGN:
        case CANVAS_ITEM_CTRL_TYPE_ADJ_MALIGN:
            size = size_index * 4 + 5; // Needs to be larger to allow for rotating.
            break;

        case CANVAS_ITEM_CTRL_TYPE_POINT:
        case CANVAS_ITEM_CTRL_TYPE_ROTATE:
        case CANVAS_ITEM_CTRL_TYPE_CENTER:
        case CANVAS_ITEM_CTRL_TYPE_SIZER:
        case CANVAS_ITEM_CTRL_TYPE_SHAPER:
        case CANVAS_ITEM_CTRL_TYPE_LPE:
        case CANVAS_ITEM_CTRL_TYPE_NODE_AUTO:
        case CANVAS_ITEM_CTRL_TYPE_NODE_CUSP:
            size = size_index * 2 + 3;
            break;

        case CANVAS_ITEM_CTRL_TYPE_INVISIPOINT:
            size = 1;
            break;

        case CANVAS_ITEM_CTRL_TYPE_DEFAULT:
        default:
            size = size_index * 2 + 1;
    }

    set_size(size);
}

void CanvasItemCtrl::set_size_default()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int size = prefs->getIntLimited("/options/grabsize/value", 3, 1, 15);
    set_size_via_index(size);
}

void CanvasItemCtrl::set_size_extra(int extra)
{
    if (_extra != extra && _pixbuf == nullptr) { // Don't enlarge pixbuf!
        _width  += (extra - _extra);
        _height += (extra - _extra);
        _extra = extra;
        _built = false;
        request_update(); // Geometry change
    }
}

void CanvasItemCtrl::set_type(Inkscape::CanvasItemCtrlType type)
{
    if (_type != type) {
        _type = type;

        // Use _type to set default values:
        set_shape_default();
        set_size_default();
        _built = false;
        request_update(); // Possible Geometry change
    }
}

void CanvasItemCtrl::set_angle(double angle)
{
    if (_angle != angle) {
        _angle = angle;
        request_update(); // Geometry change
    }
}

void CanvasItemCtrl::set_anchor(SPAnchorType anchor)
{
    if (_anchor != anchor) {
        _anchor = anchor;
        request_update(); // Geometry change
    }
}

// ---------- Protected ----------

// Helper function for build_cache():
bool point_inside_triangle(Geom::Point p1,Geom::Point p2,Geom::Point p3, Geom::Point point){
    using Geom::X;
    using Geom::Y;
    double denominator = (p1[X]*(p2[Y] - p3[Y]) + p1[Y]*(p3[X] - p2[X]) + p2[X]*p3[Y] - p2[Y]*p3[X]);
    double t1 = (point[X]*(p3[Y] - p1[Y]) + point[Y]*(p1[X] - p3[X]) - p1[X]*p3[Y] + p1[Y]*p3[X]) / denominator;
    double t2 = (point[X]*(p2[Y] - p1[Y]) + point[Y]*(p1[X] - p2[X]) - p1[X]*p2[Y] + p1[Y]*p2[X]) / -denominator;
    double see = t1 + t2;
    return 0 <= t1 && t1 <= 1 && 0 <= t2 && t2 <= 1 && see <= 1;
}


void draw_darrow(Cairo::RefPtr<Cairo::Context>cr, double size) {

    // Find points, starting from tip of one arrowhead, working clockwise.
    /*   1        4
        ╱│        │╲
       ╱ └────────┘ ╲
     0╱  2        3  ╲5
      ╲  8        7  ╱
       ╲ ┌────────┐ ╱
        ╲│9      6│╱
    */

    // Length of arrowhead (not including stroke).
    double delta = (size-1)/4.0; // Use unscaled width.

    // Tip of arrow (0)
    double tip_x = 0.5;          // At edge, allow room for stroke.
    double tip_y = size/2.0;     // Center, assuming width == height.

    // Outer corner (1)
    double out_x = tip_x + delta;
    double out_y = tip_y - delta;

    // Inner corner (2)
    double in_x = out_x;
    double in_y = out_y + (delta/2.0);

    double x0 = tip_x;                double y0 = tip_y;
    double x1 = out_x;                double y1 = out_y;
    double x2 = in_x;                 double y2 = in_y;
    double x3 = size - in_x;          double y3 = in_y;
    double x4 = size - out_x;         double y4 = out_y;
    double x5 = size - tip_x;         double y5 = tip_y;
    double x6 = size - out_x;         double y6 = size - out_y;
    double x7 = size - in_x;          double y7 = size - in_y;
    double x8 = in_x;                 double y8 = size - in_y;
    double x9 = out_x;                double y9 = size - out_y;

    // Draw arrow
    cr->move_to(x0, y0);
    cr->line_to(x1, y1);
    cr->line_to(x2, y2);
    cr->line_to(x3, y3);
    cr->line_to(x4, y4);
    cr->line_to(x5, y5);
    cr->line_to(x6, y6);
    cr->line_to(x7, y7);
    cr->line_to(x8, y8);
    cr->line_to(x9, y9);
    cr->close_path();
}

void draw_carrow(Cairo::RefPtr<Cairo::Context>cr, double size) {

    // Length of arrowhead (not including stroke).
    double delta = (size-3)/4.0; // Use unscaled width.

    // Tip of arrow
    double tip_x =         1.5;  // Edge, allow room for stroke when rotated.
    double tip_y = delta + 1.5;

    // Outer corner (1)
    double out_x = tip_x + delta;
    double out_y = tip_y - delta;

    // Inner corner (2)
    double in_x = out_x;
    double in_y = out_y + (delta/2.0);

    double x0 = tip_x;                double y0 = tip_y;
    double x1 = out_x;                double y1 = out_y;
    double x2 = in_x;                 double y2 = in_y;
    double x3 = size - in_y;        //double y3 = size - in_x;
    double x4 = size - out_y;         double y4 = size - out_x;
    double x5 = size - tip_y;         double y5 = size - tip_x;
    double x6 = x5 - delta;           double y6 = y4;
    double x7 = x5 - delta/2.0;       double y7 = y4;
    double x8 = x1;                 //double y8 = y0 + delta/2.0;
    double x9 = x1;                   double y9 = y0 + delta;

    // Draw arrow
    cr->move_to(x0, y0);
    cr->line_to(x1, y1);
    cr->line_to(x2, y2);
    cr->arc(x1, y4, x3-x2, 3.0*M_PI/2.0, 0);
    cr->line_to(x4, y4);
    cr->line_to(x5, y5);
    cr->line_to(x6, y6);
    cr->line_to(x7, y7);
    cr->arc_negative(x1, y4, x7-x8, 0, 3.0*M_PI/2.0);
    cr->line_to(x9, y9);
    cr->close_path();
}

void draw_pivot(Cairo::RefPtr<Cairo::Context>cr, double size) {

    double delta4 = (size-5)/4.0; // Keep away from edge or will clip when rotating.
    double delta8 = delta4/2;

    // Line start
    double center = size/2.0;

    cr->move_to (center - delta8, center - 2*delta4 - delta8);
    cr->rel_line_to ( delta4,  0     );
    cr->rel_line_to ( 0,       delta4);

    cr->rel_line_to ( delta4,  delta4);

    cr->rel_line_to ( delta4,  0     );
    cr->rel_line_to ( 0,       delta4);
    cr->rel_line_to (-delta4,  0     );

    cr->rel_line_to (-delta4,  delta4);

    cr->rel_line_to ( 0,       delta4);
    cr->rel_line_to (-delta4,  0     );
    cr->rel_line_to ( 0,      -delta4);

    cr->rel_line_to (-delta4, -delta4);

    cr->rel_line_to (-delta4,  0     );
    cr->rel_line_to ( 0,      -delta4);
    cr->rel_line_to ( delta4,  0     );

    cr->rel_line_to ( delta4, -delta4);
    cr->close_path();

    cr->begin_new_sub_path();
    cr->arc_negative(center, center, delta4, 0, -2 * M_PI);
}

void draw_salign(Cairo::RefPtr<Cairo::Context>cr, double size) {

    // Triangle pointing at line.

    // Basic units.
    double delta4 = (size-1)/4.0; // Use unscaled width.
    double delta8 = delta4/2;
    if (delta8 < 2) {
        // Keep a minimum gap of at least one pixel (after stroking).
        delta8 = 2;
    }

    // Tip of triangle
    double tip_x = size/2.0; // Center (also rotation point).
    double tip_y = size/2.0;

    // Corner triangle position.
    double outer = size/2.0 - delta4;

    // Outer line position
    double oline = size/2.0 + (int)delta4;

    // Inner line position
    double iline = size/2.0 + (int)delta8;

    // Draw triangle
    cr->move_to(tip_x,           tip_y);
    cr->line_to(outer,           outer);
    cr->line_to(size - outer,    outer);
    cr->close_path();

    // Draw line
    cr->move_to(outer,           iline);
    cr->line_to(size - outer,    iline);
    cr->line_to(size - outer,    oline);
    cr->line_to(outer,           oline);
    cr->close_path();
}

void draw_calign(Cairo::RefPtr<Cairo::Context>cr, double size) {

    // Basic units.
    double delta4 = (size-1)/4.0; // Use unscaled width.
    double delta8 = delta4/2;
    if (delta8 < 2) {
        // Keep a minimum gap of at least one pixel (after stroking).
        delta8 = 2;
    }

    // Tip of triangle
    double tip_x = size/2.0; // Center (also rotation point).
    double tip_y = size/2.0;

    // Corner triangle position.
    double outer = size/2.0 - delta8 - delta4;

    // End of line positin
    double eline = size/2.0 - delta8;

    // Outer line position
    double oline = size/2.0 + (int)delta4;

    // Inner line position
    double iline = size/2.0 + (int)delta8;

    // Draw triangle
    cr->move_to(tip_x,           tip_y);
    cr->line_to(outer,           tip_y);
    cr->line_to(tip_x,           outer);
    cr->close_path();

    // Draw line
    cr->move_to(iline,           iline);
    cr->line_to(iline,           eline);
    cr->line_to(oline,           eline);
    cr->line_to(oline,           oline);
    cr->line_to(eline,           oline);
    cr->line_to(eline,           iline);
    cr->close_path();
}

void draw_malign(Cairo::RefPtr<Cairo::Context>cr, double size) {

    // Basic units.
    double delta4 = (size-1)/4.0; // Use unscaled width.
    double delta8 = delta4/2;
    if (delta8 < 2) {
        // Keep a minimum gap of at least one pixel (after stroking).
        delta8 = 2;
    }

    // Tip of triangle
    double tip_0 = size/2.0;
    double tip_1 = size/2.0 - delta8;

    // Draw triangles
    cr->move_to(tip_0,           tip_1);
    cr->line_to(tip_0 - delta4,  tip_1 - delta4);
    cr->line_to(tip_0 + delta4,  tip_1 - delta4);
    cr->close_path();

    cr->move_to(size - tip_1,           tip_0);
    cr->line_to(size - tip_1 + delta4,  tip_0 - delta4);
    cr->line_to(size - tip_1 + delta4,  tip_0 + delta4);
    cr->close_path();

    cr->move_to(size - tip_0,           size - tip_1);
    cr->line_to(size - tip_0 + delta4,  size - tip_1 + delta4);
    cr->line_to(size - tip_0 - delta4,  size - tip_1 + delta4);
    cr->close_path();

    cr->move_to(tip_1,           tip_0);
    cr->line_to(tip_1 - delta4,  tip_0 + delta4);
    cr->line_to(tip_1 - delta4,  tip_0 - delta4);
    cr->close_path();

}

void CanvasItemCtrl::build_cache(int device_scale)
{
    if (_width < 2 || _height < 2) {
        return; // Nothing to render
    }

    // Get colors
    guint32 fill = 0x0;
    guint32 stroke = 0x0;
    if (_mode == CANVAS_ITEM_CTRL_MODE_XOR) {
        fill   = _fill;
        stroke = _stroke;
    } else {
        fill   = argb32_from_rgba(_fill);
        stroke = argb32_from_rgba(_stroke);
    }

    if (_shape != CANVAS_ITEM_CTRL_SHAPE_BITMAP) {
        if (_width % 2 == 0 || _height % 2 == 0) {
            std::cerr << "CanvasItemCtrl::build_cache: Width and/or height not odd integer! "
                      << _name << ":  width: " << _width << "  height: " << _height << std::endl;
        }
    }

    // Get memory for cache.
    int width = _width  * device_scale;  // Not unsigned or math errors occur!
    int height = _height * device_scale;
    int size = width * height;

    if (_cache) delete[] _cache;
    _cache = new guint32[size];
    guint32 *p = _cache;

    switch (_shape) {

        case CANVAS_ITEM_CTRL_SHAPE_SQUARE:
            // Actually any rectanglular shape.
            for (int i = 0; i < width; ++i) {
                for (int j = 0; j < width; ++j) {
                    if (i + 1 > device_scale && device_scale < width  - i  &&
                        j + 1 > device_scale && device_scale < height - j) {
                        *p++ = fill;
                    } else {
                        *p++ = stroke;
                    }
                }
            }
            _built = true;
            break;

        case CANVAS_ITEM_CTRL_SHAPE_DIAMOND:
        {   // Assume width == height.
            int m = (width+1)/2;

            for (int i = 0; i < width; ++i) {
                for (int j = 0; j < height; ++j) {
                    if (          i  +           j  > m-1+device_scale &&
                         (width-1-i) +           j  > m-1+device_scale &&
                         (width-1-i) + (height-1-j) > m-1+device_scale &&
                                i    + (height-1-j) > m-1+device_scale ) {
                        *p++ = fill;
                    } else
                    if (          i  +           j  > m-2 &&
                         (width-1-i) +           j  > m-2 &&
                         (width-1-i) + (height-1-j) > m-2 &&
                                i    + (height-1-j) > m-2 ) {
                        *p++ = stroke;
                    } else {
                        *p++ = 0;
                    }
                }
            }
            _built = true;
            break;
        }

        case CANVAS_ITEM_CTRL_SHAPE_CIRCLE:
        {   // Assume width == height.
            double rs  = width/2.0;
            double rs2 = rs*rs;
            double rf  = rs-device_scale;
            double rf2 = rf*rf;

            for (int i = 0; i < width; ++i) {
                for (int j = 0; j < height; ++j) {

                    double rx = i - (width /2.0) + 0.5;
                    double ry = j - (height/2.0) + 0.5;
                    double r2 = rx*rx + ry*ry;

                    if (r2 < rf2) {
                        *p++ = fill;
                    } else if (r2 < rs2) {
                        *p++ = stroke;
                    } else {
                        *p++ = 0;
                    }
                }
            }
            _built = true;
            break;
        }

        case CANVAS_ITEM_CTRL_SHAPE_TRIANGLE:
        {
            Geom::Affine m = Geom::Translate(Geom::Point(-width/2.0,-height/2.0));
            m *= Geom::Rotate(-_angle);
            m *= Geom::Translate(Geom::Point(width/2.0, height/2.0));

            // Construct an arrowhead (triangle) of maximum size that won't leak out of rectangle
            // defined by width and height, assuming width == height.
            double w2 = width/2.0;
            double h2 = height/2.0;
            double w2cos = w2 * cos( M_PI/6 );
            double h2sin = h2 * sin( M_PI/6 );
            Geom::Point p1s(0, h2);
            Geom::Point p2s(w2 + w2cos, h2 + h2sin);
            Geom::Point p3s(w2 + w2cos, h2 - h2sin);

            // Needed for constructing smaller arrowhead below.
            double theta = atan2( Geom::Point( p2s - p1s ) );

            p1s *= m;
            p2s *= m;
            p3s *= m;

            // Construct a smaller arrow head for fill.
            Geom::Point p1f(device_scale/sin(theta), h2);
            Geom::Point p2f(w2 + w2cos, h2 - h2sin + device_scale/cos(theta));
            Geom::Point p3f(w2 + w2cos, h2 + h2sin - device_scale/cos(theta));
            p1f *= m;
            p2f *= m;
            p3f *= m;

            for(int y = 0; y < height; y++) {
                for(int x = 0; x < width; x++) {
                    Geom::Point point = Geom::Point(x+0.5, y+0.5);

                    if (point_inside_triangle(p1f, p2f, p3f, point)) {
                        p[(y*width)+x] = fill;

                    } else if (point_inside_triangle(p1s, p2s, p3s, point)) {
                        p[(y*width)+x] = stroke;

                    } else {
                        p[(y*width)+x] = 0;
                    }
                }
            }
            break;
        }

        case CANVAS_ITEM_CTRL_SHAPE_CROSS:
            // Actually an 'X'.
            for(int y = 0; y < height; y++) {
                for(int x = 0; x < width; x++) {
                    if ( abs(x - y)             < device_scale ||
                         abs(width - 1 - x - y) < device_scale  ) {
                        *p++ = stroke;
                    } else {
                        *p++ = 0;
                    }
                }
            }
            _built = true;
            break;

        case CANVAS_ITEM_CTRL_SHAPE_PLUS:
            // Actually an '+'.
            for(int y = 0; y < height; y++) {
                for(int x = 0; x < width; x++) {
                    if ( std::abs(x-width/2)   < device_scale ||
                         std::abs(y-height/2)  < device_scale  ) {
                        *p++ = stroke;
                    } else {
                        *p++ = 0;
                    }
                }
            }
            _built = true;
            break;

        case CANVAS_ITEM_CTRL_SHAPE_DARROW: // Double arrow
        case CANVAS_ITEM_CTRL_SHAPE_SARROW: // Same shape as darrow but rendered rotated 90 degrees.
        case CANVAS_ITEM_CTRL_SHAPE_CARROW: // Double corner arrow
        case CANVAS_ITEM_CTRL_SHAPE_PIVOT:  // Fancy "plus"
        case CANVAS_ITEM_CTRL_SHAPE_SALIGN: // Side align (triangle pointing toward line)
        case CANVAS_ITEM_CTRL_SHAPE_CALIGN: // Corner align (triangle pointing into "L")
        case CANVAS_ITEM_CTRL_SHAPE_MALIGN: // Middle align (four triangles poining inward)
        {
            double size = _width; // Use unscaled width.

            auto work = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, device_scale * size, device_scale * size);
            cairo_surface_set_device_scale(work->cobj(), device_scale, device_scale); // No C++ API!
            auto cr = Cairo::Context::create(work);

            // Rotate around center
            cr->translate( size/2.0,  size/2.0);
            cr->rotate(_angle);
            cr->translate(-size/2.0, -size/2.0);

            // Construct path
            switch (_shape) {
                case CANVAS_ITEM_CTRL_SHAPE_DARROW:
                case CANVAS_ITEM_CTRL_SHAPE_SARROW:
                    draw_darrow(cr, size);
                    break;

                case CANVAS_ITEM_CTRL_SHAPE_CARROW:
                    draw_carrow(cr, size);
                    break;

                case CANVAS_ITEM_CTRL_SHAPE_PIVOT:
                    draw_pivot(cr, size);
                    break;

                case CANVAS_ITEM_CTRL_SHAPE_SALIGN:
                    draw_salign(cr, size);
                    break;

                case CANVAS_ITEM_CTRL_SHAPE_CALIGN:
                    draw_calign(cr, size);
                    break;

                case CANVAS_ITEM_CTRL_SHAPE_MALIGN:
                    draw_malign(cr, size);
                    break;

                default:
                    // Shouldn't happen
                    break;
            }

            // Fill and stroke.
            cr->set_source_rgba(SP_RGBA32_R_F(_fill),
                                SP_RGBA32_G_F(_fill),
                                SP_RGBA32_B_F(_fill),
                                SP_RGBA32_A_F(_fill));
            cr->fill_preserve();
            cr->set_source_rgba(SP_RGBA32_R_F(_stroke),
                                SP_RGBA32_G_F(_stroke),
                                SP_RGBA32_B_F(_stroke),
                                SP_RGBA32_A_F(_stroke));
            cr->set_line_width(1);
            cr->stroke();

            // Copy to buffer.
            work->flush();
            int strideb = work->get_stride();
            unsigned char* pxb = work->get_data();
            guint32 *p = _cache;
            for (int i = 0; i < device_scale * size; ++i) {
                guint32 *pb = reinterpret_cast<guint32*>(pxb + i*strideb);
                for (int j = 0; j < width; ++j) {

                    guint32 color = 0x0;

                    // Need to un-premultiply alpha and change order argb -> rgba.
                    guint32 alpha = (*pb & 0xff000000) >> 24;
                    if (alpha == 0x0) {
                        color = 0x0;
                    } else {
                        guint32 rgb = unpremul_alpha(*pb & 0xffffff, alpha);
                        color = (rgb << 8) + alpha;
                    }

                    *p++ = color;
                    pb++;
                }
            }
            _built = true;
            break;
        }

        case CANVAS_ITEM_CTRL_SHAPE_BITMAP:
        {
            if (_pixbuf) {
                unsigned char* px = gdk_pixbuf_get_pixels (_pixbuf);
                unsigned int   rs = gdk_pixbuf_get_rowstride (_pixbuf);
                for (int y = 0; y < height/device_scale; y++){
                    for (int x = 0; x < width/device_scale; x++) {
                        unsigned char *s = px + rs*y + 4*x;
                        guint32 color;
                        if (s[3] < 0x80) {
                            color = 0;
                        } else if (s[0] < 0x80) {
                            color = _stroke;
                        } else {
                            color = _fill;
                        }

                        // Fill in device_scale x device_scale block
                        for (int i = 0; i < device_scale; ++i) {
                            for (int j = 0; j < device_scale; ++j) {
                                guint* p = _cache +
                                    (x * device_scale + i) +            // Column
                                    (y * device_scale + j) * width;     // Row
                                *p = color;
                            }
                        }
                    }
                }
            } else {
                std::cerr << "CanvasItemCtrl::build_cache: No bitmap!" << std::endl;
                guint *p = _cache;
                for (int y = 0; y < height/device_scale; y++){
                    for (int x = 0; x < width/device_scale; x++) {
                        if (x == y) {
                            *p++ = 0xffff0000;
                        } else {
                            *p++ = 0;
                        }
                    }
                }
            }
            _built = true;
            break;
        }

        case CANVAS_ITEM_CTRL_SHAPE_IMAGE:
            std::cerr << "CanvasItemCtrl::build_cache: image: UNIMPLEMENTED" << std::endl;
            break;

        default:
            std::cerr << "CanvasItemCtrl::build_cache: unhandled shape!" << std::endl;
    }
}

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

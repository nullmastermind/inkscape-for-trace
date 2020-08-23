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
        auto work = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, _width, _height);
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
        for (int i = 0; i < _height; ++i) {
            guint32 *pb = reinterpret_cast<guint32*>(pxb + i*strideb);
            for (int j = 0; j < _width; ++j) {
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
        case CANVAS_ITEM_CTRL_TYPE_NODE_AUTO:
        case CANVAS_ITEM_CTRL_TYPE_ROTATE:
            _shape = CANVAS_ITEM_CTRL_SHAPE_CIRCLE;
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

    if (size_index < 1 || size_index > 7) {
        std::cerr << "CanvasItemCtrl::set_size_via_index: size_index out of range!" << std::endl;
        size_index = 3;
    }

    int size = 0;
    switch (_type) {
        case CANVAS_ITEM_CTRL_TYPE_POINT:
        case CANVAS_ITEM_CTRL_TYPE_ROTATE:
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
    int size = prefs->getIntLimited("/options/grabsize/value", 3, 1, 7);
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
        std::cout << "CanvasItemCtrl::build_cache: Somebody really does use CTRL_MODE_COLOR!" << std::endl;
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
            break;
        case CANVAS_ITEM_CTRL_SHAPE_IMAGE:
            std::cout << "CTRL  Building image: UNIMPLEMENTED" << std::endl;
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

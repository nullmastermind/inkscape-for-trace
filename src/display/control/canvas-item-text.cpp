// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * A class to represent a control textrilateral. Used to hightlight selected text.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of SPCtrlTextr
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "canvas-item-text.h"

#include <utility> // std::move

#include "color.h" // SP_RGBA_x_F

#include "ui/widget/canvas.h"

namespace Inkscape {

/**
 * Create an null control text.
 */
CanvasItemText::CanvasItemText(CanvasItemGroup *group)
    : CanvasItem(group)
{
    _name = "CanvasItemText";
    _pickable = false; // Text is never pickable.
    _fill = 0x33337fff; // Override CanvasItem default.
}

/**
 * Create a control text. Point are in document coordinates.
 */
CanvasItemText::CanvasItemText(CanvasItemGroup *group, Geom::Point const &p, Glib::ustring text)
    : CanvasItem(group)
    , _p(p)
    , _text(std::move(text))
{
    _name = "CanvasItemText";
    _pickable = false; // Text is never pickable.
    _fill = 0x33337fff; // Override CanvasItem default.

    request_update();
}

/**
 * Set a text position. Position is in document coordinates.
 */
void CanvasItemText::set_coord(Geom::Point const &p)
{
    _p = p;

    request_update();
}

/**
 * Returns distance between point in canvas units and nearest point on text.
 */
double CanvasItemText::closest_distance_to(Geom::Point const &p)
{
    double d = Geom::infinity();
    std::cerr << "CanvasItemText::closest_distance_to: Not implemented!" << std::endl;
    return d;
}

/**
 * Returns true if point p (in canvas units) is within tolerance (canvas units) distance of text.
 */
bool CanvasItemText::contains(Geom::Point const &p, double tolerance)
{
    return false; // We never select text.
}

/**
 * Update and redraw control text.
 */
void CanvasItemText::update(Geom::Affine const &affine)
{
    if (_affine == affine && !_need_update) {
        // Nothing to do.
        return;
    }

    // Queue redraw of old area (erase previous content).
    _canvas->redraw_area(_bounds);

    // Get new bounds
    _affine = affine;
    Geom::Point p = _p * _affine;

    // Measure text size
    auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 1, 1);
    auto context = Cairo::Context::create(surface);
    context->select_font_face("sans-serif", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
    context->set_font_size(_fontsize);
    Cairo::TextExtents extents;
    context->get_text_extents(_text, extents);


    // Adjust for anchor.
    double offset_x =  extents.width/2.0;
    double offset_y = -extents.height/2.0;
    switch (_anchor){
        case CANVAS_ITEM_TEXT_ANCHOR_LEFT:
            offset_x = 0.0;
            break;
        case CANVAS_ITEM_TEXT_ANCHOR_RIGHT:
            offset_x = extents.width;
            break;
        case CANVAS_ITEM_TEXT_ANCHOR_BOTTOM:
            offset_y = 0;
            break;
        case CANVAS_ITEM_TEXT_ANCHOR_TOP:
            offset_y = -extents.height;
            break;
        case CANVAS_ITEM_TEXT_ANCHOR_ZERO:
            offset_x = 0;
            offset_y = 0;
            break;
        case CANVAS_ITEM_TEXT_ANCHOR_MANUAL:
            offset_x =  (1 + _anchor_position_manual.x()) * extents.width/2;
            offset_y = -(1 + _anchor_position_manual.y()) * extents.height/2;
            break;
        case CANVAS_ITEM_TEXT_ANCHOR_CENTER:
        default:
            break;
    }
    _anchor_offset = Geom::Point(offset_x, offset_y);

    // See note at bottom.
    _bounds = Geom::Rect::from_xywh(p.x(),
                                    p.y() - extents.height,
                                    extents.width,
                                    extents.height);
    _bounds.expandBy(_border);
    _bounds *= Geom::Translate(-_anchor_offset);

    _bounds = _bounds.roundOutwards(); // Pixel alignment of background. Avoid aliasing artifacts on redraw.

    // Queue redraw of new area
    _canvas->redraw_area(_bounds);

    _need_update = false;
}

/**
 * Render text to screen via Cairo.
 */
void CanvasItemText::render(Inkscape::CanvasItemBuffer *buf)
{
    if (!buf) {
        std::cerr << "CanvasItemText::Render: No buffer!" << std::endl;
         return;
    }

    if (!_visible) {
        // Hidden
        return;
    }

    // Document to canvas
    Geom::Point p = _p * _affine;

    // Canvas to screen
    p *= Geom::Translate(-buf->rect.min());

    // Anchor offset
    p *= Geom::Translate(-_anchor_offset);

    buf->cr->save();

    // Background
    if (_use_background) {
        buf->cr->rectangle(_bounds.min().x() - buf->rect.min().x(),
                           _bounds.min().y() - buf->rect.min().y(),
                           _bounds.width(),
                           _bounds.height());
        buf->cr->set_line_width(2);
        buf->cr->set_source_rgba(SP_RGBA32_R_F(_background), SP_RGBA32_G_F(_background),
                                 SP_RGBA32_B_F(_background), SP_RGBA32_A_F(_background));
        buf->cr->fill();
    }

    // Text
    buf->cr->move_to(p.x(), p.y());
    buf->cr->text_path(_text);
    buf->cr->set_source_rgba(SP_RGBA32_R_F(_fill), SP_RGBA32_G_F(_fill),
                             SP_RGBA32_B_F(_fill), SP_RGBA32_A_F(_fill));
    buf->cr->fill();


    // Uncomment to show bounds
    // Geom::Rect bounds = _bounds;
    // bounds.expandBy(-1);
    // bounds -= buf->rect.min();
    // buf->cr->set_source_rgba(1.0, 0.0, 0.0, 1.0);
    // buf->cr->rectangle(bounds.min().x(), bounds.min().y(), bounds.width(), bounds.height());
    // buf->cr->stroke();

    buf->cr->restore();
}

void CanvasItemText::set_text(Glib::ustring const &text)
{
    if (_text != text) {
        _text = text;
        request_update(); // Might be larger than before!
    }
}

void CanvasItemText::set_fontsize(double fontsize)
{
    if (_fontsize != fontsize) {
        _fontsize = fontsize;
        request_update(); // Might be larger than before!
    }
}

void CanvasItemText::set_background(guint32 background)
{
    if (_background != background) {
        _background = background;
        _canvas->redraw_area(_bounds);
    }
    _use_background = true;
}

void CanvasItemText::set_anchor(CanvasItemTextAnchor anchor)
{
    if (_anchor != anchor) {
        _anchor = anchor;
        _canvas->request_update(); // Might be larger than before!
    }
}

void CanvasItemText::set_anchor(Geom::Point const &anchor_pt)
{
    // Used by LPE tool and live_effects/parameter/text.cpp
    if (_anchor_position_manual != anchor_pt || _anchor != CANVAS_ITEM_TEXT_ANCHOR_MANUAL) {
        _anchor_position_manual  = anchor_pt;   _anchor  = CANVAS_ITEM_TEXT_ANCHOR_MANUAL;
        _canvas->request_update(); // Might be larger than before!
    }
}

} // namespace Inkscape

/* FROM: http://lists.cairographics.org/archives/cairo-bugs/2009-March/003014.html
  - Glyph surfaces: In most font rendering systems, glyph surfaces
    have an origin at (0,0) and a bounding box that is typically
    represented as (x_bearing,y_bearing,width,height).  Depending on
    which way y progresses in the system, y_bearing may typically be
    negative (for systems similar to cairo, with origin at top left),
    or be positive (in systems like PDF with origin at bottom left).
    No matter which is the case, it is important to note that
    (x_bearing,y_bearing) is the coordinates of top-left of the glyph
    relative to the glyph origin.  That is, for example:

    Scaled-glyph space:

      (x_bearing,y_bearing) <-- negative numbers
         +----------------+
         |      .         |
         |      .         |
         |......(0,0) <---|-- glyph origin
         |                |
         |                |
         +----------------+
                  (width+x_bearing,height+y_bearing)

    Note the similarity of the origin to the device space.  That is
    exactly how we use the device_offset to represent scaled glyphs:
    to use the device-space origin as the glyph origin.
*/

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

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Simple non-transformed rectangle, usable for rubberband
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Carl Hetherington <inkscape@carlh.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright (C) 1999-2001 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2017 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 */

#include "inkscape.h"
#include "sodipodi-ctrlrect.h"
#include "sp-canvas-util.h"
#include "display/cairo-utils.h"
#include "display/sp-canvas.h"
#include <2geom/transforms.h>

/*
 * Currently we do not have point method, as it should always be painted
 * during some transformation, which takes care of events...
 *
 * Corner coords can be in any order - i.e. x1 < x0 is allowed
 */

static void sp_ctrlrect_destroy(SPCanvasItem *object);

static void sp_ctrlrect_update(SPCanvasItem *item, Geom::Affine const &affine, unsigned int flags);
static void sp_ctrlrect_render(SPCanvasItem *item, SPCanvasBuf *buf);

G_DEFINE_TYPE(CtrlRect, sp_ctrlrect, SP_TYPE_CANVAS_ITEM);

static void sp_ctrlrect_class_init(CtrlRectClass *c)
{
    SPCanvasItemClass *item_class = SP_CANVAS_ITEM_CLASS(c);

    item_class->destroy = sp_ctrlrect_destroy;
    item_class->update = sp_ctrlrect_update;
    item_class->render = sp_ctrlrect_render;
}

static void sp_ctrlrect_init(CtrlRect *cr)
{
    cr->init();
}

static void sp_ctrlrect_destroy(SPCanvasItem *object)
{
    if (SP_CANVAS_ITEM_CLASS(sp_ctrlrect_parent_class)->destroy) {
        (* SP_CANVAS_ITEM_CLASS(sp_ctrlrect_parent_class)->destroy)(object);
    }
}


static void sp_ctrlrect_render(SPCanvasItem *item, SPCanvasBuf *buf)
{
    SP_CTRLRECT(item)->render(buf);
}


static void sp_ctrlrect_update(SPCanvasItem *item, Geom::Affine const &affine, unsigned int flags)
{
    SP_CTRLRECT(item)->update(affine, flags);
}



void CtrlRect::init()
{
    _has_fill = false;
    _dashed = false;
    _checkerboard = false;

    _shadow_width = 0;

    _area = Geom::OptIntRect();

    _rect = Geom::Rect(Geom::Point(0,0),Geom::Point(0,0));

    _border_color = 0x000000ff;
    _fill_color = 0xffffffff;
    _shadow_color = 0x000000ff;
    _inverted = false;
}


void CtrlRect::render(SPCanvasBuf *buf)
{
    using Geom::X;
    using Geom::Y;

    if (!_area) {
        return;
    }

    Geom::IntRect area = *_area; // _area already includes space for shadow.
    if ( area.intersects(buf->rect) )
    {

        cairo_save(buf->ct);
        cairo_translate(buf->ct, -buf->rect.left(), -buf->rect.top());
 
        // Get canvas rotation (scale is isotropic).
        double rotation = atan2( _affine[1], _affine[0] );

        // Are we axis aligned?
        double mod_rot = fmod(rotation * M_2_PI, 1);
        bool axis_aligned = Geom::are_near( mod_rot, 0 ) || Geom::are_near( mod_rot, 1.0 );

        // Get the points we need transformed to window coordinates.
        Geom::Point rect_transformed[4];
        for (unsigned i = 0; i < 4; ++i) {
            rect_transformed[i] = _rect.corner(i) * _affine;
        }

        if(_inverted) {
            cairo_set_operator(buf->ct, CAIRO_OPERATOR_DIFFERENCE);
        }

        // Draw shadow first. Shadow extends under rectangle to reduce aliasing effects.
        if (_shadow_width > 0 && !_dashed) {
            Geom::Point const * corners = rect_transformed;
            double shadowydir = _affine.det() > 0 ? -1 : 1;

            // is the desktop y-axis downwards?
            if (SP_ACTIVE_DESKTOP && SP_ACTIVE_DESKTOP->is_yaxisdown()) {
                ++corners; // need corners 1/2/3 instead of 0/1/2
                shadowydir *= -1;
            }

            // Offset by half stroke width (_shadow_width is in window coordinates).
            // Need to handle change in handedness with flips.
            Geom::Point shadow( _shadow_width/2.0, shadowydir * _shadow_width/2.0 );
            shadow *= Geom::Rotate( rotation );

            if (axis_aligned) {
                // Snap to pixel grid (add 0.5 to center on pixel).
                cairo_move_to( buf->ct,
                               floor(corners[0][X] + shadow[X]+0.5) + 0.5,
                               floor(corners[0][Y] + shadow[Y]+0.5) + 0.5 );
                cairo_line_to( buf->ct,
                               floor(corners[1][X] + shadow[X]+0.5) + 0.5,
                               floor(corners[1][Y] + shadow[Y]+0.5) + 0.5 );
                cairo_line_to( buf->ct,
                               floor(corners[2][X] + shadow[X]+0.5) + 0.5,
                               floor(corners[2][Y] + shadow[Y]+0.5) + 0.5 );
            } else {
                cairo_move_to( buf->ct,
                               corners[0][X] + shadow[X],
                               corners[0][Y] + shadow[Y] );
                cairo_line_to( buf->ct,
                               corners[1][X] + shadow[X],
                               corners[1][Y] + shadow[Y] );
                cairo_line_to( buf->ct,
                               corners[2][X] + shadow[X],
                               corners[2][Y] + shadow[Y] );
            }               

            ink_cairo_set_source_rgba32( buf->ct, _shadow_color );
            cairo_set_line_width( buf->ct, _shadow_width + 1 );
            cairo_stroke( buf->ct );
        }


        // Setup rectangle path
        if (axis_aligned) {

            // Snap to pixel grid
            Geom::Rect outline( _rect.min() * _affine, _rect.max() * _affine);
            cairo_rectangle (buf->ct,
                             floor(outline.min()[X])+0.5,
                             floor(outline.min()[Y])+0.5,
                             floor(outline.max()[X]) - floor(outline.min()[X]),
                             floor(outline.max()[Y]) - floor(outline.min()[Y]));
        } else {

            // Angled
            cairo_move_to( buf->ct, rect_transformed[0][X], rect_transformed[0][Y] );
            cairo_line_to( buf->ct, rect_transformed[1][X], rect_transformed[1][Y] );
            cairo_line_to( buf->ct, rect_transformed[2][X], rect_transformed[2][Y] );
            cairo_line_to( buf->ct, rect_transformed[3][X], rect_transformed[3][Y] );
            cairo_close_path( buf->ct );
        }

        // This doesn't seem to be used anywhere. If it is, then we should
        // probably rotate the coordinate system and fill using a cairo_rectangle().
        if (_checkerboard) {
            cairo_pattern_t *cb = ink_cairo_pattern_create_checkerboard();
            cairo_set_source(buf->ct, cb);
            cairo_pattern_destroy(cb);
            cairo_fill_preserve(buf->ct);
        }

        if (_has_fill) {
            ink_cairo_set_source_rgba32(buf->ct, _fill_color);
            cairo_fill_preserve(buf->ct);
        }

        // Set up stroke.
        ink_cairo_set_source_rgba32(buf->ct, _border_color);
        cairo_set_line_width(buf->ct, 1);
        static double const dashes[2] = {4.0, 4.0};
        if (_dashed) cairo_set_dash(buf->ct, dashes, 2, 0);

        // Stroke rectangle.
        cairo_stroke_preserve(buf->ct);

        // Highlight the border by drawing it in _shadow_color.
        if (_shadow_width == 1 && _dashed) {
            ink_cairo_set_source_rgba32(buf->ct, _shadow_color);
            cairo_set_dash(buf->ct, dashes, 2, 4); // Dash offset by dash length.
            cairo_stroke_preserve(buf->ct);
        }

        cairo_new_path(buf->ct); // Clear path or get weird artifacts.
        cairo_restore(buf->ct);
    }
}


void CtrlRect::update(Geom::Affine const &affine, unsigned int flags)
{
    using Geom::X;
    using Geom::Y;

    if ((SP_CANVAS_ITEM_CLASS(sp_ctrlrect_parent_class))->update) {
        (SP_CANVAS_ITEM_CLASS(sp_ctrlrect_parent_class))->update(this, affine, flags);
    }

    // Note: There is no harm if 'area' is too large other than a possible small slow down in
    // rendering speed.

    // Calculate an axis-aligned bounding box that include all of transformed _rect.
    Geom::Rect bbox = _rect;
    bbox *= affine;

    // Enlarge bbox by twice shadow size (to allow for shadow on any side with a 45deg rotation).
    bbox.expandBy( 2.0 *_shadow_width );

    // Generate an integer rectangle that includes bbox.
    Geom::OptIntRect _area_old = _area;
    _area = bbox.roundOutwards();

    // std::cout << "  _rect: " << _rect << std::endl;
    // std::cout << "   bbox: " <<  bbox << std::endl;
    // std::cout << "   area: " << *_area << std::endl;

    if (_area) {
        Geom::IntRect area = *_area;
        // Windows glitches sometimes in cairo, possibly due to the way the surface is cleared.
        // Increasing '_area' won't work as the box must be drawn 'inside' the updated area.
        sp_canvas_update_bbox(this, area.left(), area.top(), area.right() + 1, area.bottom() + 1);

    } else {
        std::cerr << "CtrlRect::update: No area!!" << std::endl;
    }

    // At rendering stage, we need to know affine:
    _affine = affine;
}


void CtrlRect::setColor(guint32 b, bool h, guint f)
{
    _border_color = b;
    _has_fill = h;
    _fill_color = f;
    _requestUpdate();
}

void CtrlRect::setShadow(int s, guint c)
{
    _shadow_width = s;
    _shadow_color = c;
    _requestUpdate();
}

void CtrlRect::setInvert(bool invert) {
    _inverted = invert;
    _requestUpdate();
}

void CtrlRect::setRectangle(Geom::Rect const &r)
{
    _rect = r;
    _requestUpdate();
}

void CtrlRect::setDashed(bool d)
{
    _dashed = d;
    _requestUpdate();
}

void CtrlRect::setCheckerboard(bool d)
{
    _checkerboard = d;
    _requestUpdate();
}

void CtrlRect::_requestUpdate()
{
    sp_canvas_item_request_update(SP_CANVAS_ITEM(this));
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

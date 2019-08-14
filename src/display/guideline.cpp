// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Horizontal/vertical but can also be angled line
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Johan Engelen
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2000-2002 Lauris Kaplinski
 * Copyright (C) 2007 Johan Engelen
 * Copyright (C) 2009 Maximilian Albert
 * Copyright (C) 2017 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/coord.h>
#include <2geom/transforms.h>
#include <2geom/line.h>

#include "sp-canvas-util.h"
#include "inkscape.h"
#include "guideline.h"
#include "display/cairo-utils.h"
#include "display/sp-canvas.h"
#include "display/sodipodi-ctrl.h"
#include "object/sp-namedview.h"

static void sp_guideline_destroy(SPCanvasItem *object);

static void sp_guideline_update(SPCanvasItem *item, Geom::Affine const &affine, unsigned int flags);
static void sp_guideline_render(SPCanvasItem *item, SPCanvasBuf *buf);

static double sp_guideline_point(SPCanvasItem *item, Geom::Point p, SPCanvasItem **actual_item);

G_DEFINE_TYPE(SPGuideLine, sp_guideline, SP_TYPE_CANVAS_ITEM);

static void sp_guideline_class_init(SPGuideLineClass *c)
{
    SPCanvasItemClass *item_class = SP_CANVAS_ITEM_CLASS(c);
    item_class->destroy = sp_guideline_destroy;
    item_class->update = sp_guideline_update;
    item_class->render = sp_guideline_render;
    item_class->point = sp_guideline_point;
}

static void sp_guideline_init(SPGuideLine *gl)
{
    gl->rgba = 0x0000ff7f;

    gl->locked = false;
    gl->normal_to_line = Geom::Point(0,1);
    gl->angle = M_PI_2;
    gl->point_on_line = Geom::Point(0,0);
    gl->sensitive = 0;

    gl->origin = nullptr;
    gl->label = nullptr;
}

static void sp_guideline_destroy(SPCanvasItem *object)
{
    g_return_if_fail (object != nullptr);
    g_return_if_fail (SP_IS_GUIDELINE (object));

    SPGuideLine *gl = SP_GUIDELINE(object);

    if (gl->origin) {
        sp_canvas_item_destroy(SP_CANVAS_ITEM(gl->origin));
    }

    if (gl->label) {
        g_free(gl->label);
    }

    SP_CANVAS_ITEM_CLASS(sp_guideline_parent_class)->destroy(object);
}

static void sp_guideline_render(SPCanvasItem *item, SPCanvasBuf *buf)
{
    SPGuideLine const *gl = SP_GUIDELINE (item);

    cairo_save(buf->ct);
    cairo_translate(buf->ct, -buf->rect.left(), -buf->rect.top());
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool xrayactive = prefs->getBool("/desktop/xrayactive", false);
    SPDesktop * desktop = SP_ACTIVE_DESKTOP;
    if (xrayactive && desktop) {
        guint32 bg = desktop->getNamedView()->pagecolor;
        guint32 _color = SP_RGBA32_F_COMPOSE(
                CLAMP(((1 - SP_RGBA32_A_F(gl->rgba)) * SP_RGBA32_R_F(bg)) + (SP_RGBA32_A_F(gl->rgba) * SP_RGBA32_R_F(gl->rgba)), 0.0, 1.0),
                CLAMP(((1 - SP_RGBA32_A_F(gl->rgba)) * SP_RGBA32_G_F(bg)) + (SP_RGBA32_A_F(gl->rgba) * SP_RGBA32_G_F(gl->rgba)), 0.0, 1.0),
                CLAMP(((1 - SP_RGBA32_A_F(gl->rgba)) * SP_RGBA32_B_F(bg)) + (SP_RGBA32_A_F(gl->rgba) * SP_RGBA32_B_F(gl->rgba)), 0.0, 1.0),
                1.0);
        ink_cairo_set_source_rgba32(buf->ct, _color);
    } else {
        ink_cairo_set_source_rgba32(buf->ct, gl->rgba);
    }
    cairo_set_line_width(buf->ct, 1);
    cairo_set_line_cap(buf->ct, CAIRO_LINE_CAP_SQUARE);
    cairo_set_font_size(buf->ct, 10);

    // normal_dt does not have unit length
    Geom::Point normal_dt        = gl->normal_to_line * gl->affine.withoutTranslation();
    Geom::Point point_on_line_dt = gl->point_on_line  * gl->affine;

    if (gl->label) {
        int px = round(point_on_line_dt[Geom::X]);
        int py = round(point_on_line_dt[Geom::Y]);
        cairo_save(buf->ct);
        cairo_translate(buf->ct, px, py);
        cairo_rotate(buf->ct, atan2(normal_dt.cw()) + M_PI * (desktop && desktop->is_yaxisdown() ? 1 : 0));
        cairo_translate(buf->ct, 0, -5);
        cairo_move_to(buf->ct, 0, 0);
        cairo_show_text(buf->ct, gl->label);
        cairo_restore(buf->ct);
    }

    // Draw guide.
    // Special case horizontal and vertical lines so they accurately align to the to pixels.
    
    // Need to use floor()+0.5 such that Cairo will draw us lines with a width of a single pixel, without any aliasing.
    // For this we need to position the lines at exactly half pixels, see https://www.cairographics.org/FAQ/#sharp_lines
    // Must be consistent with the pixel alignment of the grid lines, see CanvasXYGrid::Render(), and the drawing of the rulers
    
    // Don't use isHorizontal()/isVertical() as they test only exact matches.
    if ( Geom::are_near(normal_dt[Geom::Y], 0.0) ) { // Vertical?

        double position = floor(point_on_line_dt[Geom::X]) + 0.5;
        cairo_move_to(buf->ct, position, buf->rect.top() + 0.5);
        cairo_line_to(buf->ct, position, buf->rect.bottom() - 0.5);
        cairo_stroke(buf->ct);

    } else if ( Geom::are_near(normal_dt[Geom::X], 0.0) ) { // Horizontal?

        double position = floor(point_on_line_dt[Geom::Y]) + 0.5;
        cairo_move_to(buf->ct, buf->rect.left() + 0.5, position);
        cairo_line_to(buf->ct, buf->rect.right() - 0.5, position);
        cairo_stroke(buf->ct);

    } else {

        Geom::Line guide =
            Geom::Line::from_origin_and_vector( point_on_line_dt, Geom::rot90(normal_dt) );

        // Find intersections of guide with buf rectangle. There should be zero or two.
        std::vector<Geom::Point> intersections;
        for (unsigned i = 0; i < 4; ++i) {
            Geom::LineSegment side( buf->rect.corner(i), buf->rect.corner((i+1)%4) );
            try {
                Geom::OptCrossing oc = Geom::intersection(guide, side);
                if (oc) {
                    intersections.push_back( guide.pointAt((*oc).ta));
                }
            } catch (Geom::InfiniteSolutions) {
                // Shouldn't happen as we have already taken care of horizontal/vertical guides.
                std::cerr << "sp_guideline_render: Error: Infinite intersections." << std::endl;
            }
        }

        if (intersections.size() == 2) {
            double x0 = intersections[0][Geom::X];
            double x1 = intersections[1][Geom::X];
            double y0 = intersections[0][Geom::Y];
            double y1 = intersections[1][Geom::Y];
            cairo_move_to(buf->ct, x0, y0);
            cairo_line_to(buf->ct, x1, y1);
            cairo_stroke(buf->ct);
        }
    }

    cairo_restore(buf->ct);
}

static void sp_guideline_update(SPCanvasItem *item, Geom::Affine const &affine, unsigned int flags)
{
    SPGuideLine *gl = SP_GUIDELINE(item);

    if ((SP_CANVAS_ITEM_CLASS(sp_guideline_parent_class))->update) {
        (SP_CANVAS_ITEM_CLASS(sp_guideline_parent_class))->update(item, affine, flags);
    }

    if (item->visible) {
        if (gl->locked) {
            g_object_set(G_OBJECT(gl->origin), "stroke_color", 0x0000ff88,
                                               "shape", SP_CTRL_SHAPE_CROSS,
                                               "size", 7, NULL);
        } else {
            g_object_set(G_OBJECT(gl->origin), "stroke_color", 0xff000088,
                                               "shape", SP_CTRL_SHAPE_CIRCLE,
                                               "size", 5, NULL);
        }
        gl->origin->moveto(gl->point_on_line);
        sp_canvas_item_request_update(SP_CANVAS_ITEM(gl->origin));
    }

    gl->affine = affine;

    // Rotated canvas requires large bbox for all guides.
    sp_canvas_update_bbox (item, -1000000, -1000000, 1000000, 1000000);

}

// Returns 0.0 if point is on the guideline
static double sp_guideline_point(SPCanvasItem *item, Geom::Point p, SPCanvasItem **actual_item)
{
    SPGuideLine *gl = SP_GUIDELINE (item);

    if (!gl->sensitive) {
        return Geom::infinity();
    }

    *actual_item = item;

    Geom::Point vec = gl->normal_to_line * gl->affine.withoutTranslation();
    double distance = Geom::dot((p - gl->point_on_line * gl->affine), unit_vector(vec));
    return MAX(fabs(distance)-1, 0);
}

SPCanvasItem *sp_guideline_new(SPCanvasGroup *parent, char* label, Geom::Point point_on_line, Geom::Point normal)
{
    SPCanvasItem *item = sp_canvas_item_new(parent, SP_TYPE_GUIDELINE, nullptr);
    SPGuideLine *gl = SP_GUIDELINE(item);

    normal.normalize();
    gl->label = label;
    gl->locked = false;
    gl->normal_to_line = normal;
    gl->angle = tan( -gl->normal_to_line[Geom::X] / gl->normal_to_line[Geom::Y]);
    sp_guideline_set_position(gl, point_on_line);

    gl->origin = (SPCtrl *) sp_canvas_item_new(parent, SP_TYPE_CTRL, 
                                               "anchor", SP_ANCHOR_CENTER,
                                               "mode", SP_CTRL_MODE_COLOR,
                                               "filled", FALSE,
                                               "stroked", TRUE,
                                               "stroke_color", 0x01000000, NULL);
    gl->origin->pickable = false;

    return item;
}

void sp_guideline_set_label(SPGuideLine *gl, const char* label)
{
    if (gl->label) {
        g_free(gl->label);
    }
    gl->label = g_strdup(label);

    sp_canvas_item_request_update(SP_CANVAS_ITEM (gl));
}

void sp_guideline_set_locked(SPGuideLine *gl, const bool locked)
{
    gl->locked = locked;
    sp_canvas_item_request_update(SP_CANVAS_ITEM (gl));
}

void sp_guideline_set_position(SPGuideLine *gl, Geom::Point point_on_line)
{
    gl->point_on_line = point_on_line;
    sp_canvas_item_request_update(SP_CANVAS_ITEM (gl));
}

void sp_guideline_set_normal(SPGuideLine *gl, Geom::Point normal_to_line)
{
    gl->normal_to_line = normal_to_line;
    gl->angle = tan( -normal_to_line[Geom::X] / normal_to_line[Geom::Y]);

    sp_canvas_item_request_update(SP_CANVAS_ITEM (gl));
}

void sp_guideline_set_color(SPGuideLine *gl, unsigned int rgba)
{
    gl->rgba = rgba;
    g_object_set(G_OBJECT(gl->origin), "stroke_color", rgba, NULL);
    sp_canvas_item_request_update(SP_CANVAS_ITEM(gl));
}

void sp_guideline_set_sensitive(SPGuideLine *gl, int sensitive)
{
    gl->sensitive = sensitive;
}

void sp_guideline_delete(SPGuideLine *gl)
{
    sp_canvas_item_destroy(SP_CANVAS_ITEM(gl));
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

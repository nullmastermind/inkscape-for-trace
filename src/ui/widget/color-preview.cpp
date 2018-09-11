// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Ralf Stephan <ralf@ark.in-berlin.de>
 *
 * Copyright (C) 2001-2005 Authors
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/widget/color-preview.h"
#include "display/cairo-utils.h"
#include <cairo.h>

#define SPCP_DEFAULT_WIDTH 32
#define SPCP_DEFAULT_HEIGHT 12

namespace Inkscape {
    namespace UI {
        namespace Widget {

ColorPreview::ColorPreview (guint32 rgba)
{
    _rgba = rgba;
    set_has_window(false);
    set_name("ColorPreview");
}

void
ColorPreview::on_size_allocate (Gtk::Allocation &all)
{
    set_allocation (all);
    if (get_is_drawable())
        queue_draw();
}

void
ColorPreview::get_preferred_height_vfunc(int& minimum_height, int& natural_height) const
{
    minimum_height = natural_height = SPCP_DEFAULT_HEIGHT;
}

void
ColorPreview::get_preferred_height_for_width_vfunc(int /* width */, int& minimum_height, int& natural_height) const
{
    minimum_height = natural_height = SPCP_DEFAULT_HEIGHT;
}

void
ColorPreview::get_preferred_width_vfunc(int& minimum_width, int& natural_width) const
{
    minimum_width = natural_width = SPCP_DEFAULT_WIDTH;
}

void
ColorPreview::get_preferred_width_for_height_vfunc(int /* height */, int& minimum_width, int& natural_width) const
{
    minimum_width = natural_width = SPCP_DEFAULT_WIDTH;
}

void
ColorPreview::setRgba32 (guint32 rgba)
{
    _rgba = rgba;

    if (get_is_drawable())
        queue_draw();
}

bool
ColorPreview::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
    double x, y, width, height;
    const Gtk::Allocation& allocation = get_allocation();
    x = 0;
    y = 0;
    width = allocation.get_width()/2.0;
    height = allocation.get_height();

    double radius = height / 7.5;
    double degrees = M_PI / 180.0;
    cairo_new_sub_path (cr->cobj());
    cairo_line_to(cr->cobj(), width, 0);
    cairo_line_to(cr->cobj(), width, height);
    cairo_arc (cr->cobj(), x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
    cairo_arc (cr->cobj(), x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
    cairo_close_path (cr->cobj());

    /* Transparent area */

    cairo_pattern_t *checkers = ink_cairo_pattern_create_checkerboard();

    cairo_set_source(cr->cobj(), checkers);
    cr->fill_preserve();
    ink_cairo_set_source_rgba32(cr->cobj(), _rgba);
    cr->fill();
    cairo_pattern_destroy(checkers);

    /* Solid area */

    x = width;

    cairo_new_sub_path (cr->cobj());
    cairo_arc (cr->cobj(), x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
    cairo_arc (cr->cobj(), x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
    cairo_line_to(cr->cobj(), x, height);
    cairo_line_to(cr->cobj(), x, y);
    cairo_close_path (cr->cobj());
    ink_cairo_set_source_rgba32(cr->cobj(), _rgba | 0xff);
    cr->fill();
    
    return true;
}

GdkPixbuf*
ColorPreview::toPixbuf (int width, int height)
{
    GdkRectangle carea;
    gint w2;
    w2 = width / 2;

    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *ct = cairo_create(s);

    /* Transparent area */
    carea.x = 0;
    carea.y = 0;
    carea.width = w2;
    carea.height = height;

    cairo_pattern_t *checkers = ink_cairo_pattern_create_checkerboard();

    cairo_rectangle(ct, carea.x, carea.y, carea.width, carea.height);
    cairo_set_source(ct, checkers);
    cairo_fill_preserve(ct);
    ink_cairo_set_source_rgba32(ct, _rgba);
    cairo_fill(ct);

    cairo_pattern_destroy(checkers);

    /* Solid area */
    carea.x = w2;
    carea.y = 0;
    carea.width = width - w2;
    carea.height = height;

    cairo_rectangle(ct, carea.x, carea.y, carea.width, carea.height);
    ink_cairo_set_source_rgba32(ct, _rgba | 0xff);
    cairo_fill(ct);

    cairo_destroy(ct);
    cairo_surface_flush(s);

    GdkPixbuf* pixbuf = ink_pixbuf_create_from_cairo_surface(s);
    return pixbuf;
}

}}}

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

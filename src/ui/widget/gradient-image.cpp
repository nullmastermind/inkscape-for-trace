// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * A simple gradient preview
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <sigc++/sigc++.h>

#include <glibmm/refptr.h>
#include <gdkmm/pixbuf.h>

#include <cairomm/surface.h>

#include "gradient-image.h"

#include "display/cairo-utils.h"

#include "object/sp-gradient.h"
#include "object/sp-stop.h"

namespace Inkscape {
namespace UI {
namespace Widget {
GradientImage::GradientImage(SPGradient *gradient)
    : _gradient(nullptr)
{
    set_has_window(false);
    set_gradient(gradient);
}

GradientImage::~GradientImage()
{
    if (_gradient) {
        _release_connection.disconnect();
        _modified_connection.disconnect();
        _gradient = nullptr;
    }
}

void
GradientImage::size_request(GtkRequisition *requisition) const
{
    requisition->width = 54;
    requisition->height = 12;
}

void
GradientImage::get_preferred_width_vfunc(int &minimal_width, int &natural_width) const
{
    GtkRequisition requisition;
    size_request(&requisition);
    minimal_width = natural_width = requisition.width;
}

void
GradientImage::get_preferred_height_vfunc(int &minimal_height, int &natural_height) const
{
    GtkRequisition requisition;
    size_request(&requisition);
    minimal_height = natural_height = requisition.height;
}

bool
GradientImage::on_draw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    auto allocation = get_allocation();
    
    cairo_pattern_t *check = ink_cairo_pattern_create_checkerboard();
    auto ct = cr->cobj();

    cairo_set_source(ct, check);
    cairo_paint(ct);
    cairo_pattern_destroy(check);

    if (_gradient) {
        auto p = _gradient->create_preview_pattern(allocation.get_width());
        cairo_set_source(ct, p);
        cairo_paint(ct);
        cairo_pattern_destroy(p);
    }
    
    return true;
}

void
GradientImage::set_gradient(SPGradient *gradient)
{
    if (_gradient) {
        _release_connection.disconnect();
        _modified_connection.disconnect();
    }

    _gradient = gradient;

    if (gradient) {
        _release_connection = gradient->connectRelease(sigc::mem_fun(this, &GradientImage::gradient_release));
        _modified_connection = gradient->connectModified(sigc::mem_fun(this, &GradientImage::gradient_modified));
    }

    update();
}

void
GradientImage::gradient_release(SPObject *)
{
    if (_gradient) {
        _release_connection.disconnect();
        _modified_connection.disconnect();
    }

    _gradient = nullptr;

    update();
}

void
GradientImage::gradient_modified(SPObject *, guint /*flags*/)
{
    update();
}

void
GradientImage::update()
{
    if (get_is_drawable()) {
        queue_draw();
    }
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape

GdkPixbuf*
sp_gradient_to_pixbuf (SPGradient *gr, int width, int height)
{
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *ct = cairo_create(s);

    cairo_pattern_t *check = ink_cairo_pattern_create_checkerboard();
    cairo_set_source(ct, check);
    cairo_paint(ct);
    cairo_pattern_destroy(check);

    if (gr) {
        cairo_pattern_t *p = gr->create_preview_pattern(width);
        cairo_set_source(ct, p);
        cairo_paint(ct);
        cairo_pattern_destroy(p);
    }

    cairo_destroy(ct);
    cairo_surface_flush(s);

    // no need to free s - the call below takes ownership
    GdkPixbuf *pixbuf = ink_pixbuf_create_from_cairo_surface(s);
    return pixbuf;
}


Glib::RefPtr<Gdk::Pixbuf>
sp_gradient_to_pixbuf_ref (SPGradient *gr, int width, int height)
{
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *ct = cairo_create(s);

    cairo_pattern_t *check = ink_cairo_pattern_create_checkerboard();
    cairo_set_source(ct, check);
    cairo_paint(ct);
    cairo_pattern_destroy(check);

    if (gr) {
        cairo_pattern_t *p = gr->create_preview_pattern(width);
        cairo_set_source(ct, p);
        cairo_paint(ct);
        cairo_pattern_destroy(p);
    }

    cairo_destroy(ct);
    cairo_surface_flush(s);

    Cairo::RefPtr<Cairo::Surface> sref = Cairo::RefPtr<Cairo::Surface>(new Cairo::Surface(s));
    Glib::RefPtr<Gdk::Pixbuf> pixbuf =
        Gdk::Pixbuf::create(sref, 0, 0, width, height);

    cairo_surface_destroy(s);

    return pixbuf;
}


Glib::RefPtr<Gdk::Pixbuf>
sp_gradstop_to_pixbuf_ref (SPStop *stop, int width, int height)
{
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *ct = cairo_create(s);

    /* Checkerboard background */
    cairo_pattern_t *check = ink_cairo_pattern_create_checkerboard();
    cairo_rectangle(ct, 0, 0, width, height);
    cairo_set_source(ct, check);
    cairo_fill_preserve(ct);
    cairo_pattern_destroy(check);

    if (stop) {
        /* Alpha area */
        cairo_rectangle(ct, 0, 0, width/2, height);
        ink_cairo_set_source_rgba32(ct, stop->get_rgba32());
        cairo_fill(ct);

        /* Solid area */
        cairo_rectangle(ct, width/2, 0, width, height);
        ink_cairo_set_source_rgba32(ct, stop->get_rgba32() | 0xff);
        cairo_fill(ct);
    }

    cairo_destroy(ct);
    cairo_surface_flush(s);

    Cairo::RefPtr<Cairo::Surface> sref = Cairo::RefPtr<Cairo::Surface>(new Cairo::Surface(s));
    Glib::RefPtr<Gdk::Pixbuf> pixbuf =
        Gdk::Pixbuf::create(sref, 0, 0, width, height);

    cairo_surface_destroy(s);

    return pixbuf;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :

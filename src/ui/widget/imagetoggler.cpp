// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Jon A. Cruz
 *   Johan B. C. Engelen
 *
 * Copyright (C) 2006-2008 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include "ui/widget/imagetoggler.h"

#include "ui/icon-loader.h"
#include "ui/icon-names.h"
#include "widgets/toolbox.h"

#include <iostream>

namespace Inkscape {
namespace UI {
namespace Widget {

ImageToggler::ImageToggler( char const* on, char const* off) :
    Glib::ObjectBase(typeid(ImageToggler)),
    Gtk::CellRendererPixbuf(),
    _pixOnName(on),
    _pixOffName(off),
    _property_active(*this, "active", false),
    _property_activatable(*this, "activatable", true),
    _property_pixbuf_on(*this, "pixbuf_on", Glib::RefPtr<Gdk::Pixbuf>(nullptr)),
    _property_pixbuf_off(*this, "pixbuf_off", Glib::RefPtr<Gdk::Pixbuf>(nullptr))
{
    property_mode() = Gtk::CELL_RENDERER_MODE_ACTIVATABLE;

    _property_pixbuf_on = sp_get_icon_pixbuf(_pixOnName, GTK_ICON_SIZE_MENU);
    _property_pixbuf_off = sp_get_icon_pixbuf(_pixOffName, GTK_ICON_SIZE_MENU);

    property_pixbuf() = _property_pixbuf_off.get_value();
}

void ImageToggler::get_preferred_height_vfunc(Gtk::Widget& widget,
                                              int& min_h,
                                              int& nat_h) const
{
    Gtk::CellRendererPixbuf::get_preferred_height_vfunc(widget, min_h, nat_h);

    if (min_h) {
        min_h += (min_h) >> 1;
    }
    
    if (nat_h) {
        nat_h += (nat_h) >> 1;
    }
}

void ImageToggler::get_preferred_width_vfunc(Gtk::Widget& widget,
                                             int& min_w,
                                             int& nat_w) const
{
    Gtk::CellRendererPixbuf::get_preferred_width_vfunc(widget, min_w, nat_w);

    if (min_w) {
        min_w += (min_w) >> 1;
    }
    
    if (nat_w) {
        nat_w += (nat_w) >> 1;
    }
}

void ImageToggler::render_vfunc( const Cairo::RefPtr<Cairo::Context>& cr,
                                 Gtk::Widget& widget,
                                 const Gdk::Rectangle& background_area,
                                 const Gdk::Rectangle& cell_area,
                                 Gtk::CellRendererState flags )
{
    property_pixbuf() = _property_active.get_value() ? _property_pixbuf_on : _property_pixbuf_off;
    Gtk::CellRendererPixbuf::render_vfunc( cr, widget, background_area, cell_area, flags );
}

bool
ImageToggler::activate_vfunc(GdkEvent* event,
                            Gtk::Widget& /*widget*/,
                            const Glib::ustring& path,
                            const Gdk::Rectangle& /*background_area*/,
                            const Gdk::Rectangle& /*cell_area*/,
                            Gtk::CellRendererState /*flags*/)
{
    _signal_pre_toggle.emit(event);
    _signal_toggled.emit(path);

    return false;
}


} // namespace Widget
} // namespace UI
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :



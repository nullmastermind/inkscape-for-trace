// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Theodore Janeczko
 *
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/widget/layertypeicon.h"

#include "ui/icon-loader.h"
#include "ui/icon-names.h"
#include "widgets/toolbox.h"

namespace Inkscape {
namespace UI {
namespace Widget {

LayerTypeIcon::LayerTypeIcon() :
    Glib::ObjectBase(typeid(LayerTypeIcon)),
    Gtk::CellRendererPixbuf(),
    _pixLayerName(INKSCAPE_ICON("dialog-layers")),
    _pixGroupName(INKSCAPE_ICON("layer-duplicate")),
    _pixPathName(INKSCAPE_ICON("layer-rename")),
    _property_active(*this, "active", false),
    _property_activatable(*this, "activatable", true),
    _property_pixbuf_layer(*this, "pixbuf_on", Glib::RefPtr<Gdk::Pixbuf>(nullptr)),
    _property_pixbuf_group(*this, "pixbuf_off", Glib::RefPtr<Gdk::Pixbuf>(nullptr)),
    _property_pixbuf_path(*this, "pixbuf_off", Glib::RefPtr<Gdk::Pixbuf>(nullptr))
{
    
    property_mode() = Gtk::CELL_RENDERER_MODE_ACTIVATABLE;

    _property_pixbuf_layer = sp_get_icon_pixbuf(_pixLayerName, GTK_ICON_SIZE_MENU);
    _property_pixbuf_group = sp_get_icon_pixbuf(_pixGroupName, GTK_ICON_SIZE_MENU);
    _property_pixbuf_path = sp_get_icon_pixbuf(_pixPathName, GTK_ICON_SIZE_MENU);

    property_pixbuf() = _property_pixbuf_path.get_value();
}

void LayerTypeIcon::get_preferred_height_vfunc(Gtk::Widget& widget,
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

void LayerTypeIcon::get_preferred_width_vfunc(Gtk::Widget& widget,
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

void LayerTypeIcon::render_vfunc( const Cairo::RefPtr<Cairo::Context>& cr,
                                 Gtk::Widget& widget,
                                 const Gdk::Rectangle& background_area,
                                 const Gdk::Rectangle& cell_area,
                                 Gtk::CellRendererState flags )
{
    property_pixbuf() = _property_active.get_value() == 1 ? _property_pixbuf_group : (_property_active.get_value() == 2 ? _property_pixbuf_layer : _property_pixbuf_path);
    Gtk::CellRendererPixbuf::render_vfunc( cr, widget, background_area, cell_area, flags );
}

bool
LayerTypeIcon::activate_vfunc(GdkEvent* event,
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



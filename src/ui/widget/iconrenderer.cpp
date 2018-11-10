// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Theodore Janeczko
 *
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *               Martin Owens 2018 <doctormo@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/widget/iconrenderer.h"

#include "layertypeicon.h"
#include "ui/icon-loader.h"
#include "ui/icon-names.h"
#include "widgets/toolbox.h"

namespace Inkscape {
namespace UI {
namespace Widget {

IconRenderer::IconRenderer() :
    Glib::ObjectBase(typeid(IconRenderer)),
    Gtk::CellRendererPixbuf(),
    _property_icon(*this, "icon", 0)
{
    property_mode() = Gtk::CELL_RENDERER_MODE_ACTIVATABLE;
    set_pixbuf();
}

/*
 * Called when an icon is clicked.
 */
IconRenderer::type_signal_activated IconRenderer::signal_activated()
{
    return m_signal_activated;
}

void IconRenderer::get_preferred_height_vfunc(Gtk::Widget& widget,
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

void IconRenderer::get_preferred_width_vfunc(Gtk::Widget& widget,
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

void IconRenderer::render_vfunc( const Cairo::RefPtr<Cairo::Context>& cr,
                                 Gtk::Widget& widget,
                                 const Gdk::Rectangle& background_area,
                                 const Gdk::Rectangle& cell_area,
                                 Gtk::CellRendererState flags )
{
    set_pixbuf();
    
    Gtk::CellRendererPixbuf::render_vfunc( cr, widget, background_area, cell_area, flags );
}

bool IconRenderer::activate_vfunc(GdkEvent* /*event*/,
                               Gtk::Widget& /*widget*/,
                               const Glib::ustring& path,
                               const Gdk::Rectangle& /*background_area*/,
                               const Gdk::Rectangle& /*cell_area*/,
                               Gtk::CellRendererState /*flags*/)
{
    m_signal_activated.emit(path);
    return true;
}

void IconRenderer::add_icon(Glib::ustring name)
{
    _icons.push_back(sp_get_icon_pixbuf(name.c_str(), GTK_ICON_SIZE_BUTTON));
}

void IconRenderer::set_pixbuf()
{
    int icon_index = property_icon().get_value();
    if(icon_index >= 0 && icon_index < _icons.size()) {
        property_pixbuf() = _icons[icon_index];
    } else {
        property_pixbuf() = sp_get_icon_pixbuf("image-missing", GTK_ICON_SIZE_BUTTON);
    }
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

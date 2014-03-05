/**
 * @file
 * Inkscape color swatch UI item.
 */
/* Authors:
 *   Jon A. Cruz
 *   Abhishek Sharma
 *
 * Copyright (C) 2010 Jon A. Cruz
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <errno.h>
#include <glibmm/i18n.h>
#include <gtkmm/label.h>
#include <cairo.h>
#include <gtk/gtk.h>

#include "color-item.h"

#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-style.h"
#include "display/cairo-utils.h"
#include "document.h"
#include "document-undo.h"
#include "inkscape.h" // for SP_ACTIVE_DESKTOP
#include "io/resource.h"
#include "io/sys.h"
#include "message-context.h"
#include "sp-gradient.h"
#include "sp-item.h"
#include "svg/svg-color.h"
#include "xml/node.h"
#include "xml/repr.h"
#include "verbs.h"
#include "widgets/gradient-vector.h"
#include "sp-paint-server.h"

#include "color.h" // for SP_RGBA32_U_COMPOSE


namespace Inkscape {
namespace UI {
namespace Dialogs {


bool ColorItem::on_enter_notify_event(GdkEventCrossing* event)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if ( desktop ) {
        gchar* msg = g_strdup_printf(_("Color: <b>%s</b>; <b>Click</b> to set fill, <b>Shift+click</b> to set stroke"),def);
        desktop->tipsMessageContext()->set(Inkscape::INFORMATION_MESSAGE, msg);
        g_free(msg);
    }
    return Gtk::Widget::on_enter_notify_event(event);
}

bool ColorItem::on_leave_notify_event(GdkEventCrossing* event)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if ( desktop ) {
        desktop->tipsMessageContext()->clear();
    }
    return Gtk::Widget::on_leave_notify_event(event);
}

void ColorItem::selection_modified(Selection* selection, guint flags)
{
    selection_changed(selection);
}

void ColorItem::selection_changed(Selection* selection)
{
    SPItem* item = selection->singleItem();
    SPPaintServer* grad;
    if (item && 
    (
        (item->style->fill.isPaintserver() && 
            SP_IS_GRADIENT( (grad = item->style->getFillPaintServer()) )
            && SP_GRADIENT(grad)->getVector() == gradient) ||

            (item->style->stroke.isPaintserver() && 
            SP_IS_GRADIENT( (grad = item->style->getStrokePaintServer()) ) && 
            SP_GRADIENT(grad)->getVector() == gradient)
    )
    ) 
    {
        if (!_isSelected) {
            _isSelected = true;
            queue_draw();
        }
    } else if (_isSelected) {
        _isSelected = false;
        queue_draw();
    }
}

ColorItem::ColorItem( SPGradient* grad, const gchar* name, SPDesktop* desktop ) :
    Glib::ObjectBase("coloritem"),
    Gtk::Widget(),
    def( name ),
    gradient(grad),
    _isSelected(false)
{
    set_has_window(true);
    add_events(Gdk::BUTTON_PRESS_MASK);
    add_events(Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK);
    sel_connection = desktop->selection->connectChanged(sigc::mem_fun(*this, &ColorItem::selection_changed));
    mod_connection = desktop->selection->connectModified(sigc::mem_fun(*this, &ColorItem::selection_modified));
    selection_changed(desktop->selection);
}

void ColorItem::on_size_request(Gtk::Requisition* requisition)
{
    requisition->height = 20;
    requisition->width = 20;
}

void ColorItem::on_size_allocate(Gtk::Allocation& allocation)
{
    set_allocation(allocation);
    if (m_refGdkWindow)
    {
        m_refGdkWindow->move_resize( allocation.get_x(), allocation.get_y(), allocation.get_width(), allocation.get_height() );
    }
}

void ColorItem::on_map()
{
    Gtk::Widget::on_map();
}

void ColorItem::on_unmap()
{
    Gtk::Widget::on_unmap();
}

void ColorItem::on_realize()
{
    set_realized();
    ensure_style();
    
   if(!m_refGdkWindow)
  {
    //Create the GdkWindow:

    GdkWindowAttr attributes;
    memset(&attributes, 0, sizeof(attributes));

    Gtk::Allocation allocation = get_allocation();

    //Set initial position and size of the Gdk::Window:
    attributes.x = allocation.get_x();
    attributes.y = allocation.get_y();
    attributes.width = allocation.get_width();
    attributes.height = allocation.get_height();

    attributes.event_mask = get_events () | Gdk::EXPOSURE_MASK; 
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.wclass = GDK_INPUT_OUTPUT;

    m_refGdkWindow = Gdk::Window::create(get_parent_window(), &attributes,
            GDK_WA_X | GDK_WA_Y);
    set_window(m_refGdkWindow);

    //Attach this widget's style to its Gdk::Window.
    style_attach();

    //make the widget receive expose events
    m_refGdkWindow->set_user_data(gobj());
  }
}

void ColorItem::on_unrealize()
{
    m_refGdkWindow.reset();
    
    Gtk::Widget::on_unrealize();
}

bool ColorItem::on_expose_event(GdkEventExpose* event)
{
    if(m_refGdkWindow)
    {
        Cairo::RefPtr<Cairo::Context> cr = m_refGdkWindow->create_cairo_context();
        if(event)
        {
            // clip to the area that needs to be re-exposed so we don't draw any
            // more than we need to.
            cr->rectangle(event->area.x, event->area.y, event->area.width, event->area.height);
            cr->clip();
        }

        if (gradient) {
            cairo_pattern_t *check = ink_cairo_pattern_create_checkerboard();
        
            Cairo::RefPtr<Cairo::Pattern> checkpat(new Cairo::Pattern(check));

            cr->set_source(checkpat);
            cr->paint();
            
            cairo_pattern_t *g = sp_gradient_create_preview_pattern(gradient, get_allocation().get_width());
            Cairo::RefPtr<Cairo::Pattern> gpat(new Cairo::Pattern(g));
            cr->set_source(gpat);
            cr->paint();
            gpat.clear();
            cairo_pattern_destroy(g);
            
            checkpat.clear();
        
            cairo_pattern_destroy(check);
            
            if (_isSelected) {
                cr->set_source_rgb(0, 0, 0);
                cr->set_line_width(3);
                cr->move_to(0, get_allocation().get_height());
                cr->line_to(0,0);
                cr->line_to(get_allocation().get_width(), 0);
                cr->stroke();
                cr->move_to(get_allocation().get_width(), 0);
                cr->set_source_rgb(1, 1, 1);
                cr->line_to(get_allocation().get_width(), get_allocation().get_height());
                cr->line_to(0, get_allocation().get_height());
                //cr->rectangle(0, 0, get_allocation().get_width(), get_allocation().get_height());
                cr->stroke();
            }
        } else {
            cr->set_source_rgb(1, 1, 1);
            cr->paint();
            cr->set_source_rgb(1, 0, 0);
            cr->set_line_width(3);
            cr->move_to(0,0);
            cr->line_to(get_allocation().get_width(), get_allocation().get_height());
            cr->move_to(get_allocation().get_width(), 0);
            cr->line_to(0, get_allocation().get_height());
            cr->stroke();
        }
    }
    return true;
}

ColorItem::~ColorItem()
{
    sel_connection.disconnect();
    mod_connection.disconnect();
}

} // namespace Dialogs
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

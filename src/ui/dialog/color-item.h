/** @file
 * @brief Inkscape color swatch UI item.
 */
/* Authors:
 *   Jon A. Cruz
 *
 * Copyright (C) 2010 Jon A. Cruz
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SEEN_DIALOGS_COLOR_ITEM_H
#define SEEN_DIALOGS_COLOR_ITEM_H

#include <boost/ptr_container/ptr_vector.hpp>

#include "widgets/ege-paint-def.h"
#include "widgets/eek-preview.h"
#include <gtk/gtk.h>
#include <gtkmm/widget.h>
#include "desktop.h"
#include "selection.h"

class SPGradient;

namespace Inkscape {
namespace UI {
namespace Dialogs {


/**
 * The color swatch you see on screen as a clickable box.
 */
class ColorItem : public Gtk::Widget
{
public:
    ColorItem( SPGradient * grad, const gchar* name, SPDesktop* desktop );
    virtual ~ColorItem();
    
    SPGradient * getGradient() { return gradient; }

protected:    
    
    const gchar* def;
    SPGradient * gradient;
    virtual bool on_enter_notify_event(GdkEventCrossing* event);
    virtual bool on_leave_notify_event(GdkEventCrossing* event);
    
    virtual void on_size_request(Gtk::Requisition* requisition);
    virtual void on_size_allocate(Gtk::Allocation& allocation);
    virtual void on_map();
    virtual void on_unmap();
    virtual void on_realize();
    virtual void on_unrealize();
    virtual bool on_expose_event(GdkEventExpose* event);

    Glib::RefPtr<Gdk::Window> m_refGdkWindow;

    
private:
    void selection_changed(Selection* selection);
    void selection_modified(Selection* selection, guint flags);

    sigc::connection sel_connection;
    sigc::connection mod_connection;
    bool _isSelected;
};

} // namespace Dialogs
} // namespace UI
} // namespace Inkscape

#endif // SEEN_DIALOGS_COLOR_ITEM_H

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

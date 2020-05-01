// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * A desktop dock pane to dock dialogs.
 */
/* Author:
 *   Gustav Broberg <broberg@kth.se>
 *
 * Copyright (C) 2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#include "dock.h"
#include "io/resource.h"
#include "preferences.h"
#include "desktop.h"

#include <gtkmm/adjustment.h>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>

namespace Inkscape {
namespace UI {
namespace Widget {

const int Dock::_default_empty_width = 0;
const int Dock::_default_dock_bar_width = 36;

/**
 * Get the file name of the user's dock layout.
 * @return Pointer to static string.
 */
static const char *get_docklayout_filename()
{
    static auto *filename = Inkscape::IO::Resource::profile_path("docklayout.xml");
    return filename;
}

Dock::Dock(Gtk::Orientation orientation)
    : _gdl_dock(gdl_dock_new()),
      _gdl_dock_bar(GDL_DOCK_BAR(gdl_dock_bar_new(G_OBJECT(_gdl_dock)))),
      _scrolled_window (Gtk::manage(new Gtk::ScrolledWindow))
{
    _gdl_layout = gdl_dock_layout_new(G_OBJECT(_gdl_dock));

    gtk_widget_set_name(_gdl_dock, "GdlDock");

    gtk_orientable_set_orientation(GTK_ORIENTABLE(_gdl_dock_bar),
                                   static_cast<GtkOrientation>(orientation));

    _dock_box = Gtk::manage(new Gtk::Box(orientation == Gtk::ORIENTATION_HORIZONTAL ?
                                         Gtk::ORIENTATION_VERTICAL : Gtk::ORIENTATION_HORIZONTAL));
    _dock_box->set_name("DockBox");
    _dock_box->pack_start(*Gtk::manage(Glib::wrap(GTK_WIDGET(_gdl_dock))));
    _dock_box->pack_end(*Gtk::manage(Glib::wrap(GTK_WIDGET(_gdl_dock_bar))), Gtk::PACK_SHRINK);

    _scrolled_window->set_name("DockScrolledWindow");
    _scrolled_window->add(*_dock_box);
    _scrolled_window->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    _scrolled_window->set_size_request(0);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    GdlSwitcherStyle gdl_switcher_style =
        static_cast<GdlSwitcherStyle>(prefs->getIntLimited("/options/dock/switcherstyle",
                                                                      GDL_SWITCHER_STYLE_BOTH, 0, 4));

    auto filleritem = gdl_dock_item_new("filleritem", "",
                                        static_cast<GdlDockItemBehavior>(        //
                                            GDL_DOCK_ITEM_BEH_NEVER_FLOATING |   //
                                            GDL_DOCK_ITEM_BEH_NEVER_HORIZONTAL | //
                                            GDL_DOCK_ITEM_BEH_LOCKED |           //
                                            GDL_DOCK_ITEM_BEH_CANT_DOCK_CENTER | //
                                            GDL_DOCK_ITEM_BEH_CANT_CLOSE | GDL_DOCK_ITEM_BEH_CANT_ICONIFY |
                                            GDL_DOCK_ITEM_BEH_NO_GRIP));
    gdl_dock_add_item(GDL_DOCK(_gdl_dock),
                      GDL_DOCK_ITEM(filleritem),
                      GDL_DOCK_BOTTOM);

    GdlDockMaster *master = GDL_DOCK_MASTER(gdl_dock_object_get_master(GDL_DOCK_OBJECT(_gdl_dock)));

    g_object_set(master,
            "switcher-style", gdl_switcher_style,
            NULL);

    GdlDockBarStyle gdl_dock_bar_style =
        static_cast<GdlDockBarStyle>(prefs->getIntLimited("/options/dock/dockbarstyle",
                                                                     GDL_DOCK_BAR_BOTH, 0, 3));

    gdl_dock_bar_set_style(_gdl_dock_bar, gdl_dock_bar_style);

    _scrolled_window->show_all();
}

Dock::~Dock()
{
    // assume that releaseAllReferences has been called
    g_assert(_dock_items.empty());
}

void Dock::releaseAllReferences()
{
    for (auto &conn : _connections) {
        conn.disconnect();
    }

    saveLayout();

    _dock_items.clear();
}

/**
 * Restore the layout from the config file and connect the "layout-changed" signal.
 *
 * This method must only be called once.
 */
void Dock::restoreLayout()
{
    if (!_dock_items.empty() && //
        gdl_dock_layout_load_from_file(_gdl_layout, get_docklayout_filename())) {
        gdl_dock_layout_load_layout(_gdl_layout, nullptr);
    }

    _connections.emplace_back(
        signal_layout_changed().connect(sigc::mem_fun(*this, &Inkscape::UI::Widget::Dock::_onLayoutChanged)));
}

/**
 * Store the layout to the config file.
 */
void Dock::saveLayout()
{
    gdl_dock_layout_save_layout(_gdl_layout, nullptr);
    gdl_dock_layout_save_to_file(_gdl_layout, get_docklayout_filename());
}

void Dock::addItem(DockItem& item, GdlDockPlacement placement)
{
    _dock_items.push_back(&item);

    gdl_dock_add_item(GDL_DOCK(_gdl_dock),
                      GDL_DOCK_ITEM(item.gobj()),
                      placement);
}

Gtk::Widget &Dock::getWidget()
{
     return *_scrolled_window;
}

Gtk::Paned *Dock::getParentPaned()
{
    g_return_val_if_fail(_dock_box, 0);
    Gtk::Container *parent = getWidget().get_parent();
    return (parent != nullptr ? dynamic_cast<Gtk::Paned *>(parent) : nullptr);
}

GtkWidget *Dock::getGdlWidget()
{
    return GTK_WIDGET(_gdl_dock);
}

bool Dock::isEmpty() const
{
    for (auto *item : _dock_items) {
        if (item->getState() == DockItem::DOCKED_STATE) {
            return false;
        }
    }

    return true;
}

bool Dock::hasIconifiedItems() const
{
    for (auto *item : _dock_items) {
        if (item->isIconified()) {
            return true;
        }
    }

    return false;
}

void Dock::hide()
{
    getWidget().hide();
}

void Dock::show()
{
    getWidget().show();
}

void Dock::toggleDockable(int width, int height)
{
    static int prev_horizontal_position = 0;

    Gtk::Paned *parent_paned = getParentPaned();

    if (width > 0 && height > 0) {
        prev_horizontal_position = parent_paned->get_position();

        if (getWidget().get_width() < width)
            parent_paned->set_position(parent_paned->get_width() - width);

    } else {
        parent_paned->set_position(prev_horizontal_position);
    }
}

void Dock::scrollToItem(DockItem& item)
{
    int item_x, item_y;
    item.getWidget().translate_coordinates(getWidget(), 0, 0, item_x, item_y);

    int dock_height = getWidget().get_height(), item_height = item.getWidget().get_height();
    double vadjustment = _scrolled_window->get_vadjustment()->get_value();

    if (item_y < 0)
        _scrolled_window->get_vadjustment()->set_value(vadjustment + item_y);
    else if (item_y + item_height > dock_height)
        _scrolled_window->get_vadjustment()->set_value(
            vadjustment + ((item_y + item_height) - dock_height));
}

Glib::SignalProxy0<void>
Dock::signal_layout_changed()
{
    return Glib::SignalProxy0<void>(Glib::wrap(GTK_WIDGET(_gdl_dock)),
                                    &_signal_layout_changed_proxy);
}

void Dock::_onLayoutChanged()
{
    if (isEmpty()) {
        getParentPaned()->set_position(10000);
    }
}

const Glib::SignalProxyInfo
Dock::_signal_layout_changed_proxy =
{
    "layout-changed",
    (GCallback) &Glib::SignalProxyNormal::slot0_void_callback,
    (GCallback) &Glib::SignalProxyNormal::slot0_void_callback
};


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

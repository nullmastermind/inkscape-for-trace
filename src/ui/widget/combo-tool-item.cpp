// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright  (C) 2017 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


/** \file
    A combobox that can be displayed in a toolbar.
*/

#include "combo-tool-item.h"

#include <iostream>
#include <utility>
#include <gtkmm/toolitem.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/radiomenuitem.h>
#include <gtkmm/combobox.h>
#include <gtkmm/menu.h>
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/image.h>

namespace Inkscape {
namespace UI {
namespace Widget {

ComboToolItem*
ComboToolItem::create(const Glib::ustring &group_label,
                      const Glib::ustring &tooltip,
                      const Glib::ustring &stock_id,
                      Glib::RefPtr<Gtk::ListStore> store )
{
    return new ComboToolItem(group_label, tooltip, stock_id, store);
}

ComboToolItem::ComboToolItem(const Glib::ustring &group_label,
                             const Glib::ustring &tooltip,
                             const Glib::ustring &stock_id,
                             Glib::RefPtr<Gtk::ListStore> store ) :
    _group_label( group_label ),
    _tooltip( tooltip ),
    _stock_id( stock_id ),
    _store (std::move(store)),
    _use_label (true),
    _use_icon  (true),
    _use_pixbuf (false),
    _icon_size ( Gtk::ICON_SIZE_LARGE_TOOLBAR ),
    _combobox (nullptr),
    _menuitem (nullptr)
{
    Gtk::Box* box = Gtk::manage(new Gtk::Box());
    add(*box);

    if (_use_group_label) {
        Gtk::Label *group_label = Gtk::manage (new Gtk::Label( _group_label + ": " ));
        box->add( *group_label );
    }

    // Create combobox
    _combobox = Gtk::manage (new Gtk::ComboBox());
    _combobox->set_model(_store);

    ComboToolItemColumns columns;
    if (_use_icon) {
        Gtk::CellRendererPixbuf *renderer = new Gtk::CellRendererPixbuf;
        renderer->set_property ("stock_size", Gtk::ICON_SIZE_LARGE_TOOLBAR);
        _combobox->pack_start (*renderer, false);
        _combobox->add_attribute (*renderer, "icon_name", columns.col_icon   );
    } else if (_use_pixbuf) {
        Gtk::CellRendererPixbuf *renderer = new Gtk::CellRendererPixbuf;
        //renderer->set_property ("stock_size", Gtk::ICON_SIZE_LARGE_TOOLBAR);
        _combobox->pack_start (*renderer, false);
        _combobox->add_attribute (*renderer, "pixbuf", columns.col_pixbuf   );
    }

    if (_use_label) {
        _combobox->pack_start(columns.col_label);
    }

    std::vector<Gtk::CellRenderer*> cells = _combobox->get_cells();
    for (auto & cell : cells) {
        _combobox->add_attribute (*cell, "sensitive", columns.col_sensitive);
    }

    _combobox->set_active (_active);

    _combobox->signal_changed().connect(
            sigc::mem_fun(*this, &ComboToolItem::on_changed_combobox));

    box->add (*_combobox);

    show_all();
}

void
ComboToolItem::set_active (gint active) {

    if (active < 0) {
        std::cerr << "ComboToolItem::set_active: active < 0: " << active << std::endl;
        return;
    }

    if (_active != active) {

        _active = active;

        if (_combobox) {
            _combobox->set_active (active);
        }

        if (active < _radiomenuitems.size()) {
            _radiomenuitems[ active ]->set_active();
        }
    }
}

Glib::ustring
ComboToolItem::get_active_text () {
    Gtk::TreeModel::Row row = _store->children()[_active];
    ComboToolItemColumns columns;
    Glib::ustring label = row[columns.col_label];
    return label;
}

bool
ComboToolItem::on_create_menu_proxy()
{
    if (_menuitem == nullptr) {

        _menuitem = Gtk::manage (new Gtk::MenuItem);
        Gtk::Menu *menu = Gtk::manage (new Gtk::Menu);

        Gtk::RadioButton::Group group;
        int index = 0;
        auto children = _store->children();
        for (auto row : children) {
            ComboToolItemColumns columns;
            Glib::ustring label     = row[columns.col_label     ];
            Glib::ustring icon      = row[columns.col_icon      ];
            Glib::ustring tooltip   = row[columns.col_tooltip   ];
            bool          sensitive = row[columns.col_sensitive ];

            Gtk::RadioMenuItem* button = Gtk::manage(new Gtk::RadioMenuItem(group));
            button->set_label (label);
            button->set_tooltip_text( tooltip );
            button->set_sensitive( sensitive );

            button->signal_toggled().connect( sigc::bind<0>(
              sigc::mem_fun(*this, &ComboToolItem::on_toggled_radiomenu), index++)
                );

            menu->add (*button);

            _radiomenuitems.push_back( button );
        }

        if ( _active < _radiomenuitems.size()) {
            _radiomenuitems[ _active ]->set_active();
        }
   
        _menuitem->set_submenu (*menu);
        _menuitem->show_all();
    }

    set_proxy_menu_item(_group_label, *_menuitem);
    return true;
}

void
ComboToolItem::on_changed_combobox() {

    int row = _combobox->get_active_row_number();
    if (row < 0) row = 0;  // Happens when Gtk::ListStore reconstructed
    set_active( row );
    _changed.emit (_active);
    _changed_after.emit (_active);
}

void
ComboToolItem::on_toggled_radiomenu(int n) {

    // toggled emitted twice, first for button toggled off, second for button toggled on.
    // We want to react only to the button turned on.
    if ( n < _radiomenuitems.size() &&_radiomenuitems[ n ]->get_active()) {
        set_active ( n );
        _changed.emit (_active);
        _changed_after.emit (_active);
    }
}

}
}
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

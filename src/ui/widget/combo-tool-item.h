// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_COMBO_TOOL_ITEM
#define SEEN_COMBO_TOOL_ITEM

/*
 * Authors:
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright  (C) 2017 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

/**
   A combobox that can be displayed in a toolbar
*/

#include <gtkmm/toolitem.h>
#include <gtkmm/liststore.h>
#include <sigc++/sigc++.h>
#include <vector>

namespace Gtk {
class Box;
class ComboBox;
class Label;
class MenuItem;
class RadioMenuItem;
}

namespace Inkscape {
namespace UI {
namespace Widget {
class ComboToolItemColumns : public Gtk::TreeModel::ColumnRecord {
public:
    ComboToolItemColumns() {
        add (col_label);
        add (col_value);
        add (col_icon);
        add (col_pixbuf);
        add (col_data);  // Used to store a pointer
        add (col_tooltip);
        add (col_sensitive);
    }
    Gtk::TreeModelColumn<Glib::ustring> col_label;
    Gtk::TreeModelColumn<Glib::ustring> col_value;
    Gtk::TreeModelColumn<Glib::ustring> col_icon;
    Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> >   col_pixbuf;
    Gtk::TreeModelColumn<void *>        col_data;
    Gtk::TreeModelColumn<Glib::ustring> col_tooltip;
    Gtk::TreeModelColumn<bool>          col_sensitive;
};


class ComboToolItem : public Gtk::ToolItem {

public:
    static ComboToolItem* create(const Glib::ustring &label,
                                 const Glib::ustring &tooltip,
                                 const Glib::ustring &stock_id,
                                 Glib::RefPtr<Gtk::ListStore> store,
                                 bool                 has_entry = false);

    /* Style of combobox */
    void use_label(  bool use_label  );
    void use_icon(   bool use_icon   );
    void focus_on_click( bool focus_on_click );
    void use_pixbuf( bool use_pixbuf );
    void use_group_label( bool use_group_label ); // Applies to tool item only
  
    gint get_active() { return _active; }
    Glib::ustring get_active_text();
    void set_active( gint active );
    void set_icon_size( Gtk::BuiltinIconSize size ) { _icon_size = size; }

    Glib::RefPtr<Gtk::ListStore> get_store() { return _store; }

    sigc::signal<void, int> signal_changed() { return _changed; }
    sigc::signal<void, int> signal_changed_after() { return _changed_after; }

protected:
    bool on_create_menu_proxy() override;
    void populate_combobox();

    /* Signals */
    sigc::signal<void, int> _changed;
    sigc::signal<void, int> _changed_after;  // Needed for unit tracker which eats _changed.

private:

    Glib::ustring _group_label;
    Glib::ustring _tooltip;
    Glib::ustring _stock_id;
    Glib::RefPtr<Gtk::ListStore> _store;

    gint _active;  /* Active menu item/button */

    /* Style */
    bool _use_label;
    bool _use_icon;   // Applies to menu item only
    bool _use_pixbuf;
    Gtk::BuiltinIconSize _icon_size;

    /* Combobox in tool */
    Gtk::ComboBox* _combobox;
    Gtk::Label* _group_label_widget;
    Gtk::Box* _container;

    Gtk::MenuItem* _menuitem;
    std::vector<Gtk::RadioMenuItem*> _radiomenuitems;

    /* Internal Callbacks */
    void on_changed_combobox();
    void on_toggled_radiomenu(int n);

    ComboToolItem(Glib::ustring group_label,
                  Glib::ustring tooltip,
                  Glib::ustring stock_id,
                  Glib::RefPtr<Gtk::ListStore> store,
                  bool          has_entry = false);
};
}
}
}
#endif /* SEEN_COMBO_TOOL_ITEM */

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

#ifndef SEEN_INK_SELECT_ONE_ACTION
#define SEEN_INK_SELECT_ONE_ACTION

/*
 * Authors:
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright  (C) 2017 Tavmjong Bah
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

/**
   An action (menu/toolbar item) that allows selecting one choice out of many.

   The choices may be displayed as:

   1. A group of items in a toolbar with labels and/or icons.
   2. As a drop-down menu with a labels and/or icons.
*/

#include <gtkmm/action.h>
#include <gtkmm/liststore.h>
#include <sigc++/sigc++.h>
#include <vector>

namespace Gtk {
class ComboBox;
class RadioAction;
class MenuItem;
class RadioMenuItem;
}

class InkSelectOneActionColumns : public Gtk::TreeModel::ColumnRecord {

public:
    InkSelectOneActionColumns() {
        add (col_label);
        add (col_icon);
        add (col_pixbuf);
        add (col_data);  // Used to store a pointer
        add (col_tooltip);
        add (col_sensitive);
    }
    Gtk::TreeModelColumn<Glib::ustring> col_label;
    Gtk::TreeModelColumn<Glib::ustring> col_icon;
    Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> >   col_pixbuf;
    Gtk::TreeModelColumn<void *>        col_data;
    Gtk::TreeModelColumn<Glib::ustring> col_tooltip;
    Gtk::TreeModelColumn<bool>          col_sensitive;
};


class InkSelectOneAction : public Gtk::Action {

public:

    static InkSelectOneAction* create(const Glib::ustring &name,
                                      const Glib::ustring &label,
                                      const Glib::ustring &tooltip,
                                      const Glib::ustring &stock_id,
                                      Glib::RefPtr<Gtk::ListStore> store );

    /* Style of action */
    void use_radio(  bool use_radio  ) { _use_radio  = use_radio;  }
    void use_label(  bool use_label  ) { _use_label  = use_label;  }
    void use_icon(   bool use_icon   ) { _use_icon   = use_icon;   }
    void use_pixbuf( bool use_pixbuf ) { _use_pixbuf = use_pixbuf; }
    void use_group_label( bool use_group_label ) { _use_group_label = use_group_label; }
  
    gint get_active() { return _active; }
    Glib::ustring get_active_text();
    void set_active( gint active );
    void set_icon_size( Gtk::BuiltinIconSize size ) { _icon_size = size; }

    Glib::RefPtr<Gtk::ListStore> get_store() { return _store; }

    sigc::signal<void, int> signal_changed() { return _changed; }
    sigc::signal<void, int> signal_changed_after() { return _changed_after; }

protected:

    Gtk::Widget* create_menu_item_vfunc() override;
    Gtk::Widget* create_tool_item_vfunc() override;

    /* Signals */
    sigc::signal<void, int> _changed;
    sigc::signal<void, int> _changed_after;  // Needed for unit tracker which eats _changed.

private:

    Glib::ustring _name;
    Glib::ustring _group_label;
    Glib::ustring _tooltip;
    Glib::ustring _stock_id;
    Glib::RefPtr<Gtk::ListStore> _store;

    gint _active;  /* Active menu item/button */

    /* Style */
    bool _use_radio;  // Applies to tool item only
    bool _use_label;
    bool _use_icon;   // Applies to menu item only
    bool _use_pixbuf;
    bool _use_group_label; // Applies to tool item only
    Gtk::BuiltinIconSize _icon_size;

    /* Combobox in tool */
    Gtk::ComboBox* _combobox;
  
    /* Need to track one action to get active action. */
    Glib::RefPtr<Gtk::RadioAction> _radioaction;

    Gtk::MenuItem* _menuitem;
    std::vector<Gtk::RadioMenuItem*> _radiomenuitems;

    /* Internal Callbacks */
    void on_changed_combobox();
    void on_changed_radioaction(const Glib::RefPtr<Gtk::RadioAction>& current);
    void on_toggled_radiomenu(int n);

    InkSelectOneAction (const Glib::ustring &name,
                        const Glib::ustring &group_label,
                        const Glib::ustring &tooltip,
                        const Glib::ustring &stock_id,
                        Glib::RefPtr<Gtk::ListStore> store );

};


#endif /* SEEN_INK_SELECT_ONE_ACTION */

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

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A class derived from Gtk::ToolItem that wraps a GtkComboBoxEntry.
 * Features:
 *   Setting GtkEntryBox width in characters.
 *   Passing a function for formatting cells.
 *   Displaying a warning if entry text isn't in list.
 *   Check comma separated values in text against list. (Useful for font-family fallbacks.)
 *   Setting names for GtkComboBoxEntry and GtkEntry (actionName_combobox, actionName_entry)
 *     to allow setting resources.
 *
 * Author(s):
 *   Tavmjong Bah
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2010 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INK_COMBOBOXENTRY_ACTION
#define SEEN_INK_COMBOBOXENTRY_ACTION

#include <gtkmm/toolitem.h>

namespace Inkscape {
namespace UI {
namespace Widget {

/**
 * Creates a Gtk::ToolItem subclass that wraps a Gtk::ComboBox object.
 */
class ComboBoxEntryToolItem : public Gtk::ToolItem {
private:
    Glib::ustring       _tooltip;
    Glib::ustring       _label;
    GtkTreeModel       *_model; ///< Tree Model
    GtkComboBox        *_combobox;
    GtkEntry           *_entry;
    gint                _entry_width;// Width of GtkEntry in characters.
    gint                _extra_width;// Extra Width of GtkComboBox.. to widen drop-down list in list mode.
    gpointer            _cell_data_func; // drop-down menu format
    gpointer            _separator_func;
    gboolean            _popup;      // Do we pop-up an entry-completion dialog?
    GtkEntryCompletion *_entry_completion;
    GtkWidget          *_focusWidget; ///< The widget to return focus to
    GtkCellRenderer    *_cell;

    gint                _active;     // Index of active menu item (-1 if not in list).
    gchar              *_text;       // Text of active menu item or entry box.
    gchar              *_info;       // Text for tooltip info about entry.
    gpointer            _info_cb;    // Callback for clicking info icon.
    gint                _info_cb_id;
    gboolean            _info_cb_blocked;
    gchar              *_warning;    // Text for tooltip warning that entry isn't in list.
    gpointer            _warning_cb; // Callback for clicking warning icon.
    gint                _warning_cb_id;
    gboolean            _warning_cb_blocked;

    // Signals
    sigc::signal<void> _signal_changed;

    static gint get_active_row_from_text(ComboBoxEntryToolItem *action,
                                         const gchar         *target_text,
	                                 gboolean             exclude     = false,
                                         gboolean             ignore_case = false);
    void defocus();

    static void combo_box_changed_cb( GtkComboBox* widget, gpointer data );
    static gboolean combo_box_popup_cb( ComboBoxEntryToolItem* widget, gpointer data );
    static void entry_activate_cb( GtkEntry *widget,
                                   gpointer  data );
    static gboolean match_selected_cb( GtkEntryCompletion *widget,
                                       GtkTreeModel       *model,
                                       GtkTreeIter        *iter,
                                       gpointer            data);
    static gboolean keypress_cb( GtkWidget   *widget,
                                 GdkEventKey *event,
                                 gpointer     data );

    Glib::ustring check_comma_separated_text();
 
public:
    ComboBoxEntryToolItem(const Glib::ustring name,
                          const Glib::ustring label,
                          const Glib::ustring tooltip,
                          GtkTreeModel *model,
                          gint          entry_width    = -1,
                          gint          extra_width    = -1,
                          gpointer      cell_data_func = nullptr,
                          gpointer      separator_func = nullptr,
                          GtkWidget*    focusWidget    = nullptr);

    Glib::ustring get_active_text();
    gboolean set_active_text(const gchar* text, int row=-1);

    void     set_entry_width(gint entry_width);
    void     set_extra_width(gint extra_width);

    void     popup_enable();
    void     popup_disable();
    void     focus_on_click( bool focus_on_click );

    void     set_info(      const gchar* info );
    void     set_info_cb(   gpointer info_cb );
    void     set_warning(   const gchar* warning_cb );
    void     set_warning_cb(gpointer warning );
    void     set_tooltip(   const gchar* tooltip );

    // Accessor methods
    decltype(_model)          get_model()          const {return _model;}
    decltype(_combobox)       get_combobox()       const {return _combobox;}
    decltype(_entry)          get_entry()          const {return _entry;}
    decltype(_entry_width)    get_entry_width()    const {return _entry_width;}
    decltype(_extra_width)    get_extra_width()    const {return _extra_width;}
    decltype(_cell_data_func) get_cell_data_func() const {return _cell_data_func;}
    decltype(_separator_func) get_separator_func() const {return _separator_func;}
    decltype(_popup)          get_popup()          const {return _popup;}
    decltype(_focusWidget)    get_focus_widget()   const {return _focusWidget;}

    decltype(_active)         get_active()         const {return _active;}

    decltype(_signal_changed) signal_changed() {return _signal_changed;}

    // Mutator methods
    void set_model         (decltype(_model)          model)          {_model          = model;}
    void set_combobox      (decltype(_combobox)       combobox)       {_combobox       = combobox;}
    void set_entry         (decltype(_entry)          entry)          {_entry          = entry;}
    void set_cell_data_func(decltype(_cell_data_func) cell_data_func) {_cell_data_func = cell_data_func;}
    void set_separator_func(decltype(_separator_func) separator_func) {_separator_func = separator_func;}
    void set_popup         (decltype(_popup)          popup)          {_popup          = popup;}
    void set_focus_widget  (decltype(_focusWidget)    focus_widget)   {_focusWidget    = focus_widget;}

    // This doesn't seem right... surely we should set the active row in the Combobox too?
    void set_active        (decltype(_active)         active)         {_active         = active;}
};

}
}
}
#endif /* SEEN_INK_COMBOBOXENTRY_ACTION */

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

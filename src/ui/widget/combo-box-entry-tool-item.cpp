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

/*
 * We must provide for both a toolbar item and a menu item.
 * As we don't know which widgets are used (or even constructed),
 * we must keep track of things like active entry ourselves.
 */

#include "combo-box-entry-tool-item.h"

#include <cassert>
#include <iostream>
#include <cstring>
#include <glibmm/ustring.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdkmm/display.h>

#include "ui/icon-names.h"

namespace Inkscape {
namespace UI {
namespace Widget {

ComboBoxEntryToolItem::ComboBoxEntryToolItem(Glib::ustring name,
                                     Glib::ustring label,
                                     Glib::ustring tooltip,
                                     GtkTreeModel  *model,
                                     gint           entry_width,
                                     gint           extra_width,
                                     void          *cell_data_func,
                                     void          *separator_func,
                                     GtkWidget     *focusWidget)
    : _label(std::move(label)),
      _tooltip(std::move(tooltip)),
      _model(model),
      _entry_width(entry_width),
      _extra_width(extra_width),
      _cell_data_func(cell_data_func),
      _separator_func(separator_func),
      _focusWidget(focusWidget),
      _active(-1),
      _text(strdup("")),
      _entry_completion(nullptr),
      _popup(false),
      _info(nullptr),
      _info_cb(nullptr),
      _info_cb_id(0),
      _info_cb_blocked(false),
      _warning(nullptr),
      _warning_cb(nullptr),
      _warning_cb_id(0),
      _warning_cb_blocked(false)
{
    set_name(name);

    gchar *action_name = g_strdup( get_name().c_str() );
    gchar *combobox_name = g_strjoin( nullptr, action_name, "_combobox", nullptr );
    gchar *entry_name =    g_strjoin( nullptr, action_name, "_entry", nullptr );
    g_free( action_name );

    GtkWidget* comboBoxEntry = gtk_combo_box_new_with_model_and_entry (_model);
    gtk_combo_box_set_entry_text_column (GTK_COMBO_BOX (comboBoxEntry), 0);

    // Name it so we can muck with it using an RC file
    gtk_widget_set_name( comboBoxEntry, combobox_name );
    g_free( combobox_name );

    {
        gtk_widget_set_halign(comboBoxEntry, GTK_ALIGN_START);
        gtk_widget_set_hexpand(comboBoxEntry, FALSE);
        gtk_widget_set_vexpand(comboBoxEntry, FALSE);
        add(*Glib::wrap(comboBoxEntry));
    }

    _combobox = GTK_COMBO_BOX (comboBoxEntry);

    //gtk_combo_box_set_active( GTK_COMBO_BOX( comboBoxEntry ), ink_comboboxentry_action->active );
    gtk_combo_box_set_active( GTK_COMBO_BOX( comboBoxEntry ), 0 );

    g_signal_connect( G_OBJECT(comboBoxEntry), "changed", G_CALLBACK(combo_box_changed_cb), this );

    // Optionally add separator function...
    if( _separator_func != nullptr ) {
        gtk_combo_box_set_row_separator_func( _combobox,
                GtkTreeViewRowSeparatorFunc (_separator_func),
                nullptr, nullptr );
    }

    // Optionally add formatting...
    if( _cell_data_func != nullptr ) {
        gtk_combo_box_set_popup_fixed_width (GTK_COMBO_BOX(comboBoxEntry), false);
        this->_cell = gtk_cell_renderer_text_new();
        int total = gtk_tree_model_iter_n_children (model, nullptr);
        int height = 30;
        if (total > 1000) {
            height = 30000/total;
            g_warning("You have a huge number of font families (%d), "
                      "and Cairo is limiting the size of widgets you can draw.\n"
                      "Your preview cell height is capped to %d.",
                      total, height);
        }
        gtk_cell_renderer_set_fixed_size(_cell, -1, height);
        g_signal_connect(G_OBJECT(comboBoxEntry), "popup", G_CALLBACK(combo_box_popup_cb), this);
        gtk_cell_layout_clear( GTK_CELL_LAYOUT( comboBoxEntry ) );
        gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( comboBoxEntry ), _cell, true );
        gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT( comboBoxEntry ), _cell,
                GtkCellLayoutDataFunc (_cell_data_func), 0, nullptr );
    }

    // Optionally widen the combobox width... which widens the drop-down list in list mode.
    if( _extra_width > 0 ) {
        GtkRequisition req;
        gtk_widget_get_preferred_size(GTK_WIDGET(_combobox), &req, nullptr);
        gtk_widget_set_size_request( GTK_WIDGET( _combobox ),
                req.width + _extra_width, -1 );
    }

    // Get reference to GtkEntry and fiddle a bit with it.
    GtkWidget *child = gtk_bin_get_child( GTK_BIN(comboBoxEntry) );

    // Name it so we can muck with it using an RC file
    gtk_widget_set_name( child, entry_name );
    g_free( entry_name );

    if( child && GTK_IS_ENTRY( child ) ) {

        _entry = GTK_ENTRY(child);

        // Change width
        if( _entry_width > 0 ) {
            gtk_entry_set_width_chars (GTK_ENTRY (child), _entry_width );
        }

        // Add pop-up entry completion if required
        if( _popup ) {
            popup_enable();
        }

        // Add signal for GtkEntry to check if finished typing.
        g_signal_connect( G_OBJECT(child), "activate", G_CALLBACK(entry_activate_cb), this );
        g_signal_connect( G_OBJECT(child), "key-press-event", G_CALLBACK(keypress_cb), this );
    }

    set_tooltip(_tooltip.c_str());

    show_all();
}

// Setters/Getters ---------------------------------------------------

Glib::ustring
ComboBoxEntryToolItem::get_active_text()
{
    assert(_text);
    return _text;
}

/*
 * For the font-family list we need to handle two cases:
 *   Text is in list store:
 *     In this case we use row number as the font-family list can have duplicate
 *     entries, one in the document font part and one in the system font part. In
 *     order that scrolling through the list works properly we must distinguish
 *     between the two.
 *   Text is not in the list store (i.e. default font-family is not on system):
 *     In this case we have a row number of -1, and the text must be set by hand.
 */
gboolean
ComboBoxEntryToolItem::set_active_text(const gchar* text, int row)
{
  if( strcmp( _text, text ) != 0 ) { 
    g_free( _text );
    _text = g_strdup( text );
  }

  // Get active row or -1 if none
  if( row < 0 ) {
    row = get_active_row_from_text(this, _text);
  }
  _active = row;

  // Set active row, check that combobox has been created.
  if( _combobox ) {
    gtk_combo_box_set_active( GTK_COMBO_BOX( _combobox ), _active );
  }

  // Fiddle with entry
  if( _entry ) {

    // Explicitly set text in GtkEntry box (won't be set if text not in list).
    gtk_entry_set_text( _entry, text );

    // Show or hide warning  -- this might be better moved to text-toolbox.cpp
    if( _info_cb_id != 0 &&
	!_info_cb_blocked ) {
      g_signal_handler_block (G_OBJECT(_entry),
			      _info_cb_id );
      _info_cb_blocked = true;
    }
    if( _warning_cb_id != 0 &&
	!_warning_cb_blocked ) {
      g_signal_handler_block (G_OBJECT(_entry),
			      _warning_cb_id );
      _warning_cb_blocked = true;
    }

    bool set = false;
    if( _warning != nullptr ) {
      Glib::ustring missing = check_comma_separated_text();
      if( !missing.empty() ) {
	  gtk_entry_set_icon_from_icon_name( _entry,
					     GTK_ENTRY_ICON_SECONDARY,
					     INKSCAPE_ICON("dialog-warning") );
	// Can't add tooltip until icon set
	Glib::ustring warning = _warning;
	warning += ": ";
	warning += missing;
	gtk_entry_set_icon_tooltip_text( _entry,
					 GTK_ENTRY_ICON_SECONDARY,
					 warning.c_str() );

	if( _warning_cb ) {

	  // Add callback if we haven't already
	  if( _warning_cb_id == 0 ) {
	    _warning_cb_id =
	      g_signal_connect( G_OBJECT(_entry),
				"icon-press",
				G_CALLBACK(_warning_cb),
				this);
	  }
	  // Unblock signal
	  if( _warning_cb_blocked ) {
	    g_signal_handler_unblock (G_OBJECT(_entry),
				      _warning_cb_id );
	    _warning_cb_blocked = false;
	  }
	}
	set = true;
      }
    }
 
    if( !set && _info != nullptr ) {
      gtk_entry_set_icon_from_icon_name( GTK_ENTRY(_entry),
					 GTK_ENTRY_ICON_SECONDARY,
					 INKSCAPE_ICON("edit-select-all") );
      gtk_entry_set_icon_tooltip_text( _entry,
				       GTK_ENTRY_ICON_SECONDARY,
				       _info );

      if( _info_cb ) {
	// Add callback if we haven't already
	if( _info_cb_id == 0 ) {
	  _info_cb_id =
	    g_signal_connect( G_OBJECT(_entry),
			      "icon-press",
			      G_CALLBACK(_info_cb),
			      this);
	}
	// Unblock signal
	if( _info_cb_blocked ) {
	  g_signal_handler_unblock (G_OBJECT(_entry),
				    _info_cb_id );
	  _info_cb_blocked = false;
	}
      }
      set = true;
    }

    if( !set ) {
      gtk_entry_set_icon_from_icon_name( GTK_ENTRY(_entry),
					 GTK_ENTRY_ICON_SECONDARY,
					 nullptr );
    }
  }

  // Return if active text in list
  gboolean found = ( _active != -1 );
  return found;
}

void
ComboBoxEntryToolItem::set_entry_width(gint entry_width)
{
    _entry_width = entry_width;

    // Clamp to limits
    if(entry_width < -1)  entry_width = -1;
    if(entry_width > 100) entry_width = 100;

    // Widget may not have been created....
    if( _entry ) {
        gtk_entry_set_width_chars( GTK_ENTRY(_entry), entry_width );
    }
}

void
ComboBoxEntryToolItem::set_extra_width( gint extra_width )
{
    _extra_width = extra_width;

    // Clamp to limits
    if(extra_width < -1)  extra_width = -1;
    if(extra_width > 500) extra_width = 500;

    // Widget may not have been created....
    if( _combobox ) {
        GtkRequisition req;
        gtk_widget_get_preferred_size(GTK_WIDGET(_combobox), &req, nullptr);
        gtk_widget_set_size_request( GTK_WIDGET( _combobox ), req.width + _extra_width, -1 );
    }
}

void
ComboBoxEntryToolItem::focus_on_click( bool focus_on_click )
{ 
    if (_combobox) {
      gtk_widget_set_focus_on_click(GTK_WIDGET(_combobox), focus_on_click);
    }
}

void
ComboBoxEntryToolItem::popup_enable()
{
    _popup = true;

    // Widget may not have been created....
    if( _entry ) {

        // Check we don't already have a GtkEntryCompletion
        if( _entry_completion ) return;

        _entry_completion = gtk_entry_completion_new();

        gtk_entry_set_completion( _entry, _entry_completion );
        gtk_entry_completion_set_model( _entry_completion, _model );
        gtk_entry_completion_set_text_column( _entry_completion, 0 );
        gtk_entry_completion_set_popup_completion( _entry_completion, true );
        gtk_entry_completion_set_inline_completion( _entry_completion, false );
        gtk_entry_completion_set_inline_selection( _entry_completion, true );

        g_signal_connect (G_OBJECT (_entry_completion),  "match-selected", G_CALLBACK (match_selected_cb), this);
    }
}

void
ComboBoxEntryToolItem::popup_disable()
{
  _popup = false;

  if( _entry_completion ) {
    gtk_widget_destroy(GTK_WIDGET(_entry_completion));
    _entry_completion = nullptr;
  }
}

void
ComboBoxEntryToolItem::set_tooltip(const gchar* tooltip)
{
    set_tooltip_text(tooltip);
    gtk_widget_set_tooltip_text ( GTK_WIDGET(_combobox), tooltip);

    // Widget may not have been created....
    if( _entry ) {
        gtk_widget_set_tooltip_text ( GTK_WIDGET(_entry), tooltip);
    }
}

void
ComboBoxEntryToolItem::set_info(const gchar* info)
{
    g_free( _info );
    _info = g_strdup( info );

    // Widget may not have been created....
    if( _entry ) {
        gtk_entry_set_icon_tooltip_text( GTK_ENTRY(_entry),
                                         GTK_ENTRY_ICON_SECONDARY,
                                         _info );
    }
}

void
ComboBoxEntryToolItem::set_info_cb(gpointer info_cb)
{
    _info_cb = info_cb;
}

void
ComboBoxEntryToolItem::set_warning(const gchar* warning)
{
  g_free( _warning );
  _warning = g_strdup( warning );

  // Widget may not have been created....
  if( _entry ) {
    gtk_entry_set_icon_tooltip_text( GTK_ENTRY(_entry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     _warning );
  }
}

void
ComboBoxEntryToolItem::set_warning_cb(gpointer warning_cb)
{
    _warning_cb = warning_cb;
}

// Internal ---------------------------------------------------

// Return row of active text or -1 if not found. If exclude is true,
// use 3d column if available to exclude row from checking (useful to
// skip rows added for font-families included in doc and not on
// system)
gint
ComboBoxEntryToolItem::get_active_row_from_text(ComboBoxEntryToolItem *action,
                                              const gchar         *target_text,
			                      gboolean             exclude,
                                              gboolean             ignore_case )
{
  // Check if text in list
  gint row = 0;
  gboolean found = false;
  GtkTreeIter iter;
  gboolean valid = gtk_tree_model_get_iter_first( action->_model, &iter );
  while ( valid ) {

    // See if we should exclude a row
    gboolean check = true;  // If true, font-family is on system.
    if( exclude && gtk_tree_model_get_n_columns( action->_model ) > 2 ) {
      gtk_tree_model_get( action->_model, &iter, 2, &check, -1 );
    }

    if( check ) {
      // Get text from list entry
      gchar* text = nullptr;
      gtk_tree_model_get( action->_model, &iter, 0, &text, -1 ); // Column 0

      if( !ignore_case ) {
	// Case sensitive compare
	if( strcmp( target_text, text ) == 0 ){
	  found = true;
          g_free(text);
	  break;
	}
      } else {
	// Case insensitive compare
	gchar* target_text_casefolded = g_utf8_casefold( target_text, -1 );
	gchar* text_casefolded        = g_utf8_casefold( text, -1 );
	gboolean equal = (strcmp( target_text_casefolded, text_casefolded ) == 0 );
	g_free( text_casefolded );
	g_free( target_text_casefolded );
	if( equal ) {
	  found = true;
          g_free(text);
	  break;
	}
      }
      g_free(text);
    }

    ++row;
    valid = gtk_tree_model_iter_next( action->_model, &iter );
  }

  if( !found ) row = -1;

  return row;
}

// Checks if all comma separated text fragments are in the list and
// returns a ustring with a list of missing fragments.
// This is useful for checking if all fonts in a font-family fallback
// list are available on the system.
//
// This routine could also create a Pango Markup string to show which
// fragments are invalid in the entry box itself. See:
// http://developer.gnome.org/pango/stable/PangoMarkupFormat.html
// However... it appears that while one can retrieve the PangoLayout
// for a GtkEntry box, it is only a copy and changing it has no effect.
//   PangoLayout * pl = gtk_entry_get_layout( entry );
//   pango_layout_set_markup( pl, "NEW STRING", -1 ); // DOESN'T WORK
Glib::ustring
ComboBoxEntryToolItem::check_comma_separated_text()
{
  Glib::ustring missing;

  // Parse fallback_list using a comma as deliminator
  gchar** tokens = g_strsplit( _text, ",", 0 );

  gint i = 0;
  while( tokens[i] != nullptr ) {

    // Remove any surrounding white space.
    g_strstrip( tokens[i] );

    if( get_active_row_from_text( this, tokens[i], true, true ) == -1 ) {
      missing += tokens[i];
      missing += ", ";
    }
    ++i;
  }
  g_strfreev( tokens );

  // Remove extra comma and space from end.
  if( missing.size() >= 2 ) {
    missing.resize( missing.size()-2 );
  }
  return missing;
}

// Callbacks ---------------------------------------------------

void
ComboBoxEntryToolItem::combo_box_changed_cb( GtkComboBox* widget, gpointer data )
{
  // Two things can happen to get here:
  //   An item is selected in the drop-down menu.
  //   Text is typed.
  // We only react here if an item is selected.

  // Get action
  auto action = reinterpret_cast<ComboBoxEntryToolItem *>( data );

  // Check if item selected:
  gint newActive = gtk_combo_box_get_active(widget);
  if( newActive >= 0 && newActive != action->_active ) {

    action->_active = newActive;

    GtkTreeIter iter;
    if( gtk_combo_box_get_active_iter( GTK_COMBO_BOX( action->_combobox ), &iter ) ) {

      gchar* text = nullptr;
      gtk_tree_model_get( action->_model, &iter, 0, &text, -1 );
      gtk_entry_set_text( action->_entry, text );

      g_free( action->_text );
      action->_text = text;
    }

    // Now let the world know
    action->_signal_changed.emit();
  }
}

gboolean ComboBoxEntryToolItem::combo_box_popup_cb(ComboBoxEntryToolItem *widget, gpointer data)
{
    auto w = reinterpret_cast<ComboBoxEntryToolItem *>(data);
    GtkComboBox *comboBoxEntry = GTK_COMBO_BOX(w->_combobox);
    static int already_clicked = 0;
    if ((already_clicked == 1) && w->_cell_data_func) {
        // first click is always displaying something wrong.
        // Second loading of the screen should have preallocated space, and only has to render the text now
        gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(comboBoxEntry), w->_cell,
                                           GtkCellLayoutDataFunc(w->_cell_data_func), widget, nullptr);
    }
    already_clicked++;
    return true;
}

void
ComboBoxEntryToolItem::entry_activate_cb( GtkEntry *widget,
                                        gpointer  data )
{
  // Get text from entry box.. check if it matches a menu entry.

  // Get action
  auto action = reinterpret_cast<ComboBoxEntryToolItem*>( data );

  // Get text
  g_free( action->_text );
  action->_text = g_strdup( gtk_entry_get_text( widget ) );

  // Get row
  action->_active =
    get_active_row_from_text( action, action->_text );

  // Set active row
  gtk_combo_box_set_active( GTK_COMBO_BOX( action->_combobox), action->_active );

  // Now let the world know
  action->_signal_changed.emit();
}

gboolean
ComboBoxEntryToolItem::match_selected_cb( GtkEntryCompletion* /*widget*/, GtkTreeModel* model, GtkTreeIter* iter, gpointer data )
{
  // Get action
  auto action = reinterpret_cast<ComboBoxEntryToolItem*>(data);
  GtkEntry *entry = action->_entry;

  if( entry) {
    gchar *family = nullptr;
    gtk_tree_model_get(model, iter, 0, &family, -1);

    // Set text in GtkEntry
    gtk_entry_set_text (GTK_ENTRY (entry), family );

    // Set text in ToolItem
    g_free( action->_text );
    action->_text = family;

    // Get row
    action->_active =
      get_active_row_from_text( action, action->_text );

    // Set active row
    gtk_combo_box_set_active( GTK_COMBO_BOX( action->_combobox), action->_active );

    // Now let the world know
    action->_signal_changed.emit();

    return true;
  }
  return false;
}

void
ComboBoxEntryToolItem::defocus()
{
    if ( _focusWidget ) {
        gtk_widget_grab_focus( _focusWidget );
    }
}

gboolean
ComboBoxEntryToolItem::keypress_cb( GtkWidget *entry, GdkEventKey *event, gpointer data )
{
    gboolean wasConsumed = FALSE; /* default to report event not consumed */
    guint key = 0;
    auto action = reinterpret_cast<ComboBoxEntryToolItem*>(data);
    gdk_keymap_translate_keyboard_state( Gdk::Display::get_default()->get_keymap(),
                                         event->hardware_keycode, (GdkModifierType)event->state,
                                         0, &key, nullptr, nullptr, nullptr );

    switch ( key ) {

        case GDK_KEY_Escape:
        {
            //gtk_spin_button_set_value( GTK_SPIN_BUTTON(widget), action->private_data->lastVal );
            action->defocus();
            wasConsumed = TRUE;
        }
        break;

        case GDK_KEY_Tab:
        {
            // Fire activation similar to how Return does, but also return focus to text object
            // itself
            entry_activate_cb( GTK_ENTRY (entry), data );
            action->defocus();
            wasConsumed = TRUE;
        }
        break;

        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        {
            action->defocus();
            //wasConsumed = TRUE;
        }
        break;


    }

    return wasConsumed;
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

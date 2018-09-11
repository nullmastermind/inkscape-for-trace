// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright (C) 2018 Tavmong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 *
 * The routines here create and manage a font selector widget with two parts,
 * one each for font-family and font-style.
 *
 * This is essentially a toolbar version of the 'FontSelector' widget. Someday
 * this may be merged with it.
 *
 * The main functions are:
 *   Create the font-selector toolbar widget.
 *   Update the lists when a new text selection is made.
 *   Update the Style list when a new font-family is selected, highlighting the
 *     best match to the original font style (as not all fonts have the same style options).
 *   Update the on-screen text.
 *   Provide the currently selected values.
 */

#ifndef INKSCAPE_UI_WIDGET_FONT_SELECTOR_TOOLBAR_H
#define INKSCAPE_UI_WIDGET_FONT_SELECTOR_TOOLBAR_H

#include <gtkmm/grid.h>
#include <gtkmm/treeview.h>
#include <gtkmm/comboboxtext.h>

namespace Inkscape {
namespace UI {
namespace Widget {

/**
 * A container of widgets for selecting font faces.
 *
 * It is used by Text tool toolbar. The FontSelectorToolbar class utilizes the
 * FontLister class to obtain a list of font-families and their associated styles for fonts either
 * on the system or in the document. The FontLister class is also used by the Text toolbar. Fonts
 * are kept track of by their "fontspecs" which are the same as the strings that Pango generates.
 *
 * The main functions are:
 *   Create the font-selector widget.
 *   Update the child widgets when a new text selection is made.
 *   Update the Style list when a new font-family is selected, highlighting the
 *     best match to the original font style (as not all fonts have the same style options).
 *   Emit a signal when any change is made to a child widget.
 */
class FontSelectorToolbar : public Gtk::Grid
{

public:

    /**
     * Constructor
     */
    FontSelectorToolbar ();

protected:

    // Font family
    Gtk::ComboBox         family_combo;
    Gtk::CellRendererText family_cell;

    // Font style
    Gtk::ComboBoxText     style_combo;
    Gtk::CellRendererText style_cell;

private:

    // Make a list of missing fonts for tooltip and for warning icon.
    Glib::ustring get_missing_fonts ();

    // Signal handlers
    void on_family_changed();
    void on_style_changed();
    void on_icon_pressed (Gtk::EntryIconPosition icon_position, const GdkEventButton* event);
    // bool on_match_selected (const Gtk::TreeModel::iterator& iter);
    bool on_key_press_event (GdkEventKey* key_event) override;

    // Signals
    sigc::signal<void> changed_signal;
    void changed_emit();
    bool signal_block;

public:

    /**
     * Update GUI based on font-selector values.
     */
    void update_font ();

    /**
     * Let others know that user has changed GUI settings.
     */
    sigc::connection connectChanged(sigc::slot<void> slot) {
        return changed_signal.connect(slot);
    }
};

 
} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_WIDGET_FONT_SETTINGS_TOOLBAR_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :

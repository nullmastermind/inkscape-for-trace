// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright (C) 2018 Tavmong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>
#include <glibmm/markup.h>

#include "font-selector.h"

#include "libnrtype/font-lister.h"
#include "libnrtype/font-instance.h"

// For updating from selection
#include "inkscape.h"
#include "desktop.h"
#include "object/sp-text.h"

namespace Inkscape {
namespace UI {
namespace Widget {

FontSelector::FontSelector (bool with_size, bool with_variations)
    : Gtk::Grid ()
    , family_frame (_("Font family"))
    , style_frame (C_("Font selector", "Style"))
    , size_label   (_("Font size"))
    , size_combobox (true)   // With entry
    , signal_block (false)
    , font_size (18)
{

    Inkscape::FontLister* font_lister = Inkscape::FontLister::get_instance();

    // Font family
    family_treecolumn.pack_start (family_cell, false);
    family_treecolumn.set_fixed_width (200);
    family_treecolumn.add_attribute (family_cell, "text", 0);
    family_treecolumn.set_cell_data_func (family_cell, &font_lister_cell_data_func);

    family_treeview.set_row_separator_func (&font_lister_separator_func);
    family_treeview.set_model (font_lister->get_font_list());
    family_treeview.set_name ("FontSelector: Family");
    family_treeview.set_headers_visible (false);
    family_treeview.append_column (family_treecolumn);

    family_scroll.set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    family_scroll.add (family_treeview);

    family_frame.set_hexpand (true);
    family_frame.set_vexpand (true);
    family_frame.add (family_scroll);

    // Style
    style_treecolumn.pack_start (style_cell, false);
    style_treecolumn.add_attribute (style_cell, "text", 0);
    style_treecolumn.set_cell_data_func (style_cell, sigc::mem_fun(*this, &FontSelector::style_cell_data_func));
    style_treecolumn.set_title ("Face");
    style_treecolumn.set_resizable (true);

    style_treeview.set_model (font_lister->get_style_list());
    style_treeview.set_name ("FontSelectorStyle");
    style_treeview.append_column ("CSS", font_lister->FontStyleList.cssStyle);
    style_treeview.append_column (style_treecolumn);

    style_treeview.get_column(0)->set_resizable (true);

    style_scroll.set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    style_scroll.add (style_treeview);

    style_frame.set_hexpand (true);
    style_frame.set_vexpand (true);
    style_frame.add (style_scroll);

    // Size
    size_combobox.set_name ("FontSelectorSize");
    set_sizes();
    size_combobox.set_active_text( "18" );

    // Font Variations
    font_variations.set_vexpand (true);
    font_variations_scroll.set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    font_variations_scroll.add (font_variations);

    // Grid
    set_name ("FontSelectorGrid");
    set_row_spacing(4);
    set_column_spacing(4);
    // Add extra columns to the "family frame" to change space distribution
    // by prioritizing font family over styles
    const int extra = 4;
    attach (family_frame,  0, 0, 1 + extra, 2);
    attach (style_frame,   1 + extra, 0, 2, 1);
    if (with_size) { // Glyph panel does not use size.
        attach (size_label,    1 + extra, 1, 1, 1);
        attach (size_combobox, 2 + extra, 1, 1, 1);
    }
    if (with_variations) { // Glyphs panel does not use variations.
        attach (font_variations_scroll, 0, 2, 3 + extra, 1);
    }

    // Add signals
    family_treeview.get_selection()->signal_changed().connect(sigc::mem_fun(*this, &FontSelector::on_family_changed));
    style_treeview.get_selection()->signal_changed().connect(sigc::mem_fun(*this, &FontSelector::on_style_changed));
    size_combobox.signal_changed().connect(sigc::mem_fun(*this, &FontSelector::on_size_changed));
    font_variations.connectChanged(sigc::mem_fun(*this, &FontSelector::on_variations_changed));

    show_all_children();

    // Initialize font family lists. (May already be done.) Should be done on document change.
    font_lister->update_font_list(SP_ACTIVE_DESKTOP->getDocument());
}

void
FontSelector::set_sizes ()
{
    size_combobox.remove_all();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int unit = prefs->getInt("/options/font/unitType", SP_CSS_UNIT_PT);

    int sizes[] = {
        4, 6, 8, 9, 10, 11, 12, 13, 14, 16, 18, 20, 22, 24, 28,
        32, 36, 40, 48, 56, 64, 72, 144
    };

    // Array must be same length as SPCSSUnit in style-internal.h
    //                    PX  PT  PC  MM  CM   IN  EM  EX     %
    double ratios[] = {1,  1,  1, 10,  4, 40, 100, 16,  8, 0.16};

    for (int i : sizes)
    {
        double size = i/ratios[unit];
        size_combobox.append( Glib::ustring::format(size) );
    }
}

void
FontSelector::set_fontsize_tooltip()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int unit = prefs->getInt("/options/font/unitType", SP_CSS_UNIT_PT);
    Glib::ustring tooltip = Glib::ustring::format(_("Font size"), " (", sp_style_get_css_unit_string(unit), ")");
    size_combobox.set_tooltip_text (tooltip);
}

// Update GUI.
// We keep a private copy of the style list as the font-family in widget is only temporary
// until the "Apply" button is set so the style list can be different from that in
// FontLister.
void
FontSelector::update_font ()
{
    signal_block = true;

    Inkscape::FontLister *font_lister = Inkscape::FontLister::get_instance();
    Gtk::TreePath path;
    Glib::ustring family = font_lister->get_font_family();
    Glib::ustring style  = font_lister->get_font_style();

    // Set font family
    try {
        path = font_lister->get_row_for_font (family);
    } catch (...) {
        std::cerr << "FontSelector::update_font: Couldn't find row for font-family: "
                  << family << std::endl;
        path.clear();
        path.push_back(0);
    }

    Gtk::TreePath currentPath;
    Gtk::TreeViewColumn *currentColumn;
    family_treeview.get_cursor(currentPath, currentColumn);
    if (currentPath.empty() || !font_lister->is_path_for_font(currentPath, family)) {
        family_treeview.set_cursor (path);
        family_treeview.scroll_to_row (path);
    }

    // Get font-lister style list for selected family
    Gtk::TreeModel::Row row = *(family_treeview.get_model()->get_iter (path));
    GList *styles;
    row.get_value(1, styles);

    // Copy font-lister style list to private list store, searching for match.
    Gtk::TreeModel::iterator match;
    FontLister::FontStyleListClass FontStyleList;
    Glib::RefPtr<Gtk::ListStore> local_style_list_store = Gtk::ListStore::create(FontStyleList);
    for ( ; styles; styles = styles->next ) {
        Gtk::TreeModel::iterator treeModelIter = local_style_list_store->append();
        (*treeModelIter)[FontStyleList.cssStyle] = ((StyleNames *)styles->data)->CssName;
        (*treeModelIter)[FontStyleList.displayStyle] = ((StyleNames *)styles->data)->DisplayName;
        if (style == ((StyleNames*)styles->data)->CssName) {
            match = treeModelIter;
        }
    }

    // Attach store to tree view and select row.
    style_treeview.set_model (local_style_list_store);
    if (match) {
        style_treeview.get_selection()->select (match);
    }

    Glib::ustring fontspec = font_lister->get_fontspec();
    update_variations(fontspec);

    signal_block = false;
}

void
FontSelector::update_size (double size)
{
    signal_block = true;

    // Set font size
    std::stringstream ss;
    ss << size;
    size_combobox.get_entry()->set_text( ss.str() );
    font_size = size; // Store value
    set_fontsize_tooltip();

    signal_block = false;
}


// If use_variations is true (default), we get variation values from variations widget otherwise we
// get values from CSS widget (we need to be able to keep the two widgets synchronized both ways).
Glib::ustring
FontSelector::get_fontspec(bool use_variations) {

    // Build new fontspec from GUI settings
    Glib::ustring family = "Sans";  // Default...family list may not have been constructed.
    Gtk::TreeModel::iterator iter = family_treeview.get_selection()->get_selected(); 
    if (iter) {
        (*iter).get_value(0, family);
    }

    Glib::ustring style = "Normal";
    iter = style_treeview.get_selection()->get_selected();
    if (iter) {
        (*iter).get_value(0, style);
    }

    if (family.empty()) {
        std::cerr << "FontSelector::get_fontspec: empty family!" << std::endl;
    }

    if (style.empty()) {
        std::cerr << "FontSelector::get_fontspec: empty style!" << std::endl;
    }

    Glib::ustring fontspec = family + ", ";

    if (use_variations) {
        // Clip any font_variation data in 'style' as we'll replace it.
        auto pos = style.find('@');
        if (pos != Glib::ustring::npos) {
            style.erase (pos, style.length()-1);
        }

        Glib::ustring variations = font_variations.get_pango_string();

        if (variations.empty()) {
            fontspec += style;
        } else {
            fontspec += variations;
        }
    } else {
        fontspec += style;
    }

    return fontspec;
}

void
FontSelector::style_cell_data_func (Gtk::CellRenderer *renderer, Gtk::TreeIter const &iter)
{
    Glib::ustring family = "Sans";  // Default...family list may not have been constructed.
    Gtk::TreeModel::iterator iter_family = family_treeview.get_selection()->get_selected(); 
    if (iter_family) {
        (*iter_family).get_value(0, family);
    }

    Glib::ustring style = "Normal";
    (*iter).get_value(1, style);

    Glib::ustring style_escaped  = Glib::Markup::escape_text( style );
    Glib::ustring font_desc = Glib::Markup::escape_text( family + ", " + style );
    Glib::ustring markup;

    markup = "<span font='" + font_desc + "'>" + style_escaped + "</span>";

    // std::cout << "  markup: " << markup << "  (" << name << ")" << std::endl;

    renderer->set_property("markup", markup);
}
   

// Callbacks

// Need to update style list
void
FontSelector::on_family_changed() {

    if (signal_block) return;
    signal_block = true;

    Glib::RefPtr<Gtk::TreeModel> model;
    Gtk::TreeModel::iterator iter = family_treeview.get_selection()->get_selected(model);

    if (!iter) {
        // This can happen just after the family list is recreated.
        signal_block = false;
        return;
    }

    Inkscape::FontLister *fontlister = Inkscape::FontLister::get_instance();
    fontlister->ensureRowStyles(model, iter);

    Gtk::TreeModel::Row row = *iter; 

    // Get family name
    Glib::ustring family;
    row.get_value(0, family);

    // Get style list (TO DO: Get rid of GList)
    GList *styles;
    row.get_value(1, styles);

    // Find best style match for selected family with current style (e.g. of selected text).
    Glib::ustring style = fontlister->get_font_style();
    Glib::ustring best  = fontlister->get_best_style_match (family, style);    

    // Create are own store of styles for selected font-family (the font-family selected
    // in the dialog may not be the same as stored in the font-lister class until the
    // "Apply" button is triggered).
    Gtk::TreeModel::iterator it_best;
    FontLister::FontStyleListClass FontStyleList;
    Glib::RefPtr<Gtk::ListStore>  local_style_list_store = Gtk::ListStore::create(FontStyleList);

    // Build list and find best match.
    for ( ; styles; styles = styles->next ) {
        Gtk::TreeModel::iterator treeModelIter = local_style_list_store->append();
        (*treeModelIter)[FontStyleList.cssStyle] = ((StyleNames *)styles->data)->CssName;
        (*treeModelIter)[FontStyleList.displayStyle] = ((StyleNames *)styles->data)->DisplayName;
        if (best == ((StyleNames*)styles->data)->CssName) {
            it_best = treeModelIter;
        }
    }

    // Attach store to tree view and select row.
    style_treeview.set_model (local_style_list_store);
    if (it_best) {
        style_treeview.get_selection()->select (it_best);
    }

    signal_block = false;

    // Let world know
    changed_emit();
}

void
FontSelector::on_style_changed() {
    if (signal_block) return;

    // Update variations widget if new style selected from style widget.
    signal_block = true;
    Glib::ustring fontspec = get_fontspec( false );
    update_variations(fontspec);
    signal_block = false;

    // Let world know
    changed_emit();
}

void
FontSelector::on_size_changed() {

    if (signal_block) return;

    double size;
    Glib::ustring input = size_combobox.get_active_text();
    try {
        size = std::stod (input);
    }
    catch (std::invalid_argument) {
        std::cerr << "FontSelector::on_size_changed: Invalid input: " << input << std::endl;
        size = -1;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    // Arbitrary: Text and Font preview freezes with huge font sizes.
    int max_size = prefs->getInt("/dialogs/textandfont/maxFontSize", 10000); 

    if (size <= 0) {
        return;
    }
    if (size > max_size)
        size = max_size;

    if (fabs(font_size - size) > 0.001) {
        font_size = size;
        // Let world know
        changed_emit();
    }
}

void
FontSelector::on_variations_changed() {

    if (signal_block) return;

    // Let world know
    changed_emit();
}

void
FontSelector::changed_emit() {
    signal_block = true;
    signal_changed.emit (get_fontspec());
    signal_block = false;
}

void FontSelector::update_variations(const Glib::ustring& fontspec) {
    font_variations.update(fontspec);

    // Check if there are any variations available; if not, don't expand font_variations_scroll
    bool hasContent = font_variations.variations_present();
    font_variations_scroll.set_vexpand(hasContent);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :

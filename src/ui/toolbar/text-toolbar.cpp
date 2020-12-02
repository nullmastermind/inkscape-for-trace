// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Text aux toolbar
 */
/* Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Frank Felfe <innerspace@iname.com>
 *   John Cliff <simarilius@yahoo.com>
 *   David Turner <novalis@gnu.org>
 *   Josh Andler <scislac@scislac.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Tavmjong Bah <tavmjong@free.fr>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2003 MenTaLguY
 * Copyright (C) 2001-2002 Ximian, Inc.
 * Copyright (C) 1999-2013 authors
 * Copyright (C) 2017 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>

#include "text-toolbar.h"

#include "desktop-style.h"
#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "inkscape.h"
#include "selection-chemistry.h"
#include "verbs.h"

#include "libnrtype/font-lister.h"

#include "object/sp-flowdiv.h"
#include "object/sp-flowtext.h"
#include "object/sp-root.h"
#include "object/sp-text.h"
#include "object/sp-tspan.h"
#include "object/sp-string.h"

#include "svg/css-ostringstream.h"
#include "ui/icon-names.h"
#include "ui/tools/select-tool.h"
#include "ui/tools/text-tool.h"
#include "ui/widget/canvas.h"  // Focus
#include "ui/widget/combo-box-entry-tool-item.h"
#include "ui/widget/combo-tool-item.h"
#include "ui/widget/spin-button-tool-item.h"
#include "ui/widget/unit-tracker.h"
#include "util/units.h"

#include "widgets/style-utils.h"

using Inkscape::DocumentUndo;
using Inkscape::Util::Unit;
using Inkscape::Util::Quantity;
using Inkscape::Util::unit_table;
using Inkscape::UI::Widget::UnitTracker;

//#define DEBUG_TEXT

//########################
//##    Text Toolbox    ##
//########################

// Functions for debugging:
#ifdef DEBUG_TEXT
static void sp_print_font(SPStyle *query)
{


    bool family_set   = query->font_family.set;
    bool style_set    = query->font_style.set;
    bool fontspec_set = query->font_specification.set;

    std::cout << "    Family set? " << family_set
              << "    Style set? "  << style_set
              << "    FontSpec set? " << fontspec_set
              << std::endl;
}

static void       sp_print_fontweight( SPStyle *query ) {
    const gchar* names[] = {"100", "200", "300", "400", "500", "600", "700", "800", "900",
                            "NORMAL", "BOLD", "LIGHTER", "BOLDER", "Out of range"};
    // Missing book = 380
    int index = query->font_weight.computed;
    if (index < 0 || index > 13)
        index = 13;
    std::cout << "    Weight: " << names[ index ]
              << " (" << query->font_weight.computed << ")" << std::endl;
}

static void       sp_print_fontstyle( SPStyle *query ) {

    const gchar* names[] = {"NORMAL", "ITALIC", "OBLIQUE", "Out of range"};
    int index = query->font_style.computed;
    if( index < 0 || index > 3 ) index = 3;
    std::cout << "    Style:  " << names[ index ] << std::endl;

}
#endif

static bool is_relative( Unit const *unit ) {
    return (unit->abbr == "" || unit->abbr == "em" || unit->abbr == "ex" || unit->abbr == "%");
}

static bool is_relative(SPCSSUnit const unit)
{
    return (unit == SP_CSS_UNIT_NONE || unit == SP_CSS_UNIT_EM || unit == SP_CSS_UNIT_EX ||
            unit == SP_CSS_UNIT_PERCENT);
}

// Set property for object, but unset all descendents
// Should probably be moved to desktop_style.cpp
static void recursively_set_properties(SPObject *object, SPCSSAttr *css, bool unset_descendents = true)
{
    object->changeCSS (css, "style");

    SPCSSAttr *css_unset = sp_repr_css_attr_unset_all( css );
    std::vector<SPObject *> children = object->childList(false);
    for (auto i: children) {
        recursively_set_properties(i, unset_descendents ? css_unset : css);
    }
    sp_repr_css_attr_unref (css_unset);
}

/*
 * Set the default list of font sizes, scaled to the users preferred unit
 */
static void sp_text_set_sizes(GtkListStore* model_size, int unit)
{
    gtk_list_store_clear(model_size);

    // List of font sizes for dropchange-down menu
    int sizes[] = {
        4, 6, 8, 9, 10, 11, 12, 13, 14, 16, 18, 20, 22, 24, 28,
        32, 36, 40, 48, 56, 64, 72, 144
    };

    // Array must be same length as SPCSSUnit in style.h
    float ratios[] = {1, 1, 1, 10, 4, 40, 100, 16, 8, 0.16};

    for(int i : sizes) {
        GtkTreeIter iter;
        Glib::ustring size = Glib::ustring::format(i / (float)ratios[unit]);
        gtk_list_store_append( model_size, &iter );
        gtk_list_store_set( model_size, &iter, 0, size.c_str(), -1 );
    }
}


// TODO: possibly share with font-selector by moving most code to font-lister (passing family name)
static void sp_text_toolbox_select_cb( GtkEntry* entry, GtkEntryIconPosition /*position*/, GdkEvent /*event*/, gpointer /*data*/ ) {

  Glib::ustring family = gtk_entry_get_text ( entry );
  //std::cout << "text_toolbox_missing_font_cb: selecting: " << family << std::endl;

  // Get all items with matching font-family set (not inherited!).
  std::vector<SPItem*> selectList;

  SPDesktop *desktop = SP_ACTIVE_DESKTOP;
  SPDocument *document = desktop->getDocument();
  std::vector<SPItem*> x,y;
  std::vector<SPItem*> allList = get_all_items(x, document->getRoot(), desktop, false, false, true, y);
  for(std::vector<SPItem*>::const_reverse_iterator i=allList.rbegin();i!=allList.rend(); ++i){
      SPItem *item = *i;
    SPStyle *style = item->style;

    if (style) {

      Glib::ustring family_style;
      if (style->font_family.set) {
	family_style = style->font_family.value();
	//std::cout << " family style from font_family: " << family_style << std::endl;
      }
      else if (style->font_specification.set) {
	family_style = style->font_specification.value();
	//std::cout << " family style from font_spec: " << family_style << std::endl;
      }

      if (family_style.compare( family ) == 0 ) {
        //std::cout << "   found: " << item->getId() << std::endl;
	selectList.push_back(item);
      }
    }
  }

  // Update selection
  Inkscape::Selection *selection = desktop->getSelection();
  selection->clear();
  //std::cout << "   list length: " << g_slist_length ( selectList ) << std::endl;
  selection->setList(selectList);
}

namespace Inkscape {
namespace UI {
namespace Toolbar {

TextToolbar::TextToolbar(SPDesktop *desktop)
    : Toolbar(desktop)
    , _freeze(false)
    , _text_style_from_prefs(false)
    , _outer(true)
    , _updating(false)
    , _tracker(new UnitTracker(Inkscape::Util::UNIT_TYPE_LINEAR))
    , _tracker_fs(new UnitTracker(Inkscape::Util::UNIT_TYPE_LINEAR))
    , _cusor_numbers(0)
{
    /* Line height unit tracker */
    _tracker->prependUnit(unit_table.getUnit("")); // Ratio
    _tracker->addUnit(unit_table.getUnit("%"));
    _tracker->addUnit(unit_table.getUnit("em"));
    _tracker->addUnit(unit_table.getUnit("ex"));
    _tracker->setActiveUnit(unit_table.getUnit(""));
    // We change only the display value
    _tracker->changeLabel("lines", 0, true);
    _tracker_fs->setActiveUnit(unit_table.getUnit("mm"));

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    /* Font family */
    {
        // Font list
        Inkscape::FontLister* fontlister = Inkscape::FontLister::get_instance();
        fontlister->update_font_list(desktop->getDocument());
        Glib::RefPtr<Gtk::ListStore> store = fontlister->get_font_list();
        GtkListStore* model = store->gobj();

        _font_family_item =
            Gtk::manage(new UI::Widget::ComboBoxEntryToolItem( "TextFontFamilyAction",
                                                               _("Font Family"),
                                                               _("Select Font Family (Alt-X to access)"),
                                                               GTK_TREE_MODEL(model),
                                                               -1,                // Entry width
                                                               50,                // Extra list width
                                                               (gpointer)font_lister_cell_data_func2, // Cell layout
                                                               (gpointer)font_lister_separator_func2,
                                                               GTK_WIDGET(desktop->getCanvas()->gobj()))); // Focus widget
        _font_family_item->popup_enable(); // Enable entry completion
        gchar *const info = _("Select all text with this font-family");
        _font_family_item->set_info( info ); // Show selection icon
        _font_family_item->set_info_cb( (gpointer)sp_text_toolbox_select_cb );

        gchar *const warning = _("Font not found on system");
        _font_family_item->set_warning( warning ); // Show icon w/ tooltip if font missing
        _font_family_item->set_warning_cb( (gpointer)sp_text_toolbox_select_cb );

        //ink_comboboxentry_action_set_warning_callback( act, sp_text_fontfamily_select_all );
        _font_family_item->signal_changed().connect( sigc::mem_fun(*this, &TextToolbar::fontfamily_value_changed) );
        add(*_font_family_item);

        // Change style of drop-down from menu to list
        auto css_provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(css_provider,
                                        "#TextFontFamilyAction_combobox {\n"
                                        "  -GtkComboBox-appears-as-list: true;\n"
                                        "}\n",
                                        -1, nullptr);

        auto screen = gdk_screen_get_default();
        _font_family_item->focus_on_click(false);
        gtk_style_context_add_provider_for_screen(screen,
                                                  GTK_STYLE_PROVIDER(css_provider),
                                                  GTK_STYLE_PROVIDER_PRIORITY_USER);
    }

    /* Font styles */
    {
        Inkscape::FontLister* fontlister = Inkscape::FontLister::get_instance();
        Glib::RefPtr<Gtk::ListStore> store = fontlister->get_style_list();
        GtkListStore* model_style = store->gobj();

        _font_style_item =
            Gtk::manage(new UI::Widget::ComboBoxEntryToolItem( "TextFontStyleAction",
                                                               _("Font Style"),
                                                               _("Font style"),
                                                               GTK_TREE_MODEL(model_style),
                                                               12,     // Width in characters
                                                               0,      // Extra list width
                                                               nullptr,   // Cell layout
                                                               nullptr,   // Separator
                                                               GTK_WIDGET(desktop->getCanvas()->gobj()))); // Focus widget

        _font_style_item->signal_changed().connect(sigc::mem_fun(*this, &TextToolbar::fontstyle_value_changed));
        _font_style_item->focus_on_click(false);
        add(*_font_style_item);
    }

    add_separator();

    /* Font size */
    {
        // List of font sizes for drop-down menu
        GtkListStore* model_size = gtk_list_store_new( 1, G_TYPE_STRING );
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        int unit = prefs->getInt("/options/font/unitType", SP_CSS_UNIT_PT);

        sp_text_set_sizes(model_size, unit);

        auto unit_str = sp_style_get_css_unit_string(unit);
        Glib::ustring tooltip = Glib::ustring::format(_("Font size"), " (", unit_str, ")");

        _font_size_item =
            Gtk::manage(new UI::Widget::ComboBoxEntryToolItem( "TextFontSizeAction",
                                                               _("Font Size"),
                                                               tooltip,
                                                               GTK_TREE_MODEL(model_size),
                                                               8,      // Width in characters
                                                               0,      // Extra list width
                                                               nullptr,   // Cell layout
                                                               nullptr,   // Separator
                                                               GTK_WIDGET(desktop->getCanvas()->gobj()))); // Focus widget

        _font_size_item->signal_changed().connect(sigc::mem_fun(*this, &TextToolbar::fontsize_value_changed));
        _font_size_item->focus_on_click(false);
        add(*_font_size_item);
    }
    /* Font_ size units */
    {
        _font_size_units_item = _tracker_fs->create_tool_item(_("Units"), (""));
        _font_size_units_item->signal_changed_after().connect(
            sigc::mem_fun(*this, &TextToolbar::fontsize_unit_changed));
        _font_size_units_item->focus_on_click(false);
        add(*_font_size_units_item);
    }
    {
        // Drop down menu
        std::vector<Glib::ustring> labels = {_("Smaller spacing"),  "",  "",  "",  "", C_("Text tool", "Normal"),  "", "",   "",  "",  "", _("Larger spacing")};
        std::vector<double>        values = {                 0.5, 0.6, 0.7, 0.8, 0.9,                       1.0, 1.1, 1.2, 1.3, 1.4, 1.5,                 2.0};

        auto line_height_val = 1.25;
        _line_height_adj = Gtk::Adjustment::create(line_height_val, 0.0, 1000.0, 0.1, 1.0);
        _line_height_item =
            Gtk::manage(new UI::Widget::SpinButtonToolItem("text-line-height", "", _line_height_adj, 0.1, 2));
        _line_height_item->set_tooltip_text(_("Spacing between baselines"));
        _line_height_item->set_custom_numeric_menu_data(values, labels);
        _line_height_item->set_focus_widget(desktop->getCanvas());
        _line_height_adj->signal_value_changed().connect(sigc::mem_fun(*this, &TextToolbar::lineheight_value_changed));
        //_tracker->addAdjustment(_line_height_adj->gobj()); // (Alex V) Why is this commented out?
        _line_height_item->set_sensitive(true);
        _line_height_item->set_icon(INKSCAPE_ICON("text_line_spacing"));
        add(*_line_height_item);
    }
    /* Line height units */
    {
        _line_height_units_item = _tracker->create_tool_item( _("Units"), (""));
        _line_height_units_item->signal_changed_after().connect(sigc::mem_fun(*this, &TextToolbar::lineheight_unit_changed));
        _line_height_units_item->focus_on_click(false);
        add(*_line_height_units_item);
    }

    /* Alignment */
    {
        UI::Widget::ComboToolItemColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = _("Align left");
        row[columns.col_tooltip  ] = _("Align left");
        row[columns.col_icon     ] = INKSCAPE_ICON("format-justify-left");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Align center");
        row[columns.col_tooltip  ] = _("Align center");
        row[columns.col_icon     ] = INKSCAPE_ICON("format-justify-center");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Align right");
        row[columns.col_tooltip  ] = _("Align right");
        row[columns.col_icon     ] = INKSCAPE_ICON("format-justify-right");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Justify");
        row[columns.col_tooltip  ] = _("Justify (only flowed text)");
        row[columns.col_icon     ] = INKSCAPE_ICON("format-justify-fill");
        row[columns.col_sensitive] = false;

        _align_item =
            UI::Widget::ComboToolItem::create(_("Alignment"),      // Label
                                              _("Text alignment"), // Tooltip
                                              "Not Used",          // Icon
                                              store );             // Tree store
        _align_item->use_icon( true );
        _align_item->use_label( false );
        gint mode = prefs->getInt("/tools/text/align_mode", 0);
        _align_item->set_active( mode );

        add(*_align_item);
        _align_item->focus_on_click(false);
        _align_item->signal_changed().connect(sigc::mem_fun(*this, &TextToolbar::align_mode_changed));
    }

    /* Style - Superscript */
    {
        _superscript_item = Gtk::manage(new Gtk::ToggleToolButton());
        _superscript_item->set_label(_("Toggle superscript"));
        _superscript_item->set_tooltip_text(_("Toggle superscript"));
        _superscript_item->set_icon_name(INKSCAPE_ICON("text_superscript"));
        _superscript_item->set_name("text-superscript");
        add(*_superscript_item);
        _superscript_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &TextToolbar::script_changed), _superscript_item));
        _superscript_item->set_active(prefs->getBool("/tools/text/super", false));
    }

    /* Style - Subscript */
    {
        _subscript_item = Gtk::manage(new Gtk::ToggleToolButton());
        _subscript_item->set_label(_("Toggle subscript"));
        _subscript_item->set_tooltip_text(_("Toggle subscript"));
        _subscript_item->set_icon_name(INKSCAPE_ICON("text_subscript"));
        _subscript_item->set_name("text-subscript");
        add(*_subscript_item);
        _subscript_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &TextToolbar::script_changed), _subscript_item));
        _subscript_item->set_active(prefs->getBool("/tools/text/sub", false));
    }

    /* Letter spacing */
    {
        // Drop down menu
        std::vector<Glib::ustring> labels = {_("Negative spacing"),   "",   "",   "", C_("Text tool", "Normal"),  "",  "",  "",  "",  "",  "",  "", _("Positive spacing")};
        std::vector<double>        values = {                 -2.0, -1.5, -1.0, -0.5,                         0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 4.0,                   5.0};
        auto letter_spacing_val = prefs->getDouble("/tools/text/letterspacing", 0.0);
        _letter_spacing_adj = Gtk::Adjustment::create(letter_spacing_val, -100.0, 1000.0, 0.01, 0.10);
        _letter_spacing_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("text-letter-spacing", _("Letter:"), _letter_spacing_adj, 0.1, 2));
        _letter_spacing_item->set_tooltip_text(_("Spacing between letters (px)"));
        _letter_spacing_item->set_custom_numeric_menu_data(values, labels);
        _letter_spacing_item->set_focus_widget(desktop->getCanvas());
        _letter_spacing_adj->signal_value_changed().connect(sigc::mem_fun(*this, &TextToolbar::letterspacing_value_changed));
        add(*_letter_spacing_item);

        _letter_spacing_item->set_sensitive(true);
        _letter_spacing_item->set_icon(INKSCAPE_ICON("text_letter_spacing"));
    }

    /* Word spacing */
    {
        // Drop down menu
        std::vector<Glib::ustring> labels = {_("Negative spacing"),   "",   "",   "", C_("Text tool", "Normal"),  "",  "",  "",  "",  "",  "",  "", _("Positive spacing")};
        std::vector<double>        values = {                 -2.0, -1.5, -1.0, -0.5,                         0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 4.0,                   5.0};
        auto word_spacing_val = prefs->getDouble("/tools/text/wordspacing", 0.0);
        _word_spacing_adj = Gtk::Adjustment::create(word_spacing_val, -100.0, 1000.0, 0.01, 0.10);
        _word_spacing_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("text-word-spacing", _("Word:"), _word_spacing_adj, 0.1, 2));
        _word_spacing_item->set_tooltip_text(_("Spacing between words (px)"));
        _word_spacing_item->set_custom_numeric_menu_data(values, labels);
        _word_spacing_item->set_focus_widget(desktop->getCanvas());
        _word_spacing_adj->signal_value_changed().connect(sigc::mem_fun(*this, &TextToolbar::wordspacing_value_changed));

        add(*_word_spacing_item);
        _word_spacing_item->set_sensitive(true);
        _word_spacing_item->set_icon(INKSCAPE_ICON("text_word_spacing"));
    }

    /* Character kerning (horizontal shift) */
    {
        // Drop down menu
        std::vector<double> values = { -2.0, -1.5, -1.0, -0.5,   0,  0.5,  1.0,  1.5,  2.0, 2.5 };
        auto dx_val = prefs->getDouble("/tools/text/dx", 0.0);
        _dx_adj = Gtk::Adjustment::create(dx_val, -100.0, 1000.0, 0.01, 0.1);
        _dx_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("text-dx", _("Kern:"), _dx_adj, 0.1, 2));
        _dx_item->set_custom_numeric_menu_data(values);
        _dx_item->set_tooltip_text(_("Horizontal kerning (px)"));
        _dx_item->set_focus_widget(desktop->getCanvas());
        _dx_adj->signal_value_changed().connect(sigc::mem_fun(*this, &TextToolbar::dx_value_changed));
        add(*_dx_item);
        _dx_item->set_sensitive(true);
        _dx_item->set_icon(INKSCAPE_ICON("text_horz_kern"));
    }

    /* Character vertical shift */
    {
        // Drop down menu
        std::vector<double> values = { -2.0, -1.5, -1.0, -0.5,   0,  0.5,  1.0,  1.5,  2.0, 2.5 };
        auto dy_val = prefs->getDouble("/tools/text/dy", 0.0);
        _dy_adj = Gtk::Adjustment::create(dy_val, -100.0, 1000.0, 0.01, 0.1);
        _dy_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("text-dy", _("Vert:"), _dy_adj, 0.1, 2));
        _dy_item->set_tooltip_text(_("Vertical kerning (px)"));
        _dy_item->set_custom_numeric_menu_data(values);
        _dy_item->set_focus_widget(desktop->getCanvas());
        _dy_adj->signal_value_changed().connect(sigc::mem_fun(*this, &TextToolbar::dy_value_changed));
        _dy_item->set_sensitive(true);
        _dy_item->set_icon(INKSCAPE_ICON("text_vert_kern"));
        add(*_dy_item);
    }

    /* Character rotation */
    {
        std::vector<double> values = { -90, -45, -30, -15,   0,  15,  30,  45,  90, 180 };
        auto rotation_val = prefs->getDouble("/tools/text/rotation", 0.0);
        _rotation_adj = Gtk::Adjustment::create(rotation_val, -180.0, 180.0, 0.1, 1.0);
        _rotation_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("text-rotation", _("Rot:"), _rotation_adj, 0.1, 2));
        _rotation_item->set_tooltip_text(_("Character rotation (degrees)"));
        _rotation_item->set_custom_numeric_menu_data(values);
        _rotation_item->set_focus_widget(desktop->getCanvas());
        _rotation_adj->signal_value_changed().connect(sigc::mem_fun(*this, &TextToolbar::rotation_value_changed));
        _rotation_item->set_sensitive();
        _rotation_item->set_icon(INKSCAPE_ICON("text_rotation"));
        add(*_rotation_item);
    }


    /* Writing mode (Horizontal, Vertical-LR, Vertical-RL) */
    {
        UI::Widget::ComboToolItemColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = _("Horizontal");
        row[columns.col_tooltip  ] = _("Horizontal text");
        row[columns.col_icon     ] = INKSCAPE_ICON("frmt-text-direction-horizontal");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Vertical — RL");
        row[columns.col_tooltip  ] = _("Vertical text — lines: right to left");
        row[columns.col_icon     ] = INKSCAPE_ICON("frmt-text-direction-vertical");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Vertical — LR");
        row[columns.col_tooltip  ] = _("Vertical text — lines: left to right");
        row[columns.col_icon     ] = INKSCAPE_ICON("frmt-text-direction-vertical-lr");
        row[columns.col_sensitive] = true;

        _writing_mode_item =
            UI::Widget::ComboToolItem::create( _("Writing mode"),       // Label
                                               _("Block progression"),  // Tooltip
                                               "Not Used",              // Icon
                                               store );                 // Tree store
        _writing_mode_item->use_icon(true);
        _writing_mode_item->use_label( false );
        gint mode = prefs->getInt("/tools/text/writing_mode", 0);
        _writing_mode_item->set_active( mode );
        add(*_writing_mode_item);
        _writing_mode_item->focus_on_click(false);
        _writing_mode_item->signal_changed().connect(sigc::mem_fun(*this, &TextToolbar::writing_mode_changed));
    }


    /* Text (glyph) orientation (Auto (mixed), Upright, Sideways) */
    {
        UI::Widget::ComboToolItemColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = _("Auto");
        row[columns.col_tooltip  ] = _("Auto glyph orientation");
        row[columns.col_icon     ] = INKSCAPE_ICON("text-orientation-auto");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Upright");
        row[columns.col_tooltip  ] = _("Upright glyph orientation");
        row[columns.col_icon     ] = INKSCAPE_ICON("text-orientation-upright");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Sideways");
        row[columns.col_tooltip  ] = _("Sideways glyph orientation");
        row[columns.col_icon     ] = INKSCAPE_ICON("text-orientation-sideways");
        row[columns.col_sensitive] = true;

        _orientation_item =
            UI::Widget::ComboToolItem::create(_("Text orientation"),    // Label
                                              _("Text (glyph) orientation in vertical text."),  // Tooltip
                                              "Not Used",               // Icon
                                              store );                  // List store
        _orientation_item->use_icon(true);
        _orientation_item->use_label(false);
        gint mode = prefs->getInt("/tools/text/text_orientation", 0);
        _orientation_item->set_active( mode );
        _orientation_item->focus_on_click(false);
        add(*_orientation_item);

        _orientation_item->signal_changed().connect(sigc::mem_fun(*this, &TextToolbar::orientation_changed));
    }

    // Text direction (predominant direction of horizontal text).
    {
        UI::Widget::ComboToolItemColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = _("LTR");
        row[columns.col_tooltip  ] = _("Left to right text");
        row[columns.col_icon     ] = INKSCAPE_ICON("frmt-text-direction-horizontal");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("RTL");
        row[columns.col_tooltip  ] = _("Right to left text");
        row[columns.col_icon     ] = INKSCAPE_ICON("frmt-text-direction-r2l");
        row[columns.col_sensitive] = true;

        _direction_item =
            UI::Widget::ComboToolItem::create( _("Text direction"),    // Label
                                               _("Text direction for normally horizontal text."),  // Tooltip
                                               "Not Used",               // Icon
                                               store );                  // List store
        _direction_item->use_icon(true);
        _direction_item->use_label(false);
        gint mode = prefs->getInt("/tools/text/text_direction", 0);
        _direction_item->set_active( mode );
        _direction_item->focus_on_click(false);
        add(*_direction_item);

        _direction_item->signal_changed_after().connect(sigc::mem_fun(*this, &TextToolbar::direction_changed));
    }

    show_all();

    // we emit a selection change on tool switch to text
    desktop->connectEventContextChanged(sigc::mem_fun(*this, &TextToolbar::watch_ec));
}

/*
 * Set the style, depending on the inner or outer text being selected
 */
void TextToolbar::text_outer_set_style(SPCSSAttr *css)
{
    // Calling sp_desktop_set_style will result in a call to TextTool::_styleSet() which
    // will set the style on selected text inside the <text> element. If we want to set
    // the style on the outer <text> objects we need to bypass this call.
    SPDesktop *desktop = _desktop;
    if(_outer) {
        // Apply css to parent text objects directly.
        for (auto i : desktop->getSelection()->items()) {
            SPItem *item = dynamic_cast<SPItem *>(i);
            if (dynamic_cast<SPText *>(item) || dynamic_cast<SPFlowtext *>(item)) {
                // Scale by inverse of accumulated parent transform
                SPCSSAttr *css_set = sp_repr_css_attr_new();
                sp_repr_css_merge(css_set, css);
                Geom::Affine const local(item->i2doc_affine());
                double const ex(local.descrim());
                if ((ex != 0.0) && (ex != 1.0)) {
                    sp_css_attr_scale(css_set, 1 / ex);
                }
                recursively_set_properties(item, css_set);
                sp_repr_css_attr_unref(css_set);
            }
        }
    } else {
        // Apply css to selected inner objects.
        sp_desktop_set_style (desktop, css, true, false);
    }
}

void
TextToolbar::fontfamily_value_changed()
{
#ifdef DEBUG_TEXT
    std::cout << std::endl;
    std::cout << "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM" << std::endl;
    std::cout << "sp_text_fontfamily_value_changed: " << std::endl;
#endif

     // quit if run by the _changed callbacks
    if (_freeze) {
#ifdef DEBUG_TEXT
        std::cout << "sp_text_fontfamily_value_changed: frozen... return" << std::endl;
        std::cout << "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM\n" << std::endl;
#endif
        return;
    }
    _freeze = true;

    Glib::ustring new_family = _font_family_item->get_active_text();
    css_font_family_unquote( new_family ); // Remove quotes around font family names.

    // TODO: Think about how to handle handle multiple selections. While
    // the font-family may be the same for all, the styles might be different.
    // See: TextEdit::onApply() for example of looping over selected items.
    Inkscape::FontLister* fontlister = Inkscape::FontLister::get_instance();
#ifdef DEBUG_TEXT
    std::cout << "  Old family: " << fontlister->get_font_family() << std::endl;
    std::cout << "  New family: " << new_family << std::endl;
    std::cout << "  Old active: " << fontlister->get_font_family_row() << std::endl;
    // std::cout << "  New active: " << act->active << std::endl;
#endif
    if( new_family.compare( fontlister->get_font_family() ) != 0 ) {
        // Changed font-family

        if( _font_family_item->get_active() == -1 ) {
            // New font-family, not in document, not on system (could be fallback list)
            fontlister->insert_font_family( new_family );

            // This just sets a variable in the ComboBoxEntryAction object...
            // shouldn't we also set the actual active row in the combobox?
            _font_family_item->set_active(0); // New family is always at top of list.
        }

        fontlister->set_font_family( _font_family_item->get_active() );
        // active text set in sp_text_toolbox_selection_changed()

        SPCSSAttr *css = sp_repr_css_attr_new ();
        fontlister->fill_css( css );

        SPDesktop   *desktop    = _desktop;
        if( desktop->getSelection()->isEmpty() ) {
            // Update default
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            prefs->mergeStyle("/tools/text/style", css);
        } else {
            // If there is a selection, update
            sp_desktop_set_style (desktop, css, true, true); // Results in selection change called twice.
            DocumentUndo::done(desktop->getDocument(), SP_VERB_CONTEXT_TEXT,
                               _("Text: Change font family"));
        }
        sp_repr_css_attr_unref (css);
    }

    // unfreeze
    _freeze = false;

#ifdef DEBUG_TEXT
    std::cout << "sp_text_toolbox_fontfamily_changes: exit"  << std::endl;
    std::cout << "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM" << std::endl;
    std::cout << std::endl;
#endif
}

GtkWidget *
TextToolbar::create(SPDesktop *desktop)
{
    auto tb = Gtk::manage(new TextToolbar(desktop));
    return GTK_WIDGET(tb->gobj());
}

void
TextToolbar::fontsize_value_changed()
{
    // quit if run by the _changed callbacks
    if (_freeze) {
        return;
    }
    _freeze = true;

    auto active_text = _font_size_item->get_active_text();
    char const *text = active_text.c_str();
    gchar *endptr;
    gdouble size = g_strtod( text, &endptr );
    if (endptr == text) {  // Conversion failed, non-numeric input.
        g_warning( "Conversion of size text to double failed, input: %s\n", text );
        _freeze = false;
        return;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int max_size = prefs->getInt("/dialogs/textandfont/maxFontSize", 10000); // somewhat arbitrary, but text&font preview freezes with too huge fontsizes

    if (size > max_size)
        size = max_size;

    // Set css font size.
    SPCSSAttr *css = sp_repr_css_attr_new ();
    Inkscape::CSSOStringStream osfs;
    int unit = prefs->getInt("/options/font/unitType", SP_CSS_UNIT_PT);
    if (prefs->getBool("/options/font/textOutputPx", true)) {
        osfs << sp_style_css_size_units_to_px(size, unit) << sp_style_get_css_unit_string(SP_CSS_UNIT_PX);
    } else {
        osfs << size << sp_style_get_css_unit_string(unit);
    }
    sp_repr_css_set_property (css, "font-size", osfs.str().c_str());
    double factor = size / selection_fontsize;

    // Apply font size to selected objects.
    text_outer_set_style(css);

    Unit const *unit_lh = _tracker->getActiveUnit();
    g_return_if_fail(unit_lh != nullptr);
    if (!is_relative(unit_lh) && _outer) {
        double lineheight = _line_height_adj->get_value();
        _freeze = false;
        _line_height_adj->set_value(lineheight * factor);
        _freeze = true;
    }
    // If no selected objects, set default.
    SPStyle query(_desktop->getDocument());
    int result_numbers =
        sp_desktop_query_style (_desktop, &query, QUERY_STYLE_PROPERTY_FONTNUMBERS);
    if (result_numbers == QUERY_STYLE_NOTHING)
    {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->mergeStyle("/tools/text/style", css);
    } else {
        // Save for undo
        sp_desktop_set_style (_desktop, css, true, true);
        DocumentUndo::maybeDone(_desktop->getDocument(), "ttb:size", SP_VERB_NONE,
                             _("Text: Change font size"));
    }

    sp_repr_css_attr_unref(css);

    _freeze = false;
}

void
TextToolbar::fontstyle_value_changed()
{
    // quit if run by the _changed callbacks
    if (_freeze) {
        return;
    }
    _freeze = true;

    Glib::ustring new_style = _font_style_item->get_active_text();

    Inkscape::FontLister* fontlister = Inkscape::FontLister::get_instance();

    if( new_style.compare( fontlister->get_font_style() ) != 0 ) {

        fontlister->set_font_style( new_style );
        // active text set in sp_text_toolbox_seletion_changed()

        SPCSSAttr *css = sp_repr_css_attr_new ();
        fontlister->fill_css( css );

        SPDesktop   *desktop    = _desktop;
        sp_desktop_set_style (desktop, css, true, true);


        // If no selected objects, set default.
        SPStyle query(_desktop->getDocument());
        int result_style =
            sp_desktop_query_style (desktop, &query, QUERY_STYLE_PROPERTY_FONTSTYLE);
        if (result_style == QUERY_STYLE_NOTHING) {
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            prefs->mergeStyle("/tools/text/style", css);
        } else {
            // Save for undo
            DocumentUndo::done(desktop->getDocument(), SP_VERB_CONTEXT_TEXT,
                               _("Text: Change font style"));
        }

        sp_repr_css_attr_unref (css);

    }

    _freeze = false;
}

// Handles both Superscripts and Subscripts
void
TextToolbar::script_changed(Gtk::ToggleToolButton *btn)
{
    // quit if run by the _changed callbacks
    if (_freeze) {
        return;
    }

    _freeze = true;

    // Called by Superscript or Subscript button?
    auto name = btn->get_name();
    gint prop = (btn == _superscript_item) ? 0 : 1;

#ifdef DEBUG_TEXT
    std::cout << "TextToolbar::script_changed: " << prop << std::endl;
#endif

    // Query baseline
    SPStyle query(_desktop->getDocument());
    int result_baseline = sp_desktop_query_style (_desktop, &query, QUERY_STYLE_PROPERTY_BASELINES);

    bool setSuper = false;
    bool setSub   = false;

    if (Inkscape::is_query_style_updateable(result_baseline)) {
        // If not set or mixed, turn on superscript or subscript
        if( prop == 0 ) {
            setSuper = true;
        } else {
            setSub = true;
        }
    } else {
        // Superscript
        gboolean superscriptSet = (query.baseline_shift.set &&
                                   query.baseline_shift.type == SP_BASELINE_SHIFT_LITERAL &&
                                   query.baseline_shift.literal == SP_CSS_BASELINE_SHIFT_SUPER );

        // Subscript
        gboolean subscriptSet = (query.baseline_shift.set &&
                                 query.baseline_shift.type == SP_BASELINE_SHIFT_LITERAL &&
                                 query.baseline_shift.literal == SP_CSS_BASELINE_SHIFT_SUB );

        setSuper = !superscriptSet && prop == 0;
        setSub   = !subscriptSet   && prop == 1;
    }

    // Set css properties
    SPCSSAttr *css = sp_repr_css_attr_new ();
    if( setSuper || setSub ) {
        // Openoffice 2.3 and Adobe use 58%, Microsoft Word 2002 uses 65%, LaTex about 70%.
        // 58% looks too small to me, especially if a superscript is placed on a superscript.
        // If you make a change here, consider making a change to baseline-shift amount
        // in style.cpp.
        sp_repr_css_set_property (css, "font-size", "65%");
    } else {
        sp_repr_css_set_property (css, "font-size", "");
    }
    if( setSuper ) {
        sp_repr_css_set_property (css, "baseline-shift", "super");
    } else if( setSub ) {
        sp_repr_css_set_property (css, "baseline-shift", "sub");
    } else {
        sp_repr_css_set_property (css, "baseline-shift", "baseline");
    }

    // Apply css to selected objects.
    SPDesktop *desktop = _desktop;
    sp_desktop_set_style (desktop, css, true, false);

    // Save for undo
    if(result_baseline != QUERY_STYLE_NOTHING) {
        DocumentUndo::maybeDone(_desktop->getDocument(), "ttb:script", SP_VERB_NONE,
                             _("Text: Change superscript or subscript"));
    }
    _freeze = false;
}

void
TextToolbar::align_mode_changed(int mode)
{
    // quit if run by the _changed callbacks
    if (_freeze) {
        return;
    }
    _freeze = true;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt("/tools/text/align_mode", mode);

    SPDesktop *desktop = _desktop;

    // move the x of all texts to preserve the same bbox
    Inkscape::Selection *selection = desktop->getSelection();
    auto itemlist= selection->items();
    for (auto i : itemlist) {
        SPText *text = dynamic_cast<SPText *>(i);
        // SPFlowtext *flowtext = dynamic_cast<SPFlowtext *>(i);
        if (text) {
            SPItem *item = i;

            unsigned writing_mode = item->style->writing_mode.value;
            // below, variable names suggest horizontal move, but we check the writing direction
            // and move in the corresponding axis
            Geom::Dim2 axis;
            if (writing_mode == SP_CSS_WRITING_MODE_LR_TB || writing_mode == SP_CSS_WRITING_MODE_RL_TB) {
                axis = Geom::X;
            } else {
                axis = Geom::Y;
            }

            Geom::OptRect bbox = item->geometricBounds();
            if (!bbox)
                continue;
            double width = bbox->dimensions()[axis];
            // If you want to align within some frame, other than the text's own bbox, calculate
            // the left and right (or top and bottom for tb text) slacks of the text inside that
            // frame (currently unused)
            double left_slack = 0;
            double right_slack = 0;
            unsigned old_align = item->style->text_align.value;
            double move = 0;
            if (old_align == SP_CSS_TEXT_ALIGN_START || old_align == SP_CSS_TEXT_ALIGN_LEFT) {
                switch (mode) {
                    case 0:
                        move = -left_slack;
                        break;
                    case 1:
                        move = width/2 + (right_slack - left_slack)/2;
                        break;
                    case 2:
                        move = width + right_slack;
                        break;
                }
            } else if (old_align == SP_CSS_TEXT_ALIGN_CENTER) {
                switch (mode) {
                    case 0:
                        move = -width/2 - left_slack;
                        break;
                    case 1:
                        move = (right_slack - left_slack)/2;
                        break;
                    case 2:
                        move = width/2 + right_slack;
                        break;
                }
            } else if (old_align == SP_CSS_TEXT_ALIGN_END || old_align == SP_CSS_TEXT_ALIGN_RIGHT) {
                switch (mode) {
                    case 0:
                        move = -width - left_slack;
                        break;
                    case 1:
                        move = -width/2 + (right_slack - left_slack)/2;
                        break;
                    case 2:
                        move = right_slack;
                        break;
                }
            }
            Geom::Point XY = SP_TEXT(item)->attributes.firstXY();
            if (axis == Geom::X) {
                XY = XY + Geom::Point (move, 0);
            } else {
                XY = XY + Geom::Point (0, move);
            }
            SP_TEXT(item)->attributes.setFirstXY(XY);
            item->updateRepr();
            item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        }
    }

    SPCSSAttr *css = sp_repr_css_attr_new ();
    switch (mode)
    {
        case 0:
        {
            sp_repr_css_set_property (css, "text-anchor", "start");
            sp_repr_css_set_property (css, "text-align", "start");
            break;
        }
        case 1:
        {
            sp_repr_css_set_property (css, "text-anchor", "middle");
            sp_repr_css_set_property (css, "text-align", "center");
            break;
        }

        case 2:
        {
            sp_repr_css_set_property (css, "text-anchor", "end");
            sp_repr_css_set_property (css, "text-align", "end");
            break;
        }

        case 3:
        {
            sp_repr_css_set_property (css, "text-anchor", "start");
            sp_repr_css_set_property (css, "text-align", "justify");
            break;
        }
    }

    SPStyle query(_desktop->getDocument());
    int result_numbers =
        sp_desktop_query_style (_desktop, &query, QUERY_STYLE_PROPERTY_FONTNUMBERS);

    // If querying returned nothing, update default style.
    if (result_numbers == QUERY_STYLE_NOTHING)
    {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->mergeStyle("/tools/text/style", css);
    }

    sp_desktop_set_style (desktop, css, true, true);
    if (result_numbers != QUERY_STYLE_NOTHING)
    {
        DocumentUndo::done(_desktop->getDocument(), SP_VERB_CONTEXT_TEXT,
                       _("Text: Change alignment"));
    }
    sp_repr_css_attr_unref (css);

    desktop->getCanvas()->grab_focus();

    _freeze = false;
}

void
TextToolbar::writing_mode_changed(int mode)
{
    // quit if run by the _changed callbacks
    if (_freeze) {
        return;
    }
    _freeze = true;

    SPCSSAttr   *css        = sp_repr_css_attr_new ();
    switch (mode)
    {
        case 0:
            {
                sp_repr_css_set_property (css, "writing-mode", "lr-tb");
                break;
            }

        case 1:
            {
                sp_repr_css_set_property (css, "writing-mode", "tb-rl");
                break;
            }

        case 2:
            {
                sp_repr_css_set_property (css, "writing-mode", "vertical-lr");
                break;
            }
    }

    SPStyle query(_desktop->getDocument());
    int result_numbers =
        sp_desktop_query_style (_desktop, &query, QUERY_STYLE_PROPERTY_WRITINGMODES);

    // If querying returned nothing, update default style.
    if (result_numbers == QUERY_STYLE_NOTHING)
    {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->mergeStyle("/tools/text/style", css);
    }

    sp_desktop_set_style (_desktop, css, true, true);
    if(result_numbers != QUERY_STYLE_NOTHING)
    {
        DocumentUndo::done(_desktop->getDocument(), SP_VERB_CONTEXT_TEXT,
                       _("Text: Change writing mode"));
    }
    sp_repr_css_attr_unref (css);

    _desktop->getCanvas()->grab_focus();

    _freeze = false;
}

void
TextToolbar::orientation_changed(int mode)
{
    // quit if run by the _changed callbacks
    if (_freeze) {
        return;
    }
    _freeze = true;

    SPCSSAttr   *css        = sp_repr_css_attr_new ();
    switch (mode)
    {
        case 0:
        {
            sp_repr_css_set_property (css, "text-orientation", "auto");
            break;
        }

        case 1:
        {
            sp_repr_css_set_property (css, "text-orientation", "upright");
            break;
        }

        case 2:
        {
            sp_repr_css_set_property (css, "text-orientation", "sideways");
            break;
        }
    }

    SPStyle query(_desktop->getDocument());
    int result_numbers =
        sp_desktop_query_style (_desktop, &query, QUERY_STYLE_PROPERTY_WRITINGMODES);

    // If querying returned nothing, update default style.
    if (result_numbers == QUERY_STYLE_NOTHING)
    {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->mergeStyle("/tools/text/style", css);
    }

    sp_desktop_set_style (_desktop, css, true, true);
    if(result_numbers != QUERY_STYLE_NOTHING)
    {
        DocumentUndo::done(_desktop->getDocument(), SP_VERB_CONTEXT_TEXT,
                       _("Text: Change orientation"));
    }
    sp_repr_css_attr_unref (css);
    _desktop->canvas->grab_focus();

    _freeze = false;
}

void
TextToolbar::direction_changed(int mode)
{
    // quit if run by the _changed callbacks
    if (_freeze) {
        return;
    }
    _freeze = true;

    SPCSSAttr   *css        = sp_repr_css_attr_new ();
    switch (mode)
    {
        case 0:
        {
            sp_repr_css_set_property (css, "direction", "ltr");
            break;
        }

        case 1:
        {
            sp_repr_css_set_property (css, "direction", "rtl");
            break;
        }
    }

    SPStyle query(_desktop->getDocument());
    int result_numbers =
        sp_desktop_query_style (_desktop, &query, QUERY_STYLE_PROPERTY_WRITINGMODES);

    // If querying returned nothing, update default style.
    if (result_numbers == QUERY_STYLE_NOTHING)
    {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->mergeStyle("/tools/text/style", css);
    }

    sp_desktop_set_style (_desktop, css, true, true);
    if(result_numbers != QUERY_STYLE_NOTHING)
    {
        DocumentUndo::done(_desktop->getDocument(), SP_VERB_CONTEXT_TEXT,
                       _("Text: Change direction"));
    }
    sp_repr_css_attr_unref (css);

    _desktop->getCanvas()->grab_focus();

    _freeze = false;
}

void
TextToolbar::lineheight_value_changed()
{
    // quit if run by the _changed callbacks or is not text tool
    if (_freeze || !SP_IS_TEXT_CONTEXT(_desktop->event_context)) {
        return;
    }
        
    _freeze = true;
    SPDesktop *desktop = _desktop;
    // Get user selected unit and save as preference
    Unit const *unit = _tracker->getActiveUnit();
    // @Tav same disabled unit
    g_return_if_fail(unit != nullptr);

    // This nonsense is to get SP_CSS_UNIT_xx value corresponding to unit so
    // we can save it (allows us to adjust line height value when unit changes).

    // Set css line height.
    SPCSSAttr *css = sp_repr_css_attr_new ();
    Inkscape::CSSOStringStream osfs;
    if ( is_relative(unit) ) {
        osfs << _line_height_adj->get_value() << unit->abbr;
    } else {
        // Inside SVG file, always use "px" for absolute units.
        osfs << Quantity::convert(_line_height_adj->get_value(), unit, "px") << "px";
    }

    sp_repr_css_set_property (css, "line-height", osfs.str().c_str());

    Inkscape::Selection *selection = desktop->getSelection();
    auto itemlist = selection->items();
    if (_outer) {
        // Special else makes this different from other uses of text_outer_set_style
        text_outer_set_style(css);
    } else {
        SPItem *parent = dynamic_cast<SPItem *>(*itemlist.begin());
        SPStyle *parent_style = parent->style;
        SPCSSAttr *parent_cssatr = sp_css_attr_from_style(parent_style, SP_STYLE_FLAG_IFSET);
        Glib::ustring parent_lineheight = sp_repr_css_property(parent_cssatr, "line-height", "1.25");
        SPCSSAttr *cssfit = sp_repr_css_attr_new();
        sp_repr_css_set_property(cssfit, "line-height", parent_lineheight.c_str());
        double minheight = 0;
        if (parent_style) {
            minheight = parent_style->line_height.computed;
        }
        if (minheight) {
            for (auto i : parent->childList(false)) {
                SPItem *child = dynamic_cast<SPItem *>(i);
                if (!child) {
                    continue;
                }
                recursively_set_properties(child, cssfit);
            }
        }
        sp_repr_css_set_property(cssfit, "line-height", "0");
        parent->changeCSS(cssfit, "style");
        subselection_wrap_toggle(true);
        sp_desktop_set_style(desktop, css, true, true);
        subselection_wrap_toggle(false);
        sp_repr_css_attr_unref(cssfit);
    }
    // Only need to save for undo if a text item has been changed.
    itemlist = selection->items();
    bool modmade = false;
    for (auto i : itemlist) {
        SPText *text = dynamic_cast<SPText *>(i);
        SPFlowtext *flowtext = dynamic_cast<SPFlowtext *>(i);
        if (text || flowtext) {
            modmade = true;
            break;
        }
    }

    // Save for undo
    if (modmade) {
        // Call ensureUpToDate() causes rebuild of text layout (with all proper style
        // cascading, etc.). For multi-line text with sodipodi::role="line", we must explicitly
        // save new <tspan> 'x' and 'y' attribute values by calling updateRepr().
        // Partial fix for bug #1590141.

        desktop->getDocument()->ensureUpToDate();
        for (auto i : itemlist) {
            SPText *text = dynamic_cast<SPText *>(i);
            SPFlowtext *flowtext = dynamic_cast<SPFlowtext *>(i);
            if (text || flowtext) {
                (i)->updateRepr();
            }
        }
        if (!_outer) {
            prepare_inner();
        }
        DocumentUndo::maybeDone(desktop->getDocument(), "ttb:line-height", SP_VERB_NONE, _("Text: Change line-height"));
    }

    // If no selected objects, set default.
    SPStyle query(_desktop->getDocument());
    int result_numbers = sp_desktop_query_style(desktop, &query, QUERY_STYLE_PROPERTY_FONTNUMBERS);
    if (result_numbers == QUERY_STYLE_NOTHING)
    {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->mergeStyle("/tools/text/style", css);
    }

    sp_repr_css_attr_unref (css);

    _freeze = false;
}

void
TextToolbar::lineheight_unit_changed(int /* Not Used */)
{
    // quit if run by the _changed callbacks or is not text tool
    if (_freeze || !SP_IS_TEXT_CONTEXT(_desktop->event_context)) {
        return;
    }
    _freeze = true;

    // Get old saved unit
    int old_unit = _lineheight_unit;

    // Get user selected unit and save as preference
    Unit const *unit = _tracker->getActiveUnit();
    g_return_if_fail(unit != nullptr);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // This nonsense is to get SP_CSS_UNIT_xx value corresponding to unit.
    SPILength temp_length;
    Inkscape::CSSOStringStream temp_stream;
    temp_stream << 1 << unit->abbr;
    temp_length.read(temp_stream.str().c_str());
    prefs->setInt("/tools/text/lineheight/display_unit", temp_length.unit);
    if (old_unit == temp_length.unit) {
        _freeze = false;
        return;
    }

    // Read current line height value
    double line_height = _line_height_adj->get_value();
    SPDesktop *desktop = _desktop;
    Inkscape::Selection *selection = desktop->getSelection();
    auto itemlist = selection->items();

    // Convert between units
    double font_size = 0;
    double doc_scale = 1;
    int count = 0;
    bool has_flow = false;

    for (auto i : itemlist) {
        SPText *text = dynamic_cast<SPText *>(i);
        SPFlowtext *flowtext = dynamic_cast<SPFlowtext *>(i);
        if (text || flowtext) {
            doc_scale = Geom::Affine(i->i2dt_affine()).descrim();
            font_size += i->style->font_size.computed * doc_scale;
            ++count;
        }
        if (flowtext) {
            has_flow = true;
        }
    }
    if (count > 0) {
        font_size /= count;
    } else {
        font_size = 20;
    }
    if ((unit->abbr == "" || unit->abbr == "em") && (old_unit == SP_CSS_UNIT_NONE || old_unit == SP_CSS_UNIT_EM)) {
        // Do nothing
    } else if ((unit->abbr == "" || unit->abbr == "em") && old_unit == SP_CSS_UNIT_EX) {
        line_height *= 0.5;
    } else if ((unit->abbr) == "ex" && (old_unit == SP_CSS_UNIT_EM || old_unit == SP_CSS_UNIT_NONE)) {
        line_height *= 2.0;
    } else if ((unit->abbr == "" || unit->abbr == "em") && old_unit == SP_CSS_UNIT_PERCENT) {
        line_height /= 100.0;
    } else if ((unit->abbr) == "%" && (old_unit == SP_CSS_UNIT_EM || old_unit == SP_CSS_UNIT_NONE)) {
        line_height *= 100;
    } else if ((unit->abbr) == "ex" && old_unit == SP_CSS_UNIT_PERCENT) {
        line_height /= 50.0;
    } else if ((unit->abbr) == "%" && old_unit == SP_CSS_UNIT_EX) {
        line_height *= 50;
    } else if (is_relative(unit)) {
        // Convert absolute to relative... for the moment use average font-size
        if (old_unit == SP_CSS_UNIT_NONE) old_unit = SP_CSS_UNIT_EM;
        line_height = Quantity::convert(line_height, sp_style_get_css_unit_string(old_unit), "px");

        if (font_size > 0) {
            line_height /= font_size;
        }
        if ((unit->abbr) == "%") {
            line_height *= 100;
        } else if ((unit->abbr) == "ex") {
            line_height *= 2;
        }
    } else if (old_unit == SP_CSS_UNIT_NONE || old_unit == SP_CSS_UNIT_PERCENT || old_unit == SP_CSS_UNIT_EM ||
            old_unit == SP_CSS_UNIT_EX) {
        // Convert relative to absolute... for the moment use average font-size
        if (old_unit == SP_CSS_UNIT_PERCENT) {
            line_height /= 100.0;
        } else if (old_unit == SP_CSS_UNIT_EX) {
            line_height /= 2.0;
        }
        line_height *= font_size;
        line_height = Quantity::convert(line_height, "px", unit);
    } else {
        // Convert between different absolute units (only used in GUI)
        line_height = Quantity::convert(line_height, sp_style_get_css_unit_string(old_unit), unit);
    }
    // Set css line height.
    SPCSSAttr *css = sp_repr_css_attr_new ();
    Inkscape::CSSOStringStream osfs;
    // Set css line height.
    if ( is_relative(unit) ) {
        osfs << line_height << unit->abbr;
    } else {
        osfs << Quantity::convert(line_height, unit, "px") << "px";
    }
    sp_repr_css_set_property (css, "line-height", osfs.str().c_str());

    // Update GUI with line_height value.
    _line_height_adj->set_value(line_height);
    // Update "climb rate"  The custom action has a step property but no way to set it.
    if (unit->abbr == "%") {
        _line_height_adj->set_step_increment(1.0);
        _line_height_adj->set_page_increment(10.0);
    } else {
        _line_height_adj->set_step_increment(0.1);
        _line_height_adj->set_page_increment(1.0);
    }
    // Internal function to set line-height which is spacing mode dependent.
    SPItem *parent = itemlist.empty() ? nullptr : dynamic_cast<SPItem *>(*itemlist.begin());
    SPStyle *parent_style = nullptr;
    if (parent) {
        parent_style = parent->style;
    }
    bool inside = false;
    if (_outer) {
        if (!selection->singleItem() || !parent_style || parent_style->line_height.computed != 0) {
            for (auto i = itemlist.begin(); i != itemlist.end(); ++i) {
                if (dynamic_cast<SPText *>(*i) || dynamic_cast<SPFlowtext *>(*i)) {
                    SPItem *item = *i;
                    // Scale by inverse of accumulated parent transform
                    SPCSSAttr *css_set = sp_repr_css_attr_new();
                    sp_repr_css_merge(css_set, css);
                    Geom::Affine const local(item->i2doc_affine());
                    double const ex(local.descrim());
                    if ((ex != 0.0) && (ex != 1.0)) {
                        sp_css_attr_scale(css_set, 1 / ex);
                    }
                    recursively_set_properties(item, css_set);
                    sp_repr_css_attr_unref(css_set);
                }
            }
        } else {
            inside = true;
        }
    }
    if (!_outer || inside) {
        SPCSSAttr *parent_cssatr = sp_css_attr_from_style(parent_style, SP_STYLE_FLAG_IFSET);
        Glib::ustring parent_lineheight = sp_repr_css_property(parent_cssatr, "line-height", "1.25");
        SPCSSAttr *cssfit = sp_repr_css_attr_new();
        sp_repr_css_set_property(cssfit, "line-height", parent_lineheight.c_str());
        double minheight = 0;
        if (parent_style) {
            minheight = parent_style->line_height.computed;
        }
        if (minheight) {
            for (auto i : parent->childList(false)) {
                SPItem *child = dynamic_cast<SPItem *>(i);
                if (!child) {
                    continue;
                }
                recursively_set_properties(child, cssfit);
            }
        }
        sp_repr_css_set_property(cssfit, "line-height", "0");
        parent->changeCSS(cssfit, "style");
        subselection_wrap_toggle(true);
        sp_desktop_set_style(desktop, css, true, true);
        subselection_wrap_toggle(false);
        sp_repr_css_attr_unref(cssfit);
    }
    itemlist= selection->items();
    // Only need to save for undo if a text item has been changed.
    bool modmade = false;
    for (auto i : itemlist) {
        SPText *text = dynamic_cast<SPText *>(i);
        SPFlowtext *flowtext = dynamic_cast<SPFlowtext *>(i);
        if (text || flowtext) {
            modmade = true;
            break;
        }
    }
    // Save for undo
    if(modmade) {
        // Call ensureUpToDate() causes rebuild of text layout (with all proper style
        // cascading, etc.). For multi-line text with sodipodi::role="line", we must explicitly
        // save new <tspan> 'x' and 'y' attribute values by calling updateRepr().
        // Partial fix for bug #1590141.

        desktop->getDocument()->ensureUpToDate();
        for (auto i : itemlist) {
            SPText *text = dynamic_cast<SPText *>(i);
            SPFlowtext *flowtext = dynamic_cast<SPFlowtext *>(i);
            if (text || flowtext) {
                (i)->updateRepr();
            }
        }
        if (_outer) {
            prepare_inner();
        }
        DocumentUndo::maybeDone(_desktop->getDocument(), "ttb:line-height", SP_VERB_NONE,
                             _("Text: Change line-height unit"));
    }

    // If no selected objects, set default.
    SPStyle query(_desktop->getDocument());
    int result_numbers =
        sp_desktop_query_style (_desktop, &query, QUERY_STYLE_PROPERTY_FONTNUMBERS);
    if (result_numbers == QUERY_STYLE_NOTHING)
    {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->mergeStyle("/tools/text/style", css);
    }

    sp_repr_css_attr_unref (css);

    _freeze = false;
}

void TextToolbar::fontsize_unit_changed(int /* Not Used */)
{
    // quit if run by the _changed callbacks
    Unit const *unit = _tracker_fs->getActiveUnit();
    g_return_if_fail(unit != nullptr);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // This nonsense is to get SP_CSS_UNIT_xx value corresponding to unit.
    SPILength temp_size;
    Inkscape::CSSOStringStream temp_size_stream;
    temp_size_stream << 1 << unit->abbr;
    temp_size.read(temp_size_stream.str().c_str());
    prefs->setInt("/options/font/unitType", temp_size.unit);
    selection_changed(_desktop->selection);
}

void
TextToolbar::wordspacing_value_changed()
{
    // quit if run by the _changed callbacks
    if (_freeze) {
        return;
    }
    _freeze = true;

    // At the moment this handles only numerical values (i.e. no em unit).
    // Set css word-spacing
    SPCSSAttr *css = sp_repr_css_attr_new ();
    Inkscape::CSSOStringStream osfs;
    osfs << _word_spacing_adj->get_value() << "px"; // For now always use px
    sp_repr_css_set_property (css, "word-spacing", osfs.str().c_str());
    text_outer_set_style(css);

    // If no selected objects, set default.
    SPStyle query(_desktop->getDocument());
    int result_numbers =
        sp_desktop_query_style (_desktop, &query, QUERY_STYLE_PROPERTY_FONTNUMBERS);
    if (result_numbers == QUERY_STYLE_NOTHING)
    {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->mergeStyle("/tools/text/style", css);
    } else {
        // Save for undo
        DocumentUndo::maybeDone(_desktop->getDocument(), "ttb:word-spacing", SP_VERB_NONE,
                             _("Text: Change word-spacing"));
    }

    sp_repr_css_attr_unref (css);

    _freeze = false;
}

void
TextToolbar::letterspacing_value_changed()
{
    // quit if run by the _changed callbacks
    if (_freeze) {
        return;
    }
    _freeze = true;

    // At the moment this handles only numerical values (i.e. no em unit).
    // Set css letter-spacing
    SPCSSAttr *css = sp_repr_css_attr_new ();
    Inkscape::CSSOStringStream osfs;
    osfs << _letter_spacing_adj->get_value() << "px";  // For now always use px
    sp_repr_css_set_property (css, "letter-spacing", osfs.str().c_str());
    text_outer_set_style(css);

    // If no selected objects, set default.
    SPStyle query(_desktop->getDocument());
    int result_numbers =
        sp_desktop_query_style (_desktop, &query, QUERY_STYLE_PROPERTY_FONTNUMBERS);
    if (result_numbers == QUERY_STYLE_NOTHING)
    {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->mergeStyle("/tools/text/style", css);
    }
    else
    {
        // Save for undo
        DocumentUndo::maybeDone(_desktop->getDocument(), "ttb:letter-spacing", SP_VERB_NONE,
                             _("Text: Change letter-spacing"));
    }

    sp_repr_css_attr_unref (css);

    _freeze = false;
}

void
TextToolbar::dx_value_changed()
{
    // quit if run by the _changed callbacks
    if (_freeze) {
        return;
    }
    _freeze = true;

    gdouble new_dx = _dx_adj->get_value();
    bool modmade = false;

    if( SP_IS_TEXT_CONTEXT(_desktop->event_context) ) {
        Inkscape::UI::Tools::TextTool *const tc = SP_TEXT_CONTEXT(_desktop->event_context);
        if( tc ) {
            unsigned char_index = -1;
            TextTagAttributes *attributes =
                text_tag_attributes_at_position( tc->text, std::min(tc->text_sel_start, tc->text_sel_end), &char_index );
            if( attributes ) {
                double old_dx = attributes->getDx( char_index );
                double delta_dx = new_dx - old_dx;
                sp_te_adjust_dx( tc->text, tc->text_sel_start, tc->text_sel_end, _desktop, delta_dx );
                modmade = true;
            }
        }
    }

    if(modmade) {
        // Save for undo
        DocumentUndo::maybeDone(_desktop->getDocument(), "ttb:dx", SP_VERB_NONE,
                             _("Text: Change dx (kern)"));
    }
    _freeze = false;
}

void
TextToolbar::dy_value_changed()
{
    // quit if run by the _changed callbacks
    if (_freeze) {
        return;
    }
    _freeze = true;

    gdouble new_dy = _dy_adj->get_value();
    bool modmade = false;

    if( SP_IS_TEXT_CONTEXT(_desktop->event_context) ) {
        Inkscape::UI::Tools::TextTool *const tc = SP_TEXT_CONTEXT(_desktop->event_context);
        if( tc ) {
            unsigned char_index = -1;
            TextTagAttributes *attributes =
                text_tag_attributes_at_position( tc->text, std::min(tc->text_sel_start, tc->text_sel_end), &char_index );
            if( attributes ) {
                double old_dy = attributes->getDy( char_index );
                double delta_dy = new_dy - old_dy;
                sp_te_adjust_dy( tc->text, tc->text_sel_start, tc->text_sel_end, _desktop, delta_dy );
                modmade = true;
            }
        }
    }

    if(modmade) {
        // Save for undo
        DocumentUndo::maybeDone(_desktop->getDocument(), "ttb:dy", SP_VERB_NONE,
                            _("Text: Change dy"));
    }

    _freeze = false;
}

void
TextToolbar::rotation_value_changed()
{
    // quit if run by the _changed callbacks
    if (_freeze) {
        return;
    }
    _freeze = true;

    gdouble new_degrees = _rotation_adj->get_value();

    bool modmade = false;
    if( SP_IS_TEXT_CONTEXT(_desktop->event_context) ) {
        Inkscape::UI::Tools::TextTool *const tc = SP_TEXT_CONTEXT(_desktop->event_context);
        if( tc ) {
            unsigned char_index = -1;
            TextTagAttributes *attributes =
                text_tag_attributes_at_position( tc->text, std::min(tc->text_sel_start, tc->text_sel_end), &char_index );
            if( attributes ) {
                double old_degrees = attributes->getRotate( char_index );
                double delta_deg = new_degrees - old_degrees;
                sp_te_adjust_rotation( tc->text, tc->text_sel_start, tc->text_sel_end, _desktop, delta_deg );
                modmade = true;
            }
        }
    }

    // Save for undo
    if(modmade) {
        DocumentUndo::maybeDone(_desktop->getDocument(), "ttb:rotate", SP_VERB_NONE,
                            _("Text: Change rotate"));
    }

    _freeze = false;
}

void TextToolbar::selection_modified_select_tool(Inkscape::Selection *selection, guint flags)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double factor = prefs->getDouble("/options/font/scaleLineHeightFromFontSIze", 1.0);
    if (factor != 1.0) {
        Unit const *unit_lh = _tracker->getActiveUnit();
        g_return_if_fail(unit_lh != nullptr);
        if (!is_relative(unit_lh) && _outer) {
            double lineheight = _line_height_adj->get_value();
            bool is_freeze = _freeze;
            _freeze = false;
            _line_height_adj->set_value(lineheight * factor);
            _freeze = is_freeze;
        }
        prefs->setDouble("/options/font/scaleLineHeightFromFontSIze", 1.0);
    }
}

void TextToolbar::selection_changed(Inkscape::Selection *selection) // don't bother to update font list if subsel
                                                                         // changed
{
#ifdef DEBUG_TEXT
    static int count = 0;
    ++count;
    std::cout << std::endl;
    std::cout << "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&" << std::endl;
    std::cout << "sp_text_toolbox_selection_changed: start " << count << std::endl;
#endif

    // quit if run by the _changed callbacks
    if (_freeze) {

#ifdef DEBUG_TEXT
        std::cout << "    Frozen, returning" << std::endl;
        std::cout << "sp_text_toolbox_selection_changed: exit " << count << std::endl;
        std::cout << "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&" << std::endl;
        std::cout << std::endl;
#endif
        return;
    }
    _freeze = true;

    // selection defined as argument but not used, argh!!!
    SPDesktop *desktop = _desktop;
    SPDocument *document = _desktop->getDocument();
    selection = desktop->getSelection();
    auto itemlist = selection->items();

#ifdef DEBUG_TEXT
    for(auto i : itemlist) {
        const gchar* id = i->getId();
        std::cout << "    " << id << std::endl;
    }
    Glib::ustring selected_text = sp_text_get_selected_text(_desktop->event_context);
    std::cout << "  Selected text: |" << selected_text << "|" << std::endl;
#endif

    // Only flowed text can be justified, only normal text can be kerned...
    // Find out if we have flowed text now so we can use it several places
    gboolean isFlow = false;
    std::vector<SPItem *> to_work;
    for (auto i : itemlist) {
        SPText *text = dynamic_cast<SPText *>(i);
        SPFlowtext *flowtext = dynamic_cast<SPFlowtext *>(i);
        if (text || flowtext) {
            to_work.push_back(i);
        }
        if (flowtext ||
            (text && text->style && text->style->shape_inside.set)) {
            isFlow = true;
        }
    }
    bool outside = false;
    if (selection && to_work.size() == 0) {
        outside = true;
    }

    Inkscape::FontLister *fontlister = Inkscape::FontLister::get_instance();
    fontlister->selection_update();
    // Update font list, but only if widget already created.
    if (_font_family_item->get_combobox() != nullptr) {
        _font_family_item->set_active_text(fontlister->get_font_family().c_str(), fontlister->get_font_family_row());
        _font_style_item->set_active_text(fontlister->get_font_style().c_str());
    }

    /*
     * Query from current selection:
     *   Font family (font-family)
     *   Style (font-weight, font-style, font-stretch, font-variant, font-align)
     *   Numbers (font-size, letter-spacing, word-spacing, line-height, text-anchor, writing-mode)
     *   Font specification (Inkscape private attribute)
     */
    SPStyle query(document);
    SPStyle query_fallback(document);
    int result_family = sp_desktop_query_style(desktop, &query, QUERY_STYLE_PROPERTY_FONTFAMILY);
    int result_style = sp_desktop_query_style(desktop, &query, QUERY_STYLE_PROPERTY_FONTSTYLE);
    int result_baseline = sp_desktop_query_style(desktop, &query, QUERY_STYLE_PROPERTY_BASELINES);
    int result_wmode = sp_desktop_query_style(desktop, &query, QUERY_STYLE_PROPERTY_WRITINGMODES);

    // Calling sp_desktop_query_style will result in a call to TextTool::_styleQueried().
    // This returns the style of the selected text inside the <text> element... which
    // is often the style of one or more <tspan>s. If we want the style of the outer
    // <text> objects then we need to bypass the call to TextTool::_styleQueried().
    // The desktop selection never includes the elements inside the <text> element.
    int result_numbers = 0;
    int result_numbers_fallback = 0;
    if (!outside) {
        if (_outer && this->_sub_active_item) {
            std::vector<SPItem *> qactive{ this->_sub_active_item };
            SPItem *parent = dynamic_cast<SPItem *>(this->_sub_active_item->parent);
            std::vector<SPItem *> qparent{ parent };
            result_numbers =
                sp_desktop_query_style_from_list(qactive, &query, QUERY_STYLE_PROPERTY_FONTNUMBERS);
            result_numbers_fallback =
                sp_desktop_query_style_from_list(qparent, &query_fallback, QUERY_STYLE_PROPERTY_FONTNUMBERS);
        } else if (_outer) {
            result_numbers = sp_desktop_query_style_from_list(to_work, &query, QUERY_STYLE_PROPERTY_FONTNUMBERS);
        } else {
            result_numbers = sp_desktop_query_style(desktop, &query, QUERY_STYLE_PROPERTY_FONTNUMBERS);
        }
    } else {
        result_numbers =
                sp_desktop_query_style(desktop, &query, QUERY_STYLE_PROPERTY_FONTNUMBERS);
    }
    
    /*
     * If no text in selection (querying returned nothing), read the style from
     * the /tools/text preferences (default style for new texts). Return if
     * tool bar already set to these preferences.
     */
    if (result_family  == QUERY_STYLE_NOTHING ||
        result_style   == QUERY_STYLE_NOTHING ||
        result_numbers == QUERY_STYLE_NOTHING ||
        result_wmode   == QUERY_STYLE_NOTHING ) {
        // There are no texts in selection, read from preferences.
        query.readFromPrefs("/tools/text");
#ifdef DEBUG_TEXT
        std::cout << "    read style from prefs:" << std::endl;
        sp_print_font( &query );
#endif
        if (_text_style_from_prefs) {
            // Do not reset the toolbar style from prefs if we already did it last time
            _freeze = false;
#ifdef DEBUG_TEXT
            std::cout << "    text_style_from_prefs: toolbar already set" << std:: endl;
            std::cout << "sp_text_toolbox_selection_changed: exit " << count << std::endl;
            std::cout << "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&" << std::endl;
            std::cout << std::endl;
#endif
            return;
        }

        // To ensure the value of the combobox is properly set on start-up, only mark
        // the prefs set if the combobox has already been constructed.
        if( _font_family_item->get_combobox() != nullptr ) {
            _text_style_from_prefs = true;
        }
    } else {
        _text_style_from_prefs = false;
    }

    // If we have valid query data for text (font-family, font-specification) set toolbar accordingly.
    {
        // Size (average of text selected)

        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        int unit = prefs->getInt("/options/font/unitType", SP_CSS_UNIT_PT);
        double size = 0;
        if (!size && _cusor_numbers != QUERY_STYLE_NOTHING) {
            size = sp_style_css_size_px_to_units(_query_cursor.font_size.computed, unit);
        }
        if (!size && result_numbers != QUERY_STYLE_NOTHING) {
            size = sp_style_css_size_px_to_units(query.font_size.computed, unit);
        }
        if (!size && result_numbers_fallback != QUERY_STYLE_NOTHING) {
            size = sp_style_css_size_px_to_units(query_fallback.font_size.computed, unit);
        }
        
        auto unit_str = sp_style_get_css_unit_string(unit);
        Glib::ustring tooltip = Glib::ustring::format(_("Font size"), " (", unit_str, ")");

        _font_size_item->set_tooltip(tooltip.c_str());

        Inkscape::CSSOStringStream os;
        // We dot want to parse values just show

        _tracker_fs->setActiveUnitByAbbr(sp_style_get_css_unit_string(unit));
        int rounded_size = std::round(size);
        if (std::abs((size - rounded_size)/size) < 0.0001) {
            // We use rounded_size to avoid rounding errors when, say, converting stored 'px' values to displayed 'pt' values.
            os << rounded_size;
            selection_fontsize = rounded_size;
        } else {
            os << size;
            selection_fontsize = size;
        }

        // Freeze to ignore callbacks.
        //g_object_freeze_notify( G_OBJECT( fontSizeAction->combobox ) );
        sp_text_set_sizes(GTK_LIST_STORE(_font_size_item->get_model()), unit);
        //g_object_thaw_notify( G_OBJECT( fontSizeAction->combobox ) );

        _font_size_item->set_active_text( os.str().c_str() );

        // Superscript
        gboolean superscriptSet =
            ((result_baseline == QUERY_STYLE_SINGLE || result_baseline == QUERY_STYLE_MULTIPLE_SAME ) &&
             query.baseline_shift.set &&
             query.baseline_shift.type == SP_BASELINE_SHIFT_LITERAL &&
             query.baseline_shift.literal == SP_CSS_BASELINE_SHIFT_SUPER );

        _superscript_item->set_active(superscriptSet);

        // Subscript
        gboolean subscriptSet =
            ((result_baseline == QUERY_STYLE_SINGLE || result_baseline == QUERY_STYLE_MULTIPLE_SAME ) &&
             query.baseline_shift.set &&
             query.baseline_shift.type == SP_BASELINE_SHIFT_LITERAL &&
             query.baseline_shift.literal == SP_CSS_BASELINE_SHIFT_SUB );

        _subscript_item->set_active(subscriptSet);

        // Alignment

        // Note: SVG 1.1 doesn't include text-align, SVG 1.2 Tiny doesn't include text-align="justify"
        // text-align="justify" was a draft SVG 1.2 item (along with flowed text).
        // Only flowed text can be left and right justified at the same time.
        // Disable button if we don't have flowed text.

        Glib::RefPtr<Gtk::ListStore> store = _align_item->get_store();
        Gtk::TreeModel::Row row = *(store->get_iter("3"));  // Justify entry
        UI::Widget::ComboToolItemColumns columns;
        row[columns.col_sensitive] = isFlow;

        int activeButton = 0;
        if (query.text_align.computed  == SP_CSS_TEXT_ALIGN_JUSTIFY)
        {
            activeButton = 3;
        } else {
            // This should take 'direction' into account
            if (query.text_anchor.computed == SP_CSS_TEXT_ANCHOR_START)  activeButton = 0;
            if (query.text_anchor.computed == SP_CSS_TEXT_ANCHOR_MIDDLE) activeButton = 1;
            if (query.text_anchor.computed == SP_CSS_TEXT_ANCHOR_END)    activeButton = 2;
        }
        _align_item->set_active( activeButton );

        double height = 0;
        gint line_height_unit = 0;

        if (!height && _cusor_numbers != QUERY_STYLE_NOTHING) {
            height = _query_cursor.line_height.value;
            line_height_unit = _query_cursor.line_height.unit;
        }

        if (!height && result_numbers != QUERY_STYLE_NOTHING) {
            height = query.line_height.value;
            line_height_unit = query.line_height.unit;
        }

        if (!height && result_numbers_fallback != QUERY_STYLE_NOTHING) {
            height = query_fallback.line_height.value;
            line_height_unit = query_fallback.line_height.unit;
        }

        if (line_height_unit == SP_CSS_UNIT_PERCENT) {
            height *= 100.0; // Inkscape store % as fraction in .value
        }

        // We dot want to parse values just show
        if (!is_relative(SPCSSUnit(line_height_unit))) {
            gint curunit = prefs->getInt("/tools/text/lineheight/display_unit", 1);
            // For backwards comaptibility
            if (is_relative(SPCSSUnit(curunit))) {
                prefs->setInt("/tools/text/lineheight/display_unit", 1);
                curunit = 1;
            }
            height = Quantity::convert(height, "px", sp_style_get_css_unit_string(curunit));
            line_height_unit = curunit;
        }
        _line_height_adj->set_value(height);


        // Update "climb rate"
        if (line_height_unit == SP_CSS_UNIT_PERCENT) {
            _line_height_adj->set_step_increment(1.0);
            _line_height_adj->set_page_increment(10.0);
        } else {
            _line_height_adj->set_step_increment(0.1);
            _line_height_adj->set_page_increment(1.0);
        }

        if( line_height_unit == SP_CSS_UNIT_NONE ) {
            // Function 'sp_style_get_css_unit_string' returns 'px' for unit none.
            // We need to avoid this.
            _tracker->setActiveUnitByAbbr("");
        } else {
            _tracker->setActiveUnitByAbbr(sp_style_get_css_unit_string(line_height_unit));
        }

        // Save unit so we can do conversions between new/old units.
        _lineheight_unit = line_height_unit;
        // Word spacing
        double wordSpacing;
        if (query.word_spacing.normal) wordSpacing = 0.0;
        else wordSpacing = query.word_spacing.computed; // Assume no units (change in desktop-style.cpp)

        _word_spacing_adj->set_value(wordSpacing);

        // Letter spacing
        double letterSpacing;
        if (query.letter_spacing.normal) letterSpacing = 0.0;
        else letterSpacing = query.letter_spacing.computed; // Assume no units (change in desktop-style.cpp)

        _letter_spacing_adj->set_value(letterSpacing);

        // Writing mode
        int activeButton2 = 0;
        if (query.writing_mode.computed == SP_CSS_WRITING_MODE_LR_TB) activeButton2 = 0;
        if (query.writing_mode.computed == SP_CSS_WRITING_MODE_TB_RL) activeButton2 = 1;
        if (query.writing_mode.computed == SP_CSS_WRITING_MODE_TB_LR) activeButton2 = 2;

        _writing_mode_item->set_active( activeButton2 );

        // Orientation
        int activeButton3 = 0;
        if (query.text_orientation.computed == SP_CSS_TEXT_ORIENTATION_MIXED   ) activeButton3 = 0;
        if (query.text_orientation.computed == SP_CSS_TEXT_ORIENTATION_UPRIGHT ) activeButton3 = 1;
        if (query.text_orientation.computed == SP_CSS_TEXT_ORIENTATION_SIDEWAYS) activeButton3 = 2;

        _orientation_item->set_active( activeButton3 );

        // Disable text orientation for horizontal text...
        _orientation_item->set_sensitive( activeButton2 != 0 );

        // Direction
        int activeButton4 = 0;
        if (query.direction.computed == SP_CSS_DIRECTION_LTR ) activeButton4 = 0;
        if (query.direction.computed == SP_CSS_DIRECTION_RTL ) activeButton4 = 1;
        _direction_item->set_active( activeButton4 );
    }

#ifdef DEBUG_TEXT
    std::cout << "    GUI: fontfamily.value: "     << query.font_family.value()  << std::endl;
    std::cout << "    GUI: font_size.computed: "   << query.font_size.computed   << std::endl;
    std::cout << "    GUI: font_weight.computed: " << query.font_weight.computed << std::endl;
    std::cout << "    GUI: font_style.computed: "  << query.font_style.computed  << std::endl;
    std::cout << "    GUI: text_anchor.computed: " << query.text_anchor.computed << std::endl;
    std::cout << "    GUI: text_align.computed:  " << query.text_align.computed  << std::endl;
    std::cout << "    GUI: line_height.computed: " << query.line_height.computed
              << "  line_height.value: "    << query.line_height.value
              << "  line_height.unit: "     << query.line_height.unit  << std::endl;
    std::cout << "    GUI: word_spacing.computed: " << query.word_spacing.computed
              << "  word_spacing.value: "    << query.word_spacing.value
              << "  word_spacing.unit: "     << query.word_spacing.unit  << std::endl;
    std::cout << "    GUI: letter_spacing.computed: " << query.letter_spacing.computed
              << "  letter_spacing.value: "    << query.letter_spacing.value
              << "  letter_spacing.unit: "     << query.letter_spacing.unit  << std::endl;
    std::cout << "    GUI: writing_mode.computed: " << query.writing_mode.computed << std::endl;
#endif

    // Kerning (xshift), yshift, rotation.  NB: These are not CSS attributes.
    if( SP_IS_TEXT_CONTEXT(_desktop->event_context) ) {
        Inkscape::UI::Tools::TextTool *const tc = SP_TEXT_CONTEXT(_desktop->event_context);
        if( tc ) {
            unsigned char_index = -1;
            TextTagAttributes *attributes =
                text_tag_attributes_at_position( tc->text, std::min(tc->text_sel_start, tc->text_sel_end), &char_index );
            if( attributes ) {

                // Dx
                double dx = attributes->getDx( char_index );
                _dx_adj->set_value(dx);

                // Dy
                double dy = attributes->getDy( char_index );
                _dy_adj->set_value(dy);

                // Rotation
                double rotation = attributes->getRotate( char_index );
                /* SVG value is between 0 and 360 but we're using -180 to 180 in widget */
                if( rotation > 180.0 ) rotation -= 360.0;
                _rotation_adj->set_value(rotation);

#ifdef DEBUG_TEXT
                std::cout << "    GUI: Dx: " << dx << std::endl;
                std::cout << "    GUI: Dy: " << dy << std::endl;
                std::cout << "    GUI: Rotation: " << rotation << std::endl;
#endif
            }
        }
    }

    {
        // Set these here as we don't always have kerning/rotating attributes
        _dx_item->set_sensitive(!isFlow);
        _dy_item->set_sensitive(!isFlow);
        _rotation_item->set_sensitive(!isFlow);
    }

#ifdef DEBUG_TEXT
    std::cout << "sp_text_toolbox_selection_changed: exit " << count << std::endl;
    std::cout << "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&" << std::endl;
    std::cout << std::endl;
#endif

    _freeze = false;
}

void
TextToolbar::watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec) {
    bool is_text_toolbar = SP_IS_TEXT_CONTEXT(ec);
    bool is_select_toolbar = !is_text_toolbar && SP_IS_SELECT_CONTEXT(ec);
    if (is_text_toolbar) {
        // Watch selection
        // Ensure FontLister is updated here first..................
        c_selection_changed =
            desktop->getSelection()->connectChangedFirst(sigc::mem_fun(*this, &TextToolbar::selection_changed));
        c_selection_modified = desktop->getSelection()->connectModifiedFirst(sigc::mem_fun(*this, &TextToolbar::selection_modified));
        c_subselection_changed = desktop->connectToolSubselectionChanged(sigc::mem_fun(*this, &TextToolbar::subselection_changed));
        this->_sub_active_item = nullptr;
        selection_changed(desktop->getSelection());
    } else if (is_select_toolbar) {
        c_selection_modified_select_tool = desktop->getSelection()->connectModifiedFirst(
            sigc::mem_fun(*this, &TextToolbar::selection_modified_select_tool));
    }


    if (!is_text_toolbar) {
        c_selection_changed.disconnect();
        c_selection_modified.disconnect();
        c_subselection_changed.disconnect();
    }

    if (!is_select_toolbar) {
        c_selection_modified_select_tool.disconnect();
    }
}

void
TextToolbar::selection_modified(Inkscape::Selection *selection, guint /*flags*/)
{
    this->_sub_active_item = nullptr;
    selection_changed(selection);

}

void TextToolbar::subselection_wrap_toggle(bool start)
{
    if (SP_IS_TEXT_CONTEXT(_desktop->event_context)) {
        Inkscape::UI::Tools::TextTool *const tc = SP_TEXT_CONTEXT(_desktop->event_context);
        if (tc) {
            _updating = true;
            Inkscape::Text::Layout const *layout = te_get_layout(tc->text);
            if (layout) {
                Inkscape::Text::Layout::iterator start_selection = tc->text_sel_start;
                Inkscape::Text::Layout::iterator end_selection = tc->text_sel_end;
                tc->text_sel_start = wrap_start;
                tc->text_sel_end = wrap_end;
                wrap_start = start_selection;
                wrap_end = end_selection;
            }
            _updating = start;
        }
    }
}

/*
* This function parse the just created line height in one or more lines of a text subselection
* can recibe 2 kinds of input because when we store a text element we apply a fallback that change 
* structure. This visualy is not reflected but user maybe want to change a part of this subselection
* once the falback is created, so we need more complex here to fill the gap.
* Basicaly we have a line height changed in the new wraper element/s between wrap_start and wrap_end
* this variables store starting iterator of first char in line and last char in line in a subselection
* this elements are styled well but we can have orphands text nodes before and after the subselection.
* this normaly 3 elements are inside a container as direct child of a text element, so we need to apply
* the container style to the optional previous and last text nodes warping into a new element that get the
* container style (this are not part to the subsselection). After wrap we unindent all child of the container and
* remove the container
* 
*/  
void TextToolbar::prepare_inner()
{
    Inkscape::UI::Tools::TextTool *const tc = SP_TEXT_CONTEXT(_desktop->event_context);
    if (tc) {
        Inkscape::Text::Layout *layout = const_cast<Inkscape::Text::Layout *>(te_get_layout(tc->text));
        if (layout) {
            SPDocument              *doc      = _desktop->getDocument();
            SPObject                *spobject = dynamic_cast<SPObject   *>(tc->text);
            SPItem                  *spitem   = dynamic_cast<SPItem     *>(tc->text);
            SPText                  *text     = dynamic_cast<SPText     *>(tc->text);
            SPFlowtext              *flowtext = dynamic_cast<SPFlowtext *>(tc->text);
            Inkscape::XML::Document *xml_doc = doc->getReprDoc();
            if (!spobject) {
                return;
            }

            // We check for external files with text nodes direct children of text elemet
            // and wrap it into a tspan elements as inkscape do.
            if (text) {
                std::vector<SPObject *> childs = spitem->childList(false);
                for (auto child : childs) {
                    SPString *spstring = dynamic_cast<SPString *>(child);
                    if (spstring) {
                        Glib::ustring content = spstring->string;
                        if (content != "\n") {
                            Inkscape::XML::Node *rstring = xml_doc->createTextNode(content.c_str());
                            Inkscape::XML::Node *rtspan  = xml_doc->createElement("svg:tspan");
                            //Inkscape::XML::Node *rnl     = xml_doc->createTextNode("\n");
                            rtspan->setAttribute("sodipodi:role", "line");
                            rtspan->addChild(rstring, nullptr);
                            text->getRepr()->addChild(rtspan, child->getRepr());
                            Inkscape::GC::release(rstring);
                            Inkscape::GC::release(rtspan);
                            text->getRepr()->removeChild(spstring->getRepr());
                        }
                    }
                }
            }

            // Here we remove temporary the shape to allow layout calculate where are the warp_end and warpo_start
            // position if one of this are hiden because the previous line height changed
            if (text) {
                text->hide_shape_inside();
            }
            if (flowtext) {
                flowtext->fix_overflow_flowregion(false);
            }
            SPObject *rawptr_start = nullptr;
            SPObject *rawptr_end = nullptr;
            layout->getSourceOfCharacter(wrap_start, &rawptr_start);
            layout->getSourceOfCharacter(wrap_end, &rawptr_end);
            if (text) {
                text->show_shape_inside();
            }
            if (flowtext) {
                flowtext->fix_overflow_flowregion(true);
            }
            if (!rawptr_start || !rawptr_end || !SP_IS_OBJECT(rawptr_start)|| !SP_IS_OBJECT(rawptr_end)) {
                return;
            }
            SPObject *startobj = reinterpret_cast<SPObject *>(rawptr_start);
            SPObject *endobj = reinterpret_cast<SPObject *>(rawptr_end);
            // here we try to slect the elements where we need to work
            // looping parent while text element in start and end
            // and geting this and the inbetween elements
            SPObject *start = startobj;
            SPObject *end   = endobj;
            while (start->parent != spobject)
            {
                start = start->parent;
            }
            while (end->parent != spobject)
            {
                end = end->parent;
            }
            std::vector<SPObject *> containers;
            while (start && start != end) {
                containers.push_back(start);
                start = start->getNext();
            }
            if (start) {
                containers.push_back(start);
            }
            for (auto container : containers) {
                Inkscape::XML::Node *prevchild = container->getRepr(); 
                std::vector<SPObject*> childs = container->childList(false);
                for (auto child : childs) {
                    SPString    *spstring  = dynamic_cast<SPString    *>(child);
                    SPFlowtspan *flowtspan = dynamic_cast<SPFlowtspan *>(child);
                    SPTSpan     *tspan     = dynamic_cast<SPTSpan     *>(child);
                    // we need to upper all flowtspans to container level
                    // to this we need to change the element from flowspan to flowpara
                    if (flowtspan) {
                        Inkscape::XML::Node *flowpara = xml_doc->createElement("svg:flowPara");
                        std::vector<SPObject*> fts_childs = flowtspan->childList(false);
                        bool hascontent = false;
                        // we need to move the contents to the new created element
                        // mayve we can move directly but the safer for me is duplicate
                        // inject into the new element and delete original
                        for (auto fts_child : fts_childs) {
                            // is this check necesary?
                            if (fts_child) {
                                Inkscape::XML::Node *fts_child_node = fts_child->getRepr()->duplicate(xml_doc);
                                flowtspan->getRepr()->removeChild(fts_child->getRepr());
                                flowpara->addChild(fts_child_node, nullptr);
                                Inkscape::GC::release(fts_child_node);
                                hascontent = true;
                            }
                        }
                        // if no contents we dont want to add
                        if (hascontent) {
                            flowpara->setAttribute("style", flowtspan->getRepr()->attribute("style"));
                            spobject->getRepr()->addChild(flowpara, prevchild);
                            Inkscape::GC::release(flowpara);
                            prevchild = flowpara;
                        }
                        container->getRepr()->removeChild(flowtspan->getRepr());
                    } else if (tspan) {
                        if (child->childList(false).size()) {
                            child->getRepr()->setAttribute("sodipodi:role", "line");
                            // maybe we need to move unindent function here 
                            // to be the same as other here
                            prevchild = unindent_node(child->getRepr(), prevchild);
                        } else {
                            // if no contents we dont want to add
                            container->getRepr()->removeChild(child->getRepr());
                        }
                    } else if (spstring) {
                        // we are on a text node, we act diferent if in a text or flowtext. 
                        // wrap a duplicate of the element and unindent after the prevchild
                        // and finaly delete original
                        Inkscape::XML::Node *string_node = xml_doc->createTextNode(spstring->string.c_str());
                        if (text) {
                            Inkscape::XML::Node *tspan_node = xml_doc->createElement("svg:tspan");
                            tspan_node->setAttribute("style", container->getRepr()->attribute("style"));
                            tspan_node->addChild(string_node, nullptr);
                            tspan_node->setAttribute("sodipodi:role", "line");
                            text->getRepr()->addChild(tspan_node, prevchild);
                            Inkscape::GC::release(string_node);
                            Inkscape::GC::release(tspan_node);
                            prevchild = tspan_node;
                        } else if (flowtext) {
                            Inkscape::XML::Node *flowpara_node = xml_doc->createElement("svg:flowPara");
                            flowpara_node->setAttribute("style", container->getRepr()->attribute("style"));
                            flowpara_node->addChild(string_node, nullptr);
                            flowtext->getRepr()->addChild(flowpara_node, prevchild);
                            Inkscape::GC::release(string_node);
                            Inkscape::GC::release(flowpara_node);
                            prevchild = flowpara_node;
                        }
                        container->getRepr()->removeChild(spstring->getRepr());
                    }
                }
                tc->text->getRepr()->removeChild(container->getRepr());
            }
        }
    }
}

Inkscape::XML::Node *TextToolbar::unindent_node(Inkscape::XML::Node *repr, Inkscape::XML::Node *prevchild)
{
    g_assert(repr != nullptr);

    Inkscape::XML::Node *parent = repr->parent();
    if (parent) {
        Inkscape::XML::Node *grandparent = parent->parent();
        if (grandparent) {
            SPDocument *doc = _desktop->getDocument();
            Inkscape::XML::Document *xml_doc = doc->getReprDoc();
            Inkscape::XML::Node *newrepr = repr->duplicate(xml_doc);
            parent->removeChild(repr);
            grandparent->addChild(newrepr, prevchild);
            Inkscape::GC::release(newrepr);
            newrepr->setAttribute("sodipodi:role", "line");
            return newrepr;
        }
    }
    std::cout << "error on TextToolbar.cpp::2433" << std::endl;
    return repr;
}

void TextToolbar::subselection_changed(gpointer texttool)
{
#ifdef DEBUG_TEXT
    std::cout << std::endl;
    std::cout << "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&" << std::endl;
    std::cout << "subselection_changed: start " << std::endl;
#endif
    // quit if run by the _changed callbacks
    this->_sub_active_item = nullptr;
    if (_updating) {
        return;
    }
    Inkscape::UI::Tools::TextTool *const tc = SP_TEXT_CONTEXT(texttool);
    if (tc) {
        Inkscape::Text::Layout const *layout = te_get_layout(tc->text);
        if (layout) {
            Inkscape::Text::Layout::iterator start           = layout->begin();
            Inkscape::Text::Layout::iterator end             = layout->end();
            Inkscape::Text::Layout::iterator start_selection = tc->text_sel_start;
            Inkscape::Text::Layout::iterator end_selection   = tc->text_sel_end;
#ifdef DEBUG_TEXT
            std::cout << "    GUI: Start of text: " << layout->iteratorToCharIndex(start) << std::endl;
            std::cout << "    GUI: End of text: " << layout->iteratorToCharIndex(end) << std::endl;
            std::cout << "    GUI: Start of selection: " << layout->iteratorToCharIndex(start_selection) << std::endl;
            std::cout << "    GUI: End of selection: " << layout->iteratorToCharIndex(end_selection) << std::endl;
            std::cout << "    GUI: Loop Subelements: " << std::endl;
            std::cout << "    ::::::::::::::::::::::::::::::::::::::::::::: " << std::endl;
#endif
            gint startline = layout->paragraphIndex(start_selection);
            if (start_selection == end_selection) {
                this->_outer = true;
                gint counter = 0;
                for (auto child : tc->text->childList(false)) {
                    SPItem *item = dynamic_cast<SPItem *>(child);
                    if (item && counter == startline) {
                        this->_sub_active_item = item;
                        int origin_selection = layout->iteratorToCharIndex(start_selection);
                        Inkscape::Text::Layout::iterator next = layout->charIndexToIterator(origin_selection + 1);
                        Inkscape::Text::Layout::iterator prev = layout->charIndexToIterator(origin_selection - 1);
                        //TODO: find a better way to init
                        _updating = true;
                        SPStyle query(_desktop->getDocument());
                        _query_cursor = query;
                        Inkscape::Text::Layout::iterator start_line = tc->text_sel_start;
                        start_line.thisStartOfLine();
                        if (tc->text_sel_start == start_line) {
                            tc->text_sel_start = next;
                        } else {
                            tc->text_sel_start = prev;
                        }
                        _cusor_numbers = sp_desktop_query_style(_desktop, &_query_cursor, QUERY_STYLE_PROPERTY_FONTNUMBERS);
                        tc->text_sel_start = start_selection;
                        wrap_start = tc->text_sel_start;
                        wrap_end = tc->text_sel_end;
                        wrap_start.thisStartOfLine();
                        wrap_end.thisEndOfLine();
                        _updating = false;
                        break;
                    }
                    ++counter;
                }
                selection_changed(nullptr);
            } else if ((start_selection == start && end_selection == end) ||
                       (start_selection == end && end_selection == start)) {
                // full subselection
                _cusor_numbers = 0;
                this->_outer = true;
                selection_changed(nullptr);
            } else {
                _cusor_numbers = 0;
                this->_outer = false;
                wrap_start = tc->text_sel_start;
                wrap_end = tc->text_sel_end;
                if (tc->text_sel_start > tc->text_sel_end) {
                    wrap_start.thisEndOfLine();
                    wrap_end.thisStartOfLine();
                } else {
                    wrap_start.thisStartOfLine();
                    wrap_end.thisEndOfLine();
                }
                selection_changed(nullptr);
            }
        }
    }
#ifdef DEBUG_TEXT
    std::cout << "subselection_changed: exit " << std::endl;
    std::cout << "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&" << std::endl;
    std::cout << std::endl;
#endif
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
